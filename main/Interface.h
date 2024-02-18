/*
 * Interface.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Apr 27, 2016
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

#ifndef INTERFACE_H_
#define INTERFACE_H_

#ifdef __cplusplus

#include "dsp/Filters.h"
#include "notes.h"

extern unsigned long seconds;

extern int progressive_rhythm_factor;
#define PROGRESSIVE_RHYTHM_BOOST_RATIO 2
#define PROGRESSIVE_RHYTHM_RAMP_COEF 0.999f

extern float noise_volume, noise_volume_max, noise_boost_by_sensor, mixed_sample_volume;
extern int special_effect, selected_song, selected_melody;

#define FILTERS_TYPE_NO_FILTERS 0x00
#define FILTERS_TYPE_LOW_PASS 0x01
#define FILTERS_TYPE_HIGH_PASS 0x02
#define FILTERS_ORDER_2 0x10
#define FILTERS_ORDER_4 0x20
#define FILTERS_ORDER_8 0x40
#define DEFAULT_FILTERS_TYPE_AND_ORDER (FILTERS_TYPE_LOW_PASS + FILTERS_ORDER_4)
extern int FILTERS_TYPE_AND_ORDER;

extern int ACTIVE_FILTERS_PAIRS;
#define ACTIVE_FILTER_PAIRS_ALL (FILTERS/2)
#define ACTIVE_FILTER_PAIRS_ONE_LESS (ACTIVE_FILTER_PAIRS_ALL-2)
#define ACTIVE_FILTER_PAIRS_TWO_LESS (ACTIVE_FILTER_PAIRS_ALL-4)

extern int PROGRESS_UPDATE_FILTERS_RATE;
//extern int DEFAULT_ARPEGGIATOR_FILTER_PAIR;

extern int SHIFT_CHORD_INTERVAL;

extern Filters *fil;

extern int direct_update_filters_id[FILTERS];
extern float direct_update_filters_freq[FILTERS];

extern float wind_cutoff[WIND_FILTERS];
extern float wind_cutoff_limit[2];

extern int arpeggiator_loop;

extern int use_alt_settings;
extern int use_binaural;

extern "C" {

void filters_init(float resonance);
void program_settings_reset();

void set_tuning(float c_l,float c_r,float m_l,float m_r,float a_l,float a_r);
void set_tuning_all(float freq);

void voice_say(const char *segment);

void reload_song_and_melody(char *song, char *melody);
void randomize_song_and_melody();

void transpose_song(int direction);

}

#else

//this needs to be accessible from ui.c with c-linkage
void set_tuning(float c_l,float c_r,float m_l,float m_r,float a_l,float a_r);
void set_tuning_all(float freq);

void voice_say(const char *segment);

void reload_song_and_melody(char *song, char *melody);
void randomize_song_and_melody();

void transpose_song(int direction);

#endif

#endif /* INTERFACE_H_ */
