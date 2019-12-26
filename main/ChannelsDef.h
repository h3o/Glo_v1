/*
 * ChannelsDef.h
 *
 *  Created on: Feb 02, 2019
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

#ifndef CHANNELS_DEF_H_
#define CHANNELS_DEF_H_

void channel_high_pass(int song, int melody, int sample, float sample_volume);
void channel_high_pass_high_reso(int song, int melody, int sample);
void channel_low_pass(int song, int melody);
void channel_low_pass_high_reso(int song, int melody, int sample);
void channel_direct_waves(int use_reverb);
void channel_white_noise();
void channel_antarctica();
void channel_dco();
void channel_granular();
//void channel_sampler();
void channel_reverb();
void channel_mi_clouds();

void pass_through();

#endif /* CHANNELS_DEF_H_ */
