/*
 * FilterSimpleIIR02.h
 *
 *  Created on: Apr 18, 2016
 *      Author: mayo
 *
 *  Based on code by Paul Kellett
 *  Source: http://www.musicdsp.org/showone.php?id=29
 *
 */

#ifndef V1_FILTERSIMPLEIIR02_H_
#define V1_FILTERSIMPLEIIR02_H_

class v1_FilterSimpleIIR02
{
public:
	enum FilterMode {
		FILTER_MODE_LOWPASS = 0,
		FILTER_MODE_HIGHPASS,
		FILTER_MODE_BANDPASS,
		kNumFilterModes
	};

	v1_FilterSimpleIIR02(void);
	float process(float inputValue);
	//inline void setCutoff(float newCutoff) { cutoff = newCutoff; calculateFeedbackAmount(); };
	inline void setCutoff(float newCutoff) { cutoff = newCutoff; feedbackAmount = resonance + resonance/(1.0 - cutoff); };
	inline void setCutoff2(float newCutoff) { };
	inline void setCutoffKeepFeedback(float newCutoff) { cutoff = newCutoff; };

	inline void setCutoffAndLimits(float newCutoff) {
		cutoff = newCutoff;
		cutoff_min = cutoff/2;
		cutoff_max = cutoff * 2;
		calculateFeedbackAmount();
	};

	inline void driftCutoff(float drift) {
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
	//inline void setFilterMode(FilterMode newMode) { mode = newMode; }

	inline void resetFilterBuffers() { buf0 = 0; buf1 = 0; buf2 = 0; buf3 = 0; };

	/*static*/ float iir_filter_multi_sum(float input, v1_FilterSimpleIIR02 *iir_array, int total_filters, float *mixing_volumes);

private:
    float cutoff;
    float resonance;
    //FilterMode mode;
    float feedbackAmount;
    inline void calculateFeedbackAmount() { feedbackAmount = resonance + resonance/(1.0 - cutoff); }
    float buf0,buf1,buf2,buf3;

    float cutoff_min, cutoff_max;
};

/*
opti:
buf0 += cutoff * (inputValue - buf0 + feedbackAmount * (buf0 - buf1)); //with resonance
feedbackAmount = resonance + resonance/(1.0 - cutoff);

cutoff_1 = 1.0 - cutoff //const, not needed anymore
reso_cutoff_1 = resonance + resonance/cutoff_1 //const

buf0 += cutoff * (inputValue - buf0 + (reso_cutoff_1) * (buf0 - buf1)); //with resonance

==>

reso_cutoff_1 = resonance + resonance/(1.0 - cutoff) == feedback amount !!!

*/

#endif /* V1_FILTERSIMPLEIIR02_H_ */
