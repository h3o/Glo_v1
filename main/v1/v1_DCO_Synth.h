/*
 * DCO_synth.h
 *
 *  Created on: 11 May 2017
 *      Author: mayo
 *
 * Based on "The Tiny-TS Touch Synthesizer" by Janost 2016, Sweden
 * https://janostman.wordpress.com/the-tiny-ts-diy-touch-synthesizer/
 *
 */

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef v1_DCO_SYNTH_H_
#define v1_DCO_SYNTH_H_

//#include "CV_Gate.h"
#include "MIDI.h"
#include "MusicBox.h"
#include <hw/codec.h>
//#include <hw/controls.h>
#include <hw/gpio.h>
#include <hw/leds.h>
#include <hw/sensors.h>
#include <hw/signals.h>
#include <notes.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
//#include <stm32f4xx.h>
//#include <stm32f4xx_spi.h>
//#include <sys/_stdint.h>

//variables from other modules

#define DCO_BLOCKS_MAX 24

extern unsigned long seconds;
extern bool PROG_buttons_control_tone_volume;
extern volatile int16_t sample_i16;
extern float sample_f[2],sample_mix;

class v1_DCO_Synth
{
	static const uint8_t sinetable256[256];
	uint8_t *sawtable256;
	uint8_t *squaretable256;
	uint8_t *use_table256;

	uint16_t envtick;//=549;
	volatile uint8_t DOUBLE;//=0;
	int16_t resonant_peak_mod_volume;// = 0;

	//-------- Synth parameters --------------
	//volatile uint8_t VCA=255;         //VCA level 0-255
	volatile uint8_t ATTACK;//=1;        // ENV Attack rate 0-255
	volatile uint8_t RELEASE;//=1;       // ENV Release rate 0-255
	//volatile uint8_t ENVELOPE=0;      // ENV Shape
	//volatile uint8_t TRIG=0;          //MIDItrig 1=note ON
	volatile uint16_t PDmod;          //Resonant Peak index
	volatile uint8_t ENVamt;          //Resonant Peak envelope modulation amount
	volatile uint8_t PHASEamt;        //Resonant Peak bias
	//-----------------------------------------

	uint8_t otone1[DCO_BLOCKS_MAX];
	uint8_t otone2[DCO_BLOCKS_MAX];
	uint16_t phacc[DCO_BLOCKS_MAX];
	uint16_t pdacc[DCO_BLOCKS_MAX];

	int DCO_BLOCKS_ACTIVE;// = DCO_BLOCKS_MAX;

	int32_t DCO_output[2];

	/*
	#define SEQUENCE_LENGTH 16
	float sequence[SEQUENCE_LENGTH];
	int sequencer_head=0;
	int SEQUENCER_THRESHOLD = 0;
	*/

	int DCO_mode;// = 0;
	int SET_mode;// = 0;
	int button_set_held;// = 0;

public:

	uint16_t FREQ;//=0;         //DCO pitch

	v1_DCO_Synth();
	//~v1_DCO_Synth(void);

	void v1_init();
	void v1_play_loop();
};

#endif /* v1_DCO_SYNTH_H_ */
