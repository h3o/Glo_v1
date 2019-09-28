/*
 * Bytebeat.h
 *
 *  Created on: Jul 14 2018
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

#ifndef EXTENSIONS_BYTEBEAT_H_
#define EXTENSIONS_BYTEBEAT_H_

#include <stdint.h> //for uint16_t type

void channel_bytebeat();
int16_t bytebeat_echo(int16_t sample);

void bytebeat_init();
uint32_t bytebeat_next_sample();

void bytebeat_next_song();
void bytebeat_stereo_paning();

#endif /* EXTENSIONS_BYTEBEAT_H_ */
