/*
 * notes.h
 *
 *  Created on: Oct 31, 2016
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

#ifndef NOTES_H_
#define NOTES_H_

#include <stdint.h>

#define NOTE_FREQ_A4 440.0f //440Hz of A4 note as default for calculation
#define HALFTONE_STEP_COEF (pow((double)2,(double)1/(double)12))

#define NOTES_PER_CHORD 3
extern int *set_chords_map;
extern int (*temp_song)[NOTES_PER_CHORD];
extern int temp_total_chords;

//extern char *progression_str;
//extern char *melody_str;

//extern const float notes_freqs[13];

#ifdef __cplusplus
 extern "C" {
#endif

int encode_temp_song_to_notes(int (*song)[NOTES_PER_CHORD], int chords, char **dest_buffer);
int tmp_song_find_note(int note, char *buffer);

int translate_intervals_to_notes(char *buffer, char chord_code, char *fragment);
int interval_to_note(char *buffer, int distance);

//void set_progression_str(char *chord_progression);
//void set_melody_str(char *melody);

void notes_to_LEDs(int *notes, int *leds, int notes_per_chord);
int parse_notes(char *base_notes_buf, float *bases_parsed_buf, uint8_t *led_indicators_buf, uint8_t *midi_notes_buf);

#ifdef __cplusplus
}
#endif

#endif /* NOTES_H_ */
