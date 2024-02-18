/*
 * v1_signals.c
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

#include "signals.h"

double b_noise = 19.1919191919191919191919191919191919191919;

float v1_PseudoRNG1a_next_float()
{
	b_noise = b_noise * b_noise;
	int i_noise = b_noise;
	b_noise = b_noise - i_noise;

	float b_noiseout;
	b_noiseout = b_noise - 0.5;

	b_noise = b_noise + 19;

	return b_noiseout;
}

float v1_PseudoRNG1b_next_float()
{
	double b_noiselast = b_noise;
	b_noise = b_noise + 19;
	b_noise = b_noise * b_noise;
	b_noise = (b_noise + b_noiselast) * 0.5;
	b_noise = b_noise - (int)b_noise;

	return b_noise - 0.5;
}

uint32_t PseudoRNG2_next_int32()
{
	/*
	 * http://musicdsp.org/showone.php?id=59
	 *
		Type : Linear Congruential, 32bit
		References : Hal Chamberlain, "Musical Applications of Microprocessors" (Posted by Phil Burk)
		Notes :
		This can be used to generate random numeric sequences or to synthesise a white noise audio signal.
		If you only use some of the bits, use the most significant bits by shifting right.
		Do not just mask off the low bits.
	*/

	//Calculate pseudo-random 32 bit number based on linear congruential method.

	//Change this for different random sequences.
	static unsigned long randSeed = 22222;
	randSeed = (randSeed * 196314165) + 907633515;
	return randSeed;
}
