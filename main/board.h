/*
 * board.h
 *
 *  Created on: Feb 11, 2019
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

//#define BOARD_WHALE

#ifdef BOARD_WHALE
#define BOARD_WHALE_ON_EXPANDER_V181

#endif

#define BOARD_GECHO

#ifdef BOARD_GECHO
#define BOARD_GECHO_V173
//#define BOARD_GECHO_V172
//#define OVERRIDE_RX_TX_LEDS //disable when debugging

#endif
