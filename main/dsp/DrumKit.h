/*
 * DrumKit.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 26 Nov 2016
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

#ifndef EXTENSIONS_DRUM_KIT_H_
#define EXTENSIONS_DRUM_KIT_H_

#include <stdlib.h>
#include <string.h>
#include "hw/gpio.h"
#include "hw/signals.h"
#include <hw/init.h>
#include <Interface.h>

#define DRUM_SAMPLES 4			//we uploaded 4 samples to FLASH memory
#define DRUM_CHANNELS_MAX 4 	//no real polyphony for now, just one of each samples at the time

void channel_drum_kit();

#endif /* EXTENSIONS_DRUM_KIT_H_ */
