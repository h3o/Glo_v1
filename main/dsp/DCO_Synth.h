/*
 * DCO_synth.h
 *
 *  Created on: 11 May 2017
 *      Author: mario
 *
 * Based on "The Tiny-TS Touch Synthesizer" by Janost 2016, Sweden
 * https://janostman.wordpress.com/the-tiny-ts-diy-touch-synthesizer/
 * https://www.kickstarter.com/projects/732508269/the-tiny-ts-an-open-sourced-diy-touch-synthesizer/
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

#ifndef EXTENSIONS_DCO_SYNTH_H_
#define EXTENSIONS_DCO_SYNTH_H_

#include <stdint.h> //for uint16_t type etc.

//extern void IR_sensors_LED_indicators(int *sensor_values);

//variables from other modules

#define DCO_BLOCKS_MAX 32 //48 //40 //24

//extern unsigned long seconds;
//extern bool PROG_buttons_control_tone_volume;
//extern volatile int16_t sample_i16;
//extern float sample_f[2],sample_mix;

//#define DCO_MAX_PARAMS	8	//DCO_BLOCKS_ACTIVE,FREQ,DOUBLE,PHASEamt,ENVamt,RESO,ATTACK,RELEASE
#define DCO_MAX_PARAMS	5	//FREQ,DOUBLE,PHASEamt,ENVamt,RESO
#define DCO_WAVEFORMS	4	//sine,square,saw,off
#define DRIFT_PARAMS_MAX	8	//off, slowly wander, drift or randomize every time at various timing: 1/2,1/4,1/8,1/16

#define DCO_FREQ_DEFAULT 32000

class DCO_Synth
{
	//uint8_t *sawtable256;
	//uint8_t *squaretable256;
	int erase_echo = 0;

	int param = 0, waveform = 1, drift_params = 0, drift_params_cnt;

	int dco_oversample = 1;
	int shift_param = 0;
	int dco_control_mode = 0;

	#ifdef USE_AUTOCORRELATION
	#define DCO_CONTROL_MODES 3	//acc,buttons,voice
	#else
	#define DCO_CONTROL_MODES 2	//acc,buttons
	#endif

	static const uint8_t sinetable256[256];
	uint8_t *sawtable256;
	uint8_t *squaretable256;
	uint8_t *use_table256;

	uint16_t envtick;//=549;
	volatile uint8_t DOUBLE;//=0;
	int16_t RESO;// = 0;

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
	int MIDI_note_set;// = 0;//, MIDI_note_on = 0;
	//int MIDI_data;

	DCO_Synth();
	~DCO_Synth(void);

	void v1_init();
	void v1_play_loop();

	void v2_init();
	void v2_play_loop();

	//void v3_init();
	void v3_play_loop();
	void v4_play_loop();

	//static void init_codec();
};

#endif /* EXTENSIONS_DCO_SYNTH_H_ */
