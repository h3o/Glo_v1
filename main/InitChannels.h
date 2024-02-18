/*
 * InitChannels.h
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

#ifndef INIT_CHANNELS_H_
#define INIT_CHANNELS_H_

#include <stdint.h> //for uint16_t type etc.
#include <hw/ui.h>
#include <dsp/reverb.h>
#include "glo_config.h" //for binaural_profile_t

//#define SWITCH_I2C_SPEED_MODES

//time-critical stuff
#define SHIFT_MELODY_NOTE_TIMING_BY_SAMPLE 5
#define SHIFT_CHORD_TIMING_BY_SAMPLE 3
#define SHIFT_ARP_TIMING_BY_SAMPLE 2

//--------------------------------------------------------------

extern int hidden_menu;
extern uint64_t channel;

//extern char *melody_str;
//extern char *progression_str;
//extern int progression_str_length;

extern int mixed_sample_buffer_ptr_L, mixed_sample_buffer_ptr_R;
//extern int mixed_sample_buffer_ptr_L2, mixed_sample_buffer_ptr_R2;
//extern int mixed_sample_buffer_ptr_L3, mixed_sample_buffer_ptr_R3;
//extern int mixed_sample_buffer_ptr_L4, mixed_sample_buffer_ptr_R4;

extern int16_t *mixed_sample_buffer;
extern int MIXED_SAMPLE_BUFFER_LENGTH;
extern unsigned int mixed_sample_buffer_adr;

extern int /*base_freq,*/ param_i;

//extern int n_bytes, ADC_result;
//extern uint32_t ADC_sample;//, ADC_sample0;
//extern int ADC_sample_valid;
//extern uint32_t sample32;
extern float new_mixing_vol;

extern int autocorrelation_rec_sample, autocorrelation_result_rdy, autocorrelation_result[4];
extern uint16_t *autocorrelation_buffer;

extern uint16_t t_TIMING_BY_SAMPLE_EVERY_250_MS; //optimization of timing counters
extern uint16_t t_TIMING_BY_SAMPLE_EVERY_125_MS; //optimization of timing counters

extern int wind_voices;

//extern float OpAmp_ADC12_signal_conversion_factor;

void channel_init(int bg_sample, int song_id, int melody_id, int filters_type, float resonance, int use_mclk, int set_wind_voices, int set_active_filter_pairs, int reverb_ext, int use_reverb);
void channel_deinit();

//void voice_settings_menu();

#endif /* INIT_CHANNELS_H_ */
