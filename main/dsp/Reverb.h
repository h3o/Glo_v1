/*
 * Reverb.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 16 Jul 2018
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

#ifndef EXTENSIONS_REVERB_H_
#define EXTENSIONS_REVERB_H_

#define REVERB_POLYPHONIC
//#define REVERB_OCTAVE_UP_DOWN

#ifdef REVERB_OCTAVE_UP_DOWN

	//#define REVERB_BUFFER_LENGTH (I2S_AUDIOFREQ / 24)	//longest reverb currently used (2000 samples @ 48k), 2115.833 @APLL
	#define REVERB_BUFFER_LENGTH (I2S_AUDIOFREQ / 20)	//longest reverb currently used (2400 samples @ 48k), 2539 @APLL

	#define REVERB_MIXING_GAIN_MUL_DEFAULT		9	//amount of signal to feed back to reverb loop, expressed as a fragment
	#define REVERB_MIXING_GAIN_DIV_DEFAULT		10	//e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in
    #define REVERB_MIXING_GAIN_MUL_EXT_DEFAULT	3
    #define REVERB_MIXING_GAIN_DIV_EXT_DEFAULT	4

	#define REVERB_EXT_SAMPLE_INPUT_COEFF		0.5f
	#define REVERB_EXT_SAMPLE_OUTPUT_COEFF		0.5f

	//v3: wide range
	#define BIT_CRUSHER_REVERB_MIN				(REVERB_BUFFER_LENGTH/50)		//48 samples @48k, 507 @APLL (507.8)
	#define BIT_CRUSHER_REVERB_MAX				REVERB_BUFFER_LENGTH			//2400 samples @48k, 2540 @APLL
	#define BIT_CRUSHER_REVERB_DEFAULT			(BIT_CRUSHER_REVERB_MAX/3)

	#define BIT_CRUSHER_REVERB_DEFAULT_EXT(x)	(x==0?BIT_CRUSHER_REVERB_DEFAULT*2:BIT_CRUSHER_REVERB_DEFAULT/2)
	#define BIT_CRUSHER_REVERB_MAX_EXT(x)		(x==0?BIT_CRUSHER_REVERB_MAX*2:BIT_CRUSHER_REVERB_MAX/2)
	#define BIT_CRUSHER_REVERB_MIN_EXT(x)		(x==0?BIT_CRUSHER_REVERB_MIN*2:BIT_CRUSHER_REVERB_MIN/2)
	#define BIT_CRUSHER_REVERB_CALC_EXT(x, y)	(x==0?y*2:y/2)

	//wraps at:
	//BIT_CRUSHER_REVERB=2539,len=2539
	//BIT_CRUSHER_REVERB=50,len=50
#endif

#ifdef REVERB_POLYPHONIC

	#define REVERB_BUFFER_LENGTH (I2S_AUDIOFREQ / 10)	//longest reverb currently used (4800 samples @ 48k)


	#define REVERB_MIXING_GAIN_MUL_DEFAULT		19	//amount of signal to feed back to reverb loop, expressed as a fragment
	#define REVERB_MIXING_GAIN_DIV_DEFAULT		20	//e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in
    #define REVERB_MIXING_GAIN_MUL_EXT_DEFAULT	19
    #define REVERB_MIXING_GAIN_DIV_EXT_DEFAULT	20

	#define REVERB_EXT_SAMPLE_INPUT_COEFF		0.3f
	#define REVERB_EXT_SAMPLE_OUTPUT_COEFF		1.0f

	#define BIT_CRUSHER_REVERB_MIN				5
	#define BIT_CRUSHER_REVERB_MAX				REVERB_BUFFER_LENGTH			//2400 samples @48k, 2540 @APLL
	#define BIT_CRUSHER_REVERB_DEFAULT			(BIT_CRUSHER_REVERB_MAX/3)

	#define BIT_CRUSHER_REVERB_DEFAULT_EXT(x)	(BIT_CRUSHER_REVERB_DEFAULT)
	#define BIT_CRUSHER_REVERB_MAX_EXT(x)		(BIT_CRUSHER_REVERB_MAX)
	#define BIT_CRUSHER_REVERB_MIN_EXT(x)		(BIT_CRUSHER_REVERB_MIN)
	#define BIT_CRUSHER_REVERB_CALC_EXT(x, y)	(y)

#endif

extern int bit_crusher_reverb;

void decaying_reverb(int extended_buffers);

#endif /* EXTENSIONS_REVERB_H_ */
