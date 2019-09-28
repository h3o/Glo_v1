/*
 * midi.h
 *
 *  Created on: Mar 25, 2019
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

#ifndef MIDI_H_
#define MIDI_H_

#include "init.h"

#define MIDI_CHORD_NOTES 3
#define MIDI_POLYPHONY 3

#ifdef __cplusplus
extern "C" {
#endif

void send_MIDI_notes(uint8_t *notes, int current_chord, int n_notes);

#ifdef __cplusplus
}
#endif

#endif
