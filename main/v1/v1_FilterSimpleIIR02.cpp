#include "v1_FilterSimpleIIR02.h"
#include "legacy.h"

IRAM_ATTR v1_FilterSimpleIIR02::v1_FilterSimpleIIR02(void)
{
	cutoff = 0.99;
	resonance = 0.0;
	//mode = FILTER_MODE_LOWPASS;
	buf0 = 0.0;
	buf1 = 0.0;
	buf2 = 0.0;
	buf3 = 0.0;
	calculateFeedbackAmount();
}

IRAM_ATTR float v1_FilterSimpleIIR02::process(float inputValue) {
    //buf0 += cutoff * (inputValue - buf0);
    buf0 += cutoff * (inputValue - buf0 + feedbackAmount * (buf0 - buf1)); //with resonance
    buf1 += cutoff * (buf0 - buf1);

    //grade 2 only:
    /*
    switch (mode) {
        case FILTER_MODE_LOWPASS:
            return buf1;
        case FILTER_MODE_HIGHPASS:
            return inputValue - buf0;
        case FILTER_MODE_BANDPASS:
            return buf0 - buf1;
        default:
            return 0.0;
    }
    */
    //grade 4:
    buf2 += cutoff * (buf1 - buf2);
    buf3 += cutoff * (buf2 - buf3);
    //switch (mode) {
    //    case FILTER_MODE_LOWPASS:
            return buf3;
    //    case FILTER_MODE_HIGHPASS:
    //        return inputValue - buf3;
    //    case FILTER_MODE_BANDPASS:
    //        return buf0 - buf3;
    //    default:
    //        return 0.0;
    //}
}


IRAM_ATTR float v1_FilterSimpleIIR02::iir_filter_multi_sum(float input, v1_FilterSimpleIIR02 *iir_array, int total_filters, float *mixing_volumes)
{
	float output = 0.0;
	for(int f=0;f<total_filters;f++) {
		output += iir_array[f].process(input)*mixing_volumes[f];
	}
	return output/(float)total_filters;
}


//    FilterSimpleIIR02::~FilterSimpleIIR02(void) {
//
//    };
