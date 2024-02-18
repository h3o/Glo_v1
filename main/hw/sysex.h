/*
 * sysex.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Apr 29, 2020
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

#ifndef SYSEX_H_
#define SYSEX_H_

#include "init.h"

// ----------------------------------------------------------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

void MIDI_sysex_sender();

#ifdef __cplusplus
}
#endif

#endif
