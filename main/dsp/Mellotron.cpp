/*
 * Mellotron.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 11 Jul 2021
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

#include "Mellotron.h"
#include "hw/leds.h"
#include "hw/sdcard.h"
#include "hw/midi.h"
#include "ui.h"

//#define ENABLE_ECHO

#define MELLOTRON_POLYPHONY 3 //2

#define VOICE_OVERRIDE_RAMP_COEFF	0.999f
float voice_override_ramp[2] = {0.0f, 0.0f};
int voice_override_ramp_active = 0;
#define VOICE_RAMP_EPSILON	1.0f

//#define DEBUG_MELLOTRON

void channel_SD_mellotron()
{
	int sd_playing = 0, voice_slot = 0;
	uint32_t silence = 0, ramp, sample_mix;

	char midi_note_str[10];
	//uint32_t *SD_play_buf1, *SD_play_buf2;
	int voice_file_ptr = 0;
	char *filename = (char*)malloc(300);

	uint8_t voices_playing[MELLOTRON_POLYPHONY];// = {0,0};
	FILE *SD_v[MELLOTRON_POLYPHONY];// = {NULL, NULL};
    unsigned int sd_blocks_read[MELLOTRON_POLYPHONY];
    unsigned long sd_file_size[MELLOTRON_POLYPHONY];
    //unsigned long sd_file_ptr[MELLOTRON_POLYPHONY];
    uint32_t last_sample_in_buffer[MELLOTRON_POLYPHONY];

	for(int i=0;i<MELLOTRON_POLYPHONY;i++)
	{
		voices_playing[i] = 0;
		//SD_v[i] = NULL;
	}

	printf("channel_SD_mellotron(): free mem = %u\n", xPortGetFreeHeapSize());

    char **files_list = (char**)malloc(200 * sizeof(char*));
	int files_found = 0;
	sd_card_file_list("mellotron", 1, files_list, &files_found, 1); //recursive, add dirs to the list as well

	printf("channel_SD_mellotron(): found %d files\n", files_found);
	for(int i=0;i<files_found;i++)
	{
		printf("%s\n", files_list[i]);
	}

	//if SD is already initialized, unmount as we need a different mode
	if(sd_card_ready)
	{
		// unmount partition and disable SDMMC or SPI peripheral
		esp_vfs_fat_sdmmc_unmount();
		//ESP_LOGI(TAG, "Card unmounted");
		sd_card_ready = 0;
	}

    printf("channel_SD_mellotron(): before sd_card_init(%d, ...), free mem = %u\n", MELLOTRON_POLYPHONY+1, xPortGetFreeHeapSize());

    if(!sd_card_ready)
	{
		//printf("channel_SD_mellotron(): SD Card init...\n");
		int result = sd_card_init(MELLOTRON_POLYPHONY+1, persistent_settings.SD_CARD_SPEED); //allow to open 10 concurrent files
	    if(result!=ESP_OK)
	    {
	    	printf("channel_SD_mellotron(): problem with SD Card Init, code=%d\n", result);
	    	indicate_error(0x55aa, 8, 80);
	    	return;
	    }
		//printf("done!\n");
	}

	printf("channel_SD_mellotron(): after sd_card_init(%d, ...), free mem = %u\n", MELLOTRON_POLYPHONY+1, xPortGetFreeHeapSize());

	sprintf(filename, "/sdcard/mellotron/%s/%s", files_list[0], files_list[1]);
	#ifdef DEBUG_MELLOTRON
	printf("channel_SD_mellotron(): checking the sample rate in the 1st file, opening file %s for reading\n", filename);
	#endif
	SD_v[0] = fopen(filename, "rb");

	if (SD_v[0] == NULL) {
		printf("channel_SD_mellotron(): Failed to open file %s for reading\n", filename);
	}

	//read sample rate info
	fseek(SD_v[0],24,SEEK_SET); //Sample rate
	uint32_t sampl_rate32;
	fread(&sampl_rate32,sizeof(sampl_rate32),1,SD_v[0]);
	fseek(SD_v[0],0,SEEK_SET);
	fclose(SD_v[0]);

	int sampling_rate = sampl_rate32, old_sampling_rate = current_sampling_rate;

	if(!is_valid_sampling_rate(sampling_rate))
	{
		printf("channel_SD_mellotron(): invalid sample rate detected: %d, will use %d instead\n", sampling_rate, persistent_settings.SAMPLING_RATE);
		sampling_rate = persistent_settings.SAMPLING_RATE;
	}

	if(sampling_rate!=current_sampling_rate)
	{
		printf("channel_SD_mellotron(): changing sample rate to %d\n", sampling_rate);
		set_sampling_rate(sampling_rate);
	}

    MIDI_parser_reset();

	sampleCounter = 0;

    printf("channel_SD_mellotron(): before malloc(SD_PLAYBACK_BUFFER), free mem = %u\n", xPortGetFreeHeapSize());

	//SD_play_buf = (uint32_t*)malloc(SD_PLAYBACK_BUFFER); //only allocate as many bytes as SD_PLAYBACK_BUFFER but in a 32-bit structure
	//SD_play_buf = (uint32_t*)malloc(SD_PLAYBACK_BUFFER * 2); //allocate space for double buffer
	SD_play_buf = (uint32_t*)malloc(SD_PLAYBACK_BUFFER * MELLOTRON_POLYPHONY); //allocate space for as many sectors as there is polyphony

    printf("channel_SD_mellotron(): after malloc(SD_PLAYBACK_BUFFER), free mem = %u\n", xPortGetFreeHeapSize());

    //printf("channel_SD_mellotron(): playback loop starting\n");

	channel_running = 1;
	sd_playback_channel = 1; //to disallow recording from this channel

	sensors_active = 1;

	volume_ramp = 0;
	//instead of volume ramp, set the codec volume instantly to un-mute the codec
	codec_digital_volume=codec_volume_user;
	codec_set_digital_volume();

	LED_R8_set_byte(0x00);
	LED_O4_set_byte(0x00);

	printf("channel_SD_mellotron(): loop starting\n");

	int start_notes[MELLOTRON_POLYPHONY],stop_notes[MELLOTRON_POLYPHONY], notes_started = 0, notes_stopped = 0;
	uint8_t MIDI_last_chord0[MELLOTRON_POLYPHONY];

	for(int i=0;i<MELLOTRON_POLYPHONY;i++)
	{
		start_notes[i] = 0;
		stop_notes[i] = 0;
		MIDI_last_chord0[i] = 0;
	}

	while(/*!feof(SD_f) &&*/ !event_next_channel
		//&& btn_event_ext!=BUTTON_EVENT_SHORT_PRESS+BUTTON_1
		//&& btn_event_ext!=BUTTON_EVENT_SHORT_PRESS+BUTTON_2)
		/*&& btn_event_ext!=BUTTON_EVENT_SET_PLUS+BUTTON_1
		&& btn_event_ext!=BUTTON_EVENT_SET_PLUS+BUTTON_2*/)
	{
        //Delay(200);
    	//printf("channel_SD_mellotron(): loop\n");

    	if(MIDI_notes_updated)
    	{
    		MIDI_notes_updated = 0;

    		if(MIDI_keys_pressed==1)
			{
				MIDI_last_chord[1] = 0;
			}
			if(MIDI_keys_pressed<3)
			{
				MIDI_last_chord[2] = 0;
			}
			#ifdef DEBUG_MELLOTRON
    		printf("MIDI=%d,%d,%d,%d (p=%d)\n", MIDI_last_chord[0], MIDI_last_chord[1], MIDI_last_chord[2], MIDI_last_chord[3], MIDI_keys_pressed);
			#endif

    		if(MIDI_keys_pressed==0)
    		{
    			//memcpy(stop_notes, MIDI_last_chord0, MELLOTRON_POLYPHONY);//different types!
    			for(int i=0;i<MELLOTRON_POLYPHONY;i++)
    			{
    				if(MIDI_last_chord0[i])
    				{
    					stop_notes[i] = MIDI_last_chord0[i];
    					notes_stopped = 1;
    				}
    			}
    		}
    		else
    		{
    			/*
    			for(int i=0;i<MIDI_keys_pressed;i++)
    			{
    				if(i==MELLOTRON_POLYPHONY) break;

    				if(MIDI_last_chord[i] != MIDI_last_chord0[i])
    				{
    					start_notes[i] = MIDI_last_chord[i];
    					stop_notes[i] = MIDI_last_chord0[i];
    				}
    			}
    			*/
    			for(int i=0;i<MELLOTRON_POLYPHONY;i++)
    			{
    				//start notes that did not exist previously
        			int existed = 0;
    				for(int j=0;j<MELLOTRON_POLYPHONY;j++)
        			{
    					if(MIDI_last_chord[i]==MIDI_last_chord0[j])
    					{
    						existed = 1;
    						break;
    					}

        			}
    				if(!existed)
    				{
    					start_notes[i] = MIDI_last_chord[i];
    					notes_started = 1;
    				}

    				//stop notes that disappeared
        			int still_exist = 0;
    				for(int j=0;j<MELLOTRON_POLYPHONY;j++)
        			{
    					if(MIDI_last_chord[j]==MIDI_last_chord0[i])
    					{
    						still_exist = 1;
    						break;
    					}

        			}
    				if(!still_exist)
    				{
    					stop_notes[i] = MIDI_last_chord0[i];
    					notes_stopped = 1;
    				}
    			}
    		}

			//memcpy(MIDI_last_chord0, MIDI_last_chord, 4);
    		memset(MIDI_last_chord0,0,MELLOTRON_POLYPHONY);
    		for(int i=0;i<MIDI_keys_pressed;i++) //only copy active notes, no expansions
			{
				if(i==MELLOTRON_POLYPHONY) break;
				MIDI_last_chord0[i] = MIDI_last_chord[i];
			}

			#ifdef DEBUG_MELLOTRON
    		printf("start_notes=%d,%d,%d stop_notes=%d,%d,%d\n", start_notes[0], start_notes[1], start_notes[2], stop_notes[0], stop_notes[1], stop_notes[2]);
			#endif
    		//memset(start_notes,0,MELLOTRON_POLYPHONY*sizeof(int));
    		//memset(stop_notes,0,MELLOTRON_POLYPHONY*sizeof(int));
    	}

        if(notes_stopped)
        {
			#ifdef DEBUG_MELLOTRON
    		printf("stopped_notes=%d,%d,%d\n", stop_notes[0], stop_notes[1], stop_notes[2]);
			#endif

			for(int j=0;j<MELLOTRON_POLYPHONY;j++)
			{
				if(!stop_notes[j]) continue;

				for(int i=0;i<MELLOTRON_POLYPHONY;i++)
				{
					if(voices_playing[i]==stop_notes[j])
					{
						#ifdef DEBUG_MELLOTRON
						printf("channel_SD_mellotron(): stopping voice at slot %d\n", i);
						#endif

						fclose(SD_v[i]);
						//printf("channel_SD_mellotron(): file at slot %d closed\n", i);
						//SD_v[i] = NULL;
						voices_playing[i] = 0;
						#ifdef DEBUG_MELLOTRON
						printf("sd_playing: %d => ", sd_playing);
						#endif
						sd_playing--;
						#ifdef DEBUG_MELLOTRON
						printf("%d\n", sd_playing);
						#endif

						if(voice_override_ramp_active)
						{
							voice_override_ramp[1] += (int16_t)(last_sample_in_buffer[i]);
							voice_override_ramp[0] += (int16_t)(last_sample_in_buffer[i]>>16);
							#ifdef DEBUG_MELLOTRON
							printf("voice_override_ramp += %f,%f\n", voice_override_ramp[1],voice_override_ramp[0]);
							#endif
						}
						else
						{
							voice_override_ramp[1] = (int16_t)(last_sample_in_buffer[i]);
							voice_override_ramp[0] = (int16_t)(last_sample_in_buffer[i]>>16);
							#ifdef DEBUG_MELLOTRON
							printf("voice_override_ramp => %f,%f\n", voice_override_ramp[1],voice_override_ramp[0]);
							#endif
						}

						if(fabs(voice_override_ramp[0])<VOICE_RAMP_EPSILON && fabs(voice_override_ramp[1])<VOICE_RAMP_EPSILON)
						{
							voice_override_ramp_active = 0;
							voice_override_ramp[0] = 0.0f;
							voice_override_ramp[1] = 0.0f;
						}
						else
						{
							voice_override_ramp_active = 1;
						}
						break;
					}
				}
			}

			notes_stopped = 0;
			memset(stop_notes,0,MELLOTRON_POLYPHONY*sizeof(int));
        }

        if(notes_started)
		{
			#ifdef DEBUG_MELLOTRON
    		printf("started_notes=%d,%d,%d\n", start_notes[0], start_notes[1], start_notes[2]);
			#endif

			for(int j=0;j<MELLOTRON_POLYPHONY;j++)
			{
				if(!start_notes[j]) continue;

				MIDI_to_note(start_notes[j], midi_note_str);			//with octave number
				MIDI_to_note_only(start_notes[j], midi_note_str+5);		//without octave number

				#ifdef DEBUG_MELLOTRON
				printf("channel_SD_mellotron(): MIDI note %d -> %s / %s\n", start_notes[j], midi_note_str, midi_note_str+5);
				#endif

				int sample_found = 0;
				for(int i=0;i<files_found;i+=2)
				{
					if(!strcmp(files_list[i],midi_note_str))
					{
						#ifdef DEBUG_MELLOTRON
						printf("channel_SD_mellotron(): found sample %s/%s\n", files_list[i], files_list[i+1]);
						#endif
						sample_found = 1;
						voice_file_ptr = i;
						break;
					}
				}
				if(!sample_found) //try to look up by note without octave
				{
					for(int i=0;i<files_found;i+=2)
					{
						if(!strcmp(files_list[i],midi_note_str+5))
						{
							#ifdef DEBUG_MELLOTRON
							printf("channel_SD_mellotron(): found sample %s/%s\n", files_list[i], files_list[i+1]);
							#endif
							sample_found = 1;
							voice_file_ptr = i;
							break;
						}
					}
				}

				if(!sample_found)
				{
					printf("channel_SD_mellotron(): no sample found for this note\n");
				}
				else if(sd_playing<MELLOTRON_POLYPHONY)
				{
					while(voices_playing[voice_slot]) //find free slot
					{
						voice_slot++;
						if(voice_slot==MELLOTRON_POLYPHONY)
						{
							voice_slot = 0;
						}
					}

					#ifdef DEBUG_MELLOTRON
					printf("channel_SD_mellotron(): new voice free slot: %d, sd_playing[0] = %d\n", voice_slot, sd_playing);
					#endif

					voices_playing[voice_slot] = start_notes[j];
					//char filename[200];
					sprintf(filename, "/sdcard/mellotron/%s/%s", files_list[voice_file_ptr], files_list[voice_file_ptr+1]);
					#ifdef DEBUG_MELLOTRON
					printf("channel_SD_mellotron(): Opening file %s for reading, slot = %d\n", filename, voice_slot);
					#endif
					SD_v[voice_slot] = fopen(filename, "rb");

					if (SD_v[voice_slot] == NULL) {
						printf("channel_SD_mellotron(): Failed to open file %s for reading\n", filename);
						voices_playing[voice_slot] = 0;
					}
					else
					{
						#ifdef DEBUG_MELLOTRON
						printf("channel_SD_mellotron(): file opened successfully, slot = %d\n", voice_slot);
						#endif

						//fseek(SD_f,44,SEEK_SET); //skip the header -> not a good idea, will throw off 4kb sencor reads

						fseek(SD_v[voice_slot],0,SEEK_END);
						sd_file_size[voice_slot] = ftell(SD_v[voice_slot]);
						fseek(SD_v[voice_slot],0,SEEK_SET);
						sd_blocks_read[voice_slot] = 0;
						//sd_file_ptr[voice_slot] = 0;

						#ifdef DEBUG_MELLOTRON
						printf("channel_SD_mellotron(): file size = %lu\n", sd_file_size[voice_slot]);
						#endif
						if(sd_file_size[voice_slot]<5000)
						{
							printf("channel_SD_mellotron(): file seems too short, empty or corrupt\n");
							fclose(SD_v[voice_slot]);
							voices_playing[voice_slot] = 0;
						}
						else
						{
							#ifdef DEBUG_MELLOTRON
							printf("sd_playing: %d => ", sd_playing);
							#endif
							sd_playing++;
							#ifdef DEBUG_MELLOTRON
							printf("%d\n", sd_playing);
							#endif
						}
					}
				}
			}

			notes_started = 0;
			memset(start_notes,0,MELLOTRON_POLYPHONY*sizeof(int));
		}

		if(voice_override_ramp_active)
		{
			if(fabs(voice_override_ramp[0])<VOICE_RAMP_EPSILON && fabs(voice_override_ramp[1])<VOICE_RAMP_EPSILON)
			{
				#ifdef DEBUG_MELLOTRON
				printf("voice_override_ramp_active => 0\n");
				#endif
				voice_override_ramp_active = 0;
			}
		}

		if(sd_playing)
		{
	    	//printf("channel_SD_mellotron(): SD playing\n");

			for(int i=0;i<MELLOTRON_POLYPHONY;i++)
			{
				if(voices_playing[i])
				{
					fread(SD_play_buf+i*SD_PLAYBACK_BUFFER_SEGMENT,SD_PLAYBACK_BUFFER,1,SD_v[i]);

					last_sample_in_buffer[i] = SD_play_buf[i*SD_PLAYBACK_BUFFER_SEGMENT+1023];
					#ifdef DEBUG_MELLOTRON
					printf("last_sample_in_buffer[%d] = SD_play_buf[%d] = %x\n", i, i*SD_PLAYBACK_BUFFER_SEGMENT+1023, SD_play_buf[i*SD_PLAYBACK_BUFFER_SEGMENT+1023]);
					#endif
				}
			}

			for(int sample_ptr = 0;sample_ptr < SD_PLAYBACK_BUFFER/4;sample_ptr++)
			{
				sample_mix = 0;

				for(int i=0;i<MELLOTRON_POLYPHONY;i++)
				{
					if(voices_playing[i])
					{
						if(sd_blocks_read[i] || sample_ptr > WAV_HEADER_SIZE/4) //first sector contains WAV header, we need to skip it
						{
							sample_mix += SD_play_buf[i*SD_PLAYBACK_BUFFER_SEGMENT+sample_ptr];
						}
					}
				}

				i2s_write(I2S_NUM, (void*)&sample_mix, 4, &i2s_bytes_rw, portMAX_DELAY);
			}

			for(int i=0;i<MELLOTRON_POLYPHONY;i++)
			{
				if(voices_playing[i])
				{
					sd_blocks_read[i]++;

					if(feof(SD_v[i]))
					{
						//fseek(SD_v[0],WAV_HEADER_SIZE,SEEK_SET);
						#ifdef DEBUG_MELLOTRON
						printf("channel_SD_mellotron(): playing done at slot %d\n", i);
						#endif
						fclose(SD_v[i]);
						//printf("channel_SD_mellotron(): file at slot %d closed\n", i);
						//SD_v[i] = NULL;
						voices_playing[i] = 0;
						#ifdef DEBUG_MELLOTRON
						printf("sd_playing: %d => ", sd_playing);
						#endif
						sd_playing--;
						#ifdef DEBUG_MELLOTRON
						printf("%d\n", sd_playing);
						#endif
					}
				}
			}
		}
		else
		{
			if(voice_override_ramp_active)
			{
				voice_override_ramp[0] *= VOICE_OVERRIDE_RAMP_COEFF;
				voice_override_ramp[1] *= VOICE_OVERRIDE_RAMP_COEFF;
				ramp = ((int16_t)(voice_override_ramp[0])<<16) | ((int16_t)(voice_override_ramp[1]));
				//printf("s,r=(%f,%f => %x)\n", voice_override_ramp[0], voice_override_ramp[1], ramp);
				i2s_write(I2S_NUM, (void*)&ramp, 4, &i2s_bytes_rw, portMAX_DELAY);
			}
			else
			{
				//printf("channel_SD_mellotron(): silence\n");
				//i2s_write_bytes(I2S_NUM, (char *)&silence, 4, portMAX_DELAY);
				i2s_write(I2S_NUM, (void*)&silence, 4, &i2s_bytes_rw, portMAX_DELAY);
			}
		}
	}

    printf("channel_SD_mellotron(): loop ended, freeing memory\n");

	free(SD_play_buf);
	//free(SD_play_buf1);
	//free(SD_play_buf2);

	for(int i=0;i<files_found;i++)
	{
		free(files_list[i]);
	}
    free(files_list);

    free(filename);

    printf("channel_SD_mellotron(): memory freed, umounting SD card\n");

    //all done, unmount the card
	if(sd_card_ready)
	{
		// unmount partition and disable SDMMC or SPI peripheral
		esp_vfs_fat_sdmmc_unmount();
		//ESP_LOGI(TAG, "Card unmounted");
		sd_card_ready = 0;
	}

	if(old_sampling_rate != current_sampling_rate)
	{
		printf("channel_SD_mellotron(): changing sampling rate from %d back to %d\n", current_sampling_rate, old_sampling_rate);
		set_sampling_rate(old_sampling_rate);
	}
}
