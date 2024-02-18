/*
 * binaural.c
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Mar 5, 2019
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

#include "binaural.h"
#include "glo_config.h"
#include "hw/gpio.h"
#include "hw/ui.h"
#include "hw/leds.h"
#include "dsp/Filters.h"

extern Filters *fil;

int beats_selected = 0;

void indicate_binaural(const char *wave)
{
	#ifdef BOARD_WHALE
	if(wave==NULL)
	{
		//off
		RGB_LED_R_OFF;
		RGB_LED_G_OFF;
		RGB_LED_B_OFF;
	}
	else if(wave[0]=='w')
	{
		//white
		RGB_LED_R_ON;
		RGB_LED_G_ON;
		RGB_LED_B_ON;
	}
	else if(wave[0]=='p')
	{
		//pink
		RGB_LED_R_ON;
		RGB_LED_G_OFF;
		RGB_LED_B_ON;
	}
	else if(wave[0]=='a') //alpha
	{
		//amber
		RGB_LED_R_ON;
		RGB_LED_G_ON;
		RGB_LED_B_OFF;
	}
	else if(wave[0]=='b') //beta
	{
		//blue
		RGB_LED_R_OFF;
		RGB_LED_G_OFF;
		RGB_LED_B_ON;
	}
	else if(wave[0]=='d') //delta
	{
		//green
		RGB_LED_R_OFF;
		RGB_LED_G_ON;
		RGB_LED_B_OFF;
	}
	else if(wave[0]=='t') //theta
	{
		//turquoise
		RGB_LED_R_OFF;
		RGB_LED_G_ON;
		RGB_LED_B_ON;
	}
	#endif
}

void binaural_program(void *pvParameters)
{
	printf("binaural_program(): task running on core ID=%d\n",xPortGetCoreID());

	//indicate that binaural beats are available by glowing the LED pink
	indicate_binaural("pink");

	#define BINAURAL_RAMP_RESOLUTION 100 //recalculate ramp and update tuning every 100ms
	#define BINAURAL_RAMP_STEPS_PER_SEC (1000 / BINAURAL_RAMP_RESOLUTION)

	int binaural_ptr = 0, segment = 0, i, beats_preloaded = 1;
	beats_selected = 0;
	float freq;
	int LED_blink = 0;
	#define LED_BLINK_PERIOD 20

	while(channel_running)
	{
		if(event_channel_options || (event_next_channel && beats_selected))
		{
			if(event_channel_options)
			{
				beats_selected++;
				if(beats_selected>BINAURAL_BEATS_AVAILABLE)
				{
					beats_selected = 0;
				}
				event_channel_options = 0;
				binaural_LEDs_timing_seq = 0;
			}
			else if(event_next_channel && beats_selected)
			{
				event_next_channel = 0;
				beats_selected = 0;
			}

			/*if(beats_selected==1)
			{
				binaural_ptr = 0;
				LED_blink = 0;
				//indicate which binaural wave is active by changing LED colour
				indicate_binaural(binaural_profile->name);
			}
			else*/
			if(!beats_selected)
			{
				//switch back to pink LED colour
				#ifdef BOARD_WHALE
				RGB_LED_R_ON;
				RGB_LED_G_OFF;
				RGB_LED_B_ON;
				#endif

				//reset to base frequencies
				fil->fp.tuning_chords_r = fil->fp.tuning_chords_l;
				fil->fp.tuning_chords_r = fil->fp.tuning_melody_l;
				fil->fp.tuning_chords_r = fil->fp.tuning_arp_l;

				printf("binaural_stopped,%f,%f,%f,%f,%f,%f\n", fil->fp.tuning_chords_r, fil->fp.tuning_chords_l, fil->fp.tuning_chords_r, fil->fp.tuning_melody_l, fil->fp.tuning_chords_r, fil->fp.tuning_arp_l);
			}
			else //run existing (first time only) or load the next available wave
			{
		    	if(!beats_preloaded)
		    	{
		    		printf("channel_init(): reloading the binaural_profile with next available wave\n");
		    		reload_binaural_profile(binaural_profile);
		    		printf("channel_init(): binaural_profile reloaded, total length = %d sec, coords = ", binaural_profile->total_length);
		    		for(int i=0;i<binaural_profile->total_coords;i++)
		    		{
		    			printf("%d/%f ", binaural_profile->coord_x[i], binaural_profile->coord_y[i]);
		    		}
		    		printf("\n");
		    	}
		    	else
		    	{
		    		beats_preloaded = 0;
		    	}

				binaural_ptr = 0; //restart program
				LED_blink = 0;
				indicate_binaural(binaural_profile->name); //indicate active binaural wave by colour
			}
		}

		if(beats_selected)
		{
			binaural_ptr++;

			LED_blink++;
			if(LED_blink%LED_BLINK_PERIOD==0)
			{
				indicate_binaural(binaural_profile->name);
			}
			else if(LED_blink%LED_BLINK_PERIOD==LED_BLINK_PERIOD/2)
			{
				indicate_binaural(NULL);
			}

			//find which segment we are in now
			for(i=0;i<binaural_profile->total_coords-1;i++)
			{
				if(binaural_ptr < 10*binaural_profile->coord_x[i+1])
				{
					segment = i;
					break;
				}
			}
			//get the interpolated frequency
			freq = binaural_profile->coord_y[segment] + (
				  (binaural_profile->coord_y[segment+1] - binaural_profile->coord_y[segment]) *
								 ((float)((binaural_ptr - binaural_profile->coord_x[segment]*10)) /
								 ((float)(binaural_profile->coord_x[segment+1]*10 - binaural_profile->coord_x[segment]*10))));

			printf("p%d,s%d,f=%f,", binaural_ptr, segment, freq);

			if(binaural_ptr == binaural_profile->total_length * BINAURAL_RAMP_STEPS_PER_SEC)
			{
				binaural_ptr = 0; //wrap around if end of program reached
			}

			fil->fp.tuning_chords_r = fil->fp.tuning_chords_l + freq;
			fil->fp.tuning_chords_r = fil->fp.tuning_melody_l + freq;
			fil->fp.tuning_chords_r = fil->fp.tuning_arp_l + freq;

			printf("%f,%f,%f,%f,%f,%f\n", fil->fp.tuning_chords_r, fil->fp.tuning_chords_l, fil->fp.tuning_chords_r, fil->fp.tuning_melody_l, fil->fp.tuning_chords_r, fil->fp.tuning_arp_l);
		}

		//need to detect channel ending more often
		for(i=0;i<10;i++)
		{
			Delay(BINAURAL_RAMP_RESOLUTION/10);
			if(!channel_running)
			{
				break;
			}
		}
	}

	printf("binaural_program(): task done, deleting...\n");
	vTaskDelete(NULL);
	printf("binaural_program(): task deleted\n");
}
