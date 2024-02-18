/*
 * ChannelsDef.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Feb 02, 2019
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

#ifndef CHANNELS_DEF_H_
#define CHANNELS_DEF_H_

void channel_high_pass(int song, int melody, int sample, float sample_volume);
void channel_high_pass_high_reso(int song, int melody, int sample);
void channel_low_pass(int song, int melody);
void channel_low_pass_high_reso(int song, int melody, int sample);
void channel_direct_waves(int use_reverb);
void channel_white_noise();
void channel_antarctica();
void channel_dco(int mode);
void channel_granular();
//void channel_sampler();
void channel_reverb(int extended_buffers);
void channel_mi_clouds(int mode);

void pass_through();
void channel_just_water();

#define KARPLUS_STRONG_VOICES		2*9		//left & right, with 9-voice polyphony -> works with no reverb and 1200 samples buffers
//#define KARPLUS_STRONG_VOICES		2*8		//left & right, with 8-voice polyphony
//#define KARPLUS_STRONG_VOICES		2*7		//left & right, with 7-voice polyphony
//#define KARPLUS_STRONG_VOICES		2*6		//left & right, with 6-voice polyphony
//#define KARPLUS_STRONG_VOICES		2*3		//left & right, with 3-voice polyphony
//#define KARPLUS_STRONG_VOICES		2		//left & right, monophonic
#define WAVETABLE_VOICES			12

#define ENGINE_KS_RELATIVE_GAIN		600.0f
#define ENGINE_SS_RELATIVE_GAIN		1.0f

#define SUPERSAW_VELOCITY_SCALING	64.0f

void channel_karplus_strong();
void channel_supersaw();
void channel_wavetable();
void channel_vocoder();

#endif /* CHANNELS_DEF_H_ */
