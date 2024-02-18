/*
 * Chaos.c
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Dec 28, 2016
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
#include <stdlib.h>

#include "chaos.h"
#include "notes.h"
#include "hw/signals.h"

const char *possible_chords = "cdefgabCDEFGABjklmnJKLMN";

int looks_like_a_chord(char text)
{
	if( (text >= 'a' && text <= 'g') ||
		(text >= 'A' && text <= 'G') ||
		(text >= 'j' && text <= 'n') ||
		(text >= 'J' && text <= 'N') )
	{
		return 1;
	}
	return 0;
}

const char * const nice_fragments[] = {"FGCa", "aGCD", "fGcN", "CFKN", "fJKN", "kFJd"};
const int possible_fragment_order[][16] = {{1,1,2,2,3,3,2,2,4,4,5,5,6,6,5,2},{2,2,1,1,2,1,3,1,4,4,5,5,4,5,2,3}};
//const int possible_fragment_order[][4] = {{1,2,4,3},{2,4,1,3}};

int get_music_from_noise(char **progression_buffer, char **melody_buffer)
{
	char *temp_chord_progression;
	//char *temp_melody;
	int temp_chord_progression_ptr = 0;
	//int temp_melody_ptr = 0;
	temp_chord_progression = (char*)malloc(5000); //allocate lot of temp space
	//temp_melody = (char*)malloc(3000);

	//int shuffled_chords_n = 32;
	//char *shuffled_chords;
	//int shuffled_chords_ptr = 0;
	//shuffled_chords = (char*)malloc(shuffled_chords_n + 2); //allocate array for shuffled chords

	int noise_data_n = 128;
	char *noise_data;
	noise_data = (char*)malloc(noise_data_n + 16);

	char found_sequence[4];
	int sequence_length;

	#define NOTES_PER_FRAGMENT 4
	#define FRAGMENTS_REQUIRED 8

	char fragments[FRAGMENTS_REQUIRED][NOTES_PER_FRAGMENT];

	for(int f=0;f<FRAGMENTS_REQUIRED;f++)
	{
		sequence_length = 0;
		while(sequence_length < NOTES_PER_FRAGMENT)
		{
			int noise_data_ptr = 0;
			while(noise_data_ptr < noise_data_n)
			{
				noise_data_ptr += fill_with_random_value(noise_data + noise_data_ptr);
			}

			//check if any sequence of chords appeared
			for(int i=0; i<noise_data_n && sequence_length<NOTES_PER_FRAGMENT; i++)
			{
				if(looks_like_a_chord(noise_data[i]))
				{
					found_sequence[sequence_length] = noise_data[i];
					sequence_length++;
				}
			}
		}
		strncpy(fragments[f],found_sequence,NOTES_PER_FRAGMENT);
	}

	new_random_value(); //get new value to random_value global variable
	//pick one of the possible fragment-order sequences randomly
	int fragment_order_n = random_value % (sizeof(possible_fragment_order) / sizeof(possible_fragment_order[0]));
	//find out what is the number of fragments listed in a sequence
	int total_fragments_needed = sizeof(possible_fragment_order[0]) / sizeof(typeof(possible_fragment_order[0][0]));

	int sequenced_chords_ptr = 0;
	char *sequenced_chords = (char*)malloc(total_fragments_needed * NOTES_PER_FRAGMENT + 2); //allocate array for chords in one-char notation

	for(int n_fragment=0; n_fragment < total_fragments_needed; n_fragment++)
	{
		const char *selected_fragment = fragments[possible_fragment_order[fragment_order_n][n_fragment] - 1];
		strncpy(sequenced_chords+sequenced_chords_ptr, selected_fragment, NOTES_PER_FRAGMENT);
		sequenced_chords_ptr += NOTES_PER_FRAGMENT;
	}

	for(int i=0;i<sequenced_chords_ptr;i++)
	{
		temp_chord_progression_ptr += get_chord_expansion(sequenced_chords+i, temp_chord_progression+temp_chord_progression_ptr);
		temp_chord_progression[temp_chord_progression_ptr] = ',';
		temp_chord_progression_ptr++;

		//temp_melody_ptr += get_melody_fragment(sequenced_chords+i, temp_melody+temp_melody_ptr);
	}
	temp_chord_progression[--temp_chord_progression_ptr] = 0; //delete the last comma, terminate the string

	shuffle_octaves(temp_chord_progression);

	/*
	//melody
	melody_buffer[0] = (char*)malloc(temp_melody_ptr+2);
	strncpy(melody_buffer[0],temp_melody,temp_melody_ptr);
	melody_buffer[0][temp_melody_ptr] = 0;
    */

	//allocate memory and copy over the chord progression to progression_buffer
	progression_buffer[0] = (char*)malloc(temp_chord_progression_ptr + 1);
	//strncpy(progression_buffer[0], temp_chord_progression, temp_chord_progression_ptr);
	strcpy(progression_buffer[0], temp_chord_progression);
	free(temp_chord_progression);
	//free(temp_melody);
	free(noise_data);
	free(sequenced_chords);

	return temp_chord_progression_ptr;
}

void shuffle_octaves(char *buffer)
{
	for(unsigned int i=0;i<strlen(buffer);i++)
	{
		if(buffer[i] >= '0' && buffer[i] <= '9') //if it is a number
		{
			new_random_value(); //get new value to random_value global variable
			buffer[i] = '3' + random_value % 2;
		}
	}
}

int generate_chord_progression(char **progression_buffer)
{
	char *temp_chord_progression;
	int temp_chord_progression_ptr = 0;
	temp_chord_progression = (char*)malloc(5000); //allocate lot of temp space

	int shuffled_chords_n = 32;
	char *shuffled_chords;
	int shuffled_chords_ptr = 0;
	shuffled_chords = (char*)malloc(shuffled_chords_n + 2); //allocate array for shuffled chords

	new_random_value(); //get new value to random_value global variable
	int nice_fragment_n = random_value % (sizeof(possible_fragment_order) / sizeof(possible_fragment_order[0]));

	int total_fragments = sizeof(possible_fragment_order[0]) / sizeof(typeof(possible_fragment_order[0][0]));
	for(int n_fragment=0; n_fragment < total_fragments; n_fragment++)
	{
		const char *selected_fragment = nice_fragments[possible_fragment_order[nice_fragment_n][n_fragment] - 1];

		//error: 'strncpy' output truncated before terminating nul copying as many bytes from a string as its length [-Werror=stringop-truncation]
		//strncpy(shuffled_chords+shuffled_chords_ptr, selected_fragment, strlen(selected_fragment));
		strcpy(shuffled_chords+shuffled_chords_ptr, selected_fragment);

		shuffled_chords_ptr += strlen(selected_fragment);
	}
	/*
	int shuffled_chords_n = 16;
	char *shuffled_chords;
	int possible_chords_n = strlen(possible_chords);
	shuffled_chords = (char*)malloc(shuffled_chords_n + 2); //allocate array for shuffled chords

	for(int i=0;i<shuffled_chords_n;i++)
	{
		//for now, get it from the RNG - later, try user's chaotic input too

		new_random_value(); //get new value to random_value global variable
		shuffled_chords[i] = possible_chords[random_value % possible_chords_n];
	}
	shuffled_chords[shuffled_chords_n] = 0;
	*/

	for(int i=0;i<shuffled_chords_n;i++)
	{
		temp_chord_progression_ptr += get_chord_expansion(shuffled_chords+i, temp_chord_progression+temp_chord_progression_ptr);
		temp_chord_progression[temp_chord_progression_ptr] = ',';
		temp_chord_progression_ptr++;
	}
	temp_chord_progression[--temp_chord_progression_ptr] = 0; //delete the last comma, terminate the string

	//allocate memory and copy over the chord progression to progression_buffer
	progression_buffer[0] = (char*)malloc(temp_chord_progression_ptr + 1);
	strncpy(progression_buffer[0], temp_chord_progression, temp_chord_progression_ptr);

	return temp_chord_progression_ptr;
}

int get_chord_expansion(char *chord, char *expansion)
{
	if(strncmp(chord,"c",1)==0){ strcpy(expansion,"c3d#3g3");	return 7; }
	if(strncmp(chord,"C",1)==0){ strcpy(expansion,"c3e3g3");	return 6; }
	if(strncmp(chord,"d",1)==0){ strcpy(expansion,"d3f3a3");	return 6; }
	if(strncmp(chord,"D",1)==0){ strcpy(expansion,"d3f#3a3");	return 7; }
	if(strncmp(chord,"e",1)==0){ strcpy(expansion,"e3g3b3");	return 6; }
	if(strncmp(chord,"E",1)==0){ strcpy(expansion,"e3g#3b3");	return 7; }
	if(strncmp(chord,"f",1)==0){ strcpy(expansion,"f3g#3c4");	return 7; }
	if(strncmp(chord,"F",1)==0){ strcpy(expansion,"f3a3c4");	return 6; }
	if(strncmp(chord,"g",1)==0){ strcpy(expansion,"g3a#3d4");	return 7; }
	if(strncmp(chord,"G",1)==0){ strcpy(expansion,"g3b3d4");	return 6; }
	if(strncmp(chord,"a",1)==0){ strcpy(expansion,"a3c4e4");	return 6; }
	if(strncmp(chord,"A",1)==0){ strcpy(expansion,"a3c#4e4");	return 7; }
	if(strncmp(chord,"b",1)==0){ strcpy(expansion,"b3d4f#4");	return 7; }
	if(strncmp(chord,"B",1)==0){ strcpy(expansion,"b3d#4f#4");	return 8; }

	if(strncmp(chord,"j",1)==0){ strcpy(expansion,"c#3e3g#3");	return 8; }
	if(strncmp(chord,"J",1)==0){ strcpy(expansion,"c#3f3g#3");	return 8; }
	/*if(strncmp(chord,"k",1)==0){ strcpy(expansion,"d#3f#3a#3");	return 9; }
	if(strncmp(chord,"K",1)==0){ strcpy(expansion,"d#3g3a#3");	return 8; }
	if(strncmp(chord,"l",1)==0){ strcpy(expansion,"f#3a3c#4");	return 8; }
	if(strncmp(chord,"L",1)==0){ strcpy(expansion,"f#3a#3c#4");	return 9; }
	if(strncmp(chord,"m",1)==0){ strcpy(expansion,"g#3b3d#4");	return 8; }
	if(strncmp(chord,"M",1)==0){ strcpy(expansion,"g#3c4d#4");	return 8; }
	if(strncmp(chord,"n",1)==0){ strcpy(expansion,"a#3c#4f4");	return 8; }
	if(strncmp(chord,"N",1)==0){ strcpy(expansion,"a#3d4f4");	return 7; }*/

	if(strncmp(chord,"n",1)==0){ strcpy(expansion,"d#3f#3a#3");	return 9; } //should be reordered, but let's keep backwards compatibility
	if(strncmp(chord,"N",1)==0){ strcpy(expansion,"d#3g3a#3");	return 8; }
	if(strncmp(chord,"k",1)==0){ strcpy(expansion,"f#3a3c#4");	return 8; }
	if(strncmp(chord,"K",1)==0){ strcpy(expansion,"f#3a#3c#4");	return 9; }
	if(strncmp(chord,"l",1)==0){ strcpy(expansion,"g#3b3d#4");	return 8; }
	if(strncmp(chord,"L",1)==0){ strcpy(expansion,"g#3c4d#4");	return 8; }
	if(strncmp(chord,"m",1)==0){ strcpy(expansion,"a#3c#4f4");	return 8; }
	if(strncmp(chord,"M",1)==0){ strcpy(expansion,"a#3d4f4");	return 7; }

	return 0;
}

//const char *melody_fragments_minor[] = {"023057..",".023057.","64064...",".7.7663."};
//const char *melody_fragments_major[] = {"04704805","0..4..7.","45749740","6740.3.4"};
const char *melody_fragments_minor[] = {"04704704"};
const char *melody_fragments_major[] = {"03703703"};
//const char *melody_fragments_minor[] = {"0..2..3."};//,"0..5..6."};
//const char *melody_fragments_major[] = {"0..3..7."};//,"0..2..7."};

int get_melody_fragment(char *chord, char *buffer)
{
	new_random_value(); //get new value to random_value global variable
	int melody_fragment_n;
	if(chord[0]>='a' && chord[0]<='m') //minor chord
	{
		melody_fragment_n = random_value % (sizeof(melody_fragments_minor) / sizeof(melody_fragments_minor[0]));
		//strcpy(buffer,melody_fragments_minor[melody_fragment_n]);
		//return strlen(melody_fragments_minor[melody_fragment_n]);
		return translate_intervals_to_notes(buffer,chord[0],(char *)melody_fragments_minor[melody_fragment_n]);
	}
	if(chord[0]>='A' && chord[0]<='M') //major chord
	{
		melody_fragment_n = random_value % (sizeof(melody_fragments_major) / sizeof(melody_fragments_major[0]));
		//strcpy(buffer,melody_fragments_major[melody_fragment_n]);
		//return strlen(melody_fragments_major[melody_fragment_n]);
		return translate_intervals_to_notes(buffer,chord[0],(char *)melody_fragments_major[melody_fragment_n]);
	}

	return 0;
}
