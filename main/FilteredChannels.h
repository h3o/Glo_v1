/*
 * FilteredChannels.h
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

#ifndef FILTERED_CHANNELS_H_
#define FILTERED_CHANNELS_H_

void filtered_channel(int use_bg_sample, int use_direct_waves_filters, int use_reverb, float mixed_sample_volume_coef, int controls_type);
void filtered_channel_add_echo();
void filtered_channel_adjust_reverb();

void filters_speed_test();

#endif /* FILTERED_CHANNELS_H_ */
