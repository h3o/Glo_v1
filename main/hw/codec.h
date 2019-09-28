/*
 * codec.h
 *
 *  Created on: Apr 6, 2016
 *      Author: mario
 *
 * Based on TLV320AIC3104 documentation:
 * http://www.ti.com/product/TLV320AIC3104/technicaldocuments
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

#include <stdint.h> //for uint16_t type

#ifndef __CODEC_H
#define __CODEC_H

#define I2S_AUDIOFREQ 50780	//real rate by APLL (when 48k requested) is 50781.250, rounding to nearest divisible by 4
//I (57890) I2S: APLL: Req RATE: 50780, real rate: 50781.250, BITS: 16, CLKM: 1, BCK_M: 8, MCLK: 13000000.000, SCLK: 1625000.000000, diva: 1, divb: 0

//#define I2S_AUDIOFREQ 48000	//Dekrispator on STM32F4
//I (1012) I2S: APLL: Req RATE: 48000, real rate: 46549.418, BITS: 16, CLKM: 1, BCK_M: 8, MCLK: 11916651.000, SCLK: 1489581.375000, diva: 1, divb: 0

//#define I2S_AUDIOFREQ 44100		//for some DCO modes
//#define I2S_AUDIOFREQ 42318	//real rate by APLL (when 44.1k requested)

//#define I2S_AUDIOFREQ 32000

//#define I2S_AUDIOFREQ 24000		//Dekrispator on ESP32
//I (1047) I2S: APLL: Req RATE: 24000, real rate: 23999.998, BITS: 16, CLKM: 1, BCK_M: 8, MCLK: 6143999.500, SCLK: 767999.937500, diva: 1, divb: 0

//#define I2S_AUDIOFREQ 22050	//Gecho default, Dekrispator on ESP32
//I (1011) I2S: APLL: Req RATE: 22050, real rate: 22050.008, BITS: 16, CLKM: 1, BCK_M: 8, MCLK: 5644802.000, SCLK: 705600.250000, diva: 1, divb: 0

//#define I2S_AUDIOFREQ 16000
//#define I2S_AUDIOFREQ I2S_AudioFreq_8k	//chiptunes only

#define SAMPLE_RATE_DEFAULT		I2S_AUDIOFREQ

#define SAMPLE_RATE_CLOUDS		44100
//#define SAMPLE_RATE_CLOUDS		40000
//#define SAMPLE_RATE_CLOUDS		39876
//#define SAMPLE_RATE_CLOUDS		32000
//#define SAMPLE_RATE_CLOUDS		24000

#define SAMPLE_RATE_LOW			24000	//Dekrispator on ESP32

//#define SAMPLE_RATE_VOICE_MENU	22050
#define SAMPLE_RATE_VOICE_MENU	48000	//sampling rate in which samples were recorded, otherwise the segments won't align well

#define CODEC_BUF_COUNT_DEFAULT		4
#define CODEC_BUF_LEN_DEFAULT		64
#define CODEC_BUF_COUNT_CLOUDS		4
#define CODEC_BUF_LEN_CLOUDS		512
#define CODEC_BUF_COUNT_DKR			4
#define CODEC_BUF_LEN_DKR			128

//#define AUDIOFREQ_DIV_CORRECTION 4

//#define CODEC_ENABLE_AGC		1		//1=enable, 0=disable
//#define CODEC_AGC_LEVEL		5		//target level in neg. dB, can be 5,8,12,17
//#define AGC_MAX_GAIN_STEP		6		//in dB
//#define AGC_LEVEL_MAX			48		//in dB

#define TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION sampleCounter==I2S_AUDIOFREQ /* *2 */ //-AUDIOFREQ_DIV_CORRECTION

#define TIMING_BY_SAMPLE_EVERY_1_MS		sampleCounter%(2*I2S_AUDIOFREQ/1000)//1000Hz
#define TIMING_BY_SAMPLE_EVERY_2_MS		sampleCounter%(2*I2S_AUDIOFREQ/500)	//200Hz
#define TIMING_BY_SAMPLE_EVERY_5_MS		sampleCounter%(2*I2S_AUDIOFREQ/200)	//200Hz
#define TIMING_BY_SAMPLE_EVERY_10_MS	sampleCounter%(2*I2S_AUDIOFREQ/100)	//100Hz
#define TIMING_BY_SAMPLE_EVERY_20_MS	sampleCounter%(2*I2S_AUDIOFREQ/50)	//50Hz
#define TIMING_BY_SAMPLE_EVERY_25_MS	sampleCounter%(2*I2S_AUDIOFREQ/40)	//40Hz
#define TIMING_BY_SAMPLE_EVERY_40_MS	sampleCounter%(2*I2S_AUDIOFREQ/25)	//25Hz
#define TIMING_BY_SAMPLE_EVERY_50_MS	sampleCounter%(2*I2S_AUDIOFREQ/20)	//20Hz
#define TIMING_BY_SAMPLE_EVERY_83_MS	sampleCounter%(2*I2S_AUDIOFREQ/12)	//12Hz
#define TIMING_BY_SAMPLE_EVERY_100_MS	sampleCounter%(2*I2S_AUDIOFREQ/10)	//10Hz
#define TIMING_BY_SAMPLE_EVERY_125_MS	sampleCounter%(2*I2S_AUDIOFREQ/8)	//8Hz
#define TIMING_BY_SAMPLE_EVERY_166_MS	sampleCounter%(2*I2S_AUDIOFREQ/6)	//6Hz
#define TIMING_BY_SAMPLE_EVERY_200_MS	sampleCounter%(2*I2S_AUDIOFREQ/5)	//5Hz
#define TIMING_BY_SAMPLE_EVERY_250_MS	sampleCounter%(2*I2S_AUDIOFREQ/4)	//4Hz
#define TIMING_BY_SAMPLE_EVERY_500_MS	sampleCounter%(2*I2S_AUDIOFREQ/2)	//2Hz

//#define CODEC_ANALOG_VOLUME_DEFAULT			0x18 //-12db
//#define CODEC_ANALOG_VOLUME_DEFAULT			0x0C //-6db
#define CODEC_ANALOG_VOLUME_MAX				0x00 //0db max value
#define CODEC_ANALOG_VOLUME_MIN				0x63 //99 -> -50db
#define CODEC_ANALOG_VOLUME_MUTE			0x76 //mute
#define ANALOG_VOLUME_STEP					12 //increase or decrease by 6dB

//#define CODEC_DIGITAL_VOLUME_DEFAULT		0x0C //-6db
//#define CODEC_DIGITAL_VOLUME_DEFAULT		0x12 //-9db
//#define CODEC_DIGITAL_VOLUME_DEFAULT		0x18 //-12db
//#define CODEC_DIGITAL_VOLUME_DEFAULT		0x30 //-24db
//#define CODEC_DIGITAL_VOLUME_DEFAULT		0x50 //-40db
//#define CODEC_DIGITAL_VOLUME_DEFAULT		0x70 //test

#define CODEC_DIGITAL_VOLUME_MAX			0x00 //0db max value
#define CODEC_DIGITAL_VOLUME_MIN			0x7F //127 -> -63.5db
#define CODEC_DIGITAL_VOLUME_MUTE			0x80 //mute

extern volatile uint32_t sampleCounter;
extern int add_beep;
extern int mclk_enabled;
extern int mics_off;

#define ADC_INPUT_OFF		0
#define ADC_INPUT_MIC		1
#define ADC_INPUT_LINE_IN	2

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

void codec_reset();
void codec_init();
int codec_select_input(int use_line_in);
void codec_set_analog_volume();
void codec_set_digital_volume();
void codec_set_mute(int status);
int codec_set_eq();
int codec_configure_AGC(int8_t enabled, int8_t max_gain, int8_t target_level);

void start_MCLK();
void stop_MCLK();
void set_sampling_rate(int new_rate);
void beep(int freq_div);

#ifdef __cplusplus
}
#endif

#endif /* __CODEC_H */
