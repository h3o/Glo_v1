/*
 * DrumKit.h
 *
 *  Created on: 26 Nov 2016
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

#ifndef EXTENSIONS_DRUM_KIT_H_
#define EXTENSIONS_DRUM_KIT_H_

#include <stdlib.h>
#include <string.h>
#include "hw/gpio.h"
#include "hw/signals.h"
#include <hw/init.h>
#include <Interface.h>

/*
#define DRUM_THRESHOLD_ON	0.38f
#define DRUM_THRESHOLD_OFF	0.32f

#define DRUM_BASE_ADDR1 0		//base offset where drum samples are
#define DRUM_LENGTH1 26880		//length of first sample in bytes
#define DRUM_BASE_ADDR2 (DRUM_BASE_ADDR1 + DRUM_LENGTH1)
#define DRUM_LENGTH2 29060
#define DRUM_BASE_ADDR3 (DRUM_BASE_ADDR2 + DRUM_LENGTH2)
#define DRUM_LENGTH3 33036
#define DRUM_BASE_ADDR4 (DRUM_BASE_ADDR3 + DRUM_LENGTH3)
#define DRUM_LENGTH4 141116
*/

#define DRUM_SAMPLES 4			//we uploaded 4 samples to FLASH memory
#define DRUM_CHANNELS_MAX 4 	//no real polyphony for now, just one of each samples at the time

void channel_drum_kit();

#endif /* EXTENSIONS_DRUM_KIT_H_ */
