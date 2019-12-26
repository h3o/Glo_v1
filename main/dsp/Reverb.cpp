/*
 * Reverb.cpp
 *
 *  Created on: 16 Jul 2018
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

void decaying_reverb()
{
	//program_settings_reset();
	channel_init(0, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0); //init without any features

	/*
	//echo_dynamic_loop_length = I2S_AUDIOFREQ; //set default value (can be changed dynamically)
	//#define ECHO_LENGTH_DEFAULT ECHO_BUFFER_LENGTH
	DELAY_BY_TEMPO = get_delay_by_BPM(tempo_bpm);
    echo_dynamic_loop_length = DELAY_BY_TEMPO; //ECHO_LENGTH_DEFAULT;
	//echo_buffer = (int16_t*)malloc(echo_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	//memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	memset(echo_buffer,0,ECHO_BUFFER_LENGTH*sizeof(int16_t)); //clear the entire buffer
	echo_buffer_ptr0 = 0; //reset pointer
	*/
    ECHO_MIXING_GAIN_MUL = 9; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 10; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

	#ifdef ENABLE_MCLK_AT_CHANNEL_INIT
    enable_out_clock(); //better always enabled here, otherwise getting bad glitch
	mclk_enabled = 1;
	#endif

	//codec_init(); //i2c init

	#ifdef SWITCH_I2C_SPEED_MODES
	i2c_master_deinit();
    i2c_master_init(1); //fast mode
	#endif

    //int bit_crusher_div = BIT_CRUSHER_DIV_DEFAULT;
	int bit_crusher_reverb_d = BIT_CRUSHER_REVERB_DEFAULT;

	#define DECAY_DIR_FROM		-4
	#define DECAY_DIR_TO		4
	#define DECAY_DIR_DEFAULT	1
	int decay_direction = DECAY_DIR_DEFAULT;

	/*
    REVERB_MIXING_GAIN_MUL = 9; //amount of signal to feed back to reverb loop, expressed as a fragment
    REVERB_MIXING_GAIN_DIV = 10; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

    reverb_dynamic_loop_length = BIT_CRUSHER_REVERB_DEFAULT; //set default value (can be changed dynamically)
	//reverb_buffer = (int16_t*)malloc(reverb_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	memset(reverb_buffer,0,REVERB_BUFFER_LENGTH*sizeof(int16_t)); //clear the entire buffer
	reverb_buffer_ptr0 = 0; //reset pointer

	//channel_running = 1;
    //volume_ramp = 1;
	*/

	#ifdef BOARD_WHALE
    RGB_LED_R_ON;
	#endif

	int lock_sensors = 0;

	while(!event_next_channel)
	{
		sampleCounter++;

		t_TIMING_BY_SAMPLE_EVERY_250_MS = TIMING_EVERY_250_MS;

		#define REVERB_UI_CMD_DECREASE_DECAY	1
		#define REVERB_UI_CMD_INCREASE_DECAY	2

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 27) //4Hz
		{
			ui_command = 0;

			//printf("btn_event_ext = %d\n", btn_event_ext);

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
				printf("reverb_dynamic_loop_length = %d\n", reverb_dynamic_loop_length);
			}
			else
			{
				reverb_dynamic_loop_length = bit_crusher_reverb_d;
			}
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

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 27) //4Hz
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
				//printf("ADC_LPF_ALPHA=%f\n", ADC_LPF_ALPHA);
			}
		}

		/*
		//if (TIMING_BY_SAMPLE_EVERY_125_MS == 26) //8Hz
		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 26) //4Hz
		{
			printf("bit_crusher_reverb_d=%d,len=%d\n",bit_crusher_reverb_d,reverb_dynamic_loop_length);
		}
		*/

		i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);

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
		sample32 = (add_echo(add_reverb((int16_t)(sample_mix)))) << 16;

        //sample_mix = (int16_t)(ADC_sample>>16);
        //sample_mix = sample_mix * SAMPLE_VOLUME_REVERB; //apply volume
		//LPF enabled
		sample_lpf[1] = sample_lpf[1] + ADC_LPF_ALPHA * ((float)((int16_t)(ADC_sample>>16)) - sample_lpf[1]);
		sample_mix = sample_lpf[1] * SAMPLE_VOLUME_REVERB; //apply volume

        //sample32 += add_echo((int16_t)(sample_mix));
        //sample32 += add_reverb((int16_t)(sample_mix));
        sample32 += add_echo(add_reverb((int16_t)(sample_mix)));

		#ifdef BOARD_WHALE
        if(add_beep)
		{
			if(sampleCounter & (1<<add_beep))
			{
				sample32 += (100 + (100<<16));
			}
		}
		#endif

		i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
		sd_write_sample(&sample32);

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 35) //4Hz
		{
			if(!lock_sensors && !echo_dynamic_loop_current_step) //if sensors not locked and a fixed delay is not set, use accelerometer or IR sensors
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
				//printf("echo_dynamic_loop_length = %d\n", echo_dynamic_loop_length);
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
	}
}

