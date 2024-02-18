/*
 * codec.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Apr 6, 2016
 *      Author: mario
 *
 * Based on TLV320AIC3104 documentation:
 * http://www.ti.com/product/TLV320AIC3104/technicaldocuments
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of GNU GPLv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/
 *
 */

#include <stdint.h> //for uint16_t type

#ifndef __CODEC_H
#define __CODEC_H

#define I2S_AUDIOFREQ 50780		//real rate by APLL (when 48k requested) is 50781.250, rounding to nearest divisible by 4
//I (57890) I2S: APLL: Req RATE: 50780, real rate: 50781.250, BITS: 16, CLKM: 1, BCK_M: 8, MCLK: 13000000.000, SCLK: 1625000.000000, diva: 1, divb: 0

//#define I2S_AUDIOFREQ 48000	//Dekrispator on STM32F4
//I (1012) I2S: APLL: Req RATE: 48000, real rate: 46549.418, BITS: 16, CLKM: 1, BCK_M: 8, MCLK: 11916651.000, SCLK: 1489581.375000, diva: 1, divb: 0

//#define I2S_AUDIOFREQ 44100	//for some DCO modes
//#define I2S_AUDIOFREQ 42318	//real rate by APLL (when 44.1k requested)

//#define I2S_AUDIOFREQ 32000

//#define I2S_AUDIOFREQ 24000	//Dekrispator on ESP32
//I (1047) I2S: APLL: Req RATE: 24000, real rate: 23999.998, BITS: 16, CLKM: 1, BCK_M: 8, MCLK: 6143999.500, SCLK: 767999.937500, diva: 1, divb: 0

//#define I2S_AUDIOFREQ 22050	//Gecho default, Dekrispator on ESP32
//I (1011) I2S: APLL: Req RATE: 22050, real rate: 22050.008, BITS: 16, CLKM: 1, BCK_M: 8, MCLK: 5644802.000, SCLK: 705600.250000, diva: 1, divb: 0

//#define I2S_AUDIOFREQ 16000
//#define I2S_AUDIOFREQ I2S_AudioFreq_8k	//chiptunes only

#define SAMPLE_RATE_DEFAULT			I2S_AUDIOFREQ
#define SAMPLING_RATES				5
extern const uint16_t sampling_rates[SAMPLING_RATES];
extern const uint8_t sampling_rates_indication[SAMPLING_RATES];
extern uint8_t sampling_rates_ptr;
extern uint16_t current_sampling_rate;

#define SAMPLE_RATE_CLOUDS					44100
#define SAMPLE_RATE_LOW						24000	//Dekrispator on ESP32
#define SAMPLE_RATE_LOOPER_SAMPLER			44100
#define SAMPLE_RATE_SAMPLED_DRUM_SEQUENCER	22050

#define CODEC_BUF_COUNT_DEFAULT		8
#define CODEC_BUF_LEN_DEFAULT		128
#define CODEC_BUF_COUNT_CLOUDS		4
#define CODEC_BUF_LEN_CLOUDS		128
#define CODEC_BUF_COUNT_DKR			8
#define CODEC_BUF_LEN_DKR			128

#define TIMING_BY_SAMPLE_1_SEC	sampleCounter==I2S_AUDIOFREQ
																	//freq		cycle at Fs=50780
#define TIMING_EVERY_1_MS		sampleCounter%(I2S_AUDIOFREQ/1000)	//1000Hz	0-50
#define TIMING_EVERY_2_MS		sampleCounter%(I2S_AUDIOFREQ/500)	//200Hz		0-101
#define TIMING_EVERY_4_MS		sampleCounter%(I2S_AUDIOFREQ/250)	//250Hz		0-203
#define TIMING_EVERY_5_MS		sampleCounter%(I2S_AUDIOFREQ/200)	//200Hz		0-253
#define TIMING_EVERY_10_MS		sampleCounter%(I2S_AUDIOFREQ/100)	//100Hz		0-507
#define TIMING_EVERY_20_MS		sampleCounter%(I2S_AUDIOFREQ/50)	//50Hz		0-1015
#define TIMING_EVERY_25_MS		sampleCounter%(I2S_AUDIOFREQ/40)	//40Hz		0-1269
#define TIMING_EVERY_40_MS		sampleCounter%(I2S_AUDIOFREQ/25)	//25Hz		0-2031
#define TIMING_EVERY_50_MS		sampleCounter%(I2S_AUDIOFREQ/20)	//20Hz		0-2539
#define TIMING_EVERY_83_MS		sampleCounter%(I2S_AUDIOFREQ/12)	//12Hz		0-4231
#define TIMING_EVERY_100_MS		sampleCounter%(I2S_AUDIOFREQ/10)	//10Hz		0-5078
#define TIMING_EVERY_125_MS		sampleCounter%(I2S_AUDIOFREQ/8)		//8Hz		0-6347
#define TIMING_EVERY_166_MS		sampleCounter%(I2S_AUDIOFREQ/6)		//6Hz		0-8463
#define TIMING_EVERY_200_MS		sampleCounter%(I2S_AUDIOFREQ/5)		//5Hz		0-10156
#define TIMING_EVERY_250_MS		sampleCounter%(I2S_AUDIOFREQ/4)		//4Hz		0-12695
#define TIMING_EVERY_500_MS		sampleCounter%(I2S_AUDIOFREQ/2)		//2Hz		0-25390

//#define CODEC_ANALOG_VOLUME_DEFAULT			0x18 //-12db
//#define CODEC_ANALOG_VOLUME_DEFAULT			0x0C //-6db
#define CODEC_ANALOG_VOLUME_MAX				0x00 //0db max value
#define CODEC_ANALOG_VOLUME_MIN				0x63 //99 -> -50db
#define CODEC_ANALOG_VOLUME_MUTE			0x76 //mute
#ifdef BOARD_WHALE
#define ANALOG_VOLUME_STEP					12 //increase or decrease by 6dB
#else
//#define ANALOG_VOLUME_STEP					1 //increase or decrease by 0.5dB
#define ANALOG_VOLUME_STEP					2 //increase or decrease by 1dB
#endif

//#define CODEC_DIGITAL_VOLUME_DEFAULT		0x0C //-6db
//#define CODEC_DIGITAL_VOLUME_DEFAULT		0x12 //-9db
//#define CODEC_DIGITAL_VOLUME_DEFAULT		0x18 //-12db
//#define CODEC_DIGITAL_VOLUME_DEFAULT		0x30 //-24db
//#define CODEC_DIGITAL_VOLUME_DEFAULT		0x50 //-40db
//#define CODEC_DIGITAL_VOLUME_DEFAULT		0x70 //test

#define CODEC_DIGITAL_VOLUME_MAX			0x00 //0db max value
#define CODEC_DIGITAL_VOLUME_MIN			0x7F //127 -> -63.5db
#define CODEC_DIGITAL_VOLUME_MUTE			0x80 //mute

#define AGC_LEVELS 9
extern const int8_t AGC_levels[AGC_LEVELS];
extern const uint8_t AGC_levels_indication[AGC_LEVELS];

extern volatile uint32_t sampleCounter;
extern int add_beep;
extern int mclk_enabled;
extern int mics_off;
extern uint8_t ADC_input_select;
extern uint8_t ADC_LR_enabled;

//line inputs are actually swapped in hardware, so R/L here in defined names is reversed
#define ADC_INPUT_OFF			0
#define ADC_INPUT_MIC			1
#define ADC_INPUT_LINE_IN		2
#define ADC_INPUT_BOTH_MIXED	3
//these were swapped around to match manual, as L/R is swapped somewhere along the audio pathway as well
#define ADC_INPUT_R_MIC_L_LINE	5//4	//right mic on, left line signal
#define ADC_INPUT_L_MIC_R_LINE	4//5	//left mic on, right line signal
#define ADC_INPUT_L_LINE		7//6	//mics off, only left line signal, right line possibly analysed for sync
#define ADC_INPUT_R_LINE		6//7	//mics off, only right line signal, left line possibly analysed for sync
#define ADC_INPUT_L_MIC_L_LINE	9//8	//left mic on, left line signal, right line possibly analysed for sync
#define ADC_INPUT_R_MIC_R_LINE	8//9	//right mic on, right line signal, left line possibly analysed for sync

#define ADC_INPUT_MODES			10
#define ADC_INPUT_DEFAULT		ADC_INPUT_MIC

#define ADC_LR_BOTH_ENABLED		0x03
#define ADC_LR_MUTE_L			0x02
#define ADC_LR_MUTE_R			0x01
#define ADC_LR_MUTE_BOTH		0x00
//line inputs are actually swapped in hardware, so R/L here in defined names is reversed
#define ADC_LR_ENABLED_L		(ADC_LR_enabled&0x02)
#define ADC_LR_ENABLED_R		(ADC_LR_enabled&0x01)

#define ADC_GAIN_MAX			0x60	//01100000 -> 48dB
#define ADC_GAIN_MIN			0x00	//00000000 -> 0dB
#define ADC_GAIN_STEP			2		//1dB
//#define ADC_GAIN_STEP			6		//3dB

#define MIC_BIAS_AVDD			0
#define MIC_BIAS_2_5V			1
#define MIC_BIAS_2V				2

extern int codec_analog_volume, codec_digital_volume, codec_volume_user;

#define EQ_BASS_MIN			-9
#define EQ_BASS_DEFAULT		0
#define EQ_BASS_MAX			9

#define EQ_TREBLE_MIN		-9
#define EQ_TREBLE_DEFAULT	0
#define EQ_TREBLE_MAX		9

#define EQ_BASS_STEP		3
#define EQ_TREBLE_STEP		3

extern int EQ_bass_setting, EQ_treble_setting;

#ifdef __cplusplus
 extern "C" {
#endif

void codec_init();
void codec_reset();
void codec_silence(uint32_t length);
int codec_adjust_ADC_gain(int direction);
int codec_set_mic_bias(int mic_bias);
int codec_select_input(uint8_t use_line_in);
int codec_mute_inputs(int status);
void codec_set_analog_volume();
void codec_set_digital_volume();
void codec_set_mute(int status);
int codec_set_eq();
int codec_configure_AGC(int8_t enabled, int8_t max_gain, int8_t target_level);

void start_MCLK();
void stop_MCLK();
void set_sampling_rate(int new_rate);
void beep(int freq_div);

int is_valid_sampling_rate(uint16_t rate);

#ifdef __cplusplus
}
#endif

#endif /* __CODEC_H */
