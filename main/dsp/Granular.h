/*
 * Granular.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 19 Jan 2018
 *      Author: mario
 *
 *  As explained in the coding tutorial: http://gechologic.com/granular-sampler
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of GNU GPLv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/
 *
 */

#ifndef EXTENSIONS_GRANULAR_H_
#define EXTENSIONS_GRANULAR_H_

#include <stdlib.h>
#include <string.h>

#include "hw/gpio.h"
#include "hw/signals.h"
#include "hw/init.h"
#include "Interface.h"

#define GRANULAR_DETUNE_A1	0.02f
#define GRANULAR_DETUNE_A2	0.05f
#define GRANULAR_DETUNE_A3	0.07f

#define GRANULAR_DETUNE_B1	0.01f
#define GRANULAR_DETUNE_B2	0.03f
#define GRANULAR_DETUNE_B3	0.06f

void granular_sampler(int selected_song);
void update_grain_freqs(float *freq, float *bases, int voices_used, float detune);
void granular_sampler_simple();
void load_song_for_granular(int song_id);

#endif /* EXTENSIONS_GRANULAR_H_ */
