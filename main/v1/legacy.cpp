/*
 * legacy.cpp - Filters + Echo + WT + Goertzel test/demo program
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Apr 10, 2016
 *      Author: mario
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of GNU GPLv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/
 *
 */

#include "legacy.h"
#include "hw/leds.h"
#include "hw/gpio.h"
#include "hw/init.h"
#include "hw/codec.h"
#include "hw/ui.h"
#include "hw/sdcard.h"
#include "hw/signals.h"

float volume2 = 1600.0f;
float sample_f[2],sample_mix;

v1_filters *v1_fil;

void legacy_main()
{
	#ifdef LEGACY_USE_44k_RATE
	set_sampling_rate(44100);
	#endif
	#ifdef LEGACY_USE_48k_RATE
	set_sampling_rate(50780);
	#endif
	#ifdef LEGACY_USE_22k_RATE
	set_sampling_rate(22050);
	#endif

	channel_running = 1;

	//instead of volume ramp, set the codec volume instantly to un-mute the codec
	codec_digital_volume=codec_volume_user;
	codec_set_digital_volume();
	noise_volume_max = 1.0f;

	#ifdef USE_FILTERS

	//initialize filters
	v1_fil = new v1_filters();

	#if FILTERS_TO_USE == 0
		//no filtering
	#elif FILTERS_TO_USE == 1
		fil->filter_setup();
	#elif FILTERS_TO_USE == 2
		v1_fil->filter_setup02();
	#endif
		v1_fil->fp.vol_ramp = v1_fil->fp.volume_f;

	#endif //USE_FILTERS

	seconds = 0;

	#ifdef DEBUG_SIGNAL_LEVELS
	float ADC_signal_min = 100000, ADC_signal_max = -100000;
	float signal_min = 100000, signal_max = -100000;
	#endif

	#ifdef ENABLE_LIMITER_FLOAT
    float signal_max_limit_f = 32000;
    float signal_min_limit_f = -32000;
	#endif

	#ifdef ENABLE_LIMITER_INT
	int16_t signal_max_limit_i = 24000;
	int16_t signal_min_limit_i = -24000;
	#endif

	noise_volume = noise_volume_max;

	#ifdef ENABLE_ECHO
	ECHO_MIXING_GAIN_MUL = 1; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 2; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in
    memset(echo_buffer,0,ECHO_BUFFER_LENGTH);//sizeof(echo_buffer));
    echo_buffer_ptr = 0;
    echo_buffer_ptr0 = 0;
    #endif //ENABLE_ECHO

	#ifdef USE_IR_SENSORS
    sensors_active = 1;
    int sensor_triggered[4] = {0,0,0,0};
    float new_mixing_vol; //tmp var for ARP voice volume calculation
    int arpeggiator_loop = 0;
	float noise_dim_by_sensor = SENSOR_NOISE_DIM_LEVEL_DEFAULT;
	#endif //USE_IR_SENSORS

    //codec_silence(1000);

	while(!event_next_channel)
	{
		LED_indicators();

		ADC_sample = ADC_sampleA[ADC_sample_ptr];
		ADC_sample_ptr++;

		if(ADC_sample_ptr==ADC_SAMPLE_BUFFER)
		{
			i2s_read(I2S_NUM, (void*)ADC_sampleA, 4*ADC_SAMPLE_BUFFER, &i2s_bytes_rw, 1);
			ADC_sample_ptr = 0;
		}

		if (!(sampleCounter & 0x00000001)) //left or right channel
		{
			//rand method #1
			float r = v1_PseudoRNG1a_next_float();
			//float r = PseudoRNG1b_next_float();
			memcpy(&random_value, &r, sizeof(random_value));

			//rand method #2
			//random_value = PseudoRNG2_next_int32();

			#ifdef ADD_MICROPHONE_SIGNAL
				//get microphone signal

				sample_f[0] = (float)(((int16_t)ADC_sample) / 32768.0f) * PREAMP_BOOST;
				sample_f[1] = (float)(((int16_t)(ADC_sample>>16)) / 32768.0f) * PREAMP_BOOST;

				#ifdef DEBUG_SIGNAL_LEVELS
				if(sample_f[0] > ADC_signal_max)
	    		{
	    			ADC_signal_max = sample_f[0];
	    		}
	    		if(sample_f[0] < ADC_signal_min)
	    		{
	    			ADC_signal_min = sample_f[0];
	    		}
				#endif

			#else //!ADD_MICROPHONE_SIGNAL
				sample_f[0] = 0;
				sample_f[1] = 0;
			#endif //ADD_MICROPHONE_SIGNAL

			#ifdef ADD_PLAIN_NOISE
				//add some noise too
				sample_f[0] += ( (float)(32768 - (int16_t)random_value) / 32768.0f) * noise_volume * noise_dim_by_sensor;
				sample_f[1] += ( (float)(32768 - (int16_t)(random_value>>16)) / 32768.0f) * noise_volume * noise_dim_by_sensor;
			#endif //ADD_PLAIN_NOISE
		}

		if (sampleCounter & 0x00000001) //left or right channel
		{
    		//t1 = get_micros();
			#ifdef USE_FILTERS
				#if FILTERS_TO_USE == 0
					sample_mix = sample_f[0] * volume2;
				#elif FILTERS_TO_USE == 1
					sample_mix = FilterIIR00::iir_filter_multi_sum(sample_f[0],v1_fil->iir,FILTERS/2,v1_fil->fp.mixing_volumes);
				#elif FILTERS_TO_USE == 2
					//sample_mix = v1_FilterSimpleIIR02::iir_filter_multi_sum(sample_f[0],v1_fil->iir2,FILTERS/2,v1_fil->fp.mixing_volumes)*volume2;
					sample_mix = v1_fil->iir2->iir_filter_multi_sum(sample_f[0],v1_fil->iir2,FILTERS/2,v1_fil->fp.mixing_volumes)*volume2;
				#endif
			#else
				sample_mix = sample_f[0] * PLAIN_NOISE_VOLUME;
			#endif

			#ifdef USE_SYNTH
    			sample_mix += sample_synth[0] * volume_syn;
			#endif
    			//t2 = get_micros();
    	}
    	else
    	{
			//t3 = get_micros();
			#ifdef USE_FILTERS
				#if FILTERS_TO_USE == 0
					sample_mix = sample_f[1] * volume2;
				#elif FILTERS_TO_USE == 1
					sample_mix = FilterIIR00::iir_filter_multi_sum(sample_f[1],v1_fil->iir+FILTERS/2,FILTERS/2,v1_fil->fp.mixing_volumes+FILTERS/2);
				#elif FILTERS_TO_USE == 2
					//sample_mix = v1_FilterSimpleIIR02::iir_filter_multi_sum(sample_f[1],v1_fil->iir2+FILTERS/2,FILTERS/2,v1_fil->fp.mixing_volumes+FILTERS/2)*volume2;
					sample_mix = v1_fil->iir2->iir_filter_multi_sum(sample_f[1],v1_fil->iir2+FILTERS/2,FILTERS/2,v1_fil->fp.mixing_volumes+FILTERS/2)*volume2;
				#endif
			#else
				sample_mix = sample_f[1] * PLAIN_NOISE_VOLUME;
			#endif

			#ifdef USE_SYNTH
			sample_mix += sample_synth[1] * volume_syn;
			#endif
			//t4 = get_micros();
		}

		#ifdef DEBUG_SIGNAL_LEVELS
		if(sample_mix > signal_max)
		{
			signal_max = sample_mix;
		}
		if(sample_mix < signal_min)
		{
			signal_min = sample_mix;
		}
		#endif

		#ifdef ENABLE_LIMITER_FLOAT
		if(sample_mix > signal_max_limit_f)
		{
			sample_mix = signal_max_limit_f;
		}
		if(sample_mix < signal_min_limit_f)
		{
			sample_mix = signal_min_limit_f;
		}
		#endif

		#ifdef USE_FILTERS
		sample_i16 = (int16_t)(sample_mix);//*v1_fil->fp.vol_ramp * SAMPLE_VOLUME_BOOST);
		#else
		sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME_BOOST);
		#endif

		#ifdef ENABLE_ECHO
			sample_i16 = add_echo_legacy(sample_i16);

			#ifdef ENABLE_LIMITER_INT
			if(sample_i16 > signal_max_limit_i)
			{
				sample_i16 = signal_max_limit_i;
			}
			if(sample_i16 < signal_min_limit_i)
			{
				sample_i16 = signal_min_limit_i;
			}
			#endif //ENABLE_LIMITER_INT
		#endif //ENABLE_ECHO

		#if defined(USE_FILTERS) && defined(ENABLE_CHORD_LOOP)
			if((sampleCounter==0) && (seconds%2==0)) //test - every 2 seconds
			{
				v1_fil->start_next_chord();
			}
		#endif

		#ifdef ENABLE_RHYTHM
			if (sampleCounter % (v1_I2S_AUDIOFREQ/(progressive_rhythm_factor*2)) == 0)
			{
				LEDs_RED_off();
			}

			if (sampleCounter % (v1_I2S_AUDIOFREQ/progressive_rhythm_factor) == 0)
			{
				noise_volume = noise_volume_max;
				LEDs_RED_next(progressive_rhythm_factor);
			}
			else
			{
				if(noise_volume>noise_volume_max/PROGRESSIVE_RHYTHM_BOOST_RATIO)
				{
					noise_volume *= PROGRESSIVE_RHYTHM_RAMP_COEF;
				}
			}
		#endif //ENABLE_RHYTHM

		if (sampleCounter & 0x00000001) //left or right channel
		{
			sample32 += sample_i16;
			i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
			sd_write_sample(&sample32);
			#ifdef USE_FAUX_22k_MODE
			i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
			sd_write_sample(&sample32);
			#endif
		}
		else
		{
			sample32 = sample_i16<<16;
		}

		sampleCounter++;

		if (sampleCounter == 2*v1_I2S_AUDIOFREQ /*- AUDIOFREQ_DIV_CORRECTION*/)
		{
			sampleCounter = 0;
			seconds++;

			#ifdef DEBUG_SIGNAL_LEVELS
    		printf("ADC_signal_min=%f,ADC_signal_max=%f,signal_min=%f,signal_max=%f\n", ADC_signal_min, ADC_signal_max, signal_min, signal_max);
    		ADC_signal_min = 100000;
    		ADC_signal_max = -100000;

    		signal_min = 100000;
    		signal_max = -100000;
			#endif

			#ifdef USE_FILTERS

				#ifdef ENABLE_RHYTHM

    				if(seconds%(v1_fil->chord->total_chords*2)==0)
    				{
    					progressive_rhythm_factor++;
    					if(progressive_rhythm_factor==5)
    					{
    						progressive_rhythm_factor = 1;
    					}
    					LEDs_RED_reset();
    				}
				#endif //ENABLE_RHYTHM
			#endif //USE_FILTERS
    	}

    	//float last_drift;
    	if (sampleCounter%(2*v1_I2S_AUDIOFREQ/10)==0) //every 100ms
    	{
    		if(OrangeLED_state) LED_RDY_ON; else LED_RDY_OFF;
    		OrangeLED_state = !OrangeLED_state;

			#ifdef USE_FILTERS

        		for(int d=0;d<FILTERS;d++)
        		{
        			if(v1_fil->fp.enable_mixing_deltas)
        			{
        				v1_fil->fp.mixing_volumes[d]+=v1_fil->fp.mixing_deltas[d]*0.01f;
        				if(v1_fil->fp.mixing_volumes[d]<0)
						{
							v1_fil->fp.mixing_volumes[d]=0;
						}
						else if(v1_fil->fp.mixing_volumes[d]>1)
						{
							v1_fil->fp.mixing_volumes[d]=1;
						}
					}
        		}
			#endif
    	}

		#ifdef USE_FILTERS
    		if (sampleCounter%(2*v1_I2S_AUDIOFREQ/100)==57) //every 10ms, at 5.7th ms
    		{
    			v1_fil->progress_update_filters(v1_fil, seconds%2==0);
    		}
		#endif //USE_FILTERS

		#ifdef USE_IR_SENSORS

    		if (sampleCounter%(2*I2S_AUDIOFREQ/100)==47) //every 10ms
    		//if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
    		{
    			if(SENSOR_RESO_ACTIVE_1)
    			{
    				if(SENSOR_RESO_ACTIVE_5 /*&& sensor_triggered[SENSOR_ID_BLUE] != 5*/)
    				{
    					v1_fil->iir2[seconds%8].setResonance(0.9999f);
    					//sensor_triggered[SENSOR_ID_BLUE] = 5;
    					//printf("sensor_triggered[SENSOR_ID_BLUE] => 5\n");
    				}
					else if(SENSOR_RESO_ACTIVE_4 /*&& sensor_triggered[SENSOR_ID_BLUE] != 4*/)
					{
						v1_fil->iir2[seconds%8].setResonance(0.9995f);
						//sensor_triggered[SENSOR_ID_BLUE] = 4;
						//printf("sensor_triggered[SENSOR_ID_BLUE] => 4\n");
					}
					else if(SENSOR_RESO_ACTIVE_3 /*&& sensor_triggered[SENSOR_ID_BLUE] != 3*/)
					{
						v1_fil->iir2[seconds%8].setResonance(0.999f);
						//sensor_triggered[SENSOR_ID_BLUE] = 3;
						//printf("sensor_triggered[SENSOR_ID_BLUE] => 3\n");
					}
					else if(SENSOR_RESO_ACTIVE_2 /*&& sensor_triggered[SENSOR_ID_BLUE] != 2*/)
					{
						v1_fil->iir2[seconds%8].setResonance(0.995f);
						sensor_triggered[SENSOR_ID_BLUE] = 2;
						//printf("sensor_triggered[SENSOR_ID_BLUE] => 2\n");
					}
					else //if(sensor_triggered[SENSOR_ID_BLUE] != 1)
					{
						v1_fil->iir2[seconds%8].setResonance(0.99f);
						//sensor_triggered[SENSOR_ID_BLUE] = 1;
						//printf("sensor_triggered[SENSOR_ID_BLUE] => 1\n");
					}
				}
				else //if(sensor_triggered[SENSOR_ID_BLUE])
				{
					v1_fil->iir2[seconds%8].setResonance(0.95f);
					//sensor_triggered[SENSOR_ID_BLUE] = 0;
					//printf("sensor_triggered[SENSOR_ID_BLUE] => 0\n");
				}
    		}

    		if (sampleCounter % (v1_I2S_AUDIOFREQ/2) == 0) //every 500ms
    		{
    			if(SENSOR_ARP_ACTIVE_1)
    			{
    				if(v1_fil->fp.arpeggiator_filter_pair==-1)
    				{
    					v1_fil->fp.arpeggiator_filter_pair = DEFAULT_ARPEGGIATOR_FILTER_PAIR;
    				}

					int next_chord = v1_fil->chord->current_chord-1;
					if(next_chord > v1_fil->chord->total_chords)
					{
						next_chord = 0;
					}
					else if(next_chord < 0)
					{
						next_chord = v1_fil->chord->total_chords-1;
					}

					//BUG!
					//fil->add_to_update_filters_pairs(0,fil->chord->chords[next_chord].freqs[arpeggiator_loop]);

					v1_fil->add_to_update_filters_pairs(v1_fil->fp.arpeggiator_filter_pair,
						v1_fil->chord->chords[next_chord].v1_freqs[arpeggiator_loop]);

					arpeggiator_loop++;
					if(arpeggiator_loop==CHORD_MAX_VOICES)
					{
						arpeggiator_loop = 0;
					}
					//printf("arpeggiator_loop = %d\n",arpeggiator_loop);

					if(SENSOR_ARP_ACTIVE_2)
					{
						new_mixing_vol = 1.0f + SENSOR_ARP_INCREASE * ARP_VOLUME_MULTIPLIER_DEFAULT; //good for wind channel

						v1_fil->fp.mixing_volumes[v1_fil->fp.arpeggiator_filter_pair] = new_mixing_vol;
						v1_fil->fp.mixing_volumes[v1_fil->fp.arpeggiator_filter_pair+FILTERS/2] = new_mixing_vol;

						v1_fil->iir2[v1_fil->fp.arpeggiator_filter_pair].setResonance(0.999f);
						v1_fil->iir2[v1_fil->fp.arpeggiator_filter_pair+FILTERS/2].setResonance(0.999f);
					}
					sensor_triggered[SENSOR_ID_WHITE] = 1;
    			}
    			else if(sensor_triggered[SENSOR_ID_WHITE])
    			{
    				if(v1_fil->fp.arpeggiator_filter_pair>=0)
    				{
						//volume back to normal
						v1_fil->fp.mixing_volumes[v1_fil->fp.arpeggiator_filter_pair] = 1.0f;
						v1_fil->fp.mixing_volumes[v1_fil->fp.arpeggiator_filter_pair+FILTERS/2] = 1.0f;

						//back to default resonance
						v1_fil->iir2[v1_fil->fp.arpeggiator_filter_pair].setResonanceKeepFeedback(0.95f);
						v1_fil->iir2[v1_fil->fp.arpeggiator_filter_pair+FILTERS/2].setResonanceKeepFeedback(0.95f); //refactor

						v1_fil->fp.arpeggiator_filter_pair = -1;
					}
					sensor_triggered[SENSOR_ID_WHITE] = 0;
				}
			}

			if (sampleCounter%(2*I2S_AUDIOFREQ/20)==7) //every 50ms
			{
				//map accelerometer values of y-axis (-1..1) to sample mixing volume (0..1)
				//mixed_sample_volume = SENSOR_SAMPLE_MIX_VOL * mixed_sample_volume_coef_ext;

				if(SENSOR_NOISE_DIM_ACTIVE_1)
				{
					noise_dim_by_sensor = SENSOR_NOISE_DIM_LEVEL;
					sensor_triggered[SENSOR_ID_ORANGE] = 1;
					//printf("noise_dim_by_sensor = %f\n", noise_dim_by_sensor);
				}
				else if(sensor_triggered[SENSOR_ID_ORANGE])
				{
					noise_dim_by_sensor = SENSOR_NOISE_DIM_LEVEL_DEFAULT;
					sensor_triggered[SENSOR_ID_ORANGE] = 0;
				}
				else
				{
					if(SENSOR_NOISE_RISE_ACTIVE_1)
					{
						noise_dim_by_sensor = SENSOR_NOISE_RISE_LEVEL;
						sensor_triggered[SENSOR_ID_RED] = 1;
					}
					else if(sensor_triggered[SENSOR_ID_RED])
					{
						noise_dim_by_sensor = SENSOR_NOISE_DIM_LEVEL_DEFAULT;
						sensor_triggered[SENSOR_ID_RED] = 0;
					}
				}
			}

		#endif //USE_IR_SENSORS

        //restore the dynamic limiter if triggered
        //if (TIMING_EVERY_20_MS == 33) //50Hz --> for linear curve
        if (TIMING_EVERY_5_MS == 33) //200Hz --> for geometric curve
		{
			if(limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT)
			{
				//linear recovery curve
				//limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 25; //limiter will fully recover within 0.5 seconds

				//geometric recovery curve
				limiter_coeff /= DYNAMIC_LIMITER_COEFF_MUL_V1;

				if(limiter_coeff > DYNAMIC_LIMITER_COEFF_DEFAULT)
				{
					limiter_coeff = DYNAMIC_LIMITER_COEFF_DEFAULT;
				}
			}
		}
    }

	delete(v1_fil);
	set_sampling_rate(persistent_settings.SAMPLING_RATE);
}


void LED_indicators()
{
	if (sampleCounter==0)
	{
		LED_SIG_ON;
	}
	else if (sampleCounter == v1_I2S_AUDIOFREQ)
	{
		LED_SIG_OFF;
	}

	if (sampleCounter % v1_I2S_AUDIOFREQ == 0)
	{
		LEDs_BLUE_next(8);
	}
	else if (sampleCounter % v1_I2S_AUDIOFREQ == v1_I2S_AUDIOFREQ/2)
	{
		LEDs_BLUE_off();
	}
}

int16_t add_echo_legacy(int16_t sample)
{
	if(echo_dynamic_loop_length)
	{
		//wrap the echo loop
		echo_buffer_ptr0++;
		if (echo_buffer_ptr0 >= echo_dynamic_loop_length)
		{
			echo_buffer_ptr0 = 0;
		}

		echo_buffer_ptr = echo_buffer_ptr0 + 1;
		if (echo_buffer_ptr >= echo_dynamic_loop_length)
		{
			echo_buffer_ptr = 0;
		}

		if(echo_skip_samples)
		{
			if(echo_skip_samples_from && (echo_skip_samples_from == echo_buffer_ptr))
			{
				echo_skip_samples_from = 0;
			}
			if(echo_skip_samples_from==0)
			{
				echo_buffer[echo_buffer_ptr] = 0;
				echo_skip_samples--;
			}
		}

		//add echo from the loop
		echo_mix_f = (float)sample + (float)echo_buffer[echo_buffer_ptr] * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
	}
	else
	{
		echo_mix_f = (float)sample;
	}

	echo_mix_f *= limiter_coeff;

	if((echo_mix_f < -DYNAMIC_LIMITER_THRESHOLD_ECHO_V1 || echo_mix_f > DYNAMIC_LIMITER_THRESHOLD_ECHO_V1) && limiter_coeff > 0.01f)
	{
		limiter_coeff *= DYNAMIC_LIMITER_COEFF_MUL_V1;
	}

	sample = (int16_t)echo_mix_f;

	//store result to echo, the amount defined by a fragment
	//echo_mix_f = ((float)sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV);
	//echo_mix_f *= ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
	//echo_buffer[echo_buffer_ptr0] = (int16_t)echo_mix_f;

	echo_buffer[echo_buffer_ptr0] = sample;

	return sample;
}
