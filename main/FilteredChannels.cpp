/*
 * FilteredChannels.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Nov 26, 2016
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

#include "FilteredChannels.h"
#include "InitChannels.h"
#include "Accelerometer.h"
#include "Sensors.h"
#include "Interface.h"
#include "Binaural.h"
#include "hw/init.h"
#include "hw/signals.h"
#include "hw/sdcard.h"
#include "hw/leds.h"
#include "hw/ui.h"
#include "hw/midi.h"

//#define USE_AUTOCORRELATION
#define DRY_MIX_AFTER_ECHO
//#define LIMITER_DEBUG

double r_noise = NOISE_SEED;
int i_noise;

#define NOISE_BOOST_BY_MIDI_MAX		4.0f
#define SAMPLE_VOLUME_BY_MIDI_MAX	4.0f

float mixed_sample_volume_coef_ext;

int wind_voices;

#ifdef LIMITER_DEBUG
int limiter_debug[8] = {32760, -32760, 32760, -32760, 32760, -32760, 32760, -32760};
#endif

IRAM_ATTR void filtered_channel(int use_bg_sample, int use_direct_waves_filters, int use_reverb, float mixed_sample_volume_coef, int controls_type)
{
	printf("filtered_channel(): wind_voices = %d\n",wind_voices);
	#ifdef BOARD_GECHO

	float reverb_volume = 1.0f;
	int wind_cutoff_sweep = 0;
	mixed_sample_volume_coef_ext = mixed_sample_volume_coef;

	#define DIRECT_WAVES_RANGES			8
	#define DIRECT_WAVES_RANGE_DEFAULT	3
	uint16_t direct_waves_ranges[DIRECT_WAVES_RANGES] = {100,200,300,400,600,800,1200,1600};
	uint16_t direct_waves_range = direct_waves_ranges[DIRECT_WAVES_RANGE_DEFAULT]; //1000;
	int direct_waves_range_ptr = DIRECT_WAVES_RANGE_DEFAULT;

	uint8_t reso[2] = {0,0}; //for detecting if resonance has changed and needs to be updated

	int midi_override = 0;
	int lock_sensors = 0;

	if(persistent_settings.SAMPLING_RATE != SAMPLE_RATE_DEFAULT)
	{
		set_sampling_rate(SAMPLE_RATE_DEFAULT);
	}

	if(controls_type==PARAMETER_CONTROL_SENSORS_ACCELEROMETER) //accelerometer
	{
	#endif
		//sensor 1 - arp (basic channels) or reverb (fire channel)
		#define SENSOR_REVERB_ACTIVE			(acc_res[0] > 0.2f)
		#define SENSOR_REVERB_INCREASE			(acc_res[0] + 1.0f)
		#define SENSOR_ARP_ACTIVE_1				(acc_res[0] < -0.2f)
		#define SENSOR_ARP_ACTIVE_2				(acc_res[0] < -0.4f)
		#define SENSOR_ARP_INCREASE				(-acc_res[0] - 0.4f)
		//sensor 2 - noise or sample mixing volume
		#define SENSOR_SAMPLE_MIX_VOL			(1 - acc_res[1])
		//sensor 3 - echo delay length
		#define SENSOR_DELAY_9					(acc_res[2] > 0.95f)
		#define SENSOR_DELAY_8					(acc_res[2] > 0.85f)
		#define SENSOR_DELAY_7					(acc_res[2] > 0.74f)
		#define SENSOR_DELAY_6					(acc_res[2] > 0.62f)
		#define SENSOR_DELAY_5					(acc_res[2] > 0.49f)
		#define SENSOR_DELAY_4					(acc_res[2] > 0.35f)
		#define SENSOR_DELAY_3					(acc_res[2] > 0.2f)
		#define SENSOR_DELAY_2					(acc_res[2] > 0.0f)
		#define SENSOR_DELAY_ACTIVE				(acc_res[2] > -0.2f)
		//direct resonance control is not implemented with accelerometric controls
		//#define SENSOR_RESO_ACTIVE_1			false
		//#define SENSOR_RESO_ACTIVE_2			false
		//#define SENSOR_RESO_ACTIVE_3			false
		//#define SENSOR_RESO_ACTIVE_4			false
		//#define SENSOR_RESO_ACTIVE_5			false
		//control more than one parameter by all sensors at once
		#define PARAMS_ARRAY_BY_ALL_SENSORS		(acc_res[i%ACC_RESULTS] + 0.9f)

		#define FILTERS_BLOCK_CONTROL_ACCELEROMETER
		#include "filtered_channel_loop.inc"
	#ifdef BOARD_GECHO
	}
	else
	{
		//sensor 4 - arp (basic channels) or reverb (fire channel)
		#define SENSOR_REVERB_ACTIVE			(ir_res[3] > IR_sensors_THRESHOLD_1)
		#define SENSOR_REVERB_INCREASE			(ir_res[3]*2.0f)
		#define SENSOR_ARP_ACTIVE_1				(ir_res[3] > IR_sensors_THRESHOLD_1)
		#define SENSOR_ARP_ACTIVE_2				(ir_res[3] > IR_sensors_THRESHOLD_2)
		#define SENSOR_ARP_INCREASE				(ir_res[3]*0.6f)
		//sensor 2 - noise or sample mixing volume
		#define SENSOR_SAMPLE_MIX_VOL			((1.0f-ir_res[1])*2.0f)
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
		//sensor 3 - direct resonance control
		#define SENSOR_RESO_ACTIVE_1			SENSOR_THRESHOLD_BLUE_5
		#define SENSOR_RESO_ACTIVE_2			SENSOR_THRESHOLD_BLUE_4
		#define SENSOR_RESO_ACTIVE_3			SENSOR_THRESHOLD_BLUE_3
		#define SENSOR_RESO_ACTIVE_4			SENSOR_THRESHOLD_BLUE_2
		#define SENSOR_RESO_ACTIVE_5			SENSOR_THRESHOLD_BLUE_1
		//control more than one parameter by all sensors at once
		#define PARAMS_ARRAY_BY_ALL_SENSORS		(ir_res[i%IR_SENSORS]*1.9f)

		#define FILTERS_BLOCK_CONTROL_IRS
		#include "filtered_channel_loop.inc"
	}
	#endif
}

IRAM_ATTR void filtered_channel_add_echo()
{
	if(!echo_dynamic_loop_length)
	{
		return;
	}

	//wrap the echo loop
	echo_buffer_ptr0++;
	if (echo_buffer_ptr0 >= echo_dynamic_loop_length)
	{
		echo_buffer_ptr0 = 0;
	}

	echo_buffer_ptr = echo_buffer_ptr0 + 1;
	if (echo_buffer_ptr >= echo_dynamic_loop_length)
	{
		echo_buffer_ptr = 0;
	}

	//add echo from the loop
	echo_mix_f = float(sample_i16) + float(echo_buffer[echo_buffer_ptr]) * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;

	if(echo_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER)
	{
		echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER;
	}

	if(echo_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER)
	{
		echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER;
	}

	sample_i16 = (int16_t)echo_mix_f;

	//store result to echo, the amount defined by a fragment
	//echo_mix_f = ((float)sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV);
	//echo_mix_f *= ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
	//echo_buffer[echo_buffer_ptr0] = (int16_t)echo_mix_f;

	echo_buffer[echo_buffer_ptr0] = sample_i16;
}

IRAM_ATTR void filtered_channel_adjust_reverb()
{
	bit_crusher_reverb+=3;

	if(bit_crusher_reverb>BIT_CRUSHER_REVERB_MAX)
	{
		bit_crusher_reverb = BIT_CRUSHER_REVERB_MAX;
	}

	reverb_dynamic_loop_length = bit_crusher_reverb;
}

void filters_speed_test()
{
	channel_init(0, 41, 0, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_ALL, 0, 1);
	printf("filtered_channel(): wind_voices = %d\n",wind_voices);
	float reverb_volume = 1.0f;
	int wind_cutoff_sweep = 0;
	#define DIRECT_WAVES_RANGES			8
	#define DIRECT_WAVES_RANGE_DEFAULT	3
	uint16_t direct_waves_ranges[DIRECT_WAVES_RANGES] = {100,200,300,400,600,800,1200,1600};
	uint16_t direct_waves_range = direct_waves_ranges[DIRECT_WAVES_RANGE_DEFAULT]; //1000;
	int direct_waves_range_ptr = DIRECT_WAVES_RANGE_DEFAULT;
	uint8_t reso[2] = {0,0}; //for detecting if resonance has changed and needs to be updated
	int midi_override = 0;
	int lock_sensors = 0;
	#include "filtered_channel_loop_speed_test.inc"
}
