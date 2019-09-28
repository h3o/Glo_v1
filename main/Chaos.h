/*
 * Chaos.h
 *
 *  Created on: Dec 28, 2016
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

#ifndef CHAOS_H_
#define CHAOS_H_

#ifdef __cplusplus
 extern "C" {
#endif

int get_music_from_noise(char **progression_buffer, char **melody_buffer);
int generate_chord_progression(char **progression_buffer);
int get_melody_fragment(char *chord, char *buffer);
int get_chord_expansion(char *chord, char *expansion);
void shuffle_octaves(char *buffer);

#ifdef __cplusplus
}
#endif

#endif /* MIDI_H_ */
