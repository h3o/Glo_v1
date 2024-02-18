/*
 * IIR_filters.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Apr 18, 2016
 *      Author: mario
 *
 *  Based on code by Paul Kellett
 *  Source: http://www.musicdsp.org/showone.php?id=29
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of GNU GPLv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/
 *
 */

#ifndef IIR_FILTERS_H_
#define IIR_FILTERS_H_

#include <hw/signals.h>

class IIR_Filter
{
public:

	IIR_Filter(void);
	virtual ~IIR_Filter(void);
	virtual float process(float inputValue) { return 0.0f; };
	inline void calculateFeedbackAmount() { feedbackAmount = resonance + resonance/(1.0 - cutoff); }

	//inline void setCutoff(float newCutoff) { cutoff = newCutoff; calculateFeedbackAmount(); };
	inline void setCutoff(float newCutoff) { cutoff = newCutoff; feedbackAmount = resonance + resonance/(1.0 - cutoff); };
	inline void setCutoffKeepFeedback(float newCutoff) { cutoff = newCutoff; };

	inline void setCutoffAndLimits(float newCutoff)
	{
		if(newCutoff < 0.001)
		{
			newCutoff = 0.247836739 / 2;
		}
		cutoff = newCutoff;
		cutoff_min = cutoff / 2;
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

    static float iir_filter_multi_sum(float input, IIR_Filter **iir_array, int total_filters, float *mixing_volumes);
	static float iir_filter_multi_sum_w_noise(float input, IIR_Filter **iir_array, int total_filters, float *mixing_volumes, uint16_t noise, float noise_volume);
	static float iir_filter_multi_sum_w_noise_and_wind(float input, IIR_Filter **iir_array, int total_filters, float *mixing_volumes, uint16_t noise, float noise_volume, int wind_voices);

protected:

	float cutoff, resonance, feedbackAmount;
    float buf0,buf1,buf2,buf3;
    float buf4,buf5,buf6,buf7;
    //float buf8,buf9,buf10,buf11;
    float cutoff_min, cutoff_max;
};

//class IIR_Filter_LOW_PASS_2ND_ORDER :	public IIR_Filter { public:	float process(float inputValue); };
class IIR_Filter_LOW_PASS_4TH_ORDER :	public IIR_Filter { public:	float process(float inputValue); };
//class IIR_Filter_LOW_PASS_8TH_ORDER :	public IIR_Filter { public: float process(float inputValue); };
//class IIR_Filter_HIGH_PASS_2ND_ORDER :	public IIR_Filter { public:	float process(float inputValue); };
class IIR_Filter_HIGH_PASS_4TH_ORDER :	public IIR_Filter { public:	float process(float inputValue); };
//class IIR_Filter_HIGH_PASS_8TH_ORDER :	public IIR_Filter { public:	float process(float inputValue); };

#endif /* IIR_FILTERS_H_ */
