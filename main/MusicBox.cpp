/*
 * MusicBox.cpp
 *
 *  Created on: May 30, 2016
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

#include "MusicBox.h"
//#include <InitChannels.h>
#include <Interface.h>
#include "notes.h"
#include "glo_config.h"
#include <hw/leds.h>
#include <ctype.h>
#include <string.h>

const int MusicBox::expand_multipliers[MAX_VOICES_PER_CHORD][2] = {
	/*
	//v1. - more trebles, less basses
	{0, 1},		//source 0, no change
	{1, 1},		//source 1, no change
	{2, 1}, 	//source 2, no change
	{0, -2},	//source 0, freq / 2
	{0, 2},		//source 0, freq * 2
	{1, 2},		//source 1, freq * 2
	{2, 2},		//source 2, freq * 2
	{1, -2},	//source 1, freq / 2
	{2, -2}		//source 2, freq / 2
	*/

	//v2. - less trebles, more basses
	{0, 1},		//source 0, no change
	{1, 1},		//source 1, no change
	{2, 1}, 	//source 2, no change

	{0, 2},		//source 0, freq * 2 //only powers of 2 make sense to shift octaves up and down (2,4,8)
	{0, -2},	//source 0, freq / 2
	{0, 4},		//source 0, freq * 4
	{0, -4},	//source 0, freq / 4

	{1, 2},		//source 1, freq * 2
	{2, 2},		//source 2, freq * 2 //no need if only 8 voices per chord
	{1, -2},	//source 1, freq / 2
	{2, -2},	//source 2, freq / 2 //22 filters

	{1, -4},	//source 1, freq / 4
	{2, -4},	//source 2, freq / 4 //26 filters
	{1, 4},		//source 1, freq * 4
	{2, 4},		//source 2, freq * 4 //30 filters

	{0, 8}		//source 0, freq * 8 //32 filters

	/*
	//v3. more balanced
	{0, 1},		//source 0, no change
	{1, 1},		//source 1, no change
	{2, 1}, 	//source 2, no change

	{0, 2},		//source 0, freq * 2 //only powers of 2 make sense to shift octaves up and down (2,4,8)
	{0, -2},	//source 0, freq / 2
	{0, 4},		//source 0, freq * 4

	{1, 2},
	{1, -2},
	{1, 4},

	{2, 2},
	{2, -2},
	{2, 4},

	{0, -4},
	{1, -4},
	{2, -4},

	{0, 8}
	*/
};

#ifdef MIDI_4_KEYS_POLYPHONY
const int MusicBox::expand_multipliers4[MAX_VOICES_PER_CHORD][2] = { //for 4-note polyphony
	{0, 1},
	{1, 1},
	{2, 1},
	{3, 1},

	{0, 2},
	{1, 2},
	{2, 2},
	{3, 2},

	{0, -2},
	{1, -2},
	{2, -2},
	{3, -2},

	{0, 4},
	{1, 4},
	{2, 4},
	{3, 4}
};
#endif

MusicBox::MusicBox(int song)
{
	use_melody = NULL;
	//use_song = (char*)SONGS[(song-1) * 2];

	#define PARSE_SONG_BUFFER 4000
	char *song_buf = (char*)malloc(PARSE_SONG_BUFFER);

	int total_length;

	if(song==LOAD_SONG_NVS)
	{
		total_length = load_song_nvs(song_buf, 0);
	}
	else
	{
		total_length = load_song_or_melody(song, "song", song_buf);
	}

	printf("MusicBox(%d): load song total lenght = %d\n", song, total_length);
	if(total_length > PARSE_SONG_BUFFER)
	{
		printf("MusicBox(%d): PARSE_SONG_BUFFER overflow: %d > %d\n", song, total_length, PARSE_SONG_BUFFER);
		while(1); //halt
	}
	if(total_length==0)
	{
		printf("MusicBox(%d): could not find the song in config block\n", song);
		//while(1); //halt
		//free(song_buf);
		//return;

		//indiate error
		indicate_context_setting(0xff, 4, 250);
		esp_restart();
	}

	//base_notes = (char*)malloc(strlen(use_song) * sizeof(char) + 1);
	//memcpy(base_notes, use_song, strlen(use_song) * sizeof(char));
	//base_notes[strlen(use_song) * sizeof(char)] = 0;
	base_notes = (char*)malloc(strlen(song_buf) * sizeof(char) + 1);
	memcpy(base_notes, song_buf, strlen(song_buf) * sizeof(char));
	base_notes[strlen(song_buf) * sizeof(char)] = 0;

	total_chords = get_song_total_chords(base_notes);

	bases_parsed = (float*)malloc(total_chords * BASE_FREQS_PER_CHORD * sizeof(float)); //default = 8 chords, 3 base freqs for each chord
	led_indicators = (int8_t*)malloc(total_chords * 3 * sizeof(int8_t)); //default = 8 chords, 3 LED lights per chord
	midi_notes = (uint8_t*)malloc(total_chords * 3 * sizeof(uint8_t)); //default = 8 chords, 3 notes per chord

	chords = (CHORD*) malloc(total_chords * sizeof(CHORD));

	current_chord = 0;
	current_melody_note = 0;

	melody_freqs_parsed = NULL;
	melody_indicator_leds = NULL;

	free(song_buf);
}

MusicBox::~MusicBox()
{
	printf("MusicBox::~MusicBox() total chords = %d, chord = %x\n", total_chords, (uint32_t)chords);
	if(chords != NULL)
	{
		if(allocated_freqs)
		{
			//first release all freqs arrays within chord structure: chords[c].freqs
			for(int c=0;c<total_chords;c++)
			{
				if(chords[c].freqs != NULL)
				{
					//printf("MusicBox::~MusicBox() chords[%d].freqs content = %f,%f,%f\n", c, chords[c].freqs[0], chords[c].freqs[1], chords[c].freqs[2]);

					//printf("MusicBox::~MusicBox() freeing chords[%d].freqs\n", c);
					free(chords[c].freqs);
					chords[c].freqs = NULL;
					allocated_freqs--;
				}
			}
			//printf("MusicBox::~MusicBox() allocated freqs released, %d left\n", allocated_freqs);
		}
		free(chords); chords = NULL;
	}

	//printf("Free heap: %u\n", xPortGetFreeHeapSize());

	//printf("MusicBox::~MusicBox() releasing melody_freqs_parsed\n");
	if(melody_freqs_parsed != NULL) { free(melody_freqs_parsed); melody_freqs_parsed = NULL; }

	//printf("MusicBox::~MusicBox() releasing melody_indicator_leds\n");
	if(melody_indicator_leds != NULL) { free(melody_indicator_leds); melody_indicator_leds = NULL; }

	//printf("MusicBox::~MusicBox() releasing base_notes\n");
	if(base_notes != NULL) { free(base_notes); base_notes = NULL; }

	//printf("MusicBox::~MusicBox() releasing bases_parsed\n");
	if(bases_parsed != NULL) { free(bases_parsed); bases_parsed = NULL; }

	//printf("MusicBox::~MusicBox() releasing led_indicators\n");
	if(led_indicators != NULL) { free(led_indicators); led_indicators = NULL; }

	//printf("MusicBox::~MusicBox() releasing midi_notes\n");
	if(midi_notes != NULL) { free(midi_notes); midi_notes = NULL; }
}

void MusicBox::generate(int max_voices, int allocate_freqs) {

	printf("MusicBox::generate(): total chords = %d\n", total_chords);

	int c,t;
	//float test;
	for(c=0;c<total_chords;c++)
	{
		chords[c].tones = max_voices;
		if(allocate_freqs)
		{
			chords[c].freqs = (float *) malloc(max_voices * sizeof(float));
			allocated_freqs++;
		}

		for(t=0;t<chords[c].tones;t++)
		{
			if(expand_multipliers[t][1] > 0)
			{
				chords[c].freqs[t] = bases_parsed[3*c + expand_multipliers[t][0]] * (float)expand_multipliers[t][1];
				//test = chords[c].freqs[t];
				//test++;

				//if(chords[c].freqs[t] > 2000.0f)
				//{
				//	chords[c].freqs[t] /= 2.0f;
				//}
			}
			else
			{
				chords[c].freqs[t] = bases_parsed[3*c + expand_multipliers[t][0]] / (float)(-expand_multipliers[t][1]);
				//test = chords[c].freqs[t];
				//test++;
			}
		}
	}
}

int MusicBox::get_song_total_chords(char *base_notes)
{
	if(strlen(base_notes)==0)
	{
		return 0;
	}

	int chords = 1;

	for(unsigned int i=0;i<strlen(base_notes);i++)
	{
	    if(base_notes[i]==',')
	    {
	    	chords++;
	    }
	}
	return chords;
}

int8_t* MusicBox::get_current_chord_LED()
{
	return led_indicators + 3 * current_chord;
}

uint8_t* MusicBox::get_current_chord_midi_notes()
{
	return midi_notes + 3 * current_chord;
}

int MusicBox::get_current_melody_note()
{
	return melody_indicator_leds[current_melody_note];
}

float MusicBox::get_current_melody_freq()
{
	return melody_freqs_parsed[current_melody_note];
}


int MusicBox::get_song_total_melody_notes(char *melody)
{
	if(strlen(melody)==0)
	{
		return 0;
	}

	int notes = 0;

	for(unsigned int i=0;i<strlen(melody);i++)
	{
	    if(melody[i]=='.' || (melody[i]>='a' && melody[i]<='h'))
	    {
	    	notes++;
	    }
	}
	return notes;
}

/*
void Chord::clear_old_song_and_melody()
{
	if(use_song!=NULL)
	{
		free(use_song); //free memory that was allocated by encode_temp_song_to_notes()
		use_song = NULL;
	}
	if(use_melody!=NULL)
	{
		free(use_melody);
		use_melody = NULL;
	}
}
*/
extern const int8_t LED_translate_table[];

void MusicBox::transpose_song(int direction)
{
	printf("MusicBox::transpose_song(): direction = %d\n", direction);

	int c,t;

	for(c=0;c<total_chords;c++)
	{
		for(t=0;t<chords[c].tones;t++)
		{
			chords[c].freqs[t] *= pow(HALFTONE_STEP_COEF, direction);
		}

		(midi_notes + 3 * c)[0] += direction;
		(midi_notes + 3 * c)[1] += direction;
		(midi_notes + 3 * c)[2] += direction;

		 //LED_translate_table expects notes defined as: 1=c,2=c#...
		(led_indicators + 3 * c)[0] = LED_translate_table[1+((midi_notes + 3 * c)[0])%12];
		(led_indicators + 3 * c)[1] = LED_translate_table[1+((midi_notes + 3 * c)[1])%12];
		(led_indicators + 3 * c)[2] = LED_translate_table[1+((midi_notes + 3 * c)[2])%12];
	}
}
