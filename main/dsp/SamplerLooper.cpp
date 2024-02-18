/*
 * SamplerLooper.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 10 Jul 2019
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

#include "SamplerLooper.h"
#include "InitChannels.h"
#include "Interface.h"
#include "hw/init.h"
#include "hw/ui.h"
#include "hw/gpio.h"
#include "hw/leds.h"
#include "hw/sdcard.h"

#define LOOPER_ECHO

#define LOOPER_STATUS_STOPPED		0
#define LOOPER_STATUS_RECORD_START	1
#define LOOPER_STATUS_RECORDING		2
#define LOOPER_STATUS_RECORD_STOP	3
#define LOOPER_STATUS_PLAY_START	4
#define LOOPER_STATUS_PLAYING		5
#define LOOPER_STATUS_PLAY_STOP		6

#define LOOPER_TRACKS				4

#define LOOPER_BUFFER_SIZE			1024
#define BUFFER_SIZE_IN_BYTES		(LOOPER_BUFFER_SIZE*sizeof(uint32_t))

/*
#define LOOPER_B1		1
#define LOOPER_B2		2
#define LOOPER_B3		3
#define LOOPER_B4		4
#define LOOPER_SET		5
#define LOOPER_RST		6
*/

#define SAMPLER_BUTTONS_SHORT_THRESHOLD 	1		//wait 20ms till counting as short press
#define SAMPLER_BUTTONS_LONG_THRESHOLD 		20		//wait 400ms till counting as long press

void channel_sampler_looper()
{
	//there is an external sampleCounter variable already, it will be used to timing and counting seconds
	sampleCounter = 0;
	seconds = 0;
	program_settings_reset();

	if(!sd_card_ready)
	{
	    printf("channel_sampler_looper(): SD Card init...\n");
		int result = sd_card_init(5, persistent_settings.SD_CARD_SPEED);
		if(result!=ESP_OK)
		{
			printf("channel_sampler_looper(): problem with SD Card Init, code=%d\n", result);
			return;
		}
	}
	create_dir_if_not_exists(LOOPER_SUBDIR);

    printf("channel_sampler_looper(): function loop starting\n");

	/*
    channel_running = 1;
    volume_ramp = 1;
    ui_ignore_events = 1; //so the buttons combinations won't enter context menu etc..
    */

	//xTaskCreatePinnedToCore((TaskFunction_t)&sd_recording, "sd_recording_task", 2048, NULL, 12, NULL, 1);

    uint8_t looper_status[LOOPER_TRACKS] = {0,0,0,0};

    uint32_t looper_tracks_len[LOOPER_TRACKS];

    FILE* ltf[LOOPER_TRACKS] = {NULL,NULL,NULL,NULL};

	char track_filename[LOOPER_TRACKS][40];

	//uint32_t buf_in[LOOPER_BUFFER_SIZE], buf_out[LOOPER_BUFFER_SIZE];
	uint32_t *buf_in, *buf_out;

	buf_in = (uint32_t*)malloc(LOOPER_BUFFER_SIZE*sizeof(uint32_t));
	buf_out = (uint32_t*)malloc(LOOPER_BUFFER_SIZE*sizeof(uint32_t));

	int i, any_recording = 0, any_playing = 0;

    for(i=0;i<LOOPER_TRACKS;i++)
    {
		sprintf(track_filename[i], "/sdcard/%s/loop%02u.raw", LOOPER_SUBDIR, i);

		//load tracks lengths from existing files if present
	    looper_tracks_len[i] = get_file_size(track_filename[i]) / BUFFER_SIZE_IN_BYTES;

	    printf("channel_sampler_looper(): file #%d = %s, length = %u B, %u buffers\n", i, track_filename[i], BUFFER_SIZE_IN_BYTES*looper_tracks_len[i], looper_tracks_len[i]);
    }

    int btn_counters[6] = {0,0,0,0,0,0};
    int btn_event = 0;

	LED_R8_all_OFF();
	LED_O4_all_OFF();

	int buffers = 0;
	int armed = 0;
	int button_release_wait = 0;

	set_sampling_rate(SAMPLE_RATE_LOOPER_SAMPLER);
	codec_set_mute(0); //unmute the codec

	while(!event_next_channel)
	{
		sampleCounter++;

		/*
		//this will interfere with buffer timing
		if (TIMING_BY_SAMPLE_1_SEC) //one full second passed
		{
			seconds++;
			sampleCounter = 0;
		}
		*/

		if(sampleCounter==LOOPER_BUFFER_SIZE)
		{
			sampleCounter=0;
			buffers++;

			if(armed) //ultra fast button detection
			{
				if(BUTTON_SET_ON || BUTTON_RST_ON)
				{
					printf("channel_sampler_looper(): SET or RST detected in armed mode\n");
					while(ANY_BUTTON_ON); //wait till released
					armed = 0; //cancel armed mode, no further action
					LED_R8_all_OFF();
				}
				else if(!button_release_wait && USER_BUTTON_ON)
				{
					for(int b=BUTTON_1;b<=BUTTON_4;b++)
					{
						if (BUTTON_ON(b))
						{
							btn_event = BUTTON_EVENT_PRESS + b;
							button_release_wait = b;
						}
					}
				}
				else if(button_release_wait && NO_BUTTON_ON)
				{
					btn_event = BUTTON_EVENT_RELEASE + button_release_wait;
					button_release_wait = 0;
				}
				Delay(1);
			}
			else //normal detection
			{
				//timing: LOOPER_BUFFER_SIZE (1024 samples), 1024 / Fs = 1024 / 50780 = 0.02 sec
				for(int b=0;b<BUTTONS_N;b++)
				{
					if (BUTTON_ON(b))
					{
						btn_counters[b]++;

						if (btn_counters[b] == SAMPLER_BUTTONS_LONG_THRESHOLD)
						{
							btn_event = BUTTON_EVENT_LONG_PRESS + b;
						}
					}
					else if (btn_counters[b])
					{
						if (btn_counters[b] > SAMPLER_BUTTONS_SHORT_THRESHOLD && btn_counters[b] < SAMPLER_BUTTONS_LONG_THRESHOLD)
						{
							btn_event = BUTTON_EVENT_SHORT_PRESS + b;
						}
						else if (btn_counters[b] > SAMPLER_BUTTONS_LONG_THRESHOLD)
						{
							btn_event = BUTTON_EVENT_LONG_RELEASE + b;
						}
						btn_counters[b] = 0;
					}
				}
			}

			if(any_recording)
	        {
				//if(buffers%8==0) //timing: LOOPER_BUFFER_SIZE (1024 samples) * 8 = 8192, 8192 / Fs = 8192 / 50780 = 0.16 sec
				if(buffers%32==0) //timing: LOOPER_BUFFER_SIZE (1024 samples) * 32 = 32768, 32768 / Fs = 32768 / 50780 = 0.64 sec
				{
					for(i=0;i<LOOPER_TRACKS;i++)
					{
						if(looper_status[i]==LOOPER_STATUS_RECORDING)
						{
							LED_R8_set_byte(1<<(i+2));
						}
					}
		        }
				//else if(buffers%8==4)
				else if(buffers%32==16)
				{
					LED_R8_all_OFF();
				}
        	}
		}

		//always recording
		i2s_read(I2S_NUM, (void*)&ADC_sample, 4, &i2s_bytes_rw, portMAX_DELAY);

		/*
        //mix samples for all voices (left channel)
		sample_mix = (int16_t)ADC_sample;

		#ifdef LOOPER_ECHO
		sample32 = (add_echo((int16_t)(sample_mix))) << 16;
		#else
		sample32 = ((int16_t)(sample_mix)) << 16;
		#endif

        //mix samples for all voices (right channel)
        //sample_mix = 0;
        sample_mix = (int16_t)(ADC_sample>>16);

        #ifdef LOOPER_ECHO
        sample32 += add_echo((int16_t)(sample_mix));
		#else
        sample32 += (int16_t)(sample_mix);
		#endif
		*/

        if(!sampleCounter)
        {
        	if(any_recording)
        	{
        		for(i=0;i<LOOPER_TRACKS;i++)
        		{
        			if(looper_status[i]==LOOPER_STATUS_RECORDING)
        			{
        				//fwrite(buf_in,sizeof(buf_in),1,ltf[i]);
        				fwrite(buf_in,BUFFER_SIZE_IN_BYTES,1,ltf[i]);
        				break;
        			}
        		}
        	}

			if(btn_event)
			{
				printf("channel_sampler_looper(): btn_event=%d\n",btn_event);

				for(i=0;i<LOOPER_TRACKS;i++)
				{
					/*
					if(btn_event==BUTTON_EVENT_LONG_PRESS+i+1)
					{
						if(looper_status[i]==LOOPER_STATUS_STOPPED && !any_recording && !any_playing)
						{
							looper_status[i] = LOOPER_STATUS_RECORD_START;
							LED_R8_set_byte(1<<(i+2));
						}
					}
					else if(btn_event==BUTTON_EVENT_LONG_RELEASE+i+1)
					{
						if(looper_status[i]==LOOPER_STATUS_RECORDING)
						{
							looper_status[i] = LOOPER_STATUS_RECORD_STOP;
							LED_R8_all_OFF();
						}
					}
					*/

					/*
					if(btn_event==BUTTON_EVENT_SHORT_PRESS+i+1)
					{
						if(armed && looper_status[i]==LOOPER_STATUS_STOPPED && !any_recording && !any_playing)
						{
							looper_status[i] = LOOPER_STATUS_RECORD_START;
							LED_R8_set_byte(1<<(i+2));
						}
						else if(looper_status[i]==LOOPER_STATUS_RECORDING)
						{
							looper_status[i] = LOOPER_STATUS_RECORD_STOP;
							LED_R8_set_byte(0xff * armed);
						}
					}
					*/

					if(btn_event==BUTTON_EVENT_PRESS+i+1)
					{
						if(looper_status[i]==LOOPER_STATUS_STOPPED && !any_recording && !any_playing)
						{
							looper_status[i] = LOOPER_STATUS_RECORD_START;
							LED_R8_set_byte(1<<(i+2));
						}
					}
					else if(btn_event==BUTTON_EVENT_RELEASE+i+1)
					{
						if(looper_status[i]==LOOPER_STATUS_RECORDING)
						{
							looper_status[i] = LOOPER_STATUS_RECORD_STOP;
							LED_R8_set_byte(0xff * armed);
						}
					}
					else if(!armed && btn_event==BUTTON_EVENT_SHORT_PRESS+i+1)
					{
						if(looper_status[i]==LOOPER_STATUS_STOPPED && !any_recording && !any_playing)
						{
							looper_status[i] = LOOPER_STATUS_PLAY_START;
							LED_O4_set_byte(1<<i);
						}
						else if(looper_status[i]==LOOPER_STATUS_PLAYING)
						{
							looper_status[i] = LOOPER_STATUS_PLAY_STOP;
							LED_O4_all_OFF();
						}
					}
				}

				if(btn_event==BUTTON_EVENT_SHORT_PRESS+BUTTON_SET && !any_recording && !any_playing)
				{
					armed = 1 - armed;
					LED_R8_set_byte(0xff * armed);
				}

				if(btn_event==BUTTON_EVENT_SHORT_PRESS+BUTTON_RST)
				{
					event_next_channel = 1;
				}
				btn_event = 0;
			}

			for(i=0;i<LOOPER_TRACKS;i++)
			{
				if(looper_status[i]==LOOPER_STATUS_RECORD_START)
				{
					printf("channel_sampler_looper(): Opening file %s for writing\n", track_filename[i]);
					ltf[i] = fopen(track_filename[i], "w");
					if (ltf[i] == NULL) {
						printf("channel_sampler_looper(): Failed to open file %s for writing\n", track_filename[i]);
						return;
					}
					looper_status[i]=LOOPER_STATUS_RECORDING;
					any_recording = 1;
					buffers = 0;
				}
				//else if(looper_status[i]==LOOPER_STATUS_RECORDING)
				//{
				//	fwrite(&sample32,sizeof(sample32),1,ltf[i]);
				//}
				else if(looper_status[i]==LOOPER_STATUS_PLAY_START)
				{
					printf("channel_sampler_looper(): Opening file %s for reading\n", track_filename[i]);
					ltf[i] = fopen(track_filename[i], "r");
					if (ltf[i] == NULL) {
						printf("channel_sampler_looper(): Failed to open file %s for reading\n", track_filename[i]);
						return;
					}
					looper_status[i]=LOOPER_STATUS_PLAYING;
					any_playing = 1;
					buffers = 0;
				}
				//else if(looper_status[i]==LOOPER_STATUS_PLAYING)
				//{
				//	fread(&sample32,sizeof(sample32),1,ltf[i]);
				//	if(feof(ltf[i]))
				//	{
				//		fseek(ltf[i],0,SEEK_SET);
				//	}
				//}
				else if(looper_status[i]==LOOPER_STATUS_RECORD_STOP || looper_status[i]==LOOPER_STATUS_PLAY_STOP)
				{
					printf("channel_sampler_looper(): Closing file #%d\n", i);
					fclose(ltf[i]);
					if(looper_status[i]==LOOPER_STATUS_RECORD_STOP)
					{
						looper_tracks_len[i] = buffers;
    					printf("b=>%d\n",buffers);
					}
					looper_status[i]=LOOPER_STATUS_STOPPED;
					any_recording = 0;
					any_playing = 0;
					//buffers = 0;
				}
			}

			if(any_playing)
        	{
        		for(i=0;i<LOOPER_TRACKS;i++)
        		{
        			if(looper_status[i]==LOOPER_STATUS_PLAYING)
        			{
        				if(buffers == looper_tracks_len[i])
        				{
        					fseek(ltf[i],0,SEEK_SET);
        					buffers = 0;
        					printf("b=0\n");

        					//last buffer will be mute as it usually contains button release click noise
        					memset(buf_out,0,BUFFER_SIZE_IN_BYTES);
        				}
        				else
        				{
        					fread(buf_out,BUFFER_SIZE_IN_BYTES,1,ltf[i]);
        				}
        				break;
        			}
        		}
        	}
        }

        buf_in[sampleCounter] = ADC_sample; //sample32;

        if(any_playing)
        {
        	sample32 = buf_out[sampleCounter];
        }

        //sample32 = 0; //ADC_sample; //test bypass all effects
        i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);

        //sd_write_sample(&sample32);

	}	//end while(1)

	//codec_set_mute(1); //mute the codec
	set_sampling_rate(persistent_settings.SAMPLING_RATE);

    free(buf_in);
    free(buf_out);
}

void channel_sampled_drum_sequencer()
{
	//there is an external sampleCounter variable already, it will be used to timing and counting seconds
	sampleCounter = 0;
	seconds = 0;
	program_settings_reset();

    printf("channel_sampled_drum_sequencer(): function loop starting\n");

    channel_running = 1;
    volume_ramp = 1;
    //ui_ignore_events = 1; //so the buttons combinations won't enter context menu etc..

	#define BUFFER_LENGTH	(SAMPLE_RATE_SAMPLED_DRUM_SEQUENCER-2) //to make the buffer length divisible by 8
	#define N_SEGMENTS		8
	#define SEGMENT_LENGTH	(2*BUFFER_LENGTH/N_SEGMENTS)

    uint16_t *buf1 = (uint16_t*)malloc(BUFFER_LENGTH*sizeof(uint16_t));
	if(!buf1)
	{
		printf("channel_sampled_drum_sequencer(): problem allocating buffer #1\n");
		return;
	}
	printf("channel_sampled_drum_sequencer(): buffer #1 allocated at 0x%x\n", (unsigned int)buf1);
	memset(buf1,0,BUFFER_LENGTH*sizeof(uint16_t));

	uint16_t *buf2 = (uint16_t*)malloc(BUFFER_LENGTH*sizeof(uint16_t));
	if(!buf2)
	{
		printf("channel_sampled_drum_sequencer(): problem allocating buffer #2\n");
		return;
	}
	printf("channel_sampled_drum_sequencer(): buffer #2 allocated at 0x%x\n", (unsigned int)buf2);
	memset(buf2,0,BUFFER_LENGTH*sizeof(uint16_t));

	int segment = 0, recording = 0;

	LED_R8_all_OFF();
	LED_O4_all_OFF();

	//int buffers = 0;
	//int armed = 0;
	//int button_release_wait = 0;

	set_sampling_rate(SAMPLE_RATE_SAMPLED_DRUM_SEQUENCER);
	codec_set_mute(0); //unmute the codec
	uint8_t red_byte = 0;

	while(!event_next_channel)
	{
		sampleCounter++;

		/*
		//this will interfere with buffer timing
		if (TIMING_BY_SAMPLE_1_SEC) //one full second passed
		{
			seconds++;
			sampleCounter = 0;
		}
		*/

		if(sampleCounter==BUFFER_LENGTH*2)
		{
			sampleCounter=0;
		}

		if(sampleCounter%SEGMENT_LENGTH==0)
		{
			segment++;
			if(segment==N_SEGMENTS)
			{
				segment = 0;
			}
			red_byte |= 1<<segment;
		}

		if(sampleCounter%SEGMENT_LENGTH==SEGMENT_LENGTH/2)
		{
			red_byte = 0;
		}

		if(sampleCounter%(SEGMENT_LENGTH/2)==0)
		{
			red_byte |= 1<<recording;
		}

		if(sampleCounter%(SEGMENT_LENGTH/2)==SEGMENT_LENGTH/4)
		{
			red_byte &= ~(1<<recording);
		}

		LED_R8_set_byte(red_byte);

		//always recording
		i2s_read(I2S_NUM, (void*)&ADC_sample, 4, &i2s_bytes_rw, portMAX_DELAY);

		if(segment==recording && BUTTON_SET_ON)
		{
			if(sampleCounter<BUFFER_LENGTH)
			{
				buf1[sampleCounter] = ADC_sample;
			}
			else
			{
				buf2[sampleCounter-BUFFER_LENGTH] = ADC_sample;
			}
		}

		if (TIMING_EVERY_50_MS == 13) //20Hz periodically, at given sample
		{
			if(btn_event_ext)
			{
				printf("channel_sampled_drum_sequencer(): btn_event=%d\n",btn_event_ext);

				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2)
				{
					recording++;
					if(recording>=N_SEGMENTS)
					{
						recording = 0;
					}
				}
				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1)
				{
					recording--;
					if(recording<0)
					{
						recording = N_SEGMENTS-1;
					}
				}
				btn_event_ext = 0;
			}
        }

		if(sampleCounter<BUFFER_LENGTH)
		{
			//sample32 = buf1[sampleCounter];// + (buf1[sampleCounter]<<8);
			sample_mix = buf1[sampleCounter];// + (buf1[sampleCounter]<<8);
		}
		else
		{
			//sample32 = buf2[sampleCounter-BUFFER_LENGTH];// + (buf2[sampleCounter-BUFFER_LENGTH]<<8);
			sample_mix = buf2[sampleCounter-BUFFER_LENGTH];// + (buf2[sampleCounter-BUFFER_LENGTH]<<8);
		}


        //mix samples for all voices (left channel)
		//sample_mix = (int16_t)ADC_sample;

		#ifdef LOOPER_ECHO
		sample32 = (add_echo((int16_t)(sample_mix))) << 16;
		#else
		sample32 = ((int16_t)(sample_mix)) << 16;
		#endif

        //mix samples for all voices (right channel)
        //sample_mix = 0;
        sample_mix = (int16_t)(ADC_sample>>16);

        #ifdef LOOPER_ECHO
        sample32 += add_echo((int16_t)(sample_mix));
		#else
        sample32 += (int16_t)(sample_mix);
		#endif

        i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
        //sd_write_sample(&sample32);

	}	//end while(1)

	//codec_set_mute(1); //mute the codec
	set_sampling_rate(persistent_settings.SAMPLING_RATE);

    free(buf1);
    free(buf2);
}

void channel_infinite_looper()
{
	#define MIN_V 32000		//initial minimum value, should be higher than any possible value
	#define MAX_V -32000	//initial maximum value, should be lower than any possible value

	//LEDs_all_OFF();
	//init_echo_buffer();
	//program_settings_reset();
	channel_init(0, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0, 0, 0); //init without any features

	TEMPO_BY_SAMPLE = get_tempo_by_BPM(tempo_bpm);
	DELAY_BY_TEMPO = get_delay_by_BPM(tempo_bpm);

	ECHO_MIXING_GAIN_MUL = 38; //amount of signal to feed back to echo loop, expressed as a fraction
    ECHO_MIXING_GAIN_DIV = 40; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

	#define ECHO_MIXING_GAIN_MUL_MIN	32
	#define ECHO_MIXING_GAIN_MUL_MAX	39

	limiter_coeff = DYNAMIC_LIMITER_COEFF_DEFAULT;

	int ch344_oversample = 2, warp_cnt=0, direction = 1;
	float speedup=1.0f, slowdown=1.0f, rnd=0.0f;

	LED_R8_all_OFF();
	LED_O4_all_OFF();
	//LED_W8_all_OFF();
	//LED_B5_all_OFF();
	LED_W8_set_byte(0x01<<(lround(ECHO_MIXING_GAIN_MUL)-ECHO_MIXING_GAIN_MUL_MIN));
	LED_B5_set_byte((ch344_oversample>4) ? 0x1f>>(ch344_oversample-4) : 0x1f<<(5-ch344_oversample) );

	channel_running = 1;
	volume_ramp = 1;

	while(!event_next_channel)
	{
		if (!(sampleCounter & 0x00000001)) //right channel
		{
			rnd = PseudoRNG1a_next_float_new();
			//memcpy(&random_value, &r, sizeof(random_value));

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

		//sample_i16 = add_echo(sample_i16);
		sample_i16 = reversible_echo(sample_i16, direction);

		if (sampleCounter & 0x00000001) //left channel
		{
			sample32 += sample_i16 << 16;
		}
		else
		{
			sample32 = sample_i16;
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			for(int i=0;i<ch344_oversample;i++)
			{
				if(speedup==1.0f)
				{
					i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
					sd_write_sample(&sample32);
					i2s_read(I2S_NUM, (void*)&ADC_sample, 4, &i2s_bytes_rw, portMAX_DELAY);
				}
				else if(speedup>1.99f)
				{
					if(warp_cnt%2)
					{
						i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
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
						i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
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
				}
			}
			warp_cnt++;
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
			LED_B5_set_byte((ch344_oversample>4) ? 0x1f>>(ch344_oversample-4) : 0x1f<<(5-ch344_oversample) );
		}

        if (TIMING_EVERY_10_MS==37) //100Hz
        {
        	if(SENSOR_THRESHOLD_WHITE_8)
        	{
        		speedup = 1.0f + (ir_res[3]-IR_sensors_THRESHOLD_1) / IR_sensors_THRESHOLD_8;
        		if(speedup>2.0f) speedup = 2.0f;
        		//printf("s = +%.02f\n", speedup);
        	}
        	else
        	{
        		speedup = 1.0f;
        	}

        	if(SENSOR_THRESHOLD_ORANGE_2)
        	{
        		direction = 0;
        	}
        	else
        	{
        		direction = 1;
        	}

        	if(SENSOR_THRESHOLD_BLUE_5)
            {
            	slowdown = 1.0f + (ir_res[2]-IR_sensors_THRESHOLD_1) / IR_sensors_THRESHOLD_8;
        		if(slowdown>2.0f) slowdown = 2.0f;
        		//printf("s = -%.02f\n", slowdown);
            }
        	else
        	{
        		slowdown = 1.0f;
        	}
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
			#define CH344_UI_CMD_OVERSAMPLE_DOWN	1
			#define CH344_UI_CMD_OVERSAMPLE_UP		2
			#define CH344_UI_CMD_PERSISTENCE_DOWN	3
			#define CH344_UI_CMD_PERSISTENCE_UP		4

			//map UI commands
			#ifdef BOARD_WHALE
			if(short_press_volume_plus) { ui_command = CH344_UI_CMD_OVERSAMPLE_UP; short_press_volume_plus = 0; }
			if(short_press_volume_minus) { ui_command = CH344_UI_CMD_OVERSAMPLE_DOWN; short_press_volume_minus = 0; }
			if(short_press_sequence==2) { ui_command =  CH344_UI_CMD_PERSISTENCE_UP; short_press_sequence = 0; }
			if(short_press_sequence==-2) { ui_command = CH344_UI_CMD_PERSISTENCE_DOWN; short_press_sequence = 0; }
			#endif

			#ifdef BOARD_GECHO
			if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = CH344_UI_CMD_OVERSAMPLE_DOWN; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = CH344_UI_CMD_OVERSAMPLE_UP; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_1) { ui_command = CH344_UI_CMD_PERSISTENCE_DOWN; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_2) { ui_command =  CH344_UI_CMD_PERSISTENCE_UP; btn_event_ext = 0; }
			#endif

			if(ui_command==CH344_UI_CMD_OVERSAMPLE_DOWN || ui_command==CH344_UI_CMD_OVERSAMPLE_UP)
			{
				if(ui_command==CH344_UI_CMD_OVERSAMPLE_DOWN)
				{
					if(ch344_oversample<8)
					{
						//dco_oversample*=2;
						ch344_oversample++;
					}
				}
				else
				{
					if(ch344_oversample>1)
					{
						//dco_oversample/=2;
						ch344_oversample--;
					}
				}
				printf("#344 Oversample = %d\n", ch344_oversample);
				LED_B5_set_byte((ch344_oversample>4) ? 0x1f>>(ch344_oversample-4) : 0x1f<<(5-ch344_oversample) );
			}

			if(ui_command==CH344_UI_CMD_PERSISTENCE_DOWN || ui_command==CH344_UI_CMD_PERSISTENCE_UP)
			{
				if(ui_command==CH344_UI_CMD_PERSISTENCE_DOWN)
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
				printf("#344 Delay persistence = %.02f/%.02f\n", ECHO_MIXING_GAIN_MUL, ECHO_MIXING_GAIN_DIV);
				LED_W8_set_byte(0x01<<(lround(ECHO_MIXING_GAIN_MUL)-ECHO_MIXING_GAIN_MUL_MIN));
			}
		}

	} //end skip channel cnt

	//return input select back to previous user-defined value
	codec_select_input(ADC_input_select);
}

void channel_looper_shifter()
{
	#define MIN_V 32000		//initial minimum value, should be higher than any possible value
	#define MAX_V -32000	//initial maximum value, should be lower than any possible value

	//LEDs_all_OFF();
	//init_echo_buffer();
	//program_settings_reset();
	channel_init(0, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0, 0, 0); //init without any features

	TEMPO_BY_SAMPLE = get_tempo_by_BPM(tempo_bpm);
	DELAY_BY_TEMPO = get_delay_by_BPM(tempo_bpm);

	ECHO_MIXING_GAIN_MUL = 38; //amount of signal to feed back to echo loop, expressed as a fraction
    ECHO_MIXING_GAIN_DIV = 40; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

	#define ECHO_MIXING_GAIN_MUL_MIN	32
	#define ECHO_MIXING_GAIN_MUL_MAX	39

	limiter_coeff = DYNAMIC_LIMITER_COEFF_DEFAULT;

	int ch344_oversample = 2, direction = 1;

	#define S3_LPF_ALPHA 0.016 //0.032f //0.08f
	float s3_lpf = 0;

	#define S4_LPF_ALPHA 0.016 //0.032f //0.08f
	float s4_lpf = 0;

	int echo_buffer_ptr_U = 0, echo_buffer_ptr_D = 0;
	float mixing_coeff_octave_U = 0.0f, mixing_coeff_octave_D = 0.0f;

	LED_R8_all_OFF();
	LED_O4_all_OFF();
	//LED_W8_all_OFF();
	//LED_B5_all_OFF();
	LED_W8_set_byte(0x01<<(lround(ECHO_MIXING_GAIN_MUL)-ECHO_MIXING_GAIN_MUL_MIN));
	LED_B5_set_byte((ch344_oversample>4) ? 0x1f>>(ch344_oversample-4) : 0x1f<<(5-ch344_oversample) );

	channel_running = 1;
	volume_ramp = 1;

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

		//sample_i16 = add_echo(sample_i16);
		sample_i16 = reversible_echo(sample_i16, direction);

		if (sampleCounter & 0x00000001) //left channel
		{
			if(echo_dynamic_loop_length)
			{
				echo_buffer_ptr_U += 2;
				echo_buffer_ptr_D ++; //only in one of the channels

				if (echo_buffer_ptr_U >= echo_dynamic_loop_length) { echo_buffer_ptr_U = 0; }
				if (echo_buffer_ptr_D >= echo_dynamic_loop_length) { echo_buffer_ptr_D = 0; }

				//add echo from the loop
				sample_i16 += (int)((float)echo_buffer[echo_buffer_ptr_U] * mixing_coeff_octave_U);
				sample_i16 += (int)((float)echo_buffer[echo_buffer_ptr_D] * mixing_coeff_octave_D);
			}

			sample32 += sample_i16 << 16;
		}
		else
		{
			if(echo_dynamic_loop_length)
			{
				echo_buffer_ptr_U += 2;
				//echo_buffer_ptr_D ++; //only in one of the channels

				if (echo_buffer_ptr_U >= echo_dynamic_loop_length) { echo_buffer_ptr_U = 0; }
				if (echo_buffer_ptr_D >= echo_dynamic_loop_length) { echo_buffer_ptr_D = 0; }

				//add echo from the loop
				sample_i16 += (int)((float)echo_buffer[echo_buffer_ptr_U] * mixing_coeff_octave_U);
				sample_i16 += (int)((float)echo_buffer[echo_buffer_ptr_D] * mixing_coeff_octave_D);
			}

			sample32 = sample_i16;
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			for(int i=0;i<ch344_oversample;i++)
			{
				i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
				sd_write_sample(&sample32);
				i2s_read(I2S_NUM, (void*)&ADC_sample, 4, &i2s_bytes_rw, portMAX_DELAY);
			}
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
			LED_B5_set_byte((ch344_oversample>4) ? 0x1f>>(ch344_oversample-4) : 0x1f<<(5-ch344_oversample) );
		}

        if (TIMING_EVERY_10_MS==37) //100Hz
        {
    		s4_lpf = s4_lpf + S4_LPF_ALPHA * (ir_res[3] - s4_lpf);
        	//printf("s4_lpf = %.02f\n", s4_lpf);

        	if(SENSOR_THRESHOLD_WHITE_8)
        	{
        		mixing_coeff_octave_U = s4_lpf;
        	}
        	else
        	{
    			mixing_coeff_octave_U = 0.0f;
        	}

        	if(SENSOR_THRESHOLD_ORANGE_2)
        	{
        		direction = 0;
        	}
        	else
        	{
        		direction = 1;
        	}

    		s3_lpf = s3_lpf + S3_LPF_ALPHA * (ir_res[2] - s3_lpf);
        	//printf("s3_lpf = %.02f\n", s3_lpf);

        	if(SENSOR_THRESHOLD_BLUE_5)
        	{
        		mixing_coeff_octave_D = s3_lpf;
        	}
        	else
        	{
    			mixing_coeff_octave_D = 0.0f;
        	}
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
			#define CH344_UI_CMD_OVERSAMPLE_DOWN	1
			#define CH344_UI_CMD_OVERSAMPLE_UP		2
			#define CH344_UI_CMD_PERSISTENCE_DOWN	3
			#define CH344_UI_CMD_PERSISTENCE_UP		4

			//map UI commands
			#ifdef BOARD_WHALE
			if(short_press_volume_plus) { ui_command = CH344_UI_CMD_OVERSAMPLE_UP; short_press_volume_plus = 0; }
			if(short_press_volume_minus) { ui_command = CH344_UI_CMD_OVERSAMPLE_DOWN; short_press_volume_minus = 0; }
			if(short_press_sequence==2) { ui_command =  CH344_UI_CMD_PERSISTENCE_UP; short_press_sequence = 0; }
			if(short_press_sequence==-2) { ui_command = CH344_UI_CMD_PERSISTENCE_DOWN; short_press_sequence = 0; }
			#endif

			#ifdef BOARD_GECHO
			if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = CH344_UI_CMD_OVERSAMPLE_DOWN; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = CH344_UI_CMD_OVERSAMPLE_UP; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_1) { ui_command = CH344_UI_CMD_PERSISTENCE_DOWN; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_2) { ui_command =  CH344_UI_CMD_PERSISTENCE_UP; btn_event_ext = 0; }
			#endif

			if(ui_command==CH344_UI_CMD_OVERSAMPLE_DOWN || ui_command==CH344_UI_CMD_OVERSAMPLE_UP)
			{
				if(ui_command==CH344_UI_CMD_OVERSAMPLE_DOWN)
				{
					if(ch344_oversample<8)
					{
						//dco_oversample*=2;
						ch344_oversample++;
					}
				}
				else
				{
					if(ch344_oversample>1)
					{
						//dco_oversample/=2;
						ch344_oversample--;
					}
				}
				printf("#344 Oversample = %d\n", ch344_oversample);
				LED_B5_set_byte((ch344_oversample>4) ? 0x1f>>(ch344_oversample-4) : 0x1f<<(5-ch344_oversample) );
			}

			if(ui_command==CH344_UI_CMD_PERSISTENCE_DOWN || ui_command==CH344_UI_CMD_PERSISTENCE_UP)
			{
				if(ui_command==CH344_UI_CMD_PERSISTENCE_DOWN)
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
				printf("#344 Delay persistence = %.02f/%.02f\n", ECHO_MIXING_GAIN_MUL, ECHO_MIXING_GAIN_DIV);
				LED_W8_set_byte(0x01<<(lround(ECHO_MIXING_GAIN_MUL)-ECHO_MIXING_GAIN_MUL_MIN));
			}
		}

	} //end skip channel cnt

	//return input select back to previous user-defined value
	codec_select_input(ADC_input_select);
}
