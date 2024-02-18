/*
 * leds.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Jun 21, 2016
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

#ifndef LEDS_H_
#define LEDS_H_

#include <stdint.h>
#include "board.h"

#ifdef BOARD_GECHO

/*
//good with 100k resistors
#define IR_sensors_THRESHOLD_1	0.15f
#define IR_sensors_THRESHOLD_2	0.24f
#define IR_sensors_THRESHOLD_3	0.33f
#define IR_sensors_THRESHOLD_4	0.42f
#define IR_sensors_THRESHOLD_5	0.51f
#define IR_sensors_THRESHOLD_6	0.6f
#define IR_sensors_THRESHOLD_7	0.7f
#define IR_sensors_THRESHOLD_8	0.8f
#define IR_sensors_THRESHOLD_9	0.9f
*/
/*
//good with 47k resistors
#define IR_sensors_THRESHOLD_1	0.09f
#define IR_sensors_THRESHOLD_2	0.18f
#define IR_sensors_THRESHOLD_3	0.27f
#define IR_sensors_THRESHOLD_4	0.36f
#define IR_sensors_THRESHOLD_5	0.45f
#define IR_sensors_THRESHOLD_6	0.54f
#define IR_sensors_THRESHOLD_7	0.63f
#define IR_sensors_THRESHOLD_8	0.72f
#define IR_sensors_THRESHOLD_9	0.81f
*/
/*
//hard to trigger inner one without outer one
#define IR_sensors_THRESHOLD_1	0.06f
#define IR_sensors_THRESHOLD_2	0.17f
#define IR_sensors_THRESHOLD_3	0.28f
#define IR_sensors_THRESHOLD_4	0.39f
#define IR_sensors_THRESHOLD_5	0.50f
#define IR_sensors_THRESHOLD_6	0.61f
#define IR_sensors_THRESHOLD_7	0.72f
#define IR_sensors_THRESHOLD_8	0.83f
#define IR_sensors_THRESHOLD_9	0.94f
*/

/*
//good with 47k resistors too
#define IR_sensors_THRESHOLD_1	0.080f
#define IR_sensors_THRESHOLD_2	0.176f
#define IR_sensors_THRESHOLD_3	0.273f
#define IR_sensors_THRESHOLD_4	0.369f
#define IR_sensors_THRESHOLD_5	0.465f
#define IR_sensors_THRESHOLD_6	0.561f
#define IR_sensors_THRESHOLD_7	0.658f
#define IR_sensors_THRESHOLD_8	0.754f
#define IR_sensors_THRESHOLD_9	0.850f
*/

#ifdef BOARD_GECHO_V179 //also applies to v1.80 and v1.81
#define IR_sensors_THRESHOLD_1	(0.100f+0.050f)
#define IR_sensors_THRESHOLD_2	(0.194f+0.050f)
#define IR_sensors_THRESHOLD_3	(0.288f+0.050f)
#define IR_sensors_THRESHOLD_4	(0.381f+0.050f)
#define IR_sensors_THRESHOLD_5	(0.475f+0.050f)
#define IR_sensors_THRESHOLD_6	(0.569f+0.050f)
#define IR_sensors_THRESHOLD_7	(0.663f+0.050f)
#define IR_sensors_THRESHOLD_8	(0.756f+0.050f)
#define IR_sensors_THRESHOLD_9	(0.850f+0.050f)
#else
#define IR_sensors_THRESHOLD_1	0.100f
#define IR_sensors_THRESHOLD_2	0.194f
#define IR_sensors_THRESHOLD_3	0.288f
#define IR_sensors_THRESHOLD_4	0.381f
#define IR_sensors_THRESHOLD_5	0.475f
#define IR_sensors_THRESHOLD_6	0.569f
#define IR_sensors_THRESHOLD_7	0.663f
#define IR_sensors_THRESHOLD_8	0.756f
#define IR_sensors_THRESHOLD_9	0.850f
#endif

#define SENSOR_ID_RED		0
#define SENSOR_ID_ORANGE	1
#define SENSOR_ID_BLUE		2
#define SENSOR_ID_WHITE		3

#define SENSOR_THRESHOLD_ORANGE_4 (ir_res[1] > IR_sensors_THRESHOLD_7)
#define SENSOR_THRESHOLD_ORANGE_3 (ir_res[1] > IR_sensors_THRESHOLD_5)
#define SENSOR_THRESHOLD_ORANGE_2 (ir_res[1] > IR_sensors_THRESHOLD_3)
#define SENSOR_THRESHOLD_ORANGE_1 (ir_res[1] > IR_sensors_THRESHOLD_1)

#define SENSOR_THRESHOLD_RED_9 (ir_res[0] > IR_sensors_THRESHOLD_9) //extended, not indicated by a LED
#define SENSOR_THRESHOLD_RED_8 (ir_res[0] > IR_sensors_THRESHOLD_8)
#define SENSOR_THRESHOLD_RED_7 (ir_res[0] > IR_sensors_THRESHOLD_7)
#define SENSOR_THRESHOLD_RED_6 (ir_res[0] > IR_sensors_THRESHOLD_6)
#define SENSOR_THRESHOLD_RED_5 (ir_res[0] > IR_sensors_THRESHOLD_5)
#define SENSOR_THRESHOLD_RED_4 (ir_res[0] > IR_sensors_THRESHOLD_4)
#define SENSOR_THRESHOLD_RED_3 (ir_res[0] > IR_sensors_THRESHOLD_3)
#define SENSOR_THRESHOLD_RED_2 (ir_res[0] > IR_sensors_THRESHOLD_2)
#define SENSOR_THRESHOLD_RED_1 (ir_res[0] > IR_sensors_THRESHOLD_1)

#define SENSOR_THRESHOLD_BLUE_1 (ir_res[2] > IR_sensors_THRESHOLD_8)
#define SENSOR_THRESHOLD_BLUE_2 (ir_res[2] > IR_sensors_THRESHOLD_6)
#define SENSOR_THRESHOLD_BLUE_3 (ir_res[2] > IR_sensors_THRESHOLD_4)
#define SENSOR_THRESHOLD_BLUE_4 (ir_res[2] > IR_sensors_THRESHOLD_2)
#define SENSOR_THRESHOLD_BLUE_5 (ir_res[2] > IR_sensors_THRESHOLD_1)

#define SENSOR_THRESHOLD_WHITE_1 (ir_res[3] > IR_sensors_THRESHOLD_8)
#define SENSOR_THRESHOLD_WHITE_2 (ir_res[3] > IR_sensors_THRESHOLD_7)
#define SENSOR_THRESHOLD_WHITE_3 (ir_res[3] > IR_sensors_THRESHOLD_6)
#define SENSOR_THRESHOLD_WHITE_4 (ir_res[3] > IR_sensors_THRESHOLD_5)
#define SENSOR_THRESHOLD_WHITE_5 (ir_res[3] > IR_sensors_THRESHOLD_4)
#define SENSOR_THRESHOLD_WHITE_6 (ir_res[3] > IR_sensors_THRESHOLD_3)
#define SENSOR_THRESHOLD_WHITE_7 (ir_res[3] > IR_sensors_THRESHOLD_2)
#define SENSOR_THRESHOLD_WHITE_8 (ir_res[3] > IR_sensors_THRESHOLD_1)

#define SEQUENCER_INDICATOR_DIV 4
extern int sequencer_LEDs_timing_seq;
extern uint8_t binaural_LEDs_timing_seq;
extern int SENSORS_LEDS_indication_enabled;

#define SETTINGS_INDICATOR_BASE						0x101
#define SETTINGS_INDICATOR_ANIMATE_LEFT_4			SETTINGS_INDICATOR_BASE+1
#define SETTINGS_INDICATOR_ANIMATE_RIGHT_4 			SETTINGS_INDICATOR_BASE+2
#define SETTINGS_INDICATOR_ANIMATE_LEFT_8			SETTINGS_INDICATOR_BASE+3
#define SETTINGS_INDICATOR_ANIMATE_RIGHT_8 			SETTINGS_INDICATOR_BASE+4
#define SETTINGS_INDICATOR_ANIMATE_IRS				SETTINGS_INDICATOR_BASE+5
#define SETTINGS_INDICATOR_ANIMATE_3DS				SETTINGS_INDICATOR_BASE+6
#define SETTINGS_INDICATOR_ANIMATE_ALL_LEDS_OFF		SETTINGS_INDICATOR_BASE+7
#define SETTINGS_INDICATOR_ANIMATE_ALL_LEDS_ON		SETTINGS_INDICATOR_BASE+8

#define SETTINGS_INDICATOR_ANIMATE_FILL_8_LEFT		SETTINGS_INDICATOR_BASE+9
#define SETTINGS_INDICATOR_ANIMATE_FILL_8_RIGHT		SETTINGS_INDICATOR_BASE+10
#define SETTINGS_INDICATOR_ANIMATE_CLEAR_8_LEFT		SETTINGS_INDICATOR_BASE+11
#define SETTINGS_INDICATOR_ANIMATE_CLEAR_8_RIGHT	SETTINGS_INDICATOR_BASE+12

#define SETTINGS_INDICATOR_EQ_BASS					SETTINGS_INDICATOR_BASE+21
#define SETTINGS_INDICATOR_EQ_TREBLE				SETTINGS_INDICATOR_BASE+22

#define SETTINGS_INDICATOR_MIDI_POLYPHONY			SETTINGS_INDICATOR_BASE+30

#endif

#ifdef __cplusplus
 extern "C" {
#endif

void LED_sequencer_indicators();
void display_chord(int8_t *chord_LEDs, int total_leds);

 #ifdef BOARD_GECHO

void LED_W8_all_ON();
void LED_W8_all_OFF();
void LED_B5_all_ON();
void LED_B5_all_OFF();
void LED_O4_all_ON();
void LED_O4_all_OFF();
void LED_R8_all_OFF();
void LEDs_all_OFF();

void LED_R8_set(int led, int status);
void LED_O4_set(int led, int status);
void LED_W8_set(int led, int status);

void LED_R8_set_byte(int value); //default: left to right
void LED_O4_set_byte(int value);
void LED_B5_set_byte(int value);
void LED_W8_set_byte(int value);

void LED_R8_set_byte_RL(int value); //reversed: right to left
void LED_O4_set_byte_RL(int value);
void LED_B5_set_byte_RL(int value);
void LED_W8_set_byte_RL(int value);

void all_LEDs_test();
void all_LEDs_test_seq1();
void all_LEDs_test_seq2();

void KEY_LED_on_off(int note, int on_off);
void KEY_LED_all_off();

void display_number_of_chords(int row1, int row2);
void ack_by_signal_LED();

void display_volume_level_indicator_f(float value, float min, float max);
void display_volume_level_indicator_i(int value, int min, int max);
void display_tempo_indicator(int bpm);

void display_BCD_numbers(char *digits, int length);

void display_IR_sensors_levels();

void indicate_context_setting(uint16_t leds, int blink, int delay);
void indicate_error(uint16_t leds, int blink, int delay);
void indicate_eq_bass_setting(int value, uint16_t setting);

void LED_sequencer_indicate_position(int chord);

#endif

#ifdef __cplusplus
}
#endif

#endif /* LEDS_H_ */
