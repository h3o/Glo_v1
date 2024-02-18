/*
 * Reverb.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 16 Jul 2018
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

#include <Reverb.h>
#include <Accelerometer.h>
#include <Interface.h>
#include <InitChannels.h>
#include <FilteredChannels.h>
#include <hw/signals.h>
#include <hw/gpio.h>
#include <hw/sdcard.h>
#include <hw/leds.h>
#include <string.h>
#include "hw/midi.h"

//#define CLEAR_OLD_ECHO_DATA //when buffer expands

#ifdef BOARD_WHALE
//sensor 3 - echo delay length
#define ACC_SENSOR_DELAY_9					(acc_res[2] > 0.95f)
#define ACC_SENSOR_DELAY_8					(acc_res[2] > 0.85f)
#define ACC_SENSOR_DELAY_7					(acc_res[2] > 0.74f)
#define ACC_SENSOR_DELAY_6					(acc_res[2] > 0.62f)
#define ACC_SENSOR_DELAY_5					(acc_res[2] > 0.49f)
#define ACC_SENSOR_DELAY_4					(acc_res[2] > 0.35f)
#define ACC_SENSOR_DELAY_3					(acc_res[2] > 0.2f)
#define ACC_SENSOR_DELAY_2					(acc_res[2] > 0.0f)
#define ACC_SENSOR_DELAY_ACTIVE				(acc_res[2] > -0.2f)
#else
//sensor 3 - echo delay length
#define ACC_SENSOR_DELAY_9					(acc_res[0] > 0.95f)
#define ACC_SENSOR_DELAY_8					(acc_res[0] > 0.85f)
#define ACC_SENSOR_DELAY_7					(acc_res[0] > 0.74f)
#define ACC_SENSOR_DELAY_6					(acc_res[0] > 0.62f)
#define ACC_SENSOR_DELAY_5					(acc_res[0] > 0.49f)
#define ACC_SENSOR_DELAY_4					(acc_res[0] > 0.35f)
#define ACC_SENSOR_DELAY_3					(acc_res[0] > 0.2f)
#define ACC_SENSOR_DELAY_2					(acc_res[0] > 0.0f)
#define ACC_SENSOR_DELAY_ACTIVE				(acc_res[0] > -0.2f)
#endif

//sensor 1 - echo delay length
#define IRS_SENSOR_DELAY_9					SENSOR_THRESHOLD_RED_9
#define IRS_SENSOR_DELAY_8					SENSOR_THRESHOLD_RED_8
#define IRS_SENSOR_DELAY_7					SENSOR_THRESHOLD_RED_7
#define IRS_SENSOR_DELAY_6					SENSOR_THRESHOLD_RED_6
#define IRS_SENSOR_DELAY_5					SENSOR_THRESHOLD_RED_5
#define IRS_SENSOR_DELAY_4					SENSOR_THRESHOLD_RED_4
#define IRS_SENSOR_DELAY_3					SENSOR_THRESHOLD_RED_3
#define IRS_SENSOR_DELAY_2					SENSOR_THRESHOLD_RED_2
#define IRS_SENSOR_DELAY_ACTIVE				SENSOR_THRESHOLD_RED_1

int bit_crusher_reverb;

void decaying_reverb(int extended_buffers)
{
	channel_init(0, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0, extended_buffers, 1); //init without any features, possibly use extra buffers, use reverb

    ECHO_MIXING_GAIN_MUL = 9; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 10; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

	#ifdef ENABLE_MCLK_AT_CHANNEL_INIT
    enable_out_clock(); //better always enabled here, otherwise getting bad glitch
	mclk_enabled = 1;
	#endif

	#ifdef SWITCH_I2C_SPEED_MODES
	i2c_master_deinit();
    i2c_master_init(1); //fast mode
	#endif

	int bit_crusher_reverb_d = BIT_CRUSHER_REVERB_DEFAULT;

	#define DECAY_DIR_FROM		-4
	#define DECAY_DIR_TO		4
	#define DECAY_DIR_DEFAULT	1
	int decay_direction = DECAY_DIR_DEFAULT;

	#ifdef BOARD_WHALE
    RGB_LED_R_ON;
	#endif

	int lock_sensors = 0;
	int midi_override = 0, MIDI_multi_cc_override = 0;
	float MIDI_pitch_bend = 0.0f, MIDI_pitch_base[3] = {69.0f,69.0f/2.0f,69.0f*2.0f}; //A4 is MIDI note #69

	echo_dynamic_loop_length0 = echo_dynamic_loop_length;

	while(!event_next_channel)
	{
		sampleCounter++;

		t_TIMING_BY_SAMPLE_EVERY_250_MS = TIMING_EVERY_250_MS;

		#define REVERB_UI_CMD_DECREASE_DECAY	1
		#define REVERB_UI_CMD_INCREASE_DECAY	2

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 27) //4Hz
		{
			ui_command = 0;

			//printf("Reverb(): btn_event_ext = %d\n", btn_event_ext);

			//map UI commands
			#ifdef BOARD_WHALE
			if(short_press_volume_plus) { ui_command = REVERB_UI_CMD_DECREASE_DECAY; short_press_volume_plus = 0; }
			if(short_press_volume_minus) { ui_command = REVERB_UI_CMD_INCREASE_DECAY; short_press_volume_minus = 0; }
			#endif
			#ifdef BOARD_GECHO
			if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = REVERB_UI_CMD_INCREASE_DECAY; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = REVERB_UI_CMD_DECREASE_DECAY; btn_event_ext = 0; }
			#endif
		}

		//if (TIMING_BY_SAMPLE_EVERY_125_MS == 24) //8Hz
		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 31) //4Hz
		{
			bit_crusher_reverb_d += decay_direction;

			if(bit_crusher_reverb_d>BIT_CRUSHER_REVERB_MAX)
			{
				bit_crusher_reverb_d = BIT_CRUSHER_REVERB_MAX;
			}
			if(bit_crusher_reverb_d<BIT_CRUSHER_REVERB_MIN)
			{
				bit_crusher_reverb_d = BIT_CRUSHER_REVERB_MIN;
			}

			if(SENSOR_THRESHOLD_WHITE_8)
			{
				reverb_dynamic_loop_length = (1.0f-ir_res[3]) * 0.9f * (float)BIT_CRUSHER_REVERB_MAX;
				if(reverb_dynamic_loop_length<BIT_CRUSHER_REVERB_MIN)
				{
					reverb_dynamic_loop_length = BIT_CRUSHER_REVERB_MIN;
				}
				//printf("Reverb(): reverb_dynamic_loop_length = %d\n", reverb_dynamic_loop_length);
			}
			else if(!midi_override)// && !MIDI_multi_cc_override)
			{
				reverb_dynamic_loop_length = bit_crusher_reverb_d;
			}

			#ifdef REVERB_OCTAVE_UP_DOWN
			reverb_dynamic_loop_length_ext[0] = BIT_CRUSHER_REVERB_CALC_EXT(0, reverb_dynamic_loop_length);
			reverb_dynamic_loop_length_ext[1] = BIT_CRUSHER_REVERB_CALC_EXT(1, reverb_dynamic_loop_length);
			#endif

			#ifdef REVERB_POLYPHONIC

			if(!midi_override)
			{
				reverb_dynamic_loop_length_ext[0] = reverb_dynamic_loop_length / 2;
				reverb_dynamic_loop_length_ext[1] = reverb_dynamic_loop_length * 2;

				if(reverb_dynamic_loop_length_ext[0]<BIT_CRUSHER_REVERB_MIN_EXT(0))
				{
					reverb_dynamic_loop_length_ext[0] = BIT_CRUSHER_REVERB_MIN_EXT(0);
				}
				if(reverb_dynamic_loop_length_ext[0]>BIT_CRUSHER_REVERB_MAX_EXT(0))
				{
					reverb_dynamic_loop_length_ext[0] = BIT_CRUSHER_REVERB_MAX_EXT(0);
				}

				if(reverb_dynamic_loop_length_ext[1]<BIT_CRUSHER_REVERB_MIN_EXT(1))
				{
					reverb_dynamic_loop_length_ext[1] = BIT_CRUSHER_REVERB_MIN_EXT(1);
				}
				if(reverb_dynamic_loop_length_ext[1]>BIT_CRUSHER_REVERB_MAX_EXT(1))
				{
					reverb_dynamic_loop_length_ext[1] = BIT_CRUSHER_REVERB_MAX_EXT(1);
				}
			}
			#endif
		}

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 29) //4Hz
		{
			if(event_channel_options) //stops decay
			{
				decay_direction = 0; //DECAY_DIR_DEFAULT;
				event_channel_options = 0;
				//printf("Reverb(): decay_direction = 0\n");
			}
			if(ui_command==REVERB_UI_CMD_DECREASE_DECAY) //as delay length decreases, perceived tone goes up
			{
				midi_override = 0; //reset the override so the decay can continue
				decay_direction--;
				if(decay_direction < DECAY_DIR_FROM)
				{
					decay_direction = DECAY_DIR_FROM;
				}
				//#ifdef BOARD_GECHO
				//indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_LEFT_8, 1, 50);
				//#endif
				//printf("Reverb(): decay_direction-- %d\n", decay_direction);
			}
			if(ui_command==REVERB_UI_CMD_INCREASE_DECAY) //as delay length increases, perceived tone goes low
			{
				midi_override = 0; //reset the override so the decay can continue
				decay_direction++;
				if(decay_direction > DECAY_DIR_TO)
				{
					decay_direction = DECAY_DIR_TO;
				}
				//#ifdef BOARD_GECHO
				//indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_RIGHT_8, 1, 50);
				//#endif
				//printf("Reverb(): decay_direction++ %d\n", decay_direction);
			}
			#ifdef BOARD_GECHO
			if(!decay_direction) { LED_R8_set_byte(0x18); }
			else if(decay_direction == -1) { LED_R8_set_byte(0x10); }
			else if(decay_direction == -2) { LED_R8_set_byte(0x20); }
			else if(decay_direction == -3) { LED_R8_set_byte(0x40); }
			else if(decay_direction == -4) { LED_R8_set_byte(0x80); }
			else if(decay_direction == 1) { LED_R8_set_byte(0x08); }
			else if(decay_direction == 2) { LED_R8_set_byte(0x04); }
			else if(decay_direction == 3) { LED_R8_set_byte(0x02); }
			else if(decay_direction == 4) { LED_R8_set_byte(0x01); }
			#endif
		}
		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 12695/2) //after 0.5s
		{
			LED_R8_all_OFF();
		}

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 27 && !MIDI_multi_cc_override) //4Hz
		{
			if(use_acc_or_ir_sensors==PARAMETER_CONTROL_SENSORS_ACCELEROMETER)
			{
				if(acc_res[1]>0)
				{
					ADC_LPF_ALPHA = acc_res[1] / 2 + 0.5f;
				}
				else if(acc_res[1]>-0.2f)  { ADC_LPF_ALPHA = 0.4f; }
				else if(acc_res[1]>-0.3f)  { ADC_LPF_ALPHA = 0.3f; }
				else if(acc_res[1]>-0.45f) { ADC_LPF_ALPHA = 0.2f; }
				else if(acc_res[1]>-0.65f) { ADC_LPF_ALPHA = 0.1f; }
				else if(acc_res[1]>-0.85f) { ADC_LPF_ALPHA = 0.05f;}
			}
			else //if(use_acc_or_ir_sensors==PARAMETER_CONTROL_SENSORS_IRS)
			{
				/*
				if(SENSOR_THRESHOLD_WHITE_1) { ADC_LPF_ALPHA = 1.0f; }
				else if(SENSOR_THRESHOLD_WHITE_2) { ADC_LPF_ALPHA = 0.9f; }
				else if(SENSOR_THRESHOLD_WHITE_3) { ADC_LPF_ALPHA = 0.8f; }
				else if(SENSOR_THRESHOLD_WHITE_4) { ADC_LPF_ALPHA = 0.7f; }
				else if(SENSOR_THRESHOLD_WHITE_5) { ADC_LPF_ALPHA = 0.6f; }
				else if(SENSOR_THRESHOLD_WHITE_6) { ADC_LPF_ALPHA = 0.5f; }
				else if(SENSOR_THRESHOLD_WHITE_7) { ADC_LPF_ALPHA = 0.4f; }
				else if(SENSOR_THRESHOLD_WHITE_8) { ADC_LPF_ALPHA = 0.3f; }
				else { ADC_LPF_ALPHA = 0.2f; }
				*/
				if(SENSOR_THRESHOLD_BLUE_1) { ADC_LPF_ALPHA = 1.0f; }
				else if(SENSOR_THRESHOLD_BLUE_2) { ADC_LPF_ALPHA = 0.8f; }
				else if(SENSOR_THRESHOLD_BLUE_3) { ADC_LPF_ALPHA = 0.6f; }
				else if(SENSOR_THRESHOLD_BLUE_4) { ADC_LPF_ALPHA = 0.4f; }
				else if(SENSOR_THRESHOLD_BLUE_5) { ADC_LPF_ALPHA = 0.3f; }
				else { ADC_LPF_ALPHA = 0.2f; }
				//printf("Reverb(): ADC_LPF_ALPHA=%f\n", ADC_LPF_ALPHA);
			}
		}

		/*
		//if (TIMING_BY_SAMPLE_EVERY_125_MS == 26) //8Hz
		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 26) //4Hz
		{
			printf("Reverb(): bit_crusher_reverb_d=%d,len=%d\n",bit_crusher_reverb_d,reverb_dynamic_loop_length);
		}
		*/

		ADC_sample = ADC_sampleA[ADC_sample_ptr];
		ADC_sample_ptr++;

		if(ADC_sample_ptr==ADC_SAMPLE_BUFFER)
		{
			i2s_read(I2S_NUM, (void*)ADC_sampleA, 4*ADC_SAMPLE_BUFFER, &i2s_bytes_rw, 1);
			ADC_sample_ptr = 0;
		}

		/*
		if(sampleCounter%bit_crusher_div==0)
		{
			ADC_sample0 = ADC_sample;
		}
		else
		{
			ADC_sample = ADC_sample0;
		}
		*/

		//sample_mix = (int16_t)ADC_sample;
		//sample_mix = sample_mix * SAMPLE_VOLUME_REVERB; //apply volume
		//LPF enabled
		sample_lpf[0] = sample_lpf[0] + ADC_LPF_ALPHA * ((float)((int16_t)ADC_sample) - sample_lpf[0]);
		sample_mix = sample_lpf[0] * SAMPLE_VOLUME_REVERB; //apply volume


		//sample32 = (add_echo((int16_t)(sample_mix))) << 16;
		//sample32 = (add_reverb((int16_t)(sample_mix))) << 16;
		if(extended_buffers)
		{
			//sample32 = (add_echo(add_reverb_ext(add_reverb((int16_t)(sample_mix))))) << 16;
			//sample32 = (add_echo(add_reverb((int16_t)(sample_mix)) + add_reverb_ext((int16_t)(sample_mix)))) << 16; //does not sound as expected
			sample32 = (add_echo(add_reverb_ext_all3((int16_t)(sample_mix)))) << 16;
		}
		else
		{
			sample32 = (add_echo(add_reverb((int16_t)(sample_mix)))) << 16;
		}

        //sample_mix = (int16_t)(ADC_sample>>16);
        //sample_mix = sample_mix * SAMPLE_VOLUME_REVERB; //apply volume
		//LPF enabled
		sample_lpf[1] = sample_lpf[1] + ADC_LPF_ALPHA * ((float)((int16_t)(ADC_sample>>16)) - sample_lpf[1]);
		sample_mix = sample_lpf[1] * SAMPLE_VOLUME_REVERB; //apply volume

        //sample32 += add_echo((int16_t)(sample_mix));
        //sample32 += add_reverb((int16_t)(sample_mix));
		if(extended_buffers)
		{
			//sample32 += add_echo(add_reverb_ext(add_reverb((int16_t)(sample_mix))));
			//sample32 += add_echo(add_reverb((int16_t)(sample_mix)) + add_reverb_ext((int16_t)(sample_mix))); //does not sound as expected
			sample32 += add_echo(add_reverb_ext_all3((int16_t)(sample_mix)));
		}
		else
		{
			sample32 += add_echo(add_reverb((int16_t)(sample_mix)));
		}

		#ifdef BOARD_WHALE
        if(add_beep)
		{
			if(sampleCounter & (1<<add_beep))
			{
				sample32 += (100 + (100<<16));
			}
		}
		#endif

		i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
		sd_write_sample(&sample32);

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 35) //4Hz
		{
			if(!lock_sensors && !echo_dynamic_loop_current_step && !MIDI_controllers_active_CC) //if sensors not locked, fixed delay is not set and it is not overriden by MIDI CC wheel, use accelerometer or IR sensors
			{
				if(use_acc_or_ir_sensors==PARAMETER_CONTROL_SENSORS_ACCELEROMETER)
				{
					if(ACC_SENSOR_DELAY_ACTIVE)
					{
						if(ACC_SENSOR_DELAY_9)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 64;
						}
						else if(ACC_SENSOR_DELAY_8)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 32;
						}
						else if(ACC_SENSOR_DELAY_7)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 16;
						}
						else if(ACC_SENSOR_DELAY_6)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 8;
						}
						else if(ACC_SENSOR_DELAY_5)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 4;
						}
						else if(ACC_SENSOR_DELAY_4)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 3;
						}
						else if(ACC_SENSOR_DELAY_3)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 2;
						}
						else if(ACC_SENSOR_DELAY_2)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 3 * 2;
						}
						else
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO;
						}

						if(tempo_bpm < global_settings.TEMPO_BPM_DEFAULT)
						{
							echo_dynamic_loop_length /= 2;
						}
					}
					else
					{
						if(tempo_bpm < global_settings.TEMPO_BPM_DEFAULT)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO;
						}
						else //if 120BPM or faster, can expand to 3/2 of the 1-sec buffer length
						{
							echo_dynamic_loop_length = (DELAY_BY_TEMPO * 3) / 2;
						}
					}
				}
				else //if(use_acc_or_ir_sensors==PARAMETER_CONTROL_SENSORS_IRS)
				{
					if(IRS_SENSOR_DELAY_ACTIVE)
					{
						if(IRS_SENSOR_DELAY_9)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 64;
						}
						else if(IRS_SENSOR_DELAY_8)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 32;
						}
						else if(IRS_SENSOR_DELAY_7)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 16;
						}
						else if(IRS_SENSOR_DELAY_6)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 8;
						}
						else if(IRS_SENSOR_DELAY_5)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 4;
						}
						else if(IRS_SENSOR_DELAY_4)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 3;
						}
						else if(IRS_SENSOR_DELAY_3)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 2;
						}
						else if(IRS_SENSOR_DELAY_2)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 3 * 2;
						}
						else
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO;
						}

						if(tempo_bpm < global_settings.TEMPO_BPM_DEFAULT)
						{
							echo_dynamic_loop_length /= 2;
						}
					}
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
				}
				//printf("Reverb(): echo_dynamic_loop_length = %d\n", echo_dynamic_loop_length);
			}
		}

		#ifdef CLEAR_OLD_ECHO_DATA
		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 37) //4Hz
		{
			if(echo_dynamic_loop_length0 < echo_dynamic_loop_length)
			{
				//printf("Cleaning echo buffer from echo_dynamic_loop_length0 to echo_dynamic_loop_length\n");
				//echo_length_updated = echo_dynamic_loop_length0;
				//echo_dynamic_loop_length0 = echo_dynamic_loop_length;
			//}
			//if(echo_length_updated)
			//{
				//memset(echo_buffer+echo_dynamic_loop_length0*sizeof(int16_t),0,(echo_dynamic_loop_length-echo_dynamic_loop_length0)*sizeof(int16_t)); //clear memory
				//echo_length_updated = 0;
				echo_skip_samples = echo_dynamic_loop_length - echo_dynamic_loop_length0;
				echo_skip_samples_from = echo_dynamic_loop_length0;
			}
			echo_dynamic_loop_length0 = echo_dynamic_loop_length;
		}
		#endif

		if (TIMING_EVERY_20_MS == 33) //50Hz
		{
			if(limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT)
			{
				limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //limiter will fully recover within 0.4 second
			}
		}

		if (TIMING_EVERY_10_MS == 419) //100Hz
		{
			if(!midi_override && MIDI_keys_pressed)
			{
				printf("Reverb(): MIDI active, will receive notes\n");
				midi_override = 1;
				decay_direction = 0;
			}

			if(midi_override)
			{
				if(MIDI_notes_updated)
				{
					LED_W8_all_OFF();
					LED_B5_all_OFF();

					if(MIDI_last_chord[0])
					{
						MIDI_to_LED(MIDI_last_chord[0], 1);

						MIDI_pitch_base[0] = (float)MIDI_last_chord[0];
						reverb_dynamic_loop_length = (float)SAMPLE_RATE_DEFAULT / MIDI_note_to_freq_ft(MIDI_pitch_base[0]+MIDI_pitch_bend);

						if(reverb_dynamic_loop_length<BIT_CRUSHER_REVERB_MIN)
						{
							reverb_dynamic_loop_length = BIT_CRUSHER_REVERB_MIN;
						}
						if(reverb_dynamic_loop_length>BIT_CRUSHER_REVERB_MAX)
						{
							reverb_dynamic_loop_length = BIT_CRUSHER_REVERB_MAX;
						}

						#ifdef REVERB_OCTAVE_UP_DOWN
						reverb_dynamic_loop_length_ext[0] = BIT_CRUSHER_REVERB_CALC_EXT(0, reverb_dynamic_loop_length);
						reverb_dynamic_loop_length_ext[1] = BIT_CRUSHER_REVERB_CALC_EXT(1, reverb_dynamic_loop_length);
						#endif

						#ifdef REVERB_POLYPHONIC

						if(MIDI_last_chord[1])
						{
							MIDI_to_LED(MIDI_last_chord[1], 1);

							MIDI_pitch_base[1] = (float)MIDI_last_chord[1];
							reverb_dynamic_loop_length_ext[0] = (float)SAMPLE_RATE_DEFAULT / MIDI_note_to_freq_ft(MIDI_pitch_base[1]+MIDI_pitch_bend);

							if(reverb_dynamic_loop_length_ext[0]<BIT_CRUSHER_REVERB_MIN_EXT(0))
							{
								reverb_dynamic_loop_length_ext[0] = BIT_CRUSHER_REVERB_MIN_EXT(0);
							}
							if(reverb_dynamic_loop_length_ext[0]>BIT_CRUSHER_REVERB_MAX_EXT(0))
							{
								reverb_dynamic_loop_length_ext[0] = BIT_CRUSHER_REVERB_MAX_EXT(0);
							}
						}

						if(MIDI_last_chord[2])
						{
							MIDI_to_LED(MIDI_last_chord[2], 1);

							MIDI_pitch_base[2] = (float)MIDI_last_chord[2];
							reverb_dynamic_loop_length_ext[1] = (float)SAMPLE_RATE_DEFAULT / MIDI_note_to_freq_ft(MIDI_pitch_base[2]+MIDI_pitch_bend);

							if(reverb_dynamic_loop_length_ext[1]<BIT_CRUSHER_REVERB_MIN_EXT(1))
							{
								reverb_dynamic_loop_length_ext[1] = BIT_CRUSHER_REVERB_MIN_EXT(1);
							}
							if(reverb_dynamic_loop_length_ext[1]>BIT_CRUSHER_REVERB_MAX_EXT(1))
							{
								reverb_dynamic_loop_length_ext[1] = BIT_CRUSHER_REVERB_MAX_EXT(1);
							}
						}

						#endif

						//printf("Reverb(): MIDI notes = %d-%d-%d, reverb_dynamic_loop_length = %d, ext = %d, %d\n", MIDI_last_chord[0], MIDI_last_chord[1], MIDI_last_chord[2], reverb_dynamic_loop_length, reverb_dynamic_loop_length_ext[0], reverb_dynamic_loop_length_ext[1]);
					}

					MIDI_notes_updated = 0;
				}
				if(MIDI_controllers_active_CC && !MIDI_multi_cc_override)
				{
					if(MIDI_controllers_updated==MIDI_WHEEL_CONTROLLER_CC_UPDATED)
					{
						//printf("Reverb(): MIDI_WHEEL_CONTROLLER_CC => %d\n", MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]);

						if(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]<8)
						{
							echo_dynamic_loop_length = (DELAY_BY_TEMPO * 3) / 2;
						}
						else if(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]<16)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO;
						}
						else if(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]<24)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 3 * 2;
						}
						else if(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]<32)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 2;
						}
						else if(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]<40)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 3;
						}
						else if(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]<48)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 4;
						}
						else if(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]<56)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 5;
						}
						else if(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]<64)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 6;
						}
						else if(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]<72)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 7;
						}
						else if(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]<80)
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / 8;
						}
						else
						{
							echo_dynamic_loop_length = DELAY_BY_TEMPO / (8 + MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC] - 80);
						}

						if(tempo_bpm < global_settings.TEMPO_BPM_DEFAULT)
						{
							echo_dynamic_loop_length /= 2;
						}

						MIDI_controllers_updated = 0;
					}
				}
				if(MIDI_controllers_active_PB && !MIDI_multi_cc_override)
				{
					if(MIDI_controllers_updated==MIDI_WHEEL_CONTROLLER_PB_UPDATED)
					{
						//printf("Reverb(): MIDI_WHEEL_CONTROLLER_PB => %d\n", MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB]);

						MIDI_pitch_bend = ((float)MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB]/127.0f) * 2.0f - 1.0f;

						reverb_dynamic_loop_length = (float)SAMPLE_RATE_DEFAULT / MIDI_note_to_freq_ft(MIDI_pitch_base[0]+MIDI_pitch_bend);

						if(reverb_dynamic_loop_length<BIT_CRUSHER_REVERB_MIN)
						{
							reverb_dynamic_loop_length = BIT_CRUSHER_REVERB_MIN;
						}
						if(reverb_dynamic_loop_length>BIT_CRUSHER_REVERB_MAX)
						{
							reverb_dynamic_loop_length = BIT_CRUSHER_REVERB_MAX;
						}

						#ifdef REVERB_OCTAVE_UP_DOWN
						reverb_dynamic_loop_length_ext[0] = BIT_CRUSHER_REVERB_CALC_EXT(0, reverb_dynamic_loop_length);
						reverb_dynamic_loop_length_ext[1] = BIT_CRUSHER_REVERB_CALC_EXT(1, reverb_dynamic_loop_length);
						#endif

						#ifdef REVERB_POLYPHONIC

						reverb_dynamic_loop_length_ext[0] = (float)SAMPLE_RATE_DEFAULT / MIDI_note_to_freq_ft(MIDI_pitch_base[1]+MIDI_pitch_bend);

						if(reverb_dynamic_loop_length_ext[0]<BIT_CRUSHER_REVERB_MIN_EXT(0))
						{
							reverb_dynamic_loop_length_ext[0] = BIT_CRUSHER_REVERB_MIN_EXT(0);
						}
						if(reverb_dynamic_loop_length_ext[0]>BIT_CRUSHER_REVERB_MAX_EXT(0))
						{
							reverb_dynamic_loop_length_ext[0] = BIT_CRUSHER_REVERB_MAX_EXT(0);
						}

						reverb_dynamic_loop_length_ext[1] = (float)SAMPLE_RATE_DEFAULT / MIDI_note_to_freq_ft(MIDI_pitch_base[2]+MIDI_pitch_bend);

						if(reverb_dynamic_loop_length_ext[1]<BIT_CRUSHER_REVERB_MIN_EXT(1))
						{
							reverb_dynamic_loop_length_ext[1] = BIT_CRUSHER_REVERB_MIN_EXT(1);
						}
						if(reverb_dynamic_loop_length_ext[1]>BIT_CRUSHER_REVERB_MAX_EXT(1))
						{
							reverb_dynamic_loop_length_ext[1] = BIT_CRUSHER_REVERB_MAX_EXT(1);
						}

						#endif

						//printf("Reverb(): MIDI_pitch_bend => %f, reverb_dynamic_loop_length = %d\n", MIDI_pitch_bend, reverb_dynamic_loop_length);
						MIDI_controllers_updated = 0;
					}
				}
			}
		}
		if (TIMING_EVERY_10_MS == 499) //100Hz
		{
			if(midi_ctrl_cc>0 && midi_ctrl_cc_active)
			{
				if(midi_ctrl_cc_updated[0])
				{
					MIDI_multi_cc_override = 1;
					//printf("Reverb(): MIDI_multi_cc_override => 1\n");

					ADC_LPF_ALPHA = (float)midi_ctrl_cc_values[0]/127.0f;
					//printf("midi_ctrl_cc_updated[0] = %d, midi_ctrl_cc_values[0] = %d, ADC_LPF_ALPHA = %f\n", midi_ctrl_cc_updated[0], midi_ctrl_cc_values[0], ADC_LPF_ALPHA);

					midi_ctrl_cc_updated[0] = 0;
				}
			}
			if(midi_ctrl_cc>1 && midi_ctrl_cc_active)
			{
				if(midi_ctrl_cc_updated[1])
				{
					MIDI_multi_cc_override = 1;
					//printf("Reverb(): MIDI_multi_cc_override => 1\n");

					reverb_dynamic_loop_length = (1.0f-((float)midi_ctrl_cc_values[1]/127.0f)) * 0.9f * (float)BIT_CRUSHER_REVERB_MAX;
					if(reverb_dynamic_loop_length<BIT_CRUSHER_REVERB_MIN)
					{
						reverb_dynamic_loop_length = BIT_CRUSHER_REVERB_MIN;
					}
					bit_crusher_reverb_d = reverb_dynamic_loop_length;

					//printf("midi_ctrl_cc_updated[1] = %d, midi_ctrl_cc_values[1] = %d, reverb_dynamic_loop_length = %d\n", midi_ctrl_cc_updated[1], midi_ctrl_cc_values[1], reverb_dynamic_loop_length);

					midi_ctrl_cc_updated[1] = 0;
				}
			}

			if(midi_ctrl_cc>2 && midi_ctrl_cc_active)
			{
				if(midi_ctrl_cc_updated[2])
				{
					MIDI_multi_cc_override = 1;
					//printf("Reverb(): MIDI_multi_cc_override => 1\n");

					echo_dynamic_loop_length = (int)((float)midi_ctrl_cc_values[2] / 127.0f * (float)ECHO_BUFFER_LENGTH_DEFAULT);
					echo_dynamic_loop_length0 = echo_dynamic_loop_length;
					//printf("midi_ctrl_cc_updated[2] = %d, midi_ctrl_cc_values[2] = %d, echo_dynamic_loop_length = %d\n", midi_ctrl_cc_updated[2], midi_ctrl_cc_values[2], echo_dynamic_loop_length);

					midi_ctrl_cc_updated[2] = 0;
				}
			}

			if(midi_ctrl_cc>3 && midi_ctrl_cc_active)
			{
				if(midi_ctrl_cc_updated[3])
				{
					MIDI_multi_cc_override = 1;
					//printf("Reverb(): MIDI_multi_cc_override => 1\n");

					echo_dynamic_loop_length = (int)(((float)midi_ctrl_cc_values[3]) / 127.0f * (float)echo_dynamic_loop_length0);
					//printf("midi_ctrl_cc_updated[3] = %d, midi_ctrl_cc_values[3] = %d, echo_dynamic_loop_length0 = %d, echo_dynamic_loop_length = %d\n", midi_ctrl_cc_updated[3], midi_ctrl_cc_values[3], echo_dynamic_loop_length0, echo_dynamic_loop_length);
					midi_ctrl_cc_updated[3] = 0;
				}
			}
		}
	}
}

/*
-------------------------------------------
buff	note	midi note
-------------------------------------------
228		a4		69		50780 / 228 = 222.7
304		e4		64
457		a3		57		50780 / 457 = 111.1
609		e3		52		50780 / 609 = 83.4
914		a2		45
1853	a1		33
-------------------------------------------
*/
