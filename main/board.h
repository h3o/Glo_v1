/*
 * board.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Feb 11, 2019
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

//#define BOARD_WHALE

#ifdef BOARD_WHALE
#define BOARD_WHALE_ON_EXPANDER_V181

#else

#define BOARD_GECHO

//for v1.79-v1.80
#define BOARD_GECHO_V179 //leave v1.73 defined as well, also applies to v1.80 and v1.81
#define BOARD_GECHO_V181 //expanders addresses are different in this version

//for v1.73-v1.78, leave defined for higher versions too
#define BOARD_GECHO_V173 //applies up to v1.78

//#define BOARD_GECHO_V172
//#define OVERRIDE_RX_TX_LEDS //disable when debugging

#endif
