/*
 * Chord.cpp
 *
 *  Created on: May 30, 2016
 *      Author: mayo
 */

#include "v1_Chord.h"
#include <string.h>
#include <ctype.h>
#include <math.h>

v1_Chord::v1_Chord() {
	// TODO Auto-generated constructor stub

	base_notes = (char*)malloc(strlen(USE_SONG) * sizeof(char));
	memcpy(base_notes, USE_SONG, strlen(USE_SONG) * sizeof(char));

	total_chords = get_song_total_chords(base_notes);

	bases_parsed = (float*)malloc(total_chords * BASE_FREQS_PER_CHORD * sizeof(float)); //default = 8 chords, 3 base freqs for each chord

	chords = (v1_CHORD*) malloc(total_chords * sizeof(v1_CHORD));
	current_chord = 0;
}

v1_Chord::~v1_Chord() {
	//printf("MusicBox::~MusicBox() total chords = %d, chord = %x\n", total_chords, (uint32_t)chords);
	if(chords != NULL)
	{
//		if(allocated_freqs)
		{
			//first release all freqs arrays within chord structure: chords[c].freqs
			for(int c=0;c<total_chords;c++)
			{
				if(chords[c].v1_freqs != NULL)
				{
					//printf("MusicBox::~MusicBox() chords[%d].freqs content = %f,%f,%f\n", c, chords[c].freqs[0], chords[c].freqs[1], chords[c].freqs[2]);

					//printf("MusicBox::~MusicBox() freeing chords[%d].freqs\n", c);
					free(chords[c].v1_freqs);
					chords[c].v1_freqs = NULL;
//					allocated_freqs--;
				}
			}
			//printf("MusicBox::~MusicBox() allocated freqs released, %d left\n", allocated_freqs);
		}
		free(chords); chords = NULL;
	}

	//printf("Free heap: %u\n", xPortGetFreeHeapSize());

	//printf("MusicBox::~MusicBox() releasing melody_freqs_parsed\n");
//	if(melody_freqs_parsed != NULL) { free(melody_freqs_parsed); melody_freqs_parsed = NULL; }

	//printf("MusicBox::~MusicBox() releasing melody_indicator_leds\n");
//	if(melody_indicator_leds != NULL) { free(melody_indicator_leds); melody_indicator_leds = NULL; }

	//printf("MusicBox::~MusicBox() releasing base_notes\n");
	if(base_notes != NULL) { free(base_notes); base_notes = NULL; }

	//printf("MusicBox::~MusicBox() releasing bases_parsed\n");
	if(bases_parsed != NULL) { free(bases_parsed); bases_parsed = NULL; }

	//printf("MusicBox::~MusicBox() releasing led_indicators\n");
//	if(led_indicators != NULL) { free(led_indicators); led_indicators = NULL; }

	//printf("MusicBox::~MusicBox() releasing midi_notes\n");
//	if(midi_notes != NULL) { free(midi_notes); midi_notes = NULL; }
}

void v1_Chord::parse_notes(char *base_notes_buf, float *bases_parsed_buf) {

	int ptr,len=strlen(base_notes_buf),halftones,octave=0,/*parsed_chord=0,*/parsed_note=0,octave_shift;
	double note_freq;
	double halftone_step_coef = pow((double)2, (double)1/(double)12);
	char note;

	for(ptr=0;ptr<len;ptr++)
	{
		base_notes_buf[ptr] = tolower(base_notes_buf[ptr]);
	}

	for(ptr=0;ptr<len;ptr++)
	{
		note = base_notes_buf[ptr];
		if(note >= 'a' && note <= 'h')
		{
			//get the note
			halftones = halftone_lookup[note - 'a'];

			if(base_notes_buf[ptr+1]=='#')
			{
				halftones++;
				ptr++;
			}
			ptr++;

			//get the octave
			if(base_notes_buf[ptr] >= '1' && base_notes_buf[ptr] <= '7')
			{
				octave = base_notes_buf[ptr] - '1' + 1;
			}

			if(base_notes_buf[ptr+1]==',')
			{
				ptr++;
			}

			//add halftones... 12 halftones is plus one octave -> frequency * 2
			note_freq = NOTE_FREQ_A4 * pow(halftone_step_coef, halftones);

			//shift according to the octave
			octave_shift = octave - 4; //ref note is A4 - fourth octave
			if(note>='c')
			{
				octave_shift--;
			}

			if(octave_shift > 0) //shift up
			{
				do
				{
					note_freq *= 2;
					octave_shift--;
				}
				while(octave_shift!=0);
			}
			else if(octave_shift < 0) //shift down
			{
				do
				{
					note_freq /= 2;
					octave_shift++;
				}
				while(octave_shift!=0);
			}

			bases_parsed_buf[/*3*parsed_chord + */parsed_note] = (float)note_freq;

			parsed_note++;
			/*
			if(parsed_note == BASE_FREQS_PER_CHORD)
			{
				parsed_note = 0;
				parsed_chord++;
			}
			*/
		}
	}
}

void v1_Chord::generate(int max_voices) {

	int c,t;
	float test;
	for(c=0;c<total_chords;c++)
	{
		chords[c].tones = max_voices;
		chords[c].v1_freqs = (float *) malloc(max_voices * sizeof(float));

		for(t=0;t<chords[c].tones;t++)
		{
			if(expand_multipliers[t][1] > 0)
			{
				chords[c].v1_freqs[t] = bases_parsed[3*c + expand_multipliers[t][0]] * (float)expand_multipliers[t][1];
				test = chords[c].v1_freqs[t];
				test++;
			}
			else
			{
				chords[c].v1_freqs[t] = bases_parsed[3*c + expand_multipliers[t][0]] / (float)(-expand_multipliers[t][1]);
				test = chords[c].v1_freqs[t];
				test++;
			}
		}
	}
}

int v1_Chord::get_song_total_chords(char *base_notes)
{
	if(strlen(base_notes)==0)
	{
		return 0;
	}

	int chords = 1;

	for(int i=0;i<strlen(base_notes);i++)
	{
	    if(base_notes[i]==',')
	    {
	    	chords++;
	    }
	}
	return chords;
}
