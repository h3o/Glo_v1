/*
 * midi.h
 *
 *  Created on: Mar 25, 2019
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

#ifndef MIDI_H_
#define MIDI_H_

#include "init.h"

#define MIDI_CHORD_NOTES 3
#define MIDI_OUT_POLYPHONY 3

extern uint8_t MIDI_last_chord[];
extern uint8_t MIDI_last_melody_note;
extern uint8_t MIDI_last_melody_note_velocity;
extern uint8_t MIDI_keys_pressed, MIDI_note_on, MIDI_notes_updated, MIDI_controllers_updated, /*MIDI_controllers_active,*/ MIDI_controllers_active_PB, MIDI_controllers_active_CC;
extern uint8_t MIDI_ctrl[];

//https://ccrma.stanford.edu/~craig/articles/linuxmidi/misc/essenmidi.html
//Command	Meaning					# parameters	param 1			param 2
//0x80		Note-off				2				key				velocity
//0x90		Note-on					2				key				veolcity
//0xA0		Aftertouch				2				key				touch
//0xB0		Continuous controller	2				controller #	controller value
//0xC0		Patch change			2				instrument #
//0xD0		Channel Pressure		1				pressure
//0xE0		Pitch bend				2				lsb (7 bits)	msb (7 bits)
//0xF0		(non-musical commands)

#define MIDI_MSG_NOTE_OFF			0x80
#define MIDI_MSG_NOTE_ON			0x90
#define MIDI_MSG_CONT_CTRL			0xb0
#define MIDI_MSG_PITCH_BEND			0xe0
#define MIDI_MSG_TIMING_CLOCK		0xf8	//Timing Clock (Sys Realtime)

#define MIDI_WHEEL_CONTROLLERS		2
#define MIDI_WHEEL_CONTROLLER_CC	0	//continuous controller
#define MIDI_WHEEL_CONTROLLER_PB	1	//pitch bend
#define MIDI_WHEEL_CONTROLLER_CC_UPDATED	1
#define MIDI_WHEEL_CONTROLLER_PB_UPDATED	2

//states during parsing MIDI message
#define MIDI_PARSING_NONE			0
#define MIDI_PARSING_PRESSED		1
#define MIDI_PARSING_RELEASED		2
#define MIDI_PARSING_VELOCITY		3
#define MIDI_PARSING_CONT_CTRL		11
#define MIDI_PARSING_CONT_CTRL_N	12	//controller #
//#define MIDI_PARSING_CONT_CTRL_P	13	//parameter
#define MIDI_PARSING_PITCH_BEND		21
#define MIDI_PARSING_PITCH_BEND_N	22	//controller #
//#define MIDI_PARSING_PITCH_BEND_P	23	//parameter
#define MIDI_PARSING_IGNORE			9	//ignore the 3rd byte, unsupported controller # or parameter

// ----------------------------------------------------------------------------------------------------------------------------------

#define MIDI_SYNC_MODE_OFF				0	//MIDI, sync or CV not used
#define MIDI_SYNC_MODE_MIDI_IN_OUT		1	//clock derived and chords captured from MIDI if signal present, chords transmitted while playing
#define MIDI_SYNC_MODE_MIDI_IN			2	//clock derived and chords captured from MIDI if signal present, nothing transmitted
#define MIDI_SYNC_MODE_MIDI_OUT			3	//chords transmitted via MIDI while playing, no clock sent, incoming signal ignored
#define MIDI_SYNC_MODE_MIDI_CLOCK_OUT	4	//chords and clock transmitted via MIDI while playing
#define MIDI_SYNC_MODE_SYNC_IN			5	//clock derived from sync if signal present, CV ignored
#define MIDI_SYNC_MODE_SYNC_OUT			6	//clock transmitted via sync while playing, CV not sent
#define MIDI_SYNC_MODE_SYNC_CV_IN		7	//clock derived from sync if signal present, CV used if channel supports it
#define MIDI_SYNC_MODE_SYNC_CV_OUT		8	//clock transmitted via sync while playing, CV sent if channel generates it

#define MIDI_SYNC_MODES					9

#define MIDI_SYNC_MODE_DEFAULT			MIDI_SYNC_MODE_MIDI_IN_OUT

#define MIDI_POLYPHONY_HOLD_COMBINED			0	//3 keys pressed considered a chord, one pressed will spread up and down an octave
#define MIDI_POLYPHONY_HOLD_CHORD				1	//last 3 pressed keys considered a chord, released at note off
#define MIDI_POLYPHONY_HOLD_SINGLE_NOTE			2	//last pressed key filling chord variable, 3 times the same note, released at note off
#define MIDI_POLYPHONY_HOLD_OCTAVE_UP_DOWN		3	//last pressed key filling chord variable, octave up/down, released at note off
#define MIDI_POLYPHONY_SUST_CHORD				4	//last 3 pressed keys considered a chord, persists after note off
#define MIDI_POLYPHONY_SUST_SINGLE_NOTE			5	//last pressed key filling chord variable, 3 times the same note, persists after note off
#define MIDI_POLYPHONY_SUST_OCTAVE_UP_DOWN		6	//last pressed key filling chord variable, octave up/down, persists after note off

#define MIDI_POLYPHONY_MODES					7

#define MIDI_POLYPHONY_DEFAULT					MIDI_POLYPHONY_HOLD_COMBINED

#define MIDI_POLYPHONY_HOLD_TYPE	(midi_polyphony>=MIDI_POLYPHONY_HOLD_COMBINED && midi_polyphony<=MIDI_POLYPHONY_HOLD_OCTAVE_UP_DOWN)

#define MIDI_AFTERTOUCH_CHANNEL_PRESSURE		0	//0xD0		Channel Pressure
#define MIDI_AFTERTOUCH_POLYPHONIC				1	//0xA0		Aftertouch

#define MIDI_AFTERTOUCH_DEFAULT					MIDI_AFTERTOUCH_CHANNEL_PRESSURE

#define MIDI_RX_CHANNEL_DEFAULT					1
#define MIDI_TX_CHANNEL_DEFAULT					1
#define MIDI_CONT_CTRL_N_DEFAULT				1

// ----------------------------------------------------------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

void send_MIDI_notes(uint8_t *notes, int current_chord, int n_notes);

void gecho_start_receive_MIDI();
void gecho_stop_receive_MIDI();

void MIDI_parser_reset();

float MIDI_note_to_freq(int8_t note);
void MIDI_to_LED(int note, int on_off);
int MIDI_to_note(int8_t midi_note, char *buf);

#ifdef __cplusplus
}
#endif

#endif
