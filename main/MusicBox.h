/*
 * MusicBox.h
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

#ifndef MUSICBOX_H_
#define MUSICBOX_H_

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

typedef struct {
	int tones;
	float *freqs = NULL;
} CHORD;

class MusicBox {
public:
	MusicBox(int song);
	virtual ~MusicBox();
	void generate(int max_voices, int allocate_freqs);
	int get_song_total_chords(char *base_notes);
	uint8_t* get_current_chord_LED();
	uint8_t* get_current_chord_midi_notes();
	int get_current_melody_note();
	float get_current_melody_freq();
	int get_song_total_melody_notes(char *melody);

	CHORD *chords = NULL;

#define MAX_VOICES_PER_CHORD 16 //9 //only 8 needed for 16 filters by default
#define BASE_FREQS_PER_CHORD 3

	static const int expand_multipliers[MAX_VOICES_PER_CHORD][2];

	char *base_notes = NULL;
	float *bases_parsed = NULL;
	uint8_t *led_indicators = NULL;
	uint8_t *midi_notes = NULL;

	float *melody_freqs_parsed = NULL;
	uint8_t *melody_indicator_leds = NULL;

	//char *use_song;
	char *use_melody = NULL;

	//for sequential playback (test)
	int total_chords; //number of chors in whole song (default 8)
	int current_chord; //the chord that currently plays
	int total_melody_notes; //number of notes in melody string
	int current_melody_note; //the note that currently plays

	int allocated_freqs = 0;

	static const char *SONGS[];
	char *XAOS_MELODY = NULL; //for dynamically created melody
};

#endif /* MUSICBOX_H_ */
