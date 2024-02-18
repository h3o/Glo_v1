/*
 * v1_FilterSimpleIIR02.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Apr 18, 2016
 *      Author: mario
 *
 *    Based on: code by Paul Kellett
 *      Source: http://www.musicdsp.org/showone.php?id=29
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of GNU GPLv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/
 *
 */

#include "v1_FilterSimpleIIR02.h"
#include "legacy.h"

IRAM_ATTR v1_FilterSimpleIIR02::v1_FilterSimpleIIR02(void)
{
	cutoff = 0.99;
	resonance = 0.0;
	buf0 = 0.0;
	buf1 = 0.0;
	buf2 = 0.0;
	buf3 = 0.0;
	calculateFeedbackAmount();
}

IRAM_ATTR float v1_FilterSimpleIIR02::process(float inputValue)
{
    buf0 += cutoff * (inputValue - buf0 + feedbackAmount * (buf0 - buf1)); //with resonance
    buf1 += cutoff * (buf0 - buf1);
    buf2 += cutoff * (buf1 - buf2);
    buf3 += cutoff * (buf2 - buf3);

    return buf3;
}

IRAM_ATTR float v1_FilterSimpleIIR02::iir_filter_multi_sum(float input, v1_FilterSimpleIIR02 *iir_array, int total_filters, float *mixing_volumes)
{
	float output = 0.0;

	for(int f=0;f<total_filters;f++) {
		output += iir_array[f].process(input)*mixing_volumes[f];
	}

	return output/(float)total_filters;
}
