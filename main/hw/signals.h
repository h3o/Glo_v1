/*
 * signals.h
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

#ifndef SIGNALS_H_
#define SIGNALS_H_

#include <stdbool.h>
#include <stdint.h> //for uint16_t type
#include "codec.h"

#define ECHO_BUFFER_STATIC //needs to be dynamic to support other engines, but that did not work due to ESP32 malloc limitation (largest free block not large enough)

//===== BEGIN NOISE SEED BLOCK ===========================================

//#define NOISE_SEED 19.0000000000000000000000000000000000000001
#define NOISE_SEED 19.1919191919191919191919191919191919191919
//#define NOISE_SEED 13.1313131313131313131313131313131313131313
//#define NOISE_SEED 19.7228263362716688952313412753412622364256 //k.l.
//#define NOISE_SEED 19.8381511698353141312312412453512642346256 //d.r.
//#define NOISE_SEED 19.6729592785901037505892041201203552160120 //a.f.

//===== END NOISE SEED BLOCK =============================================

extern uint32_t sample32;
//extern int n_bytes, ADC_result;
extern uint32_t ADC_sample, ADC_sample0;
#define ADC_SAMPLE_BUFFER 100 //in 4 byte samples (16bit stereo)
extern uint32_t ADC_sampleA[];
extern int ADC_sample_ptr;
//extern int ADC_sample_valid = 0;

extern volatile int16_t sample_i16;
extern volatile uint16_t sample_u16;

extern float sample_f[2],sample_mix,/*volume2,*/sample_lpf[4],sample_hpf[4];

extern int autocorrelation_rec_sample;
#define AUTOCORRELATION_REC_SAMPLES 512 //256 //1024
extern uint16_t *autocorrelation_buffer;
extern int autocorrelation_result[4], autocorrelation_result_rdy;
//extern int autocorrelation_buffer_full;

#define OpAmp_ADC12_signal_conversion_factor OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT

extern float ADC_LPF_ALPHA, ADC_HPF_ALPHA;

extern uint32_t random_value;

#ifdef __cplusplus
 extern "C" {
#endif

void new_random_value();
int fill_with_random_value(char *buffer);
float PseudoRNG1a_next_float_new();
float PseudoRNG1a_next_float_new01();

//-------------------------- sample and echo buffers -------------------

#define ECHO_DYNAMIC_LOOP_STEPS 15
//the length is now controlled dynamically (as defined in signals.c)

#define ECHO_BUFFER_LENGTH (I2S_AUDIOFREQ * 3 / 2)	//76170 samples, 152340 bytes, 1.5 sec
//#define ECHO_BUFFER_LENGTH	90000 //max that fits = 76392

#define ECHO_BUFFER_LENGTH_DEFAULT 				ECHO_BUFFER_LENGTH
//#define ECHO_BUFFER_LENGTH_DEFAULT 				(I2S_AUDIOFREQ * 3 / 2)	//1.5 sec, longest delay currently used (default)

#define ECHO_DYNAMIC_LOOP_LENGTH_DEFAULT_STEP 	0	//refers to echo_dynamic_loop_steps in signals.c
#define ECHO_DYNAMIC_LOOP_LENGTH_ECHO_OFF	 	(ECHO_DYNAMIC_LOOP_STEPS-1)	//corresponds to what is in echo_dynamic_loop_steps structure as 0

#ifdef ECHO_BUFFER_STATIC
extern int16_t echo_buffer[];	//the buffer is allocated statically
#else
extern int16_t *echo_buffer;	//the buffer is allocated dynamically
#endif

//extern int echo_buffer_ptr0, echo_buffer_ptr;		//pointers for echo buffer
extern int echo_dynamic_loop_length;
extern int echo_dynamic_loop_length0;//, echo_length_updated;
extern int echo_skip_samples, echo_skip_samples_from;
extern int echo_dynamic_loop_current_step;
extern float ECHO_MIXING_GAIN_MUL, ECHO_MIXING_GAIN_DIV;
extern float echo_mix_f;
extern const uint8_t echo_dynamic_loop_indicator[ECHO_DYNAMIC_LOOP_STEPS];
extern const int echo_dynamic_loop_steps[ECHO_DYNAMIC_LOOP_STEPS];
extern bool PROG_add_echo;
extern int echo_buffer_ptr0, echo_buffer_ptr;

#define ARP_VOLUME_MULTIPLIER_DEFAULT	3.333f
#define ARP_VOLUME_MULTIPLIER_MANUAL	5.0f
#define FIXED_ARP_LEVEL_STEP 			25
#define FIXED_ARP_LEVEL_MAX 			100
extern int fixed_arp_level;

//extern int16_t reverb_buffer[];	//the buffer is allocated statically
extern int16_t *reverb_buffer;	//the buffer is allocated dynamically

extern int reverb_buffer_ptr0, reverb_buffer_ptr;		//pointers for reverb buffer
extern int reverb_dynamic_loop_length;
extern float REVERB_MIXING_GAIN_MUL, REVERB_MIXING_GAIN_DIV;
extern float reverb_mix_f;

#define REVERB_BUFFERS_EXT	2
extern int16_t *reverb_buffer_ext[REVERB_BUFFERS_EXT];	//extra reverb buffers for higher and lower octaves
extern int reverb_buffer_ptr0_ext[REVERB_BUFFERS_EXT], reverb_buffer_ptr_ext[REVERB_BUFFERS_EXT];	//pointers for reverb buffer
extern int reverb_dynamic_loop_length_ext[REVERB_BUFFERS_EXT];
extern float REVERB_MIXING_GAIN_MUL_EXT, REVERB_MIXING_GAIN_DIV_EXT;

extern float SAMPLE_VOLUME;
#define SAMPLE_VOLUME_DEFAULT 9.0f
#define SAMPLE_VOLUME_REVERB 2.0f

//this controls limits for global volume
//#define SAMPLE_VOLUME_MAX 24.0f
//#define SAMPLE_VOLUME_MIN 1.0f

//#define OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT	0.00150f //was 0.005 for rgb-black-mini
#define OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT	0.00250f //was 0.005 for rgb-black-mini
//#define OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT	0.01000f
#define OPAMP_ADC12_CONVERSION_FACTOR_MIN		0.00010f
#define OPAMP_ADC12_CONVERSION_FACTOR_MAX		0.02000f
#define OPAMP_ADC12_CONVERSION_FACTOR_STEP		0.00010f

#define COMPUTED_SAMPLE_MIXING_LIMIT_UPPER		 32000.0f //was in mini black
#define COMPUTED_SAMPLE_MIXING_LIMIT_LOWER		-32000.0f //was in mini black
#define COMPUTED_SAMPLE_MIXING_LIMIT_UPPER_INT	 32000
#define COMPUTED_SAMPLE_MIXING_LIMIT_LOWER_INT	-32000

//#define COMPUTED_SAMPLE_MIXING_LIMIT_UPPER	16000.0f //32000.0f was in mini black
//#define COMPUTED_SAMPLE_MIXING_LIMIT_LOWER	-16000.0f //-32000.0f was in mini black
//#define COMPUTED_SAMPLE_MIXING_LIMIT_UPPER	5000.0f
//#define COMPUTED_SAMPLE_MIXING_LIMIT_LOWER	-5000.0f

#define DYNAMIC_LIMITER_THRESHOLD			20000.0f
#define DYNAMIC_LIMITER_STEPDOWN			0.1f
#define DYNAMIC_LIMITER_RECOVER				0.05f

#ifdef BOARD_WHALE
#define DYNAMIC_LIMITER_THRESHOLD_ECHO		10000.0f
#else
#define DYNAMIC_LIMITER_THRESHOLD_ECHO		16000.0f
#define DYNAMIC_LIMITER_THRESHOLD_ECHO_DCO	DYNAMIC_LIMITER_THRESHOLD_ECHO //at 24000.0f it is glitching
#define DYNAMIC_LIMITER_THRESHOLD_ECHO_V1	8000.0f
#endif

#define DYNAMIC_LIMITER_COEFF_MUL		0.99f
#define DYNAMIC_LIMITER_COEFF_MUL_DCO	0.9f	//need faster limiter for DCO channel
#define DYNAMIC_LIMITER_COEFF_MUL_V1	0.996f	//slower limiter for channel #1 legacy channel
#define DYNAMIC_LIMITER_COEFF_DEFAULT 	1.0f
extern float limiter_coeff;

//#define PREAMP_BOOST 					9		//multiplier for signal from ADC1/ADC2 (microphones and piezo pickups input)
#define MAIN_VOLUME						64.0f	//multiplier for samples mixing volume (float to int)

#define UNFILTERED_SIGNAL_VOLUME		64.0f	//for mixing white noise in
#define FLASH_SAMPLES_MIXING_VOLUME		0.8f	//for mixing the samples from flash
#define COMPUTED_SAMPLES_MIXING_VOLUME	2.0f	//used to adjust volume when converting from float to int

//---------------------------------------------------------------------------

void init_echo_buffer();
void deinit_echo_buffer();

int16_t add_reverb(int16_t sample);
//int16_t add_reverb_ext(int16_t sample);
int16_t add_reverb_ext_all3(int16_t sample);
int16_t add_echo(int16_t sample);
int16_t reversible_echo(int16_t sample, int direction);

#ifdef __cplusplus
}
#endif

#endif /* SIGNALS_H_ */
