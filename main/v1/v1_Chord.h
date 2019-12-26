/*
 * Chord.h
 *
 *  Created on: May 30, 2016
 *      Author: mayo
 */

#ifndef V1_CHORD_H_
#define V1_CHORD_H_

#include <stdlib.h>

typedef struct {
	int tones;
	float *v1_freqs;
} v1_CHORD;

class v1_Chord {
public:
	v1_Chord();
	virtual ~v1_Chord();
	void parse_notes(char *base_notes_buf, float *bases_parsed_buf);
	void generate(int max_voices);
	int get_song_total_chords(char *base_notes);

	v1_CHORD *chords;

//#define TOTAL_CHORDS 8 //this is going to be a variable now
#define TOTAL_CHORDS_DEFAULT 8 //this is going to be a variable now
#define VOICES_PER_CHORD 8
#define BASE_FREQS_PER_CHORD 3
#define NOTE_FREQ_A4 440 //440Hz of A4 note as default for calculation

	/*
	//default sequence
	const int bases_hardcoded[TOTAL_CHORDS_DEFAULT][BASE_FREQS_PER_CHORD] = {

		//inspired by The Cure - Wrong number
		{440/4, 523, 659/4}, //a-c-e = a minor
		{440/2, 523, 659/2}, //a-c-e = a minor
		{440/2, 523/2, 659/4}, //a-c-e = a minor
		{440/2, 523, 659/2}, //a-c-e = a minor
		{294, 370, 440}, //d-f#-a = d major
		{330, 415, 494}, //e-g#-b = e major
		{440, 554, 659}, //a-c#-e = a major
		{440, 554, 659*2} //a-c#-e = a major

		/ *
		//Johann Pachelbel - Canon in D
		{294, 370, 440}, //d-f#-a = d major
		{440, 554, 659}, //a-c#-e = a major
		{494, 587, 740}, //b-d-f# = b major
		{370, 466, 554}, //f#-a#-c# = f# major
		{392, 494, 587}, //g-h-d = g major
		{587, 740, 880}, //d-f#-a = d major
		{659, 784, 494}, //e-g-h = e minor
		{440, 554, 659}, //a-c#-e = a major
		* /

		//{330, 392, 523}, //e major
		//{294, 349, 440}, //d-f-a = d minor
		//{330, 392, 523}, //e-g-c = c major
		//{311, 392, 523}, //d#-g-c = c minor

		//{523, 659, 784}, //c major
		//{392, 466, 587}, //g minor
		//{392, 494, 587}, //g major
		//{294, 370, 466}, //off
		//{294, 370, 494}, //off
		//{466, 587, 784}, //g minor ?
		//{466, 587, 784}, //g major ?
		//{466, 587, 740},
		//{466, 587, 698},
	};
	*/

	char *base_notes;
	float *bases_parsed;

	//inspired by The Cure - Wrong number
	//#define SONG1_0 "a2c5e3,a3c5e4,a3c4e3,a3c5e4,d4f#4a4,e4g#4b4,a4c#5e5,a4c#5e6"

	#define SONG1 "a2c5e3,a3c5e4,a3c4e3,a3c5e4,d4f#4a4,e4g#4b4,a4c#5e5,a4c#5e6,a2c5e3,a3c5e4,a3c4e3,a3c5e4,d4f#4a4,e4g#4b4,a4c#5e5,a4c#5e6,a3c4e4,b3d4g4,c4e4g4,d4f4a4,e4g#4b4,f4a4c4,g4b4d5,g3b4d6,a2c5e3,b3d5g4,c4e4g4,d3f5a4,e4g#4b4,f4a4c4,g4b5d5,g4b4d4,g#4c5d#5,f4g#4c5,d4f4g#4,b3d4f4,g4b4d5,f4g#4c4,d#4g4a#4,d3g4b4,a3c4e4,b3d4g4,c4e4g4,d4f4a4,e4g#4b4,f4a4c4,g4b4d5,g3b4d6,a2c5e3,b3d5g4,c3e4g3,d3f5a4,e4g#4b4,f4a4c4,g4b5d5,g4b5d3,a3c6e4,a4c6e5,a4c3e4,a4c5e4,d4f#5a4,e4g#5b4,a4c#5e5,a4c#5e4"

	//Johann Pachelbel - Canon in D

	//#define SONG2 "d4f#4a4,a4c#5e5,b4d5f#5,f#4b4c#5,g4b4d5,d5f#5a5,e5g5b4,a4c#5e5"
	#define SONG2 "d3f#3a3,a3c#4e4,b3d4f#4,f#3b3c#4,g3b3d4,d4f#4a4,e4g4b3,a3c#4e4"

	//#define SONG3 "f3g#3c4,f3g#4c4,c#3f3g#3,c#4f3g#4,c3d#3g3,c3d#3g2,a#2c#3f3,a#3c#3f4"
	//#define SONG3 "f3g#3c4,f3g#4c4,c#3f3g#3,c#4f3g#4,a#d#3g3,a#d#3g2,a#2c#3f3,a#3c#3f4" //broken syntax - weirldy cool one

	//#define SONG3 "f3g#3c4,f3g#4c4,c#3f3g#3,c#4f3g#4,c3d#3g3,c3d#3g2,a#2c#3f3,a#3c#3f4"
	//#define SONG3 "f3g#3c4,f3g#4c4,c#3f3g#3,c#4f3g#4,a#3d#3g3,a#3d#3g4,a#3c#3f3,a#3c#3f4"
	#define SONG3 "f3g#3c4,f3g#4c4,c#3f3g#3,c#4f3g#4,c4e#4g4,c4e#4g3,a#3c#3f3,a#3c#3f4"

	//TODO: bad apple, ghost in the shell

	#define USE_SONG SONG1

	const int expand_multipliers[VOICES_PER_CHORD][2] = {
		/*
		//v1. - more trebles, less basses
		{0, 1},		//source 0, no change
		{1, 1},		//source 1, no change
		{2, 1}, 	//source 2, no change
		{0, -2},	//source 0, freq / 2
		{0, 2},		//source 0, freq * 2
		{1, 2},		//source 1, freq * 2
		{2, 2},		//source 2, freq * 2
		{1, -2}		//source 1, freq / 2
		*/

		//v2. - less trebles, more basses
		{0, 1},		//source 0, no change
		{1, 1},		//source 1, no change
		{2, 1}, 	//source 2, no change
		{0, 2},		//source 0, freq * 2
		{0, -2},	//source 0, freq / 2
		{0, 4},		//source 0, freq * 4
		{0, -4},	//source 0, freq / 4
		{1, 2}		//source 1, freq * 2
		//{2, 2}	//source 2, freq * 2 //no need if only 8 chords
	};

	//for sequential playback (test)
	int total_chords; //number of chors in whole song (default 8)
	int current_chord; //the chord that currently plays

	const int halftone_lookup[8] = {
		0,	//a
		2,	//b
		3,	//c
		5,	//d
		7,	//e
		8,	//f
		10,	//g
		2	//h
	};

};

#endif /* V1_CHORD_H_ */
