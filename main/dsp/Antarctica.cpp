/*
 * Antarctica.cpp
 *
 *  Created on: Sep 24, 2016
 *      Author: mayo
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

#include <dsp/IIR_filters.h>
//#include <hw/codec.h>
//#include <hw/gpio.h>
#include <hw/init.h>
#include <hw/ui.h>
//#include <hw/signals.h>
//#include <hw/controls.h>
#include <hw/sdcard.h>
#include <hw/leds.h>
#include "Antarctica.h"
#include "Binaural.h"
#include "glo_config.h"
#include <string.h>

volatile uint32_t a_sampleCounter = 0;
volatile int16_t a_sample_i16 = 0;
uint32_t a_sample32;

//#define WIND_FILTERS 2
//#define WIND_FILTERS 4
#define WIND_FILTERS 8
//#define WIND_FILTERS 12

#if WIND_FILTERS == 2
int wind_freqs[] = {880};
float cutoff[WIND_FILTERS] = {0.159637183,0.159637183};
float cutoff_limit[2] = {0.079818594};
#endif

#if WIND_FILTERS == 4
int a_wind_freqs[] = {880,880};
float cutoff[WIND_FILTERS] = {0.159637183,0.159637183,0.159637183,0.159637183};
float cutoff_limit[2] = {0.079818594,0.319274376};
//float cutoff[WIND_FILTERS] = {0.359637183,0.459637183,0.4159637183,0.3159637183};
//float cutoff_limit[2] = {0.279818594,0.5319274376};
//float cutoff[WIND_FILTERS] = {0.1759637183,0.18159637183,0.1578159637183,0.1983159637183};
//float cutoff_limit[2] = {0.15,0.20};
#endif

#if WIND_FILTERS == 8
int a_wind_freqs[] = {880,880,880,880};
float cutoff[WIND_FILTERS] = {0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183};
float cutoff_limit[2] = {0.079818594,0.319274376};
#endif

#if WIND_FILTERS == 12
int a_wind_freqs[] = {880,880,880,880,880,880};
float cutoff[WIND_FILTERS] = {0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183};
float cutoff_limit[2] = {0.079818594,0.319274376};
#endif

IIR_Filter *a_iir[WIND_FILTERS];

uint32_t a_random_value;

float a_sample[WIND_FILTERS/2],a_sample_mix;
unsigned long a_seconds;

float A_SAMPLE_VOLUME = 2.0f; //1.0f; //4.0f; //0.375f;

float resonance = 0.970;
float feedback;
//float resonance_limit[2] = {0.950,0.995};
//float feedback_limit[2];

float a_volume = 400.0f; //400.0f;

int i, cutoff_sweep = 0;

float mixing_volumes[WIND_FILTERS];
int mixing_deltas[WIND_FILTERS];

//#define BUTTON_SET_ON	(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)==1)	//SET button: PA0

void filter_setup02()
{
	for(int i=0;i<WIND_FILTERS;i++)
	{
		a_iir[i] = new IIR_Filter_LOW_PASS_4TH_ORDER;
	}

	for(i=0;i<WIND_FILTERS;i++)
	{
		a_iir[i]->setResonance(resonance);
		a_iir[i]->setCutoff((float)a_wind_freqs[i%(WIND_FILTERS/2)] / (float)I2S_AUDIOFREQ * 2 * 2);
		mixing_volumes[i]=1.0f;
		mixing_deltas[i]=0;
	}

	//feedback boundaries
	feedback = 0.96;
	//feedback_limit[0] = 0.5;
	//feedback_limit[1] = 1.0;
}

void song_of_wind_and_ice()
{
	printf("ARCTIC_WIND_TEST: filter_setup02()\n");
	filter_setup02();
	printf("ARCTIC_WIND_TEST: filters set up successfully\n");

    a_seconds = 0;

	#ifdef BOARD_WHALE
    BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_SHORT;
	#endif

    float signal_min = 100000, signal_max = -100000;
    float signal_max_limit = 1100;
    float signal_min_limit = -250;

    int isochronic = 0;
	#define ISOCHRONIC_MAX 5 //off,alpha,beta,delta,theta
    const char *isochronic_LED[ISOCHRONIC_MAX] = {"w","a","b","d","t"};

    isochronic_def *iso_def = new isochronic_def;
    load_isochronic_def(iso_def,"[isochronic_tones]");

    a_volume = iso_def->WIND_VOLUME;
    signal_min_limit = iso_def->SIGNAL_LIMIT_MIN;
    signal_max_limit = iso_def->SIGNAL_LIMIT_MAX;

    int iso_freq_to_sample_timing[8];
    iso_freq_to_sample_timing[0] = ((float)2*I2S_AUDIOFREQ / (float)iso_def->ALPHA_LOW);
    iso_freq_to_sample_timing[1] = ((float)2*I2S_AUDIOFREQ / (float)iso_def->ALPHA_HIGH);
    iso_freq_to_sample_timing[2] = ((float)2*I2S_AUDIOFREQ / (float)iso_def->BETA_LOW);
    iso_freq_to_sample_timing[3] = ((float)2*I2S_AUDIOFREQ / (float)iso_def->BETA_HIGH);
    iso_freq_to_sample_timing[4] = ((float)2*I2S_AUDIOFREQ / (float)iso_def->DELTA_LOW);
    iso_freq_to_sample_timing[5] = ((float)2*I2S_AUDIOFREQ / (float)iso_def->DELTA_HIGH);
    iso_freq_to_sample_timing[6] = ((float)2*I2S_AUDIOFREQ / (float)iso_def->THETA_LOW);
    iso_freq_to_sample_timing[7] = ((float)2*I2S_AUDIOFREQ / (float)iso_def->THETA_HIGH);
    printf("Isochronic timing-by-sample: %d, %d, %d, %d, %d, %d, %d, %d\n",
    		iso_freq_to_sample_timing[0],
    		iso_freq_to_sample_timing[1],
    		iso_freq_to_sample_timing[2],
    		iso_freq_to_sample_timing[3],
    		iso_freq_to_sample_timing[4],
    		iso_freq_to_sample_timing[5],
    		iso_freq_to_sample_timing[6],
    		iso_freq_to_sample_timing[7]);

    int iso_program_timing[4];
    iso_program_timing[0] =  round((float)iso_def->PROGRAM_LENGTH / (float)((iso_def->ALPHA_HIGH - iso_def->ALPHA_LOW) * 2));
    iso_program_timing[1] =  round((float)iso_def->PROGRAM_LENGTH / (float)((iso_def->BETA_HIGH - iso_def->BETA_LOW) * 2));
    iso_program_timing[2] =  round((float)iso_def->PROGRAM_LENGTH / (float)((iso_def->DELTA_HIGH - iso_def->DELTA_LOW) * 2));
    iso_program_timing[3] =  round((float)iso_def->PROGRAM_LENGTH / (float)((iso_def->THETA_HIGH - iso_def->THETA_LOW) * 2));
    printf("Isochronic program stepping: %d, %d, %d, %d\n",
    		iso_program_timing[0],
			iso_program_timing[1],
			iso_program_timing[2],
			iso_program_timing[3]);

    int iso_freq_to_sample_step[4];
    //iso_freq_to_sample_step[0] = (float)(iso_freq_to_sample_timing[0] - iso_freq_to_sample_timing[1]) / (float)(iso_def->ALPHA_HIGH - iso_def->ALPHA_LOW);
    //iso_freq_to_sample_step[1] = (float)(iso_freq_to_sample_timing[2] - iso_freq_to_sample_timing[3]) / (float)(iso_def->BETA_HIGH - iso_def->BETA_LOW);
    //iso_freq_to_sample_step[2] = (float)(iso_freq_to_sample_timing[4] - iso_freq_to_sample_timing[5]) / (float)(iso_def->DELTA_HIGH - iso_def->DELTA_LOW);
    //iso_freq_to_sample_step[3] = (float)(iso_freq_to_sample_timing[6] - iso_freq_to_sample_timing[7]) / (float)(iso_def->THETA_HIGH - iso_def->THETA_LOW);
    iso_freq_to_sample_step[0] = (float)(iso_freq_to_sample_timing[0] - iso_freq_to_sample_timing[1]) / (float)(iso_def->PROGRAM_LENGTH/2);
    iso_freq_to_sample_step[1] = (float)(iso_freq_to_sample_timing[2] - iso_freq_to_sample_timing[3]) / (float)(iso_def->PROGRAM_LENGTH/2);
    iso_freq_to_sample_step[2] = (float)(iso_freq_to_sample_timing[4] - iso_freq_to_sample_timing[5]) / (float)(iso_def->PROGRAM_LENGTH/2);
    iso_freq_to_sample_step[3] = (float)(iso_freq_to_sample_timing[6] - iso_freq_to_sample_timing[7]) / (float)(iso_def->PROGRAM_LENGTH/2);
    printf("Isochronic program sample step: %d, %d, %d, %d\n",
    		iso_freq_to_sample_step[0],
			iso_freq_to_sample_step[1],
			iso_freq_to_sample_step[2],
			iso_freq_to_sample_step[3]);

    int iso_program_dir = -1, iso_program_step = 0, iso_program_freq = 0, iso_sample = 0;;

    int options_menu = 0;
	int options_indicator = 0;

    while(!event_next_channel)
    {
		if (!(a_sampleCounter & 0x00000001)) //right channel
		{
			float r = PseudoRNG1a_next_float();
			memcpy(&a_random_value, &r, sizeof(a_random_value));

    		a_sample[0] = (float)(32768 - (int16_t)a_random_value) / 32768.0f;
    		a_sample[1] = (float)(32768 - (int16_t)(a_random_value>>16)) / 32768.0f;
		}

		if (a_sampleCounter & 0x00000001) //left channel
		{
			a_sample32 += a_sample_i16;

			if(add_beep)
			{
				if(a_sampleCounter & (1<<(add_beep+1))) //here sample counter has 2x faster timing
				{
					a_sample32 += (100 + (100<<16));
				}
			}

			i2s_push_sample(I2S_NUM, (char *)&a_sample32, portMAX_DELAY);
			sd_write_sample(&a_sample32);
		}
		else
		{
			a_sample32 = a_sample_i16 << 16;
		}

		//while(!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		//SPI_I2S_SendData(CODEC_I2S, a_sample_i16);

		//pwr_button_shutdown();

		if (a_sampleCounter & 0x00000001) //left channel
		{
			a_sample_mix = IIR_Filter::iir_filter_multi_sum(a_sample[0], &a_iir[0], WIND_FILTERS/2, mixing_volumes) * a_volume;
		}
		else
		{
			a_sample_mix = IIR_Filter::iir_filter_multi_sum(a_sample[1], &a_iir[WIND_FILTERS/2], WIND_FILTERS/2, mixing_volumes+WIND_FILTERS/2) * a_volume;
		}

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

		a_sample_i16 = (int16_t)(a_sample_mix * A_SAMPLE_VOLUME);

		a_sampleCounter++;

		//if (a_sampleCounter==1337 || a_sampleCounter==20480)
		//{
		//	printf("a_sample_i16 value = %d\n", a_sample_i16);
		//}

		if (a_sampleCounter==I2S_AUDIOFREQ)
    	{
    		//LED_BLUE_OFF;
    	}
    	else if (a_sampleCounter==2*I2S_AUDIOFREQ)
    	{
    		//LED_BLUE_ON;
    		a_sampleCounter = 0;
    		a_seconds++;

    		if (a_seconds % 3 == 0)
    		{
    			for (int d=0; d<WIND_FILTERS; d++)
    			{
    				mixing_deltas[d] = (a_random_value >> d) & 0x00000001;
    			}
    		}

    		if(isochronic)
    		{
    			iso_program_step++;
    			//if(iso_program_step==iso_program_timing[isochronic-1])
    			//{
        			//iso_program_step = 0;

        			iso_program_freq += iso_program_dir * iso_freq_to_sample_step[isochronic-1];
        			if(iso_program_freq>=iso_freq_to_sample_timing[(isochronic-1)*2])
        			{
        				iso_program_dir = -iso_program_dir;
        			}
        			else if(iso_program_freq<=iso_freq_to_sample_timing[(isochronic-1)*2+1])
        			{
        				iso_program_dir = -iso_program_dir;
        			}
    			//}
    			printf("iso_program_freq=%d, iso_program_dir=%d, iso_program_step=%d\n", iso_program_freq, iso_program_dir, iso_program_step);
    		}
    	}

		if (a_sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
    	{
        	cutoff_sweep++;
        	cutoff_sweep %= WIND_FILTERS;

        	cutoff[cutoff_sweep] += 0.005 - (float)(0.01 * ((a_random_value) & 0x00000001));
        	if (cutoff[cutoff_sweep] < cutoff_limit[0])
        	{
        		cutoff[cutoff_sweep] = cutoff_limit[0];
        	}
        	else if (cutoff[cutoff_sweep] > cutoff_limit[1])
        	{
        		cutoff[cutoff_sweep] = cutoff_limit[1];
        	}

			mixing_volumes[cutoff_sweep] += mixing_deltas[cutoff_sweep] * 0.01f;
			if (mixing_volumes[cutoff_sweep] < 0)
			{
				mixing_volumes[cutoff_sweep] = 0;
			}
			else if (mixing_volumes[cutoff_sweep] > 1)
			{
				mixing_volumes[cutoff_sweep] = 1;
			}

			a_iir[cutoff_sweep]->setCutoff(cutoff[cutoff_sweep]);
    	}

		if(isochronic)
		{
			iso_sample--;
			//if (a_sampleCounter%iso_program_freq==0)
			if(iso_sample==iso_def->TONE_LENGTH)
			{
				a_iir[0]->setCutoff(iso_def->TONE_CUTOFF);
				a_iir[WIND_FILTERS/2]->setCutoff(iso_def->TONE_CUTOFF);
        		mixing_volumes[0]=iso_def->TONE_VOLUME;
        		mixing_volumes[WIND_FILTERS/2]=iso_def->TONE_VOLUME;
        		indicate_binaural(isochronic_LED[isochronic]);
			}
			//else if (a_sampleCounter%iso_program_freq==iso_def->TONE_LENGTH)
			else if (!iso_sample)
			{
				//back to default cut-off
				a_iir[0]->setCutoff(cutoff[0]);
				a_iir[WIND_FILTERS/2]->setCutoff(cutoff[WIND_FILTERS/2]);
        		iso_sample = iso_program_freq;
        		indicate_binaural(NULL);
			}
		}

		ui_command = 0;

		if (a_sampleCounter%(2*I2S_AUDIOFREQ/10)==13) //every 100ms
		{
			if(event_channel_options)
			{
				event_channel_options = 0;
				isochronic++;
				if(isochronic==ISOCHRONIC_MAX)
				{
					isochronic = 0;
					//need to reset cut-off in case the tone was just on
					a_iir[0]->setCutoff(cutoff[0]);
					a_iir[WIND_FILTERS/2]->setCutoff(cutoff[WIND_FILTERS/2]);
					mixing_volumes[0]=1.0f;
					mixing_volumes[WIND_FILTERS/2]=1.0f;
				}
				//indicate_binaural(isochronic_LED[isochronic]);
				iso_program_dir = -1;
				iso_program_step = 0;
				iso_program_freq = iso_freq_to_sample_timing[(isochronic-1)*2];
				iso_sample = iso_program_freq;
			}

			#define ANTARCTICA_UI_CMD_TONE_VOLUME_INCREASE		1
			#define ANTARCTICA_UI_CMD_TONE_VOLUME_DECREASE		2
			#define ANTARCTICA_UI_CMD_TONE_LENGTH_INCREASE		3
			#define ANTARCTICA_UI_CMD_TONE_LENGTH_DECREASE		4
			//#define ANTARCTICA_UI_CMD_CUTOFF_INCREASE			5
			//#define ANTARCTICA_UI_CMD_CUTOFF_DECREASE			6

			//map UI commands
			#ifdef BOARD_WHALE
			if(short_press_volume_plus) { ui_command = ANTARCTICA_UI_CMD_TONE_VOLUME_INCREASE; short_press_volume_plus = 0; }
			if(short_press_volume_minus) { ui_command = ANTARCTICA_UI_CMD_TONE_VOLUME_DECREASE; short_press_volume_minus = 0; }
			if(short_press_sequence==2) { ui_command = ANTARCTICA_UI_CMD_TONE_LENGTH_INCREASE; short_press_sequence = 0; }
			if(short_press_sequence==-2) { ui_command = ANTARCTICA_UI_CMD_TONE_LENGTH_DECREASE; short_press_sequence = 0; }
			//if(short_press_sequence==3) { ui_command = ANTARCTICA_UI_CMD_CUTOFF_INCREASE; short_press_sequence = 0; }
			//if(short_press_sequence==-3) { ui_command = ANTARCTICA_UI_CMD_CUTOFF_DECREASE; short_press_sequence = 0; }
			#endif

			#ifdef BOARD_GECHO

			if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_1 || btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_2)
			{
				//flip the flag
				options_menu = 1 - options_menu;
				if(!options_menu)
				{
					LED_O4_all_OFF();
					settings_menu_active = 0;
				}
				else
				{
					settings_menu_active = 1;
				}
				btn_event_ext = 0;
			}

			if(btn_event_ext)
			{
				printf("btn_event_ext=%d, options_menu=%d\n",btn_event_ext, options_menu);
			}

			if(options_menu)
			{
				options_indicator++;
				LED_O4_set_byte(0x0f*(options_indicator%2));

				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = ANTARCTICA_UI_CMD_TONE_VOLUME_DECREASE; btn_event_ext = 0; }
				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = ANTARCTICA_UI_CMD_TONE_VOLUME_INCREASE; btn_event_ext = 0; }
				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_3) { ui_command = ANTARCTICA_UI_CMD_TONE_LENGTH_DECREASE; btn_event_ext = 0; }
				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_4) { ui_command = ANTARCTICA_UI_CMD_TONE_LENGTH_INCREASE; btn_event_ext = 0; }

				//if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_1) {  }
				//if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_2) {  }
				//if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_3) {  }
				//if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_4) {  }
			}
			else
			{

			}

			btn_event_ext = 0;
			#endif

			if(ui_command)
			{
				if(ui_command==ANTARCTICA_UI_CMD_TONE_VOLUME_INCREASE && iso_def->TONE_VOLUME < iso_def->TONE_VOLUME_MAX)
				{
					iso_def->TONE_VOLUME += iso_def->TONE_VOLUME_STEP;
				}
				if(ui_command==ANTARCTICA_UI_CMD_TONE_VOLUME_DECREASE && iso_def->TONE_VOLUME > iso_def->TONE_VOLUME_STEP)
				{
					iso_def->TONE_VOLUME -= iso_def->TONE_VOLUME_STEP;
				}
				if(ui_command==ANTARCTICA_UI_CMD_TONE_LENGTH_INCREASE && iso_def->TONE_LENGTH < iso_def->TONE_LENGTH_MAX)
				{
					iso_def->TONE_LENGTH += iso_def->TONE_LENGTH_STEP;
				}
				if(ui_command==ANTARCTICA_UI_CMD_TONE_LENGTH_DECREASE && iso_def->TONE_LENGTH > iso_def->TONE_LENGTH_STEP)
				{
					iso_def->TONE_LENGTH -= iso_def->TONE_LENGTH_STEP;
				}
				/*
				if(ui_command==ANTARCTICA_UI_CMD_CUTOFF_INCREASE && iso_def->TONE_CUTOFF < iso_def->TONE_CUTOFF_MAX)
				{
					iso_def->TONE_CUTOFF += iso_def->TONE_CUTOFF_STEP;
				}
				if(ui_command==ANTARCTICA_UI_CMD_CUTOFF_DECREASE && iso_def->TONE_CUTOFF > iso_def->TONE_CUTOFF_STEP)
				{
					iso_def->TONE_CUTOFF -= iso_def->TONE_CUTOFF_STEP;
				}
				*/
				printf("TONE_VOLUME=%f, TONE_LENGTH=%d, TONE_CUTOFF=%f\n", iso_def->TONE_VOLUME, iso_def->TONE_LENGTH, iso_def->TONE_CUTOFF);
			}
		}
    }

    for(int i=0;i<WIND_FILTERS;i++)
	{
		delete(a_iir[i]);
	}

    delete(iso_def);
}
