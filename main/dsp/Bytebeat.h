/*
 * Bytebeat.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Jul 14 2018
 *      Author: mario
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of GNU GPLv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/
 *
 *  Bytebeat uses math formulas discovered by Viznut a.k.a. Ville-Matias Heikkilä and his friends.
 *
 *  Sources and more information: http://viznut.fi/en/
 *                                http://viznut.fi/texts-en/bytebeat_algorithmic_symphonies.html
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
