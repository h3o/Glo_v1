/*
 * legacy.h - Filters + Echo + WT + Goertzel test/demo program
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Apr 10, 2016
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

#include <esp_attr.h>
#include <stdlib.h>
#include <string.h>

#include "v1_filters.h"
#include "v1_gpio.h"
#include "v1_signals.h"

#define ENABLE_ECHO
#define ENABLE_RHYTHM
#define ENABLE_CHORD_LOOP
#define ADD_MICROPHONE_SIGNAL
#define ADD_PLAIN_NOISE

#define ENABLE_LIMITER_FLOAT
#define ENABLE_LIMITER_INT

#define USE_FAUX_22k_MODE
#define USE_IR_SENSORS

//sensor 4 - arp
#define SENSOR_ARP_ACTIVE_1				SENSOR_THRESHOLD_WHITE_8
#define SENSOR_ARP_ACTIVE_2				SENSOR_THRESHOLD_WHITE_7
#define SENSOR_ARP_ACTIVE_3				SENSOR_THRESHOLD_WHITE_6
#define SENSOR_ARP_ACTIVE_4				SENSOR_THRESHOLD_WHITE_5
#define SENSOR_ARP_ACTIVE_5				SENSOR_THRESHOLD_WHITE_4
#define SENSOR_ARP_ACTIVE_6				SENSOR_THRESHOLD_WHITE_3
#define SENSOR_ARP_ACTIVE_7				SENSOR_THRESHOLD_WHITE_2
#define SENSOR_ARP_ACTIVE_8				SENSOR_THRESHOLD_WHITE_1
#define SENSOR_ARP_INCREASE				(ir_res[3]*0.6f)

//sensor 3 - resonance
#define SENSOR_RESO_ACTIVE_1			SENSOR_THRESHOLD_BLUE_5
#define SENSOR_RESO_ACTIVE_2			SENSOR_THRESHOLD_BLUE_4
#define SENSOR_RESO_ACTIVE_3			SENSOR_THRESHOLD_BLUE_3
#define SENSOR_RESO_ACTIVE_4			SENSOR_THRESHOLD_BLUE_2
#define SENSOR_RESO_ACTIVE_5			SENSOR_THRESHOLD_BLUE_1

//sensor 2 - noise mixing volume decrease
#define SENSOR_NOISE_DIM_ACTIVE_1		SENSOR_THRESHOLD_ORANGE_1
#define SENSOR_NOISE_DIM_LEVEL			(1.0f-ir_res[1])
#define SENSOR_NOISE_DIM_LEVEL_DEFAULT	1.0f

//sensor 1 - noise mixing volume increase
#define SENSOR_NOISE_RISE_ACTIVE_1		SENSOR_THRESHOLD_RED_1
#define SENSOR_NOISE_RISE_LEVEL			(1.0f+2*ir_res[0])

/*
//sensor 1 - echo delay length
#define SENSOR_DELAY_9					SENSOR_THRESHOLD_RED_9
#define SENSOR_DELAY_8					SENSOR_THRESHOLD_RED_8
#define SENSOR_DELAY_7					SENSOR_THRESHOLD_RED_7
#define SENSOR_DELAY_6					SENSOR_THRESHOLD_RED_6
#define SENSOR_DELAY_5					SENSOR_THRESHOLD_RED_5
#define SENSOR_DELAY_4					SENSOR_THRESHOLD_RED_4
#define SENSOR_DELAY_3					SENSOR_THRESHOLD_RED_3
#define SENSOR_DELAY_2					SENSOR_THRESHOLD_RED_2
#define SENSOR_DELAY_ACTIVE				SENSOR_THRESHOLD_RED_1
*/

//#define LEGACY_USE_44k_RATE
#define LEGACY_USE_48k_RATE
//#define LEGACY_USE_22k_RATE

#define PLAIN_NOISE_VOLUME 48.0f //tested on v2 board
//#define PLAIN_NOISE_VOLUME 20.0f //stm32f4-disc1

#define PREAMP_BOOST 24.0f //tested on v2 board
//#define PREAMP_BOOST 12.0f //stm32f4-disc1
//#define PREAMP_BOOST 8 //gecho v1 board

#define ARP_VOLUME_MULTIPLIER_DEFAULT	3.333f

//#define DEBUG_SIGNAL_LEVELS

extern int sample_counter;

void LED_indicators();

#ifdef USE_FILTERS
	extern v1_filters *v1_fil;
#endif

extern float sample_f[2],sample_synth[2],sample_mix;
extern unsigned long seconds;

#define SAMPLE_VOLUME_BOOST_MAX 5.0f
extern float SAMPLE_VOLUME_BOOST;

extern float volume2;
extern float noise_volume, noise_volume_max;

#ifdef ENABLE_RHYTHM
	extern int progressive_rhythm_factor;

	#define PROGRESSIVE_RHYTHM_BOOST_RATIO 2
	#define PROGRESSIVE_RHYTHM_RAMP_COEF 0.999f
#endif

#define AUDIOFREQ_DIV_CORRECTION 4

void legacy_main();
int16_t add_echo_legacy(int16_t sample);

extern uint32_t ADC_sample, ADC_sample0;
extern uint32_t random_value;
extern volatile int16_t sample_i16;
