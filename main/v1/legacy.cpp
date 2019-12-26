/*
 * Filters+Echo+WT+Goertzel test/demo program
 *
 *  Created on: Apr 10, 2016
 *      Author: mayo
 */

#include "legacy.h"
#include "../hw/leds.h"
#include "../hw/gpio.h"
#include "../hw/init.h"
#include "../hw/codec.h"
#include "../hw/ui.h"
//#include "../hw/signals.h"
//#include "v1_signals.h"

float volume2 = 800.0f;
//float volume2 = 1200.0f;
//float volume2 = 1600.0f;

bool use_RNG = false;
int progressive_jamming_factor = 1;
int PlaybackPosition=0;
float SAMPLE_VOLUME_BOOST = 8; //12;//1.0f;
float sample_f[2],/*sample_synth[2],*/sample_mix;
int random_direction = 0;
int sample_counter = 0;

v1_filters *v1_fil;

void legacy_main()
{
	//set_sampling_rate(22050);
	#ifdef LEGACY_USE_44k_RATE
	set_sampling_rate(44100);
	#endif
	#ifdef LEGACY_USE_48k_RATE
	//set_sampling_rate(46550);
	set_sampling_rate(50780);
	#endif
	#ifdef LEGACY_USE_22k_RATE
	set_sampling_rate(22050);
	#endif

	channel_running = 1;

	//volume_ramp = 1;
	//instead of volume ramp, set the codec volume instantly to un-mute the codec
	codec_digital_volume=codec_volume_user;
	codec_set_digital_volume();
	noise_volume_max = 1.0f;

#ifdef USE_FILTERS
	//initialize filters
	v1_fil = new v1_filters();

	//initFilter(&filt);
	#if FILTERS_TO_USE == 0
		//no filtering
	#elif FILTERS_TO_USE == 1
		fil->filter_setup();
	#elif FILTERS_TO_USE == 2
		v1_fil->filter_setup02();
	#endif

		v1_fil->fp.vol_ramp = v1_fil->fp.volume_f;
#endif

	seconds = 0;

	#ifdef DEBUG_SIGNAL_LEVELS
	float ADC_signal_min = 100000, ADC_signal_max = -100000;
	float signal_min = 100000, signal_max = -100000;
	#endif

	#ifdef ENABLE_LIMITER
    //light
    //float signal_max_limit = 16000;
    //float signal_min_limit = -4000;
    float signal_max_limit = 32000;
    float signal_min_limit = -32000;
    //float signal_max_limit = 24000;
    //float signal_min_limit = -24000;
    //float signal_max_limit = 16000;
    //float signal_min_limit = -16000;
	#endif

    noise_volume = noise_volume_max;

#ifdef ENABLE_ECHO
    memset(echo_buffer,0,ECHO_BUFFER_LENGTH);//sizeof(echo_buffer));
    echo_buffer_ptr0 = 0;
#endif

    //codec_silence(1000);

	while(!event_next_channel)
	{
		LED_indicators();

		if (!(sampleCounter & 0x00000001)) //left or right channel
		{
			/*
			//STM32F4 HW RNG
			while(RNG_GetFlagStatus(RNG_FLAG_DRDY)== RESET)
			{
			}
			random_value = RNG_GetRandomNumber();
			*/

			float r = v1_PseudoRNG1a_next_float();
			//float r = PseudoRNG1b_next_float();
			memcpy(&random_value, &r, sizeof(random_value));

			//random_value = PseudoRNG2_next_int32();

			if(use_RNG)
			{
				//convert RNG 16-bit to sample
				sample_f[0] = (float)(32768 - (int16_t)random_value) / 32768.0f;
				sample_f[1] = (float)(32768 - (int16_t)(random_value>>16)) / 32768.0f;
			}
			else
			{
#ifdef ADD_MICROPHONE_SIGNAL
				//get microphone signal
				//sample_f[0] = (float)(4096 - (int16_t)ADC1_read()) / 4096.0f * PREAMP_BOOST;
				//sample_f[1] = (float)(4096 - (int16_t)ADC2_read()) / 4096.0f * PREAMP_BOOST;

				//sample_f[0] = (float)(65536 - (ADC_sample&0xffff)) / 65536.0f * PREAMP_BOOST;
				//sample_f[1] = (float)(65536 - (ADC_sample>>16)) / 65536.0f * PREAMP_BOOST;

				//sample_f[0] = (float)(32768 - (ADC_sample&0xffff)) / 32768.0f * PREAMP_BOOST;
				//sample_f[1] = (float)(32768 - (ADC_sample>>16)) / 32768.0f * PREAMP_BOOST;

				i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
				//#ifdef USE_FAUX_22k_MODE
				//i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
				//#endif

				//sample_f[0] = (float)(32768 - (int16_t)ADC_sample) / 32768.0f * PREAMP_BOOST;
				//sample_f[1] = (float)(32768 - (int16_t)(ADC_sample>>16)) / 32768.0f * PREAMP_BOOST;

				//sample_f[0] = (float)((int16_t)ADC_sample);
				//sample_f[1] = (float)((int16_t)(ADC_sample>>16));

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

#else
				sample_f[0] = 0;
				sample_f[1] = 0;
#endif

#ifdef ADD_PLAIN_NOISE
				//add some noise too
				sample_f[0] += ( (float)(32768 - (int16_t)random_value) / 32768.0f) * noise_volume;
				sample_f[1] += ( (float)(32768 - (int16_t)(random_value>>16)) / 32768.0f) * noise_volume;
#endif
			}
		}

        //while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
    	{
        	//SPI_I2S_SendData(CODEC_I2S, sample_i16);
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

			#ifdef ENABLE_LIMITER
    		if(sample_mix > signal_max_limit)
    		{
    			sample_mix = signal_max_limit;
    		}
    		if(sample_mix < signal_min_limit)
    		{
    			sample_mix = signal_min_limit;
    		}
			#endif

#ifdef USE_FILTERS
    		sample_i16 = (int16_t)(sample_mix*v1_fil->fp.vol_ramp * SAMPLE_VOLUME_BOOST);
#else
    		sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME_BOOST);
#endif

#ifdef ENABLE_ECHO
    		//add echo
    		echo_buffer_ptr0++;
    		if(echo_buffer_ptr0 >= ECHO_BUFFER_LENGTH)
    		{
    			echo_buffer_ptr0 = 0;
    		}

    		echo_buffer_ptr = echo_buffer_ptr0 + 1;
    		if(echo_buffer_ptr >= ECHO_BUFFER_LENGTH)
    		{
    			echo_buffer_ptr = 0;
    		}

    		sample_i16 += echo_buffer[echo_buffer_ptr];

    		//store result to echo
    		echo_buffer[echo_buffer_ptr0] = sample_i16 * 1 / 2; //stm32f4-disc1
    		//echo_buffer[echo_buffer_ptr0] = sample_i16 * 2 / 3; //gecho v0.01
#endif

#if defined(USE_FILTERS) && defined(ENABLE_CHORD_LOOP)
    		if((sampleCounter==0) && (seconds%2==0)) //test - every 2 seconds
    		{
    			v1_fil->start_next_chord();
    		}
#endif

#ifdef ENABLE_JAMMING
    		if (sampleCounter % (v1_I2S_AUDIOFREQ/(progressive_jamming_factor*2)) == 0)
    		{
    			LEDs_RED_off();
    		}

    		if (sampleCounter % (v1_I2S_AUDIOFREQ/progressive_jamming_factor) == 0)
    		{
    			noise_volume = noise_volume_max;
    			LEDs_RED_next(progressive_jamming_factor);
    		}
    		else
    		{
    			if(noise_volume>noise_volume_max/PROGRESSIVE_JAMMING_BOOST_RATIO)
    			{
    				noise_volume *= PROGRESSIVE_JAMMING_RAMP_COEF;
    			}
    		}
#endif

            if (sampleCounter & 0x00000001) //left or right channel
            {
            	sample32 += sample_i16;
            	//while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
                //SPI_I2S_SendData(CODEC_I2S, sample_i16);
            	i2s_push_sample(I2S_NUM, (char*)&sample32, portMAX_DELAY);
				#ifdef USE_FAUX_22k_MODE
				i2s_pop_sample(I2S_NUM, (char*)&ADC_sample0, portMAX_DELAY);
            	i2s_push_sample(I2S_NUM, (char*)&sample32, portMAX_DELAY);
				#endif
            }
            else
            {
            	sample32 = sample_i16<<16;
            }

        	sampleCounter++;
    	}

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

	#ifdef ENABLE_JAMMING
    		//if(seconds%(v1_fil->chord->total_chords*2*2)==0)
        	if(seconds%(v1_fil->chord->total_chords*2)==0)
			{
				progressive_jamming_factor++;
				if(progressive_jamming_factor==5)
				{
					progressive_jamming_factor = 1;
				}
				LEDs_RED_reset();
			}
	#endif

#endif

			uint8_t CodecCommandBuffer[5];
    	}

    	//float last_drift;
    	if (sampleCounter%(2*v1_I2S_AUDIOFREQ/10)==0) //every 100ms
    	//if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/100)==0) //every 10ms
    	{
    		//if(RedLED_state) LED_RED_ON else LED_RED_OFF;
    		//RedLED_state = !RedLED_state;
    		//if(OrangeLED_state) LED_ORANGE_ON else LED_ORANGE_OFF;
    		if(OrangeLED_state) LED_RDY_ON; else LED_RDY_OFF;
    		OrangeLED_state = !OrangeLED_state;

#ifdef USE_FILTERS
    		//for(int d=0;d<FILTERS/2;d++)
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
#endif

#ifdef USE_IR_SENSORS
    	if (sampleCounter%(2*I2S_AUDIOFREQ/100)==0) //every 10ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
    	{
    		if(ADC_process_sensors()==1)
    		{
    			int r1 = ADC_last_result[0];
    			int r2 = ADC_last_result[1];
    			int r3 = ADC_last_result[2];
    			ADC_result_ready = 0;

    			if(ADC_last_result[0] > 500)
    			{
    				r1 = 0;
    				v1_fil->iir2[seconds%8].setResonance(0.999f);
    			}
    			else
    			{
    				v1_fil->iir2[seconds%8].setResonance(0.99f);
    			}
    		}
    	}
#endif
    	//queue_codec_ctrl_process();
    }

	delete(v1_fil);
	set_sampling_rate(persistent_settings.SAMPLING_RATE);
}


void LED_indicators()
{
	if (sampleCounter==0)
	{
		//LEDs_BLUE_next(4);
		//LEDs_RED_off();

		//LED_BLUE_ON;
		LED_SIG_ON;
	}
	else if (sampleCounter == v1_I2S_AUDIOFREQ)
	{
		//LED_BLUE_OFF;
		LED_SIG_OFF;

		//LEDs_BLUE_off();
		//LEDs_RED_next(4);
	}

	if (sampleCounter % v1_I2S_AUDIOFREQ == 0)
	{
		LEDs_BLUE_next(8);
		//LEDs_RED_off();
	}
	else if (sampleCounter % v1_I2S_AUDIOFREQ == v1_I2S_AUDIOFREQ/2)
	{
		LEDs_BLUE_off();
		//LEDs_RED_next(progressive_jamming_factor);
	}
}

