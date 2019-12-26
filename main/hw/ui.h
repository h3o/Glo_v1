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

extern int select_channel_RDY_idle_blink;
extern int event_next_channel;
extern int event_channel_options;
extern int settings_menu_active;
//extern int menu_indicator_active;
extern int context_menu_active;
extern int last_settings_level;
//extern int default_setting_cycle;
extern int ui_command;
extern int ui_ignore_events;
extern int ui_button3_enabled;
extern int ui_button4_enabled;

// ----------------------------------------------------------------------------------------------------------------------------------

#ifdef BOARD_WHALE

extern int volume_buttons_cnt;
extern int play_button_cnt, short_press_button_play, long_press_button_play, button_play;
extern int play_and_plus_button_cnt, long_press_button_play_and_plus;

extern int hidden_menu;
#define MAIN_CHANNELS 25 //maximum number of directly accessible channels
extern int remapped_channels[MAIN_CHANNELS+1], remapped_channels_found;

#define PLAY_BUTTON_LONG_PRESS 				100		//1000ms is required to detect long press
#define PLAY_AND_PLUS_BUTTON_LONG_PRESS 	200		//2000ms is required to detect long press to activate special action
//#define HIDDEN_MENU_TIMEOUT 				600		//x10ms = 6 sec

extern int button_volume_plus, button_volume_minus, button_volume_both;
extern int short_press_volume_minus, short_press_volume_plus, short_press_volume_both, long_press_volume_both, long_press_volume_plus, long_press_volume_minus;
extern int buttons_ignore_timeout;
extern int short_press_sequence;

#endif

// ----------------------------------------------------------------------------------------------------------------------------------

#ifdef BOARD_GECHO

#define BUTTON_SET	0
#define BUTTON_1	1
#define BUTTON_2	2
#define BUTTON_3	3
#define BUTTON_4	4
#define BUTTON_RST	5

#define BUTTONS_N	6

#define BUTTON_EVENT_NONE			0
#define BUTTON_EVENT_SHORT_PRESS	10
#define BUTTON_EVENT_LONG_PRESS		20
#define BUTTON_EVENT_SET_PLUS		30
#define BUTTON_EVENT_RST_PLUS		40
#define BUTTON_EVENT_LONG_RELEASE	50
#define BUTTON_EVENT_PRESS			60	//unspecified length press & release in a fast reaction mode
#define BUTTON_EVENT_RELEASE		70

#define SET_RST_BUTTONS_LONG_THRESHOLD 		100		//wait 1s till counting as long press
#define USER_BUTTONS_LONG_THRESHOLD 		50		//wait 500ms till counting as long press

#define BUTTON_LONG_THRESHOLD(x)	((x==0||x==5)?SET_RST_BUTTONS_LONG_THRESHOLD:USER_BUTTONS_LONG_THRESHOLD)

extern int user_buttons_cnt;
//extern int btn_event;
extern int btn_event_ext;
extern int next_event_lock;

#define SD_CARD_SAMPLE_NOT_USED		0
#define SD_CARD_SAMPLE_OPENING		1
#define SD_CARD_SAMPLE_OPENED		2
extern uint8_t load_SD_card_sample;
extern uint8_t SD_card_sample_buffer_half;


#define PARAMETER_CONTROL_SENSORS_ACCELEROMETER		0
#define PARAMETER_CONTROL_SENSORS_IRS				1
#define PARAMETER_CONTROL_SENSORS_DEFAULT			PARAMETER_CONTROL_SENSORS_IRS
extern int use_acc_or_ir_sensors;
//extern float normalized_params[];

#define PARAMETER_CONTROL_SELECTED_ACC	(!use_acc_or_ir_sensors)
#define PARAMETER_CONTROL_SELECTED_IRS	(use_acc_or_ir_sensors)

#define CONTEXT_MENU_ITEMS				8

#define CONTEXT_MENU_AGC_MAX_GAIN		1
#define CONTEXT_MENU_ANALOG_VOLUME		2
#define CONTEXT_MENU_BASS_TREBLE		3
#define CONTEXT_MENU_TEMPO				4
#define CONTEXT_MENU_TRANSPOSE			5
#define CONTEXT_MENU_TUNING				6
#define CONTEXT_MENU_S1_S2				7
#define CONTEXT_MENU_S3_S4				8

#define CONTEXT_MENU_WITH_MUTED_ADC		(context_menu_active>=2&&context_menu_active<=4)
#define CONTEXT_MENU_ALLOW_LEDS			4 //the level where S1-S4 override resides

#define MAIN_MENU_LEVEL_0				-1	//waiting for sub-menu selection (buttons 1-4)
#define MAIN_MENU_LEVEL_1				1	//sub-menu #1 selected
#define MAIN_MENU_LEVEL_2				2	//sub-menu #2 selected
#define MAIN_MENU_LEVEL_3				3	//sub-menu #3 selected
#define MAIN_MENU_LEVEL_4				4	//sub-menu #4 selected

#define MAIN_MENU_LEDS_OFF_LEVEL		MAIN_MENU_LEVEL_1
#define MAIN_MENU_LEDS_OFF_BTN			BUTTON_1

#define MAIN_MENU_SENSORS_CONFIG_LEVEL	MAIN_MENU_LEVEL_1
#define MAIN_MENU_SENSORS_CONFIG_BTN	BUTTON_2

#define MAIN_MENU_ACC_INVERT_LEVEL		MAIN_MENU_LEVEL_1
#define MAIN_MENU_ACC_INVERT_BTN		BUTTON_3

#define MAIN_MENU_ACC_ORIENTATION_LEVEL	MAIN_MENU_LEVEL_1
#define MAIN_MENU_ACC_ORIENTATION_BTN	BUTTON_4

#define MAIN_MENU_AGC_ENABLE_LEVEL		MAIN_MENU_LEVEL_2
#define MAIN_MENU_AGC_ENABLE_BTN		BUTTON_1

#define MAIN_MENU_AUTO_POWER_OFF_LEVEL	MAIN_MENU_LEVEL_2
#define MAIN_MENU_AUTO_POWER_OFF_BTN	BUTTON_2

#define MAIN_MENU_SD_CARD_SPEED_LEVEL	MAIN_MENU_LEVEL_2
#define MAIN_MENU_SD_CARD_SPEED_BTN		BUTTON_3

#define MAIN_MENU_SAMPLING_RATE_LEVEL	MAIN_MENU_LEVEL_2
#define MAIN_MENU_SAMPLING_RATE_BTN		BUTTON_4

//#define MAIN_MENU_RESERVED_LEVEL		MAIN_MENU_LEVEL_3
//#define MAIN_MENU_RESERVED_BTN			BUTTON_1

//#define MAIN_MENU_RESERVED_LEVEL		MAIN_MENU_LEVEL_3
//#define MAIN_MENU_RESERVED_BTN			BUTTON_2

#define MAIN_MENU_MIDI_SYNC_MODE_LEVEL	MAIN_MENU_LEVEL_3
#define MAIN_MENU_MIDI_SYNC_MODE_BTN	BUTTON_3

#define MAIN_MENU_MIDI_POLYPHONY_LEVEL	MAIN_MENU_LEVEL_3
#define MAIN_MENU_MIDI_POLYPHONY_BTN	BUTTON_4

#endif

// ----------------------------------------------------------------------------------------------------------------------------------

#define BUTTONS_SHORT_PRESS 				5		//50ms is required to detect button press
#define BUTTONS_CNT_DELAY 					10		//general button scan timing in ms
#define VOLUME_BUTTONS_CNT_DIV 				10		//increase or decrease every 100ms
#define VOLUME_BUTTONS_CONTEXT_MENU_DELAY	50		//additional delay for long press in context menu

#ifdef BOARD_WHALE

#define VOLUME_BUTTONS_CNT_THRESHOLD 		50		//wait 500ms till adjusting volume
#define PLUS_OR_MINUS_BUTTON_LONG_PRESS		100		//1sec for long single volume button press event
#define VOLUME_BUTTONS_BOTH_LONG 			100		//1sec for long both volume buttons press event

#define BUTTONS_IGNORE_TIMEOUT_LONG			100		//ignore next events timeout after long press event (useful when releasing slowly and not at the same time)

#define BUTTONS_SEQUENCE_TIMEOUT_DEFAULT	120		//maximum space between short key presses in a special command sequence is 1200ms
#define BUTTONS_SEQUENCE_TIMEOUT_SHORT		50		//in extra channels, short interval is better

extern int BUTTONS_SEQUENCE_TIMEOUT;

#define CONTEXT_MENU_ITEMS			4

#define CONTEXT_MENU_BASS_TREBLE	1
#define CONTEXT_MENU_TEMPO			2
#define CONTEXT_MENU_TUNING			3
#define CONTEXT_MENU_ECHO_ARP		4

#endif

#define SEQ_PLUS_MINUS				23
#define SEQ_MINUS_PLUS				32

#define PRESS_ORDER_MAX 				19	//maximum lenght of main-menu buttons sequence to parse
#define SCAN_BUTTONS_LOOP_DELAY 		50	//delay in ms between checking on new button press events
#define SET_RST_BUTTON_THRESHOLD_100ms 	2	//SCAN_BUTTONS_LOOP_DELAY * x ms -> 100ms
#define SET_RST_BUTTON_THRESHOLD_1s 	20	//SCAN_BUTTONS_LOOP_DELAY * x ms -> 1 sec
#define SET_RST_BUTTON_THRESHOLD_3s 	60	//SCAN_BUTTONS_LOOP_DELAY * x ms -> 3 sec

extern int tempo_bpm, TEMPO_BY_SAMPLE, MELODY_BY_SAMPLE, DELAY_BY_TEMPO;
extern int tempoCounter, melodyCounter;
extern int MIDI_SYNC_derived_tempo;

//#define TEMPO_BPM_DEFAULT 120
//#define TEMPO_BPM_MIN 10
//#define TEMPO_BPM_MAX 330
//#define TEMPO_BPM_STEP 5

//#define TUNING_DEFAULT 440.0f

#define TUNING_ALL_432	432.0f,432.0f,432.0f,432.0f,432.0f,432.0f
#define TUNING_ALL_440	440.0f,440.0f,440.0f,440.0f,440.0f,440.0f

extern double global_tuning;
extern uint8_t midi_sync_mode, midi_polyphony;
extern uint8_t sensors_active, accelerometer_active;
extern uint32_t sensors_override;

//#define TUNING_INCREASE_COEFF_5_CENTS 1.0028922878693670715023923823933 //240th root of 2
//#define TUNING_INCREASE_COEFF_10_CENTS 1.0057929410678534309188527497122 //120th root of 2
//#define TUNING_INCREASE_COEFF_20_CENTS 1.0116194403019224846862305670455 //60th root of 2
//#define TUNING_INCREASE_COEFF_25_CENTS 1.0145453349375236414538678576629 //48th root of 2

//#define GLOBAL_TUNING_MIN 391.9954359818f
//#define GLOBAL_TUNING_MAX 493.8833012561f

extern settings_t global_settings;
extern persistent_settings_t persistent_settings;

#define PERSISTENT_SETTINGS_UPDATE_TIMER		200 //wait 2 sec before writing updated settings to Flash
#define PERSISTENT_SETTINGS_UPDATE_TIMER_LONG	500 //wait 5 sec before writing updated settings to Flash

extern int settings_indication_in_progress;

#ifdef __cplusplus
 extern "C" {
#endif

#ifdef BOARD_GECHO
void process_buttons_controls_gecho(void *pvParameters);
#endif
#ifdef BOARD_WHALE
void process_buttons_controls_whale(void *pvParameters);
#endif
void context_menu_action(int level, int direction);

//void voice_menu_say(const char *item, voice_menu_t *items, int total_items);

#ifdef BOARD_GECHO
uint64_t select_channel();

#define GET_USER_NUMBER_CANCELED	9999
uint64_t get_user_number();
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
