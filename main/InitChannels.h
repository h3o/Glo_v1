/*
 * InitChannels.h
 *
 *  Created on: Nov 26, 2016
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

#ifndef INIT_CHANNELS_H_
#define INIT_CHANNELS_H_

#include <stdint.h> //for uint16_t type etc.
#include <hw/ui.h>
#include "glo_config.h" //for binaural_profile_t

//#define SWITCH_I2C_SPEED_MODES

//time-critical stuff
#define SHIFT_MELODY_NOTE_TIMING_BY_SAMPLE 5
#define SHIFT_CHORD_TIMING_BY_SAMPLE 3
#define SHIFT_ARP_TIMING_BY_SAMPLE 2

//--------------------------------------------------------------

	/*
	//v1: short reverb (higher freqs)
	#define BIT_CRUSHER_REVERB_MIN 			(REVERB_BUFFER_LENGTH/50)		//48 samples @48k
	#define BIT_CRUSHER_REVERB_MAX 			(REVERB_BUFFER_LENGTH/4)		//600 samples @48k
	*/

	/*
	//v2: longer reverb (bass)
	#define BIT_CRUSHER_REVERB_MIN 			(REVERB_BUFFER_LENGTH/5)		//480 samples @48k, 507 @APLL (507.8)
	#define BIT_CRUSHER_REVERB_MAX 			REVERB_BUFFER_LENGTH			//2400 samples @48k, 2540 @APLL
	//#define BIT_CRUSHER_REVERB_DEFAULT 	(BIT_CRUSHER_REVERB_MIN*10)		//240 samples @48k
	//#define BIT_CRUSHER_REVERB_DEFAULT 	(BIT_CRUSHER_REVERB_MIN*20)		//480 samples @48k
	#define BIT_CRUSHER_REVERB_DEFAULT 		(BIT_CRUSHER_REVERB_MIN*2)
	*/

	//v3: wide range
	#define BIT_CRUSHER_REVERB_MIN			(REVERB_BUFFER_LENGTH/50)		//48 samples @48k, 507 @APLL (507.8)
	#define BIT_CRUSHER_REVERB_MAX			REVERB_BUFFER_LENGTH			//2400 samples @48k, 2540 @APLL
	#define BIT_CRUSHER_REVERB_DEFAULT		(BIT_CRUSHER_REVERB_MAX/3)

	//wraps at:
	//BIT_CRUSHER_REVERB=2539,len=2539
	//BIT_CRUSHER_REVERB=50,len=50

extern int bit_crusher_reverb;

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

void channel_init(int bg_sample, int song_id, int melody_id, int filters_type, float resonance, int use_mclk, int set_wind_voices, int set_active_filter_pairs);
void channel_deinit();

//void voice_settings_menu();

#endif /* INIT_CHANNELS_H_ */
