/*
 * ui.h
 *
 *  Created on: Nov 5, 2018
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

#ifndef UI_H_
#define UI_H_

#include "glo_config.h"
#include "board.h"

extern int event_next_channel;
extern int event_channel_options;
extern int settings_menu_active;

extern int context_menu_active;

#ifdef BOARD_WHALE

extern int play_button_cnt, short_press_button_play, long_press_button_play, button_play;
extern int play_and_plus_button_cnt, long_press_button_play_and_plus;

extern int hidden_menu;
#define MAIN_CHANNELS 25 //maximum number of directly accessible channels
extern int remapped_channels[MAIN_CHANNELS+1], remapped_channels_found;

#define PLAY_BUTTON_LONG_PRESS 				100		//1000ms is required to detect long press
#define PLAY_AND_PLUS_BUTTON_LONG_PRESS 	200		//2000ms is required to detect long press to activate special action
//#define HIDDEN_MENU_TIMEOUT 				600		//x10ms = 6 sec

#endif

#ifdef BOARD_GECHO

#define RST_BUTTON_SHORT_PRESS				5		//50ms
#define SET_BUTTON_SHORT_PRESS				5		//50ms
#define RST_BUTTON_LONG_PRESS				100		//1sec
#define SET_BUTTON_LONG_PRESS				100		//1sec
extern int rst_button_cnt, long_press_rst_button, short_press_rst_button;
extern int set_button_cnt, long_press_set_button, short_press_set_button;

#endif

//shared for both Gecho and Glo
#define BUTTONS_SHORT_PRESS 				5		//50ms is required to detect button press
#define BUTTONS_CNT_DELAY 					10		//general button scan timing in ms
#define VOLUME_BUTTONS_CNT_THRESHOLD 		50		//wait 500ms till adjusting volume
#define VOLUME_BUTTONS_CNT_DIV 				10		//increase or decrease every 100ms
#define PLUS_OR_MINUS_BUTTON_LONG_PRESS		100		//2sec for long single volume button press event
#define VOLUME_BUTTONS_BOTH_LONG 			100		//2sec for long both volume buttons press event

extern int volume_buttons_cnt, button_volume_plus, button_volume_minus, button_volume_both;
extern int short_press_volume_minus, short_press_volume_plus, short_press_volume_both, long_press_volume_both, long_press_volume_plus, long_press_volume_minus;
extern int buttons_ignore_timeout;
extern int short_press_sequence;

#define BUTTONS_IGNORE_TIMEOUT_LONG			100		//ignore next events timeout after long press event (useful when releasing slowly and not at the same time)

#define BUTTONS_SEQUENCE_TIMEOUT_DEFAULT	120		//maximum space between short key presses in a special command sequence is 1200ms
#define BUTTONS_SEQUENCE_TIMEOUT_SHORT		50		//in extra channels, short interval is better

extern int BUTTONS_SEQUENCE_TIMEOUT;

#define SEQ_PLUS_MINUS				23
#define SEQ_MINUS_PLUS				32

#define PRESS_ORDER_MAX 			19	//maximum lenght of main-menu buttons sequence to parse
#define SCAN_BUTTONS_LOOP_DELAY 	50	//delay in ms between checking on new button press events
#define PWR_BUTTON_THRESHOLD_1 		2	//SCAN_BUTTONS_LOOP_DELAY * x ms -> 100ms
#define PWR_BUTTON_THRESHOLD_2 		20	//SCAN_BUTTONS_LOOP_DELAY * x ms -> 1 sec
#define PWR_BUTTON_THRESHOLD_3 		60	//SCAN_BUTTONS_LOOP_DELAY * x ms -> 3 sec

#define CONTEXT_MENU_ITEMS			4

#define CONTEXT_MENU_BASS_TREBLE	1
#define CONTEXT_MENU_TEMPO			2
#define CONTEXT_MENU_TUNING			3
#define CONTEXT_MENU_ECHO_ARP		4

extern int tempo_bpm, TEMPO_BY_SAMPLE, MELODY_BY_SAMPLE, DELAY_BY_TEMPO;
extern int tempoCounter, melodyCounter;

//#define TEMPO_BPM_DEFAULT 120
//#define TEMPO_BPM_MIN 10
//#define TEMPO_BPM_MAX 330
//#define TEMPO_BPM_STEP 5

//#define TUNING_DEFAULT 440.0f

#define TUNING_ALL_432	432.0f,432.0f,432.0f,432.0f,432.0f,432.0f
#define TUNING_ALL_440	440.0f,440.0f,440.0f,440.0f,440.0f,440.0f

extern double global_tuning;

//#define TUNING_INCREASE_COEFF_5_CENTS 1.0028922878693670715023923823933 //240th root of 2
//#define TUNING_INCREASE_COEFF_10_CENTS 1.0057929410678534309188527497122 //120th root of 2
//#define TUNING_INCREASE_COEFF_20_CENTS 1.0116194403019224846862305670455 //60th root of 2
//#define TUNING_INCREASE_COEFF_25_CENTS 1.0145453349375236414538678576629 //48th root of 2

//#define GLOBAL_TUNING_MIN 391.9954359818f
//#define GLOBAL_TUNING_MAX 493.8833012561f

extern settings_t global_settings;
extern persistent_settings_t persistent_settings;

#define PERSISTENT_SETTINGS_UPDATE_TIMER 200 //wait 2 sec before writing updated settings to Flash

#ifdef __cplusplus
 extern "C" {
#endif

void process_buttons_controls_whale(void *pvParameters);
void process_buttons_controls_gecho(void *pvParameters);
void context_menu_action(int level, int direction);

#ifdef BOARD_WHALE
void voice_menu_say(const char *item, voice_menu_t *items, int total_items);
#endif

#ifdef BOARD_GECHO
uint64_t select_channel();
#endif

int get_tempo_by_BPM(int bpm);
int get_delay_by_BPM(int bpm);

#ifdef BOARD_WHALE
void check_auto_power_off();
#endif

#ifdef __cplusplus
}
#endif

#endif /* UI_H_ */
