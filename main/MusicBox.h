/*
 * MusicBox.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: May 30, 2016
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

#ifndef MUSICBOX_H_
#define MUSICBOX_H_

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

typedef struct {
	int8_t tones;
	float *freqs = NULL;
} CHORD;

class MusicBox {
public:
	MusicBox(int song);
	virtual ~MusicBox();
	void generate(int max_voices, int allocate_freqs);
	int get_song_total_chords(char *base_notes);
	int8_t* get_current_chord_LED();
	uint8_t* get_current_chord_midi_notes();
	int get_current_melody_note();
	float get_current_melody_freq();
	int get_song_total_melody_notes(char *melody);
	void transpose_song(int direction);

	CHORD *chords = NULL;

#define MAX_VOICES_PER_CHORD 16 //only first 12 needed for 24 filters by default
#define BASE_FREQS_PER_CHORD 3

#define LOAD_SONG_NVS	999

	static const int expand_multipliers[MAX_VOICES_PER_CHORD][2];
	static const int expand_multipliers4[MAX_VOICES_PER_CHORD][2];
	float midi_chords_expanded[MAX_VOICES_PER_CHORD];

	char *base_notes = NULL;
	float *bases_parsed = NULL;
	int8_t *led_indicators = NULL;
	uint8_t *midi_notes = NULL;

	float *melody_freqs_parsed = NULL;
	int8_t *melody_indicator_leds = NULL;

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
