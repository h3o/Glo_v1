/*
 * Dekrispator.h
 *
 *  Created on: 29 Jan 2019
 *      Author: mario
 *
 *  Port & integration of Dekrispator FM Synth by Xavier Halgand (https://github.com/MrBlueXav/Dekrispator)
 *  Please find the source code and license information within the files located in ../dkr/ subdirectory
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

#ifndef EXTENSIONS_DEKRISPATOR_H_
#define EXTENSIONS_DEKRISPATOR_H_

#include <stdint.h> //for uint16_t type

//#define SEND_MIDI_NOTES
#define USE_LOW_SAMPLE_RATE
//#define USE_FAUX_LOW_SAMPLE_RATE
//#define USE_LARGER_BUFFERS

#ifdef USE_LOW_SAMPLE_RATE
#define DEKRISPATOR_PATCH_STORE_TIMEOUT SAMPLE_RATE_LOW*10
#else
#define DEKRISPATOR_PATCH_STORE_TIMEOUT SAMPLE_RATE_DEFAULT*10
#endif

//DEKRISPATOR=======================================================================================
//DEKRISPATOR=======================================================================================
//DEKRISPATOR=======================================================================================
// Includes ------------------------------------------------------------------
//#include <math.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <stdbool.h>

// Local includes ------------------------------------------------------------------
//#include "usb_bsp.h"
//#include "usbh_core.h"
//#include "usbh_usr.h"
//#include "usbh_midi_core.h"

#include <dkr/soundGen.h>
//#include "my_stm32f4_discovery.h"
//#include "stm32f4xx.h"
//#include "stm32f4xx_conf.h"
//#include "my_stm32f4_discovery_audio_codec.h"
//#include "stm32fxxx_it.h"
#include <dkr/delay.h>
#include <dkr/chorusFD.h>
#include <dkr/random.h>
#include <dkr/CONSTANTS.h>
#include <dkr/drifter.h>
#include <dkr/resonantFilter.h>
#include <dkr/adsr.h>
#include <dkr/sequencer.h>

//bool	demoMode = true;
//bool	freeze = false;
//extern uint16_t audiobuff[BUFF_LEN]; // THE audio buffer

#define DEKRISPATOR_ENABLED
#define DEKRISPATOR_ECHO

#define MP_PARAMS 41
#define MF_PARAMS 10

typedef struct
{
	int		patch_no;
	int		patch[MP_PARAMS];
	int		fx[MF_PARAMS];

} dekrispator_patch_t ;

extern dekrispator_patch_t *dkr_patch;

void channel_dekrispator();

//DEKRISPATOR=======================================================================================
//DEKRISPATOR=======================================================================================
//DEKRISPATOR=======================================================================================

#endif /* EXTENSIONS_DEKRISPATOR_H_ */
