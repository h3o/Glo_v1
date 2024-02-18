/*
 * v1_signals.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Apr 27, 2016
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

#ifndef V1_SIGNALS_H_
#define V1_SIGNALS_H_

#include <stdint.h>

//#define v1_I2S_AUDIOFREQ 22050
//#define v1_I2S_AUDIOFREQ 24000
//#define v1_I2S_AUDIOFREQ 23275 //half of the real APLL rate 46550
#define v1_I2S_AUDIOFREQ 25390 //half of the real APLL rate 50780
//#define v1_I2S_AUDIOFREQ 44100
//#define v1_I2S_AUDIOFREQ 50780

#ifdef __cplusplus
 extern "C" {
#endif

void RNG_Config (void);

float v1_PseudoRNG1a_next_float();
float v1_PseudoRNG1b_next_float();

uint32_t PseudoRNG2_next_int32();

#ifdef __cplusplus
}
#endif

#endif /* V1_SIGNALS_H_ */
