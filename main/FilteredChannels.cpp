/*
 * FilteredChannels.cpp
 *
 *  Created on: Nov 26, 2016
 *      Author: mario
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/gechologists/
 *
 */

#include <FilteredChannels.h>
#include <InitChannels.h>
#include <Accelerometer.h>
#include <Interface.h>
#include <Binaural.h>
#include <hw/init.h>
#include <hw/signals.h>
#include <hw/sdcard.h>
#include <hw/leds.h>
#include <hw/ui.h>

//#define USE_AUTOCORRELATION
#define DRY_MIX_AFTER_ECHO
//#define LIMITER_DEBUG

static volatile double r_noise = NOISE_SEED;
static volatile int i_noise;

float reverb_volume = 1.0f;

int wind_voices;
int wind_cutoff_sweep = 0;

#ifdef LIMITER_DEBUG
int limiter_debug[8] = {32760, -32760, 32760, -32760, 32760, -32760, 32760, -32760};
#endif

void filtered_channel(int use_bg_sample, int use_direct_waves_filters, int use_reverb, float mixed_sample_volume_coef)
{
	printf("filtered_channel(): wind_voices = %d\n",wind_voices);

	while(!event_next_channel || beats_selected)
    {
		//t_TIMING_BY_SAMPLE_EVERY_250_MS = TIMING_BY_SAMPLE_EVERY_250_MS;
		t_TIMING_BY_SAMPLE_EVERY_125_MS = TIMING_BY_SAMPLE_EVERY_125_MS;

		//=============================================================================================
		//=============================== L/R channel block begin =====================================
		//=============================================================================================

		//---------------
		//PART 1A - RIGHT
		//---------------
		//else
		//{
				//#ifdef USE_BG_SAMPLE
				if(use_bg_sample)
				{
					//------------- BG SAMPLE ----------------------------------
					//random_value = 0; //remove white noise

					sample_f[0] = ( (float)(32768 - (int16_t)mixed_sample_buffer[mixed_sample_buffer_ptr_L++]) / 32768.0f) * mixed_sample_volume;// * noise_volume * noise_boost_by_sensor;

					if(mixed_sample_buffer_ptr_L == MIXED_SAMPLE_BUFFER_LENGTH)
					{
						mixed_sample_buffer_ptr_L = 0;
					}

					sample_f[0] += (float)((int16_t)ADC_sample) * OpAmp_ADC12_signal_conversion_factor;

					//#else
				} else {
					#ifdef USE_BYTEBEAT

						random_value = bytebeat_next_sample();
						//printf("rnd=%x",random_value);

					#else

						//r = PseudoRNG1a_next_float();
						//memcpy(&random_value, &r, sizeof(random_value));

						//random_value = 0; //remove white noise

						r_noise = r_noise + 19;
						r_noise = r_noise * r_noise;
						//i_noise = r_noise;
						//r_noise = r_noise - i_noise;
						////return b_noise - 0.5;

						//memcpy(&random_value, &r_noise, sizeof(random_value));

						/*
						random_value = 19 + random_value * random_value;
						if(random_value&0x8000)
						{
							random_value >>= 1;
						}
						//printf("%d ", random_value);
						*/

					#endif

					//sample_f[0] = 0;
					sample_f[0] = (float)((int16_t)ADC_sample) * OpAmp_ADC12_signal_conversion_factor;
				//#endif
				}

				/*
				#ifdef USE_BG_SAMPLE
					//------------- BG SAMPLE ----------------------------------
					sample_f[0] += ( (float)(32768 - (int16_t)mixed_sample_buffer[mixed_sample_buffer_ptr_L++]) / 32768.0f) * mixed_sample_volume;// * noise_volume * noise_boost_by_sensor;

					if(mixed_sample_buffer_ptr_L == MIXED_SAMPLE_BUFFER_LENGTH)
					{
						mixed_sample_buffer_ptr_L = 0;
					}
					//------------- BG SAMPLE ----------------------------------
				#endif
				*/
			//}

			if(wind_voices)
			{
				//printf("[condition]wind_voices\n");
				sample_mix = IIR_Filter::iir_filter_multi_sum_w_noise_and_wind(
					sample_f[0],
					fil->iir2,
					ACTIVE_FILTERS_PAIRS,
					fil->fp.mixing_volumes,
					//(int16_t)random_value,
					((uint16_t *)&r_noise)[0],
					//0,
					noise_volume * noise_boost_by_sensor,
					wind_voices
				) * MAIN_VOLUME;
			}
			else
			{
				sample_mix = IIR_Filter::iir_filter_multi_sum_w_noise(
					sample_f[0],
					fil->iir2,
					ACTIVE_FILTERS_PAIRS,
					fil->fp.mixing_volumes,
					//(int16_t)random_value,
					((uint16_t *)&r_noise)[0],
					//0,
					noise_volume * noise_boost_by_sensor
				) * MAIN_VOLUME;
			}

			sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);
		//}

		#ifdef LIMITER_DEBUG
		if(sample_i16<limiter_debug[0]) limiter_debug[0] = sample_i16;
		if(sample_i16>limiter_debug[1]) limiter_debug[1] = sample_i16;
		#endif

		//---------------
		//PART 2AB - BOTH
		if(PROG_add_echo)
		{
			filtered_channel_add_echo();
		}

		#ifdef LIMITER_DEBUG
		if(sample_i16<limiter_debug[2]) limiter_debug[2] = sample_i16;
		if(sample_i16>limiter_debug[3]) limiter_debug[3] = sample_i16;
		#endif

		//---------------
		//PART 3B and 3A - LEFT & RIGHT
    	if(use_reverb)
    	{
			//if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 24) //4Hz
			//if (TIMING_BY_SAMPLE_EVERY_500_MS == 24) //2Hz
			if (TIMING_BY_SAMPLE_EVERY_250_MS == 24) //2Hz
			{
				filtered_channel_adjust_reverb();
			}

			//if (sampleCounter & 0x00000001) //left channel
			//{
			//	//sample32 += sample_i16;
			//	sample32 += (int16_t)((float)add_reverb(sample_i16) * reverb_volume + (float)sample_i16 * (1.0f-reverb_volume));
			//}
			//else
			//{
				//sample32 = sample_i16 << 16;
				//sample32 = ((int16_t)((float)add_reverb(sample_i16) * reverb_volume  + (float)sample_i16 * (1.0f-reverb_volume))) << 16;
				sample_i16 = (int16_t)((float)add_reverb(sample_i16) * reverb_volume  + (float)sample_i16 * (1.0f-reverb_volume));
			//}
    	}
    	else
    	{
			//if (sampleCounter & 0x00000001) //left channel
			//{
			//	#ifdef DRY_MIX_AFTER_ECHO
			//	sample32 += sample_i16 + (int16_t)(sample_f[1]*MAIN_VOLUME*SAMPLE_VOLUME);
			//	#else
			//	sample32 += sample_i16;
			//	#endif
			//}
			//else
			//{
				#ifdef DRY_MIX_AFTER_ECHO
				//sample32 = (sample_i16 + (int16_t)(sample_f[0]*MAIN_VOLUME*SAMPLE_VOLUME)) << 16;
				//#else
				//sample32 = sample_i16 << 16;
    			sample_i16 += (int16_t)(sample_f[0]*MAIN_VOLUME*SAMPLE_VOLUME);
				#endif
			//}
    	}

		sample32 = sample_i16 << 16;

		if(sample_i16 < -DYNAMIC_LIMITER_THRESHOLD && SAMPLE_VOLUME > 0)
		{
			SAMPLE_VOLUME -= DYNAMIC_LIMITER_STEPDOWN;
		}

		//sampleCounter++; //????????????????????????????? xxx

    	//---------------
		//PART 1B - LEFT
		//---------------
		//if (sampleCounter & 0x00000001) //left channel
		//{
			sample_f[1] = (float)((int16_t)(ADC_sample >> 16)) * OpAmp_ADC12_signal_conversion_factor;

			#ifdef USE_AUTOCORRELATION
			if(autocorrelation_rec_sample)
			{
				autocorrelation_buffer[--autocorrelation_rec_sample] = (int16_t)ADC_sample;
			}
			#endif

			//#ifdef USE_BG_SAMPLE
			if(use_bg_sample)
			{
				//------------- BG SAMPLE ----------------------------------
				sample_f[1] += ( (float)(32768 - (int16_t)mixed_sample_buffer[mixed_sample_buffer_ptr_R++]) / 32768.0f) * mixed_sample_volume;// * noise_volume * noise_boost_by_sensor;

				if(mixed_sample_buffer_ptr_R == MIXED_SAMPLE_BUFFER_LENGTH)
				{
					mixed_sample_buffer_ptr_R = 0;
				}
				//------------- BG SAMPLE ----------------------------------
			//#endif
			}
			else
			{
				//r_noise = r_noise + 19;
				//r_noise = r_noise * r_noise;

				i_noise = r_noise;
				r_noise = r_noise - i_noise;
				////return b_noise - 0.5;
			}

			if(wind_voices)
			{
				sample_mix = IIR_Filter::iir_filter_multi_sum_w_noise_and_wind(
					sample_f[1],
					fil->iir2 + FILTERS / 2,
					ACTIVE_FILTERS_PAIRS,
					fil->fp.mixing_volumes + FILTERS / 2,
					//(int16_t)(random_value >> 16),
					((uint16_t *)&r_noise)[1],
					//0,
					noise_volume * noise_boost_by_sensor,
					wind_voices
				) * MAIN_VOLUME;
			}
			else
			{
				sample_mix = IIR_Filter::iir_filter_multi_sum_w_noise(
					sample_f[1],
					fil->iir2 + FILTERS / 2,
					ACTIVE_FILTERS_PAIRS,
					fil->fp.mixing_volumes + FILTERS / 2,
					//(int16_t)(random_value >> 16),
					((uint16_t *)&r_noise)[1],
					//0,
					noise_volume * noise_boost_by_sensor
				) * MAIN_VOLUME;
			}

			sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);
		//}

		#ifdef LIMITER_DEBUG
		if(sample_i16<limiter_debug[4]) limiter_debug[4] = sample_i16;
		if(sample_i16>limiter_debug[5]) limiter_debug[5] = sample_i16;
		#endif

		//---------------
		//PART 2AB - BOTH
		if(PROG_add_echo)
		{
			filtered_channel_add_echo();
		}

		#ifdef LIMITER_DEBUG
		if(sample_i16<limiter_debug[6]) limiter_debug[6] = sample_i16;
		if(sample_i16>limiter_debug[7]) limiter_debug[7] = sample_i16;
		#endif

		//---------------
		//PART 3B and 3A - LEFT & RIGHT
    	if(use_reverb)
    	{
			//if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 24) //4Hz
			//if (TIMING_BY_SAMPLE_EVERY_500_MS == 24) //2Hz
			if (TIMING_BY_SAMPLE_EVERY_250_MS == 24) //2Hz
			{
				filtered_channel_adjust_reverb();
			}

			//if (sampleCounter & 0x00000001) //left channel
			//{
				//sample32 += sample_i16;
				//sample32 += (int16_t)((float)add_reverb(sample_i16) * reverb_volume + (float)sample_i16 * (1.0f-reverb_volume));
				sample_i16 = (int16_t)((float)add_reverb(sample_i16) * reverb_volume + (float)sample_i16 * (1.0f-reverb_volume));
			//}
			//else
			//{
			//	//sample32 = sample_i16 << 16;
			//	sample32 = ((int16_t)((float)add_reverb(sample_i16) * reverb_volume  + (float)sample_i16 * (1.0f-reverb_volume))) << 16;
			//}
    	}
    	else
    	{
			//if (sampleCounter & 0x00000001) //left channel
			//{
				#ifdef DRY_MIX_AFTER_ECHO
    			//sample32 += sample_i16 + (int16_t)(sample_f[1]*MAIN_VOLUME*SAMPLE_VOLUME);
				//#else
				//sample32 += sample_i16;
    			sample_i16 += (int16_t)(sample_f[1]*MAIN_VOLUME*SAMPLE_VOLUME);
				#endif
			//}
			//else
			//{
			//	#ifdef DRY_MIX_AFTER_ECHO
			//	sample32 = (sample_i16 + (int16_t)(sample_f[0]*MAIN_VOLUME*SAMPLE_VOLUME)) << 16;
			//	#else
			//	sample32 = sample_i16 << 16;
			//	#endif
			//}
    	}

    	sample32 += sample_i16;

		if(sample_i16 < -DYNAMIC_LIMITER_THRESHOLD && SAMPLE_VOLUME > 0)
		{
			SAMPLE_VOLUME -= DYNAMIC_LIMITER_STEPDOWN;
		}

		//=============================================================================================
    	//=============================== L/R channel block end =======================================
		//=============================================================================================

		/*
		if (a_sample_mix > signal_max)
		{
			signal_max = a_sample_mix;
		}
		if (a_sample_mix < signal_min)
		{
			signal_min = a_sample_mix;
		}

		if (a_sample_mix > signal_max_limit)
		{
			a_sample_mix = signal_max_limit;
		}
		if (a_sample_mix < signal_min_limit)
		{
			a_sample_mix = signal_min_limit;
		}
		*/

		if(add_beep)
		{
			if(volume_ramp)
			{
				sample32 = 0;
			}
			//if(sampleCounter & 0x10)
			if(sampleCounter & (1<<add_beep))
			{
				sample32 += (100 + (100<<16));
				//sample32 = (100 + (100<<16));
			}
		}

		//if (sampleCounter & 0x00000001) //left channel
		//{
	    	//n_bytes =
	    	i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
	    	sd_write_sample(sample32);

			//ADC_result =
			i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
			//int ADC_result = i2s_read_bytes(I2S_NUM, (char*)&ADC_sample, sizeof(ADC_sample), portMAX_DELAY);

			/*
			//if(ADC_result!=ESP_FAIL)
			if(ADC_result==4)
			{
				ADC_sample_valid = 1;
			}
			else
			{
				ADC_sample_valid = 0;
			}
			*/
		//}
		//else
		//{
		//}

		//#ifdef USE_DIRECT_WAVES_FILTERS
		if(use_direct_waves_filters)
		{
			//if (t_TIMING_BY_SAMPLE_EVERY_250_MS==21) //4Hz
			if (t_TIMING_BY_SAMPLE_EVERY_125_MS==21) //4Hz
				//if (TIMING_BY_SAMPLE_EVERY_100_MS==21) //10Hz
				//if (TIMING_BY_SAMPLE_EVERY_25_MS==21) //40Hz
			{
				//for (int i=0;i<WAVES_FILTERS;i++)
				for (int i=0;i<FILTERS/2;i++)
				{
					//param_i = ADC_last_result[i] + base_freq;
					param_i = (acc_res[i%ACC_RESULTS] + 0.9f) * 1000.0f;
					if (param_i < 10) {
						param_i = 10;
					}

					direct_update_filters_id[i] = i;
					direct_update_filters_freq[i] = param_i;
				}

				//printf("fil->start_update_filters_pairs(): updating %d filter pairs...\n",FILTERS/2);
				fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, FILTERS/2); //WAVES_FILTERS);
				//printf("fil->start_update_filters_pairs(): %d filter pairs updated!\n",FILTERS/2);
			}
			//#endif
		}
		else
		{
			//#ifdef USE_CHORD_PROGRESSION
			//melody
			//if (t_TIMING_BY_SAMPLE_EVERY_125_MS == SHIFT_MELODY_NOTE_TIMING_BY_SAMPLE) //4Hz periodically, at given sample
			melodyCounter--;
			if(melodyCounter==0)
			{
				melodyCounter = MELODY_BY_SAMPLE;

				if(fil->fp.melody_filter_pair >= 0)
				{
					fil->start_next_melody_note();
				}
			}

			//if((seconds % SHIFT_CHORD_INTERVAL == 0) && (sampleCounter==SHIFT_CHORD_TIMING_BY_SAMPLE)) //test - every 2 seconds, at given sample
			if((seconds % SHIFT_CHORD_INTERVAL == 0) && (tempoCounter==SHIFT_CHORD_TIMING_BY_SAMPLE))
			{
				#ifdef BOARD_GECHO
				display_chord(fil->chord->get_current_chord_LED(), 3);
				#endif
				fil->start_next_chord(wind_voices);
			}
			//#endif

			//if (a_sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
			//if (TIMING_BY_SAMPLE_EVERY_100_MS == 13) //10Hz periodically, at given sample
			if (TIMING_BY_SAMPLE_EVERY_50_MS == 13) //10Hz periodically, at given sample
			{
				if(wind_voices)
				{
					wind_cutoff_sweep++;
					wind_cutoff_sweep %= WIND_FILTERS;

					wind_cutoff[wind_cutoff_sweep] += 0.005 - (float)(0.01 * ((i_noise) & 0x00000001));
					if (wind_cutoff[wind_cutoff_sweep] < wind_cutoff_limit[0])
					{
						wind_cutoff[wind_cutoff_sweep] = wind_cutoff_limit[0];
					}
					else if (wind_cutoff[wind_cutoff_sweep] > wind_cutoff_limit[1])
					{
						wind_cutoff[wind_cutoff_sweep] = wind_cutoff_limit[1];
					}

					/*
					mixing_volumes[cutoff_sweep] += mixing_deltas[cutoff_sweep] * 0.01f;
					if (mixing_volumes[cutoff_sweep] < 0)
					{
						mixing_volumes[cutoff_sweep] = 0;
					}
					else if (mixing_volumes[cutoff_sweep] > 1)
					{
						mixing_volumes[cutoff_sweep] = 1;
					}
					 */

					fil->iir2[wind_cutoff_sweep]->setCutoff(wind_cutoff[wind_cutoff_sweep]);
				}
			}
		}

		//#ifdef USE_ACCELEROMETER

		//if (t_TIMING_BY_SAMPLE_EVERY_125_MS == 97) //4Hz
		if(melodyCounter == SHIFT_ARP_TIMING_BY_SAMPLE)
		{
			if(!use_direct_waves_filters)
			{
				//#ifdef USE_ARPEGGIATOR

				if(fixed_arp_level || acc_res[0] < -0.2f)
				{
					if(fil->fp.arpeggiator_filter_pair==-1)
					{
						fil->fp.arpeggiator_filter_pair = DEFAULT_ARPEGGIATOR_FILTER_PAIR;
						//fil->iir2[0].setCutoff(fil->chord->chords[fil->chord->current_chord].freqs[0]);
						//fil->iir2[arpeggiator_pair].setResonanceKeepFeedback(0.999f);
						//fil->iir2[arpeggiator_pair+FILTERS/2].setResonanceKeepFeedback(0.999f);

						//printf("ARP on\n");
					}

					//fil->fp.melody_filter_pair = 0;
					int next_chord = fil->chord->current_chord-1;
					if(next_chord > fil->chord->total_chords)
					{
						next_chord = 0;
					}
					else if(next_chord < 0)
					{
						next_chord = fil->chord->total_chords-1;
					}

					//BUG!!
					//fil->add_to_update_filters_pairs(0,fil->chord->chords[next_chord].freqs[arpeggiator_loop]);
					fil->add_to_update_filters_pairs(fil->fp.arpeggiator_filter_pair,fil->chord->chords[next_chord].freqs[arpeggiator_loop], fil->fp.tuning_arp_l, fil->fp.tuning_arp_r);

					arpeggiator_loop++;
					if(arpeggiator_loop==CHORD_MAX_VOICES)
					{
						arpeggiator_loop=0;
					}

					if(fixed_arp_level || acc_res[0] < -0.4f)
					{
						if(fixed_arp_level)
						{
							//new_mixing_vol = 1.0f + ((float)fixed_arp_level / FIXED_ARP_LEVEL_MAX) * 3.333f; //good for wind channel
							new_mixing_vol = 1.0f + ((float)fixed_arp_level / FIXED_ARP_LEVEL_MAX) * ARP_VOLUME_MULTIPLIER_MANUAL; //good for wind channel
						}
						else
						{
							//new_mixing_vol = 1.0f + (-acc_res[0] - 0.4f) * 1.6f; //default for sea and forest
							//new_mixing_vol = 1.0f + (-acc_res[0] - 0.4f) * 2.4f;
							//new_mixing_vol = 1.0f + (-acc_res[0] - 0.4f) * 3.333f; //good for wind channel
							new_mixing_vol = 1.0f + (-acc_res[0] - 0.4f) * ARP_VOLUME_MULTIPLIER_DEFAULT; //good for wind channel
						}

						//fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair] = 2.0f;
						//fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair+FILTERS/2] = 2.0f;
						fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair] = new_mixing_vol;
						fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair+FILTERS/2] = new_mixing_vol;

						fil->iir2[fil->fp.arpeggiator_filter_pair]->setResonance(0.999f);//setResonanceKeepFeedback(0.999f);
						fil->iir2[fil->fp.arpeggiator_filter_pair+FILTERS/2]->setResonance(0.999f);//setResonanceKeepFeedback(0.999f);
					}

				}
				else
				{
					if(fil->fp.arpeggiator_filter_pair>=0)
					{
						//volume back to normal
						fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair] = 1.0f;
						fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair+FILTERS/2] = 1.0f;

						//back to default resonance
						fil->iir2[fil->fp.arpeggiator_filter_pair]->setResonanceKeepFeedback(0.99f);
						fil->iir2[fil->fp.arpeggiator_filter_pair+FILTERS/2]->setResonanceKeepFeedback(0.99f); //refactor

						fil->fp.arpeggiator_filter_pair = -1;

						//printf("ARP off\n");
					}
					//fil->fp.melody_filter_pair = -1;
				}

				//#endif

			}
			else
			{
				if(acc_res[0] > 0.2f)
				{
					reverb_volume = 1.0f;
				}
				else
				{
					//acc_res[2] <= -0.2f -> from -1 to -0.2, needs to map to 0..1
					reverb_volume = (acc_res[0] + 1.0f) * (1/0.8f);
				}
			}

			//map accelerometer values of y-axis (-1..1) to sample mixing volume (0..2)
			//mixed_sample_volume = 1 - acc_res[1];
			/*
			if(acc_res[1]<0)
			{
				mixed_sample_volume = 1 - 3*acc_res[1]; //to sample mixing volume (0..4)
			}
			*/
			//map accelerometer values of y-axis (-1..1) to sample mixing volume (0..1)
			mixed_sample_volume = (1 - acc_res[1]) * mixed_sample_volume_coef; //0.5f;

			noise_boost_by_sensor = mixed_sample_volume;

			if(!echo_dynamic_loop_current_step) //if a fixed delay is not set, use accelerometer
			{
				if(acc_res[2] > -0.2f)
				{
					if(acc_res[2] > 0.95f)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 64;
					}
					else if(acc_res[2] > 0.83f)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 32;
					}
					else if(acc_res[2] > 0.7f)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 16;
					}
					else if(acc_res[2] > 0.5f)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 8;
					}
					else if(acc_res[2] > 0.3f)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 4;
					}
					else if(acc_res[2] > 0.0f)
					{
						if(tempo_bpm < global_settings.TEMPO_BPM_DEFAULT)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 3;
						}
						else
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 2;
						}
					}
					else
					{
						if(tempo_bpm < global_settings.TEMPO_BPM_DEFAULT)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 2;
						}
						else
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO;
						}
					}
				}

				/*
				//map accelerometer values of z-axis (-0.3..1) to echo delay length (I2S_AUDIOFREQ..I2S_AUDIOFREQ/128 cca)
				if(acc_res[2] > -0.3f)
				{
					//normalized difference = acc_res[2] + 0.3 => 1 to 130
					new_delay_length = 1 + (acc_res[2] + 0.3f) * 100;
					echo_dynamic_loop_length = I2S_AUDIOFREQ / (int)new_delay_length;
				}
				*/
				else
				{
					if(tempo_bpm < global_settings.TEMPO_BPM_DEFAULT)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO;
					}
					else
					{
						echo_dynamic_loop_length = (DELAY_BY_TEMPO * 3) / 2;
					}
				}
				//printf("echo_dynamic_loop_length = %d\n", echo_dynamic_loop_length);
				//}
			}

			if(short_press_volume_minus && !context_menu_active && !settings_menu_active)
			{
				short_press_volume_minus = 0;
				set_tuning(TUNING_ALL_432);
				global_tuning = 432.0f;
				printf("Tuning set to %f\n", global_tuning);
				persistent_settings_store_tuning();
			}
			if(short_press_volume_plus && !context_menu_active && !settings_menu_active)
			{
				short_press_volume_plus = 0;
				set_tuning(TUNING_ALL_440);
				global_tuning = 440.0f;
				printf("Tuning set to %f\n", global_tuning);
				persistent_settings_store_tuning();
			}
			if(short_press_sequence==2 && !context_menu_active && !settings_menu_active)
			{
				short_press_sequence = 0;
				codec_set_mute(1); //this takes a while so will stop the sound so there is no buzzing
				randomize_song_and_melody();
				codec_set_mute(0);
				printf("randomize_song_and_melody() done\n");
			}
			if(short_press_sequence==-3 && !context_menu_active && !settings_menu_active)
			{
				short_press_sequence = 0;
				fil->release();
				fil->setup(FILTERS_TYPE_LOW_PASS + FILTERS_ORDER_4);
				printf("fil->setup(): LOW_PASS\n");
			}
			if(short_press_sequence==3 && !context_menu_active && !settings_menu_active)
			{
				short_press_sequence = 0;
				fil->release();
				fil->setup(FILTERS_TYPE_HIGH_PASS + FILTERS_ORDER_4);
				printf("fil->setup(): HIGH_PASS\n");
			}
			if(short_press_sequence==-4 && !context_menu_active && !settings_menu_active)
			{
				short_press_sequence = 0;
				fil->fp.resonance -= FILTERS_RESONANCE_STEP;
				fil->update_resonance();
				printf("fil->setup(): resonance=%f\n", fil->fp.resonance);
			}
			if(short_press_sequence==4 && !context_menu_active && !settings_menu_active)
			{
				short_press_sequence = 0;
				fil->fp.resonance += FILTERS_RESONANCE_STEP;
				if(fil->fp.resonance > FILTERS_RESONANCE_HIGHEST)
				{
					fil->fp.resonance = FILTERS_RESONANCE_HIGHEST;
				}
				fil->update_resonance();
				printf("fil->setup(): resonance=%f\n", fil->fp.resonance);
			}
		}

		//if (sampleCounter%(2*I2S_AUDIOFREQ/PROGRESS_UPDATE_FILTERS_RATE)==27) //every X ms, at 0.213 ms
		//if (TIMING_BY_SAMPLE_EVERY_10_MS==27) //100Hz
		if (TIMING_BY_SAMPLE_EVERY_5_MS==27) //100Hz
		{
			fil->progress_update_filters(fil, false);

			if(SAMPLE_VOLUME < SAMPLE_VOLUME_DEFAULT)
			{
				SAMPLE_VOLUME += DYNAMIC_LIMITER_RECOVER;
			}
		}

//#endif

		sampleCounter++;
        tempoCounter--;

        //if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION) //one full second passed
		if(tempoCounter==0)
    	{
    		seconds++;
    		sampleCounter = 0;
    		//printf("1sec... \n");
			tempoCounter = TEMPO_BY_SAMPLE;

			#ifdef BOARD_WHALE
			check_auto_power_off();
			#endif

			#ifdef LIMITER_DEBUG
			printf("limiter_debug: %d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%f\n",
				limiter_debug[0],
				limiter_debug[1],
				limiter_debug[2],
				limiter_debug[3],
				limiter_debug[4],
				limiter_debug[5],
				limiter_debug[6],
				limiter_debug[7],
				SAMPLE_VOLUME);

			limiter_debug[0] = 32760;
			limiter_debug[1] = -32760;
			limiter_debug[2] = 32760;
			limiter_debug[3] = -32760;
			limiter_debug[4] = 32760;
			limiter_debug[5] = -32760;
			limiter_debug[6] = 32760;
			limiter_debug[7] = -32760;
			#endif
    	}

    } //end skip channel cnt
}

void filtered_channel_add_echo()
{
	//wrap the echo loop
	echo_buffer_ptr0++;
	if(echo_buffer_ptr0 >= echo_dynamic_loop_length)
	{
		echo_buffer_ptr0 = 0;
	}

	echo_buffer_ptr = echo_buffer_ptr0 + 1;
	if(echo_buffer_ptr >= echo_dynamic_loop_length)
	{
		echo_buffer_ptr = 0;
	}

	//add echo from the loop
	echo_mix_f = float(sample_i16) + float(echo_buffer[echo_buffer_ptr]) * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;

	if(echo_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER)
	{
		echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER;
	}

	if(echo_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER)
	{
		echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER;
	}

	sample_i16 = (int16_t)echo_mix_f;

	//store result to echo, the amount defined by a fragment
	//echo_mix_f = ((float)sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV);
	//echo_mix_f *= ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
	//echo_buffer[echo_buffer_ptr0] = (int16_t)echo_mix_f;
	echo_buffer[echo_buffer_ptr0] = sample_i16;
}

void filtered_channel_adjust_reverb()
{
	//if(acc_res[1] < -0.3f)
	//{
		bit_crusher_reverb+=3;

		if(bit_crusher_reverb>BIT_CRUSHER_REVERB_MAX)
		{
			bit_crusher_reverb = BIT_CRUSHER_REVERB_MAX;
		}
	/*
	}
	else if(acc_res[1] < 0.1f)
	{
		bit_crusher_reverb++;

		if(bit_crusher_reverb>BIT_CRUSHER_REVERB_MAX)
		{
			bit_crusher_reverb = BIT_CRUSHER_REVERB_MAX;
		}
	}
	else if(acc_res[1] < 0.4f)
	{

	}
	else
	{
		bit_crusher_reverb--;

		if(bit_crusher_reverb<BIT_CRUSHER_REVERB_MIN)
		{
			bit_crusher_reverb = BIT_CRUSHER_REVERB_MIN;
		}
	}
	*/

	reverb_dynamic_loop_length = bit_crusher_reverb;
	//reverb_dynamic_loop_length = I2S_AUDIOFREQ / bit_crusher_reverb;
}
