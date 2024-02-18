/*
 * v1_FilterSimpleIIR02.h
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

#ifndef V1_FILTERSIMPLEIIR02_H_
#define V1_FILTERSIMPLEIIR02_H_

class v1_FilterSimpleIIR02
{

public:

	enum FilterMode
	{
		FILTER_MODE_LOWPASS = 0,
		FILTER_MODE_HIGHPASS,
		FILTER_MODE_BANDPASS,
		kNumFilterModes
	};

	v1_FilterSimpleIIR02(void);
	float process(float inputValue);
	inline void setCutoff(float newCutoff) { cutoff = newCutoff; feedbackAmount = resonance + resonance/(1.0 - cutoff); };
	inline void setCutoff2(float newCutoff) { };
	inline void setCutoffKeepFeedback(float newCutoff) { cutoff = newCutoff; };

	inline void setCutoffAndLimits(float newCutoff)
	{
		cutoff = newCutoff;
		cutoff_min = cutoff/2;
		cutoff_max = cutoff * 2;
		calculateFeedbackAmount();
	};

	inline void driftCutoff(float drift)
	{
		cutoff += drift;
		if(cutoff<cutoff_min)
		{
			cutoff = cutoff_min;
		}
		else if(cutoff>cutoff_max)
		{
			cutoff = cutoff_max;
		}
		calculateFeedbackAmount();
	};

	inline void setResonance(float newResonance) { resonance = newResonance; calculateFeedbackAmount(); };
	inline void setResonanceKeepFeedback(float newResonance) { resonance = newResonance; };
	inline void setResonanceAndFeedback(float newResonance, float newFeedback) { resonance = newResonance; feedbackAmount = newFeedback; };

	inline void resetFilterBuffers() { buf0 = 0; buf1 = 0; buf2 = 0; buf3 = 0; };
	inline void disturbFilterBuffers() { buf0 /= 2; buf1 = 0; };

	float iir_filter_multi_sum(float input, v1_FilterSimpleIIR02 *iir_array, int total_filters, float *mixing_volumes);

private:

    float cutoff;
    float resonance;
    float feedbackAmount;
    inline void calculateFeedbackAmount() { feedbackAmount = resonance + resonance/(1.0 - cutoff); }
    float buf0,buf1,buf2,buf3;
    float cutoff_min, cutoff_max;
};

#endif /* V1_FILTERSIMPLEIIR02_H_ */
