/*
 * FilterFlanger.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 10 Nov 2019
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

#include <string.h>

#include "FilterFlanger.h"
#include "InitChannels.h"
#include "Interface.h"
#include "hw/init.h"
#include "hw/ui.h"
#include "hw/gpio.h"
#include "hw/leds.h"
#include "hw/sdcard.h"

IRAM_ATTR void channel_filter_flanger()
{
	#define MIN_V 32000		//initial minimum value, should be higher than any possible value
	#define MAX_V -32000	//initial maximum value, should be lower than any possible value

	channel_init(0, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0, 0, 0); //init without any features

	TEMPO_BY_SAMPLE = get_tempo_by_BPM(tempo_bpm);
	DELAY_BY_TEMPO = get_delay_by_BPM(tempo_bpm);

	ECHO_MIXING_GAIN_MUL = 38; //amount of signal to feed back to echo loop, expressed as a fraction
    ECHO_MIXING_GAIN_DIV = 40; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

	#define ECHO_MIXING_GAIN_MUL_MIN	32
	#define ECHO_MIXING_GAIN_MUL_MAX	39

	limiter_coeff = DYNAMIC_LIMITER_COEFF_DEFAULT;

	int ff_len = 0, ff_len0 = 0, ff_ptr = 0;//, direction = 1;
	#define FLANGER_RANGE 1000

	uint32_t *flanger_line = (uint32_t*)malloc((FLANGER_RANGE+1)*sizeof(uint32_t));
	memset(flanger_line,0,FLANGER_RANGE*sizeof(uint32_t));

	//float speedup=1.0f, slowdown=1.0f;
	//float rnd;
	int ff_oversample = 2;

	LED_R8_all_OFF();
	LED_O4_all_OFF();
	//LED_W8_all_OFF();
	//LED_B5_all_OFF();

	LED_W8_set_byte(0x01<<(lround(ECHO_MIXING_GAIN_MUL)-ECHO_MIXING_GAIN_MUL_MIN));
	LED_B5_set_byte((ff_oversample>4) ? 0x1f>>(ff_oversample-4) : 0x1f<<(5-ff_oversample) );

	channel_running = 1;
	volume_ramp = 1;

	sample_hpf[0] = 0; sample_hpf[1] = 0; sample_hpf[2] = 0; sample_hpf[3] = 0;
	sample_lpf[0] = 0; sample_lpf[1] = 0; sample_lpf[2] = 0; sample_lpf[3] = 0;

	#define HPF_ALPHA_MIN 0.02f
	#define HPF_ALPHA_MAX 0.65f

	#define LPF_ALPHA_MIN 0.05f
	#define LPF_ALPHA_MAX 1.00f

	ADC_LPF_ALPHA = LPF_ALPHA_MAX;
	ADC_HPF_ALPHA = HPF_ALPHA_MIN;

	#define S4_LPF_ALPHA 0.06f //0.025f
	float s4_lpf = 0;

	while(!event_next_channel)
	{
		if (!(sampleCounter & 0x00000001)) //right channel
		{
			sample_f[0] = (float)((int16_t)(ADC_sample >> 16)) * OpAmp_ADC12_signal_conversion_factor;
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			sample_f[1] = (float)((int16_t)ADC_sample) * OpAmp_ADC12_signal_conversion_factor;

			sample_mix = sample_f[1] * MAIN_VOLUME;

			sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);
		}
		else
		{
			sample_mix = sample_f[0] * MAIN_VOLUME;

			sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);
		}

		/*
		//LPF enabled
		sample_lpf[0] = sample_lpf[0] + ADC_LPF_ALPHA * ((float)((int16_t)ADC_sample) - sample_lpf[0]);
		sample_mix = sample_lpf[0] * SAMPLE_VOLUME_REVERB; //apply volume

		//LPF enabled
		sample_lpf[1] = sample_lpf[1] + ADC_LPF_ALPHA * ((float)((int16_t)(ADC_sample>>16)) - sample_lpf[1]);
		sample_mix = sample_lpf[1] * SAMPLE_VOLUME_REVERB; //apply volume
		*/

		sample_i16 = add_echo(sample_i16);
		//sample_i16 = reversible_echo(sample_i16, direction);

		if (sampleCounter & 0x00000001) //left channel
		{
			//LPF enabled
			sample_lpf[0] += ADC_LPF_ALPHA * (((float)sample_i16) - sample_lpf[0]);
			sample_i16 = sample_lpf[0] * (1 + (1-ADC_LPF_ALPHA) * 1); //normalize the effect of high frequencies carrying some of the energy
			//sample_lpf[0] *= (1 + (1-ADC_LPF_ALPHA) * 1); //normalize the effect of high frequencies carrying some of the energy

			//HPF enabled
			sample_hpf[0] += ADC_HPF_ALPHA * (((float)sample_i16) - sample_hpf[0]);
			//sample_hpf[0] += ADC_HPF_ALPHA * (sample_lpf[0] - sample_hpf[0]);
			//sample_i16 -= sample_hpf[0]; //return inputValue - buf0 -> 1st order
			sample_hpf[1] += ADC_HPF_ALPHA * (sample_hpf[0] - sample_hpf[1]);
			sample_i16 -= sample_hpf[1]; //return inputValue - buf1 -> 2nd order

			//sample_i16 = sample_lpf[0] - sample_hpf[1]; //return inputValue - buf1 -> 2nd order
			sample_i16 += ((float)sample_i16) * ADC_HPF_ALPHA * 3; //normalize the effect of low frequencies carrying some of the energy
			//sample_i16 = (sample_lpf[0] - sample_hpf[1]) * ADC_HPF_ALPHA * 3; //return inputValue - buf1 -> 2nd order and normalize the effect of low frequencies carrying some of the energy

			sample32 += sample_i16 << 16;

			flanger_line[ff_ptr] = sample32;
			ff_ptr++;
			if(ff_ptr>ff_len)
			{
				ff_ptr = 0;
			}
			if(ff_len)
			{
				sample32 += flanger_line[ff_ptr];

				//sample32 /= 2; //halve the volume -> does not work for signed values
				/*
				//this works but makes signal too low
				sample_i16 = sample32 >> 16;
				sample_i16 /= 2;
				sample32 = ((int16_t)sample32)/2;
				sample32 += sample_i16 << 16;
				*/
			}
		}
		else
		{
			//LPF enabled
			sample_lpf[1] += ADC_LPF_ALPHA * (((float)sample_i16) - sample_lpf[1]);
			sample_i16 = sample_lpf[1] * (1 + (1-ADC_LPF_ALPHA) * 1); //normalize the effect of high frequencies carrying some of the energy
			//sample_lpf[1] *= (1 + (1-ADC_LPF_ALPHA) * 1); //normalize the effect of high frequencies carrying some of the energy

			//HPF enabled
			sample_hpf[2] += ADC_HPF_ALPHA * (((float)sample_i16) - sample_hpf[2]);
			//sample_hpf[2] += ADC_HPF_ALPHA * (sample_lpf[1] - sample_hpf[2]);
			//sample_i16 -= sample_hpf[2]; //return inputValue - buf0 -> 1st order
			sample_hpf[3] += ADC_HPF_ALPHA * (sample_hpf[2] - sample_hpf[3]);
			sample_i16 -= sample_hpf[3]; //return inputValue - buf1 -> 2nd order
			sample_i16 += ((float)sample_i16) * ADC_HPF_ALPHA * 3; //normalize the effect of low frequencies carrying some of the energy
			//sample_i16 = (sample_lpf[1] - sample_hpf[3]) * ADC_HPF_ALPHA * 3; //return inputValue - buf1 -> 2nd order and normalize the effect of low frequencies carrying some of the energy

			sample32 = sample_i16;
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			for(int i=0;i<ff_oversample;i++)
			{
				/*if(speedup==1.0f)
				{*/
					i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
					sd_write_sample(&sample32);
					//}
					//for(int i=0;i<ch344_oversample;i++)
					//{
					i2s_read(I2S_NUM, (void*)&ADC_sample, 4, &i2s_bytes_rw, portMAX_DELAY);
				/*}
				else if(speedup>1.99f)
				{
					if(warp_cnt%2)
					{
						i2s_write(I2S_NUM, (void*)&sample32, 4, &bytes_written, portMAX_DELAY);
						sd_write_sample(&sample32);
						i2s_read(I2S_NUM, (void*)&ADC_sample, 4, &i2s_bytes_rw, portMAX_DELAY);
					}
				}
				else
				{
					//if(warp_cnt%(101-speedup))
					//if(rnd+0.01f < 1.0f/speedup-0.5f)
					if(rnd < 1.0f/speedup-0.5f)
					{
						//printf("<%.02f\n",1.0f/speedup-0.5f);
						i2s_write(I2S_NUM, (void*)&sample32, 4, &bytes_written, portMAX_DELAY);
						sd_write_sample(&sample32);
						i2s_read(I2S_NUM, (void*)&ADC_sample, 4, &i2s_bytes_rw, portMAX_DELAY);
					}
					//else
					//{
						//printf(">%.02f\n",1.0f/speedup-0.5f);
					//}
				}
				if(slowdown!=1.0f)
				{
					//if(rnd > 1.0f/slowdown-1.0f) //--> low end level OK
					//if(rnd > 1.0f/slowdown-0.5f) //--> upper end level OK
					if(rnd > 2.0f/slowdown-1.5f)
					{
						i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
						sd_write_sample(&sample32);
						i2s_read(I2S_NUM, (void*)&ADC_sample, 4, &i2s_bytes_rw, portMAX_DELAY);
					}
				}*/
			}
			/*warp_cnt++;
			*/
		}

		if (TIMING_EVERY_20_MS == 33) //50Hz
		{
			if(limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT)
			{
				limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //timing @ 50Hz, limiter will fully recover within 0.4 second
			}
		}

		//if(tempoCounter%(TEMPO_BY_SAMPLE/SEQUENCER_INDICATOR_DIV)==0)
		//if(tempoCounter%(DELAY_BY_TEMPO/SEQUENCER_INDICATOR_DIV)==0)
		if(echo_dynamic_loop_length && tempoCounter%(echo_dynamic_loop_length/SEQUENCER_INDICATOR_DIV)==0)
        {
        	LED_sequencer_indicators();
        }

        sampleCounter++;
        tempoCounter--;

        if(tempoCounter==0) //one full second passed
		{
			seconds++;
			sampleCounter = 0;
			tempoCounter = TEMPO_BY_SAMPLE;

			#ifdef BOARD_WHALE
			check_auto_power_off();
			#endif

			LED_W8_set_byte(0x01<<(lround(ECHO_MIXING_GAIN_MUL)-ECHO_MIXING_GAIN_MUL_MIN));
			LED_B5_set_byte((ff_oversample>4) ? 0x1f>>(ff_oversample-4) : 0x1f<<(5-ff_oversample) );
		}

        //if (TIMING_EVERY_10_MS==37) //100Hz
        if (TIMING_EVERY_100_MS==37) //10Hz
    	//if (TIMING_EVERY_250_MS == 27) //4Hz
        {
        	s4_lpf = s4_lpf + S4_LPF_ALPHA * (ir_res[3] - s4_lpf);
        	//printf("s4_lpf = %.02f\n", s4_lpf);

        	if(SENSOR_THRESHOLD_WHITE_8)
        	{
        		ff_len0 = ff_len;
        		ff_len = 1 + FLANGER_RANGE * s4_lpf; //ir_res[3];
        		if(ff_len > FLANGER_RANGE)
        		{
        			ff_len = FLANGER_RANGE;
        		}
        		//printf("ff = +%d\n", ff);

        		if(ff_len > ff_len0) //erase the new segment
        		{
        			memset(flanger_line+ff_len0,0,(ff_len-ff_len0)*sizeof(uint32_t));
        		}
        	}
        	else
        	{
        		ff_len = 0;
        	}

        	/*
        	if(SENSOR_THRESHOLD_ORANGE_2)
        	{
        		direction = 0;
        	}
        	else
        	{
        		direction = 1;
        	}
        	*/

        	/*
        	if(SENSOR_THRESHOLD_BLUE_5)
            {
            	slowdown = 1.0f + (ir_res[2]-IR_sensors_THRESHOLD_1) / IR_sensors_THRESHOLD_8;
        		if(slowdown>2.0f) slowdown = 2.0f;
        		printf("s = -%.02f\n", slowdown);
            }
        	else
        	{
        		slowdown = 1.0f;
        	}
        	*/
			/*
        	if(SENSOR_THRESHOLD_BLUE_1) { ADC_LPF_ALPHA = 1.0f; }
			else if(SENSOR_THRESHOLD_BLUE_2) { ADC_LPF_ALPHA = 0.8f; }
			else if(SENSOR_THRESHOLD_BLUE_3) { ADC_LPF_ALPHA = 0.6f; }
			else if(SENSOR_THRESHOLD_BLUE_4) { ADC_LPF_ALPHA = 0.4f; }
			else if(SENSOR_THRESHOLD_BLUE_5) { ADC_LPF_ALPHA = 0.3f; }
			else { ADC_LPF_ALPHA = 0.2f; }
			*/
        	ADC_LPF_ALPHA = 1 - ir_res[2];
        	if(ADC_LPF_ALPHA < LPF_ALPHA_MIN)
        	{
        		ADC_LPF_ALPHA = LPF_ALPHA_MIN;
        	}
        	if(ADC_LPF_ALPHA > LPF_ALPHA_MAX)
        	{
        		ADC_LPF_ALPHA = LPF_ALPHA_MAX;
        	}
			//printf("ADC_LPF_ALPHA=%f\n", ADC_LPF_ALPHA);

        	ADC_HPF_ALPHA = ir_res[1]*HPF_ALPHA_MAX;
        	if(ADC_HPF_ALPHA < HPF_ALPHA_MIN)
        	{
        		ADC_HPF_ALPHA = HPF_ALPHA_MIN;
        	}
        	/*
        	if(ADC_HPF_ALPHA > HPF_ALPHA_MAX)
        	{
        		ADC_HPF_ALPHA = HPF_ALPHA_MAX;
        	}
        	*/
			//printf("ADC_HPF_ALPHA=%f\n", ADC_HPF_ALPHA);
        }

		//sensor 1 - echo delay length
		#define SENSOR_DELAY_9					SENSOR_THRESHOLD_RED_9
		#define SENSOR_DELAY_8					SENSOR_THRESHOLD_RED_8
		#define SENSOR_DELAY_7					SENSOR_THRESHOLD_RED_7
		#define SENSOR_DELAY_6					SENSOR_THRESHOLD_RED_6
		#define SENSOR_DELAY_5					SENSOR_THRESHOLD_RED_5
		#define SENSOR_DELAY_4					SENSOR_THRESHOLD_RED_4
		#define SENSOR_DELAY_3					SENSOR_THRESHOLD_RED_3
		#define SENSOR_DELAY_2					SENSOR_THRESHOLD_RED_2
		#define SENSOR_DELAY_ACTIVE				SENSOR_THRESHOLD_RED_1

        if (TIMING_EVERY_250_MS == 7919) //4Hz, 1000th prime
		{
			if(/*!lock_sensors &&*/ !echo_dynamic_loop_current_step) //if sensors not locked and a fixed delay is not set, use accelerometer or IR sensors
			{
				if(SENSOR_DELAY_ACTIVE)
				{
					if(SENSOR_DELAY_9)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 64;
					}
					else if(SENSOR_DELAY_8)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 32;
					}
					else if(SENSOR_DELAY_7)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 16;
					}
					else if(SENSOR_DELAY_6)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 8;
					}
					else if(SENSOR_DELAY_5)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 4;
					}
					else if(SENSOR_DELAY_4)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 3;
					}
					else if(SENSOR_DELAY_3)
					{
						echo_dynamic_loop_length = DELAY_BY_TEMPO / 2;
					}
					else if(SENSOR_DELAY_2)
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
				//printf("echo_dynamic_loop_length = %d\n", echo_dynamic_loop_length);
			}
		//}

		ui_command = 0;

		//if (TIMING_EVERY_20_MS==31) //50Hz
		//{
			#define FF_UI_CMD_OVERSAMPLE_DOWN	1
			#define FF_UI_CMD_OVERSAMPLE_UP		2
			#define FF_UI_CMD_PERSISTENCE_DOWN	3
			#define FF_UI_CMD_PERSISTENCE_UP		4

			//map UI commands
			#ifdef BOARD_WHALE
			if(short_press_volume_plus) { ui_command = FF_UI_CMD_OVERSAMPLE_UP; short_press_volume_plus = 0; }
			if(short_press_volume_minus) { ui_command = FF_UI_CMD_OVERSAMPLE_DOWN; short_press_volume_minus = 0; }
			if(short_press_sequence==2) { ui_command =  FF_UI_CMD_PERSISTENCE_UP; short_press_sequence = 0; }
			if(short_press_sequence==-2) { ui_command = FF_UI_CMD_PERSISTENCE_DOWN; short_press_sequence = 0; }
			#endif

			#ifdef BOARD_GECHO
			if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = FF_UI_CMD_OVERSAMPLE_DOWN; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = FF_UI_CMD_OVERSAMPLE_UP; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_1) { ui_command = FF_UI_CMD_PERSISTENCE_DOWN; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_2) { ui_command =  FF_UI_CMD_PERSISTENCE_UP; btn_event_ext = 0; }
			#endif

			if(ui_command==FF_UI_CMD_OVERSAMPLE_DOWN || ui_command==FF_UI_CMD_OVERSAMPLE_UP)
			{
				if(ui_command==FF_UI_CMD_OVERSAMPLE_DOWN)
				{
					if(ff_oversample<8)
					{
						ff_oversample++;
					}
				}
				else
				{
					if(ff_oversample>1)
					{
						ff_oversample--;
					}
				}
				printf("FF Oversample = %d\n", ff_oversample);
				LED_B5_set_byte((ff_oversample>4) ? 0x1f>>(ff_oversample-4) : 0x1f<<(5-ff_oversample) );
			}

			if(ui_command==FF_UI_CMD_PERSISTENCE_DOWN || ui_command==FF_UI_CMD_PERSISTENCE_UP)
			{
				if(ui_command==FF_UI_CMD_PERSISTENCE_DOWN)
				{
					if(ECHO_MIXING_GAIN_MUL>ECHO_MIXING_GAIN_MUL_MIN)
					{
						ECHO_MIXING_GAIN_MUL--;
					}
				}
				else
				{
					if(ECHO_MIXING_GAIN_MUL<ECHO_MIXING_GAIN_MUL_MAX)
					{
						ECHO_MIXING_GAIN_MUL++;
					}
				}
				printf("FF Delay persistence = %.02f/%.02f\n", ECHO_MIXING_GAIN_MUL, ECHO_MIXING_GAIN_DIV);
				LED_W8_set_byte(0x01<<(lround(ECHO_MIXING_GAIN_MUL)-ECHO_MIXING_GAIN_MUL_MIN));
			}
		}

	} //end skip channel cnt

	//return input select back to previous user-defined value
	codec_select_input(ADC_input_select);

	free(flanger_line);
}
