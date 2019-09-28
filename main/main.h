/*
 * main.h
 *
 *  Created on: Jan 23, 2018
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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h> //for uint16_t type

#include <glo_config.h>
#include <hw/init.h>
#include <hw/codec.h>
#include <hw/gpio.h>
#include <hw/signals.h>
#include <hw/leds.h>
#include <hw/Accelerometer.h>
#include <hw/Sensors.h>
#include <hw/sdcard.h>
#include <hw/ui.h>

#include <ChannelsDef.h>
#include <InitChannels.h>

#include <Interface.h>
#include <dsp/FreqDetect.h>

#include <dsp/Bytebeat.h>
#include <dsp/Chopper.h>
#include <dsp/DrumKit.h>

#include <mi/clouds/clouds.h>

//-----------------------------------------------------------------------------------

Filters *fil = NULL;

//float rnd_f;
//uint32_t random_value;

int run_program = 1;
int tempo = 1;
int chord = 0;

float new_mixing_vol; //tmp var for accelerometer-driven ARP voice volume calculation
//float new_delay_length; //tmp var for accelerometer-driven echo delay length calculation
