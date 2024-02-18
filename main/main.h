/*
 * main.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Jan 23, 2018
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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h> //for uint16_t type

#include "glo_config.h"
#include "hw/init.h"
#include "hw/codec.h"
#include "hw/gpio.h"
#include "hw/signals.h"
#include "hw/leds.h"
#include "hw/Accelerometer.h"
#include "hw/Sensors.h"
#include "hw/sdcard.h"
#include "hw/ui.h"
#include "hw/fw_update.h"
#include "hw/midi.h"
#include "hw/sync.h"
#include "hw/sysex.h"

#include "ChannelsDef.h"
#include "ProgramSong.h"
#include "InitChannels.h"
#include "FilteredChannels.h"

#include "Interface.h"
#include "dsp/FreqDetect.h"

#include "dsp/Bytebeat.h"
#include "dsp/Chopper.h"
#include "dsp/DrumKit.h"
#include "dsp/SamplerLooper.h"
#include "dsp/FilterFlanger.h"
#include "dsp/Mellotron.h"

#include "mi/clouds/clouds.h"
