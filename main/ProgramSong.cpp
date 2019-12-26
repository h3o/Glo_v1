/*
 * ProgramSong.cpp
 *
 *  Created on: Nov 00, 2019
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

#include <ProgramSong.h>
#include <InitChannels.h>
#include <FilteredChannels.h>

#include <Interface.h>
#include <hw/codec.h>
#include <hw/midi.h>
#include <hw/leds.h>
#include <hw/sdcard.h>
#include <hw/gpio.h>
//#include <dsp/FreqDetect.h>
#include <dsp/Filters.h>

#include <string.h>

void program_song(int action)
{
    printf("program_song(): action = %d\n", action);
	//song = new MusicBox(0);

	if(action==PROGRAM_SONG_ACTION_NEW)
	{
		char *notes_buf = (char*)malloc(10000);

		int res = edit_chords_by_buttons();
		if(res!=-1)
		{
		    if(res==0) //entered all required chords
		    {
		    	printf("program_song(): user created song with %d chords at 0x%lx\n", temp_total_chords, (unsigned long)temp_song);
		    	encode_temp_song_to_notes(temp_song, temp_total_chords, &notes_buf);
		    	printf("program_song(): song = [%s]\n", notes_buf);
		    	store_song_nvs(notes_buf, 0);
		    	indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_FILL_8_RIGHT, 1, 100);
		    }
		    else //exited without completing
			{
		    	printf("program_song(): user has not finished creating new song\n");
				indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_CLEAR_8_LEFT, 1, 50);
			}
	    	whale_restart();
		}
		else
		{
			int notes_collected = 0;
			char *notes_buf_ptr = notes_buf;
			int note_len;

			sampleCounter = 0;
			seconds = 0;
			program_settings_reset();

			printf("program_song(PROGRAM_SONG_ACTION_NEW): function loop starting\n");

			channel_running = 1;
			volume_ramp = 1;
			int done = 0;


			while(!event_next_channel && !done)
			{
				sampleCounter++;

				if(MIDI_note_on)
				{
					if(notes_collected%3==0)
					{
						if(notes_collected)
						{
							notes_buf_ptr[0] = ',';
							notes_buf_ptr++;
							notes_buf_ptr[0] = 0;
						}

						LED_W8_all_OFF();
						LED_B5_all_OFF();
						LED_sequencer_indicate_position(notes_collected/3);
					}

					note_len = MIDI_to_note(MIDI_last_chord[0], notes_buf_ptr);
					notes_buf_ptr += note_len;

					MIDI_to_LED(MIDI_last_chord[0], 1);

					notes_collected++;
					MIDI_note_on = 0;

					printf("program_song(): collected %d notes: %s\n", notes_collected, notes_buf);
				}

				if(event_channel_options)
				{
					indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_FILL_8_RIGHT, 1, 100);
					store_song_nvs(notes_buf, 0);
					done = 1;
				}
			}
			if(!done) //exited without saving
			{
				indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_CLEAR_8_LEFT, 1, 50);
			}
		}
		free(notes_buf);
		printf("program_song(PROGRAM_SONG_ACTION_NEW): function loop finished\n");
	}
	else if(action==PROGRAM_SONG_ACTION_LOAD)
	{
		char *buf = (char*)malloc(10000);
		printf("program_song(): waiting for user to enter song id\n");
		int song_id = get_user_number();
		if(song_id==GET_USER_NUMBER_CANCELED)
		{
			printf("program_song(): user canceled action\n");
		}
		else
		{
			printf("program_song(): user entered song_id = %d\n", song_id);

			if(song_id)
			{
				if(load_song_nvs(buf, song_id))
				{
					printf("program_song(): song loaded = [%s]\n", buf);
					store_song_nvs(buf, 0);
					indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_FILL_8_RIGHT, 1, 100);
				}
				else
				{
					indicate_context_setting(0xff, 4, 100);
				}
			}
		}
		free(buf);
	}
	else if(action==PROGRAM_SONG_ACTION_STORE)
	{
		char *buf = (char*)malloc(10000);
		printf("program_song(): waiting for user to enter song id\n");
		int song_id = get_user_number();
		if(song_id==GET_USER_NUMBER_CANCELED)
		{
			printf("program_song(): user canceled action\n");
		}
		else
		{
			printf("program_song(): user entered song_id = %d\n", song_id);

			if(song_id)
			{
				if(load_song_nvs(buf, 0))
				{
					store_song_nvs(buf, song_id);
					indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_FILL_8_RIGHT, 1, 100);
				}
				else
				{
					indicate_context_setting(0xff, 4, 100);
				}
			}
		}
		free(buf);
	}
	else if(action==PROGRAM_SONG_ACTION_CLEAR)
	{
		printf("program_song(): waiting for user to enter song id\n");
		int song_id = get_user_number();
		printf("program_song(): user entered song_id = %d\n", song_id);
		if(song_id==GET_USER_NUMBER_CANCELED)
		{
			printf("program_song(): user canceled action\n");
		}
		else if(delete_song_nvs(song_id))
		{
			indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_CLEAR_8_LEFT, 3, 50);
		}
		else
		{
			indicate_context_setting(0xff, 4, 100);
		}
	}
}

int current_chord = 0;
int temp_total_chords = 8; //can be 1-8 with multiplier 1-4 (so, total of 1-32)
int total_chords_row1 = 4;
int total_chords_row2 = 2; //default 4x2 = 8 chords

int *current_chord_LEDs; //pointer into another array
int current_melody_LED;
float current_melody_freq;
int preview_chord_running = 0;

void display_chord_position()
{
	//int current_chord = 0;
	//int temp_total_chords = 8; //can be 1-8 with multiplier 1-4 (so, total of 1-32)
	//int total_chords_row1 = 8;
	//int total_chords_row2 = 1;

	LED_R8_all_OFF();

	//light up a Red LED according to current chord's position
	LED_R8_set(current_chord % total_chords_row1, 1);

	LED_O4_all_OFF();

	//light up an Orange LED according to current chord's position
	LED_O4_set(current_chord / total_chords_row1, 1);
}

int button_pressed_timing[6] = {0,0,0,0,0,0};
#define BUTTON_PRESSED_THRESHOLD 100

int user_button_status(int button)
{
	if(button==0)
	{
		return BUTTON_U1_ON;
	}
	if(button==1)
	{
		return BUTTON_U2_ON;
	}
	if(button==2)
	{
		return BUTTON_U3_ON;
	}
	if(button==3)
	{
		return BUTTON_U4_ON;
	}
	if(button==4)
	{
		return BUTTON_SET_ON;
	}
	if(button==5)
	{
		return BUTTON_RST_ON;
	}
	return 0;
}

int get_button_pressed()
{
	for(int i=0;i<6;i++)
	{
		if(user_button_status(i))
		{
			if(button_pressed_timing[i] > -1)
			{
				button_pressed_timing[i]++;
				if(button_pressed_timing[i] == BUTTON_PRESSED_THRESHOLD)
				{
					button_pressed_timing[i] = -1; //blocked until released
					return i+1; //return button number (1-6)
				}
			}
		}
		else
		{
			button_pressed_timing[i] = 0;
		}
	}
	return 0;
}

void reset_button_pressed_timings()
{
	for(int i=0;i<6;i++)
	{
		button_pressed_timing[i] = 0;
	}
}

void wait_till_all_buttons_released()
{
	int wait = 1;
	while(wait)
	{
		wait = 0;
		for(int i=0;i<6;i++)
		{
			if(user_button_status(i))
			{
				wait = 1;
			}
		}
	}
	reset_button_pressed_timings();
}

int set_number_of_chords() //define number of chords
{
	int button_pressed;

	display_number_of_chords(total_chords_row1, total_chords_row2);

	while(1)
	{
		if(MIDI_keys_pressed)
		{
			return -1;
		}

		if((button_pressed = get_button_pressed()))
		{
			if(button_pressed == 1) //U1 button pressed -> decrement number of chords
			{
				total_chords_row1--;
				if(total_chords_row1==0)
				{
					total_chords_row1 = 8;
				}
			}
			if(button_pressed == 2) //U2 button pressed -> increment number of chords
			{
				total_chords_row1++;
				if(total_chords_row1==9)
				{
					total_chords_row1 = 1;
				}
			}
			if(button_pressed == 3) //U3 button pressed -> decrement multiplier
			{
				total_chords_row2--;
				if(total_chords_row2==0)
				{
					total_chords_row2 = 4;
				}
			}
			if(button_pressed == 4) //U4 button pressed -> increment multiplier
			{
				total_chords_row2++;
				if(total_chords_row2==5)
				{
					total_chords_row2 = 1;
				}
			}
			/*if(button_pressed == 4) //U4 button pressed -> set defaults
			{
				total_chords_row1 = 8;
				total_chords_row2 = 1;
			}*/

			display_number_of_chords(total_chords_row1, total_chords_row2);

			if(button_pressed == 5) //SET pressed -> next step
			{
				button_pressed = 0; //clear so will not trigger button press  event in next programming step
				return total_chords_row1 * total_chords_row2;
			}

			if(button_pressed == 6) //RST pressed -> exit channel
			{
				button_pressed = 0; //clear so will not trigger button press  event in next programming step
				return 0;
			}
		}
	}
}

int set_number_of_chords(int *row1, int *row2) //define number of chords, that returns more detailed information
{
	int chords = set_number_of_chords();
	*row1 = total_chords_row1;
	*row2 = total_chords_row2;
	return chords;
}

void goto_previous_chord()
{
	current_chord--;

	if(current_chord == -1)
	{
		current_chord = temp_total_chords - 1;
	}
}

void goto_next_chord()
{
	current_chord++;

	if(current_chord >= temp_total_chords)
	{
		current_chord = 0;
	}
}

int chord_blink_timer = 0;
void blink_current_chord(int chord)
{
	if(chord_blink_timer==0)
	{
		LED_R8_all_OFF();
		LED_O4_all_OFF();
	}
	if(chord_blink_timer%40000==0)
	{
		//get correct combination of leds
		int led_row1 = current_chord % total_chords_row1;
		int led_row2 = current_chord / total_chords_row1;
		LED_R8_set(led_row1, 1); //turn on LED, numbered from 0
		LED_O4_set(led_row2, 1); //turn on LED, numbered from 0
	}
	if(chord_blink_timer%40000==32000)
	{
		LED_R8_all_OFF();
		LED_O4_all_OFF();
	}
	chord_blink_timer++;
}

void start_playing_current_chord(int (*temp_song)[NOTES_PER_CHORD], int current_chord)
{
	int chord_notes[NOTES_PER_CHORD];

	for(int i=0;i<NOTES_PER_CHORD;i++)
	{
		chord_notes[i] = temp_song[current_chord][i];
	}
	if(chord_notes[0]==0) //no chord defined yet
	{

	}
}

#define FEED_DAC_WITH_SILENCE sample32=0;i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY)

void change_current_chord(int (*temp_song)[NOTES_PER_CHORD], int direction, int alteration)
{
	//FEED_DAC_WITH_SILENCE;

	int new_chord_set = 0; //indicates whether the chord was set to previously empty space, and not copied over

	if(temp_song[current_chord][0]==0) //if no chord at this position yet, set the default one
	{
		//printf("change_current_chord(): no chord yet at position %d, setting default one\n", current_chord);
		if(current_chord==0 || temp_song[current_chord-1][0]==0) //if first chord or none to copy from, fill in default
		{
			temp_song[current_chord][0] = 301; //c3
			temp_song[current_chord][1] = 305; //e3
			temp_song[current_chord][2] = 308; //g3

			new_chord_set = 1;

			//printf("change_current_chord(): default chord set at position %d\n", current_chord);
		}
		else //copy over previous chord
		{
			temp_song[current_chord][0] = temp_song[current_chord-1][0];
			temp_song[current_chord][1] = temp_song[current_chord-1][1];
			temp_song[current_chord][2] = temp_song[current_chord-1][2];

			//printf("change_current_chord(): previous chord copied over from position %d to %d\n", current_chord-1, current_chord);
		}
		//printf("change_current_chord(): setting chords map flag at position %d\n", current_chord);
		set_chords_map[current_chord]++;
	}

	//FEED_DAC_WITH_SILENCE;

	if(direction!=0 && !new_chord_set)
	{
		//check boundaries
		if((direction > 0) && (temp_song[current_chord][0] >= 500))
		{

		}
		else if((direction < 0) && (temp_song[current_chord][0] <= 201))
		{

		}
		else
		{
			for(int i=0;i<NOTES_PER_CHORD;i++)
			{
				temp_song[current_chord][i] += direction;

				if(temp_song[current_chord][i] % 100 == 0) //carry down
				{
					temp_song[current_chord][i] = 100*(temp_song[current_chord][i]/100-1)+12;
				}
				if(temp_song[current_chord][i] % 100 == 13) //carry up
				{
					temp_song[current_chord][i] = 100*(temp_song[current_chord][i]/100+1)+1;
				}

				//FEED_DAC_WITH_SILENCE;
			}
		}
	}
	else if(alteration > 0)
	{
		//FEED_DAC_WITH_SILENCE;

		if(alteration==12) //major->minor
		{
			if(temp_song[current_chord][1] - temp_song[current_chord][0] == 4) //currently major
			{
				temp_song[current_chord][1] --; //shift to minor
			}
			else //if(temp_song[current_chord][1] - temp_song[current_chord][0] == 3) //currently minor, exotic chords remain unchanged
			{
				//temp_song[current_chord][1] ++;
				temp_song[current_chord][1] = temp_song[current_chord][0] + 4; //back to major, resets exotic chords too
				temp_song[current_chord][2] = temp_song[current_chord][0] + 7;
			}
		}
		if(alteration==13) //move the 2nd note around
		{
			temp_song[current_chord][1] ++;

			//check if ended up conflicting with 3rd note
			if(temp_song[current_chord][1] == temp_song[current_chord][2])
			{
				temp_song[current_chord][1] = temp_song[current_chord][0] + 2; //go to two half-tones above key note
			}

			if(temp_song[current_chord][1] - temp_song[current_chord][0] > 5) //if more than tertia
			{
				temp_song[current_chord][1] = temp_song[current_chord][0] + 2; //go to two half-tones above key note
			}
		}
		if(alteration==14) //move the 3rd note around
		{
			temp_song[current_chord][2] ++;
			if(temp_song[current_chord][2] - temp_song[current_chord][0] > 10) //if more than three half-tones above kvinta
			{
				temp_song[current_chord][2] = temp_song[current_chord][0] + 5; //go to 2 half-tones under kvinta

				//check if ended up conflicting with 2nd note
				if(temp_song[current_chord][2] == temp_song[current_chord][1])
				{
					temp_song[current_chord][2]++; //move one half-tone up
				}
			}
		}
	}
	//FEED_DAC_WITH_SILENCE;
}

void start_preview_chord(int *notes, int notes_per_chord)
{
	if(notes[0]==0)
	{
		return;
	}

	//codec_reset(); //in case it was just running
	//int running = 1;

	//FEED_DAC_WITH_SILENCE;
	preview_chord_running = 0; //stop previous preview if still running

	float bases[notes_per_chord];
	float bases_expanded[CHORD_MAX_VOICES];
	int octave,note,octave_shift;
	double note_freq;
	double halftone_step_coef = HALFTONE_STEP_COEF;

	//convert notes to frequencies
	for(int n=0;n<notes_per_chord;n++)
	{
		//FEED_DAC_WITH_SILENCE;
		octave = notes[n] / 100;
		note = notes[n] % 100 - 1; //note numbered from 0 to 11
		//add halftones... 12 halftones is plus one octave -> frequency * 2

		//note_freq = pow(halftone_step_coef, note+3);

		//pow is slow, substituted with multiplications so we can feed dack during calculation
		note_freq = 1;

		for(int h=0;h<note+3;h++)
		{
			//FEED_DAC_WITH_SILENCE;
			note_freq *= halftone_step_coef;
		}

		//FEED_DAC_WITH_SILENCE;
		note_freq *= NOTE_FREQ_A4;

		//shift according to the octave
		octave_shift = octave - 4; //ref note is A4 - fourth octave

		if(octave_shift > 0) //shift up
		{
			do
			{
				//FEED_DAC_WITH_SILENCE;
				note_freq *= 2;
				octave_shift--;
			}
			while(octave_shift!=0);
		}
		else if(octave_shift < 0) //shift down
		{
			do
			{
				//FEED_DAC_WITH_SILENCE;
				note_freq /= 2;
				octave_shift++;
			}
			while(octave_shift!=0);
		}

		bases[n] = note_freq;
	}

	for(int t=0;t<CHORD_MAX_VOICES;t++)
	{
		//FEED_DAC_WITH_SILENCE;
		if(fil->chord->expand_multipliers[t][1] > 0)
		{
			bases_expanded[t] = bases[fil->chord->expand_multipliers[t][0]] * (float)fil->chord->expand_multipliers[t][1];
		}
		else
		{
			bases_expanded[t] = bases[fil->chord->expand_multipliers[t][0]] / (float)(-fil->chord->expand_multipliers[t][1]);
		}
		//if(bases_expanded[t]>5000)
		//{
		//	bases_expanded[t] /= 2;
		//}
	}

	for(int i=0;i<FILTERS;i++)
	{
		//FEED_DAC_WITH_SILENCE;
		fil->iir2[i]->setResonanceKeepFeedback(0.9995f);
		fil->iir2[i]->setCutoffAndLimits(bases_expanded[i%CHORD_MAX_VOICES] * FILTERS_FREQ_CORRECTION / (float)I2S_AUDIOFREQ * 2 * 3);
		//reset volumes, they might be turned down by magnetic ring capture method
		fil->fp.mixing_volumes[i] = 1.0f;
		fil->fp.mixing_volumes[FILTERS/2 + i] = 1.0f;
	}

	//FEED_DAC_WITH_SILENCE;
	sampleCounter = 0;
	seconds = 0;
	preview_chord_running = 1;
	codec_set_mute(0);
}

void display_and_preview_current_chord(int (*temp_song)[NOTES_PER_CHORD])
{
	int8_t led_numbers[NOTES_PER_CHORD];

	//printf("display_and_preview_current_chord(): notes_to_LEDs\n");
	notes_to_LEDs(temp_song[current_chord],led_numbers,NOTES_PER_CHORD);
	//printf("display_and_preview_current_chord(): display_chord\n");
	display_chord(led_numbers, NOTES_PER_CHORD); //array of 3, 0-7 for whites, 10-14 for blues
	//printf("display_and_preview_current_chord(): start_preview_chord\n");
	start_preview_chord(temp_song[current_chord], NOTES_PER_CHORD);
	//printf("display_and_preview_current_chord(): start_preview_chord completed\n");
}

void set_current_chord(int (*temp_song)[NOTES_PER_CHORD], int *notes, int notes_n)
{
	for(int i=0;i<notes_n;i++)
	{
		temp_song[current_chord][i] = notes[i]; //301==c3, 305==e3, 308==g3
	}
	set_chords_map[current_chord]++;

	display_and_preview_current_chord(temp_song);
}

int count_unset_chords()
{
	int unset = 0;
	for(int i=0;i<temp_total_chords;i++)
	{
		if(set_chords_map[i]==0)
		{
			unset++;
		}
	}
	return unset;
}

//int preview_chord_mute = 0;
void process_preview_chord()
{
	float r;

	if(preview_chord_running)
	{
		if (!(sampleCounter & 0x00000001)) //left or right channel
		{
			r = PseudoRNG1a_next_float();
			memcpy(&random_value, &r, sizeof(random_value));
		}

		//printf("[x]");

		if (sampleCounter & 0x00000001) //left or right channel
		{
			sample_f[0] = ( (float)(32768 - (int16_t)random_value) / 32768.0f) * noise_volume;
			sample_mix = IIR_Filter::iir_filter_multi_sum(sample_f[0],fil->iir2,FILTERS/2,fil->fp.mixing_volumes)*MAIN_VOLUME; //volume2;
			//sample_mix = sample_f[0] * volume2 / 5;
		}
		else
		{
			sample_f[1] = ( (float)(32768 - (int16_t)(random_value>>16)) / 32768.0f) * noise_volume;
			sample_mix = IIR_Filter::iir_filter_multi_sum(sample_f[1],fil->iir2+FILTERS/2,FILTERS/2,fil->fp.mixing_volumes+FILTERS/2)*MAIN_VOLUME; //volume2;
			//sample_mix = sample_f[1] * volume2 / 5;
		}

		sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);

		if (!(sampleCounter & 0x00000001)) //left or right channel
		{
			sample32 = sample_i16 << 16;
		}
		else
		{
			sample32 += sample_i16;

			i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
		    //sd_write_sample(&sample32);

			//i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
		}

		/*
		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE)) {};
		//if(preview_chord_mute)
		//{
		//	SPI_I2S_SendData(CODEC_I2S, 0);
		//	preview_chord_mute--;
		//}
		//else
		//{
			SPI_I2S_SendData(CODEC_I2S, sample_i16);
		//}
		*/

		sampleCounter++;

		if (TIMING_BY_SAMPLE_1_SEC)
		{
			sampleCounter = 0;
			seconds++;

			if(seconds == 2)
			{
				preview_chord_running = 0;
				//codec_reset();
				codec_set_mute(1); //stop the sound so there is no buzzing

				//turn off codec volume
				//mute_master_playback(1);

				seconds = 0;
			}
		}
	}
	else
	{
		FEED_DAC_WITH_SILENCE;
		//preview_chord_mute = 10000;
	}
}

void indicate_not_enough_chords()
{
	int i,j;

	for(j=0;j<6;j++)
	{
		if(preview_chord_running)
		{
			process_preview_chord();
			//LED_R8_set_byte(int value);
			//LED_O4_set_byte(int value);

			display_number_of_chords(total_chords_row1, total_chords_row2);
			for(i=0;i<SAMPLE_RATE_DEFAULT/8;i++)
			{
				process_preview_chord();
			}
			LED_R8_set_byte(0);
			LED_O4_set_byte(0);
			for(i=0;i<SAMPLE_RATE_DEFAULT/8;i++)
			{
				process_preview_chord();
			}
		}
		else
		{
			display_number_of_chords(total_chords_row1, total_chords_row2);
			Delay(250/4);
			LED_R8_set_byte(0);
			LED_O4_set_byte(0);
			Delay(250/4);
		}
	}
}

int button_combination_already_returned = 0;

int get_button_combination()
{
	int i,j;

	for(i=0;i<6;i++)
	{
		if(button_pressed_timing[i] == -1) //already held
		{
			//check other buttons for combo
			for(j=0;j<6;j++)
			{
				if(i!=j)
				{
					if(user_button_status(j)) //other button pressed simultaneously
					{
						//button_pressed_timing[i] = 0;
						button_pressed_timing[j] = 0;
						button_combination_already_returned = 1;
						return 10*(i+1) + j+1;
					}
				}
			}

			//check if button just released
			{
				if(!user_button_status(i)) //not pressed anymore
				{
					button_pressed_timing[i] = 0;
					if(button_combination_already_returned)
					{
						button_combination_already_returned = 0;
						return 0;
					}

					return i+1; //return button number (1-6)
				}
			}
		}
		else if(user_button_status(i))
		{
			if(button_pressed_timing[i] > -1)
			{
				button_pressed_timing[i]++;
				if(button_pressed_timing[i] == BUTTON_PRESSED_THRESHOLD)
				{
					button_pressed_timing[i] = -1; //blocked until released
					return 0;
				}
			}
		}
		else //???
		{
			button_pressed_timing[i] = 0;
		}
	}
	return 0;
}

int edit_chords_by_buttons()
{
	int finished = 0;
	int button_pressed;

	printf("edit_chords_by_buttons(): codec_digital_volume=%d, codec_volume_user=%d\n", codec_digital_volume, codec_volume_user);
	//preserve the user-defined codec volume so it is not overwritten when muting codec before anything plays
	codec_digital_volume=codec_volume_user;
	codec_set_mute(1); //stop the sound so there is no buzzing

	temp_total_chords = set_number_of_chords();

	if(temp_total_chords==-1) //MIDI detected
	{
		printf("edit_chords_by_buttons(): MIDI detected\n");
		return -1;
	}

	if(!temp_total_chords) //RST pressed
	{
		finished = 2; //exit the channel
	}
	else
	{
		//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
		channel_init(0, 1000, 0, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 0, 0, ACTIVE_FILTER_PAIRS_ALL);
		printf("edit_chords_by_buttons(): channel_init() completed\n");

	    channel_running = 0;
	    volume_ramp = 0;
		sensors_active = 0;
		accelerometer_active = 0;

		printf("edit_chords_by_buttons(): allocating memory for %d chords\n", temp_total_chords);
		temp_song = (int (*)[NOTES_PER_CHORD])malloc(temp_total_chords * sizeof(*temp_song));
		memset(temp_song,0,temp_total_chords * sizeof(*temp_song));
		set_chords_map = (int *)malloc(temp_total_chords * sizeof(int));
		memset(set_chords_map,0,temp_total_chords * sizeof(int));
		printf("edit_chords_by_buttons(): memory allocated and cleared\n");

		printf("edit_chords_by_buttons(): display_chord_position()\n");
		display_chord_position();
		printf("edit_chords_by_buttons(): display_chord_position() completed\n");

	    //check if the first chord is currently defined and play it
		printf("edit_chords_by_buttons(): if(temp_song[current_chord][2]>0) -> current_chord = %d\n", current_chord);
		printf("edit_chords_by_buttons(): temp_song[current_chord][2] = %d\n", temp_song[current_chord][2]>0);
		if(temp_song[current_chord][2]>0)
		{
			printf("edit_chords_by_buttons(): display_and_preview_current_chord(temp_song)\n");
			display_and_preview_current_chord(temp_song);
			//start_playing_current_chord(temp_song,current_chord);
		}
	}

	//reset_button_pressed_timings();
	printf("edit_chords_by_buttons(): wait_till_all_buttons_released()\n");
	wait_till_all_buttons_released();

    /*
	channel_running = 1;
    volume_ramp = 1;
    ui_button3_enabled = 0;
    ui_button4_enabled = 0;
    */
	codec_set_mute(0); //un-mute the codec

	printf("edit_chords_by_buttons(): while(!finished)\n");
	while(!finished)
	{
		button_pressed = get_button_combination();

		process_preview_chord();

		if(button_pressed)
		{
			printf("edit_chords_by_buttons(): button_pressed = %d\n", button_pressed);

			//???????????????????????????????????????????????????????????????
			if(button_pressed > 10)
			{
				//while(any_button_held()) //wait till all released, while feeding the DAC
				while(user_button_status(button_pressed%10-1)) //wait till 2nd button is released
				{
					process_preview_chord();
				}
				change_current_chord(temp_song, 0, button_pressed); //current chord with alteration
				display_and_preview_current_chord(temp_song);
			}
			if(button_pressed == 1) //previous chord
			{
				printf("edit_chords_by_buttons(): change to lower chord\n");
				change_current_chord(temp_song, -1, 0);
				//printf("edit_chords_by_buttons(): change to lower chord completed\n");
				display_and_preview_current_chord(temp_song);
				//printf("edit_chords_by_buttons(): display_and_preview_current_chord() completed\n");
			}
			if(button_pressed == 2) //next chord
			{
				printf("edit_chords_by_buttons(): change to higher chord\n");
				change_current_chord(temp_song, +1, 0);
				//printf("edit_chords_by_buttons(): change to higher chord completed\n");
				display_and_preview_current_chord(temp_song);
				//printf("edit_chords_by_buttons(): display_and_preview_current_chord() completed\n");
			}
			if(button_pressed == 3)
			{
				printf("edit_chords_by_buttons(): go to previous chord\n");
				goto_previous_chord();
				display_chord_position();
				display_and_preview_current_chord(temp_song);
				//printf("edit_chords_by_buttons(): display_and_preview_current_chord() completed\n");
			}
			if(button_pressed == 4)
			{
				printf("edit_chords_by_buttons(): go to next chord\n");
				goto_next_chord();
				display_chord_position();
				display_and_preview_current_chord(temp_song);
				//printf("edit_chords_by_buttons(): display_and_preview_current_chord() completed\n");
			}
			if(button_pressed == 5) //SET pressed
			{
				printf("edit_chords_by_buttons(): SET pressed\n");
				if(count_unset_chords())
				{
					printf("edit_chords_by_buttons(): not enough chords\n");
					//some chords not set, indicate error
					indicate_not_enough_chords();
					display_chord_position();
				}
				else //if(!preview_chord_running)
				{
					printf("edit_chords_by_buttons(): enough chords, finished\n");
					//preview finished and all chords are set, move on to the next step
					finished = 1;
				}
			}
			if(button_pressed == 6) //RST pressed
			{
				printf("edit_chords_by_buttons(): RST pressed, exiting\n");
				finished = 2;
			}

			//process_preview_chord();

			/*
			blink_current_chord(current_chord);
			*/
		}

		process_preview_chord();
	}

	if(preview_chord_running)
	{
		codec_set_mute(1); //stop the sound so there is no buzzing
		preview_chord_running = 0;
	}

	if(finished==1) //finished normally, not exited by RST
	{
		//indicate enough chords set/collected
		for(int j=0;j<2;j++)
		{
			display_number_of_chords(total_chords_row1, total_chords_row2);
			Delay(500);
			LED_R8_set_byte(0);
			LED_O4_set_byte(0);
			Delay(500);
		}
		return 0;
	}
	return 1;
}

