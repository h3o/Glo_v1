/*
 * IIR_filters.cpp
 *
 *  Created on: Apr 18, 2016
 *      Author: mario
 *
 *  Based on code by Paul Kellett
 *  Source: http://www.musicdsp.org/showone.php?id=29
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

#include "IIR_filters.h"
#include "dsp/Filters.h"
//#include "stdio.h"
#include "esp_attr.h"

//#define ICACHE_FLASH_ATTR __attribute__((section(".irom0.text")))

IIR_Filter::IIR_Filter(void)
{
	cutoff = 0.99;
	resonance = 0.0;
	buf0 = 0.0;
	buf1 = 0.0;
	buf2 = 0.0;
	buf3 = 0.0;
	buf4 = 0.0;
	buf5 = 0.0;
	buf6 = 0.0;
	buf7 = 0.0;
	//buf8 = 0.0;
	//buf9 = 0.0;
	//buf10 = 0.0;
	//buf11 = 0.0;
	calculateFeedbackAmount();
}

IIR_Filter::~IIR_Filter(void)
{

}

/*
IIR_Filter_LOW_PASS_4TH_ORDER::~IIR_Filter(void)
{

}

IIR_Filter_HIGH_PASS_4TH_ORDER::~IIR_Filter(void)
{

}
*/

float IIR_Filter_LOW_PASS_4TH_ORDER::process(float inputValue) {
    //buf0 += cutoff * (inputValue - buf0);
    buf0 += cutoff * (inputValue - buf0 + feedbackAmount * (buf0 - buf1)); //with resonance
    buf1 += cutoff * (buf0 - buf1);

    //2nd order outputs:

    //return buf1;					//low-pass
    //return inputValue - buf0;		//high-pass
    //return buf0 - buf1;			//band-pass

    buf2 += cutoff * (buf1 - buf2);
    buf3 += cutoff * (buf2 - buf3);

    //4th order outputs:

    return buf3; 					//low-pass (default)
    //return -buf3; 					//low-pass (phase inverted)
}

float IIR_Filter_HIGH_PASS_4TH_ORDER::process(float inputValue) {
    buf0 += cutoff * (inputValue - buf0 + feedbackAmount * (buf0 - buf1)); //with resonance
    buf1 += cutoff * (buf0 - buf1);
    buf2 += cutoff * (buf1 - buf2);
    buf3 += cutoff * (buf2 - buf3);

    return inputValue - buf3;		//high-pass
    //return buf3 - inputValue;		//high-pass (phase inverted)
}

/*
float IIR_Filter_LOW_PASS_2ND_ORDER::process(float inputValue) {
    buf0 += cutoff * (inputValue - buf0 + feedbackAmount * (buf0 - buf1)); //with resonance
    buf1 += cutoff * (buf0 - buf1);
    return buf1; //2nd order only
}

float IIR_Filter_HIGH_PASS_2ND_ORDER::process(float inputValue) {
    buf0 += cutoff * (inputValue - buf0 + feedbackAmount * (buf0 - buf1)); //with resonance
    buf1 += cutoff * (buf0 - buf1);
    return inputValue - buf1; //2nd order only
}

float IIR_Filter_LOW_PASS_8TH_ORDER::process(float inputValue) {
    buf0 += cutoff * (inputValue - buf0 + feedbackAmount * (buf0 - buf1)); //with resonance
    buf1 += cutoff * (buf0 - buf1);
    buf2 += cutoff * (buf1 - buf2);
    buf3 += cutoff * (buf2 - buf3);
    buf4 += cutoff * (buf3 - buf4);
    buf5 += cutoff * (buf4 - buf5);
    buf6 += cutoff * (buf5 - buf6);
    buf7 += cutoff * (buf6 - buf7);

    //8th order outputs:
    return buf7;
}

float IIR_Filter_HIGH_PASS_8TH_ORDER::process(float inputValue) {
    buf0 += cutoff * (inputValue - buf0 + feedbackAmount * (buf0 - buf1)); //with resonance
    buf1 += cutoff * (buf0 - buf1);
    buf2 += cutoff * (buf1 - buf2);
    buf3 += cutoff * (buf2 - buf3);
    buf4 += cutoff * (buf3 - buf4);
    buf5 += cutoff * (buf4 - buf5);
    buf6 += cutoff * (buf5 - buf6);
    buf7 += cutoff * (buf6 - buf7);

    //8th order outputs:
    return inputValue - buf7;
}
*/

float IIR_Filter::iir_filter_multi_sum(float input, IIR_Filter **iir_array, int total_filters, float *mixing_volumes)
{
	float output = 0.0;

	for(int f=0;f<total_filters;f++) {
		output += iir_array[f]->process(input)*mixing_volumes[f];
	}

	return output/(float)total_filters;
}


float IIR_Filter::iir_filter_multi_sum_w_noise(float input, IIR_Filter **iir_array, int total_filters, float *mixing_volumes, uint16_t noise, float noise_volume)
{
	//float output = input * FILTERS / 2; //add 100% dry signal (before echo)
	//float output = input * FILTERS / 4; //add 50% dry signal (before echo)
	float output = input * FILTERS / 8; //add 25% dry signal (before echo)
	//float output = 0; //no dry mix before echo

	input += (float)(32768 - noise) / 32768.0f * noise_volume;

	for(int f=0;f<total_filters;f++) {
		output += iir_array[f]->process(input) * mixing_volumes[f];
		//noise <<= 1;
		//noise |= f;
	}

	return output/(float)total_filters;
}

float IIR_Filter::iir_filter_multi_sum_w_noise_and_wind(float input, IIR_Filter **iir_array, int total_filters, float *mixing_volumes, uint16_t noise, float noise_volume, int wind_voices)
{
	//float output = input * FILTERS / 2; //add 100% dry signal (before echo)
	float output = input * FILTERS / 4; //add 50% dry signal (before echo)
	//float output = input * FILTERS / 8; //add 25% dry signal (before echo)
	//float output = 0; //no dry mix before echo

	float f_noise = (float)(32768 - noise) / 32768.0f;
	input += f_noise * noise_volume * 2;
	f_noise *= (2-noise_volume);

	int f;

	for(f=0;f<wind_voices;f++) {
		output += iir_array[f]->process(f_noise) * mixing_volumes[f] * 2;
		//noise <<= 1;
		//noise |= f;
		//printf("[value]iir_filter_multi_sum_w_noise_and_wind(): mixing_volumes[%d] = %f\n",f, mixing_volumes[f]);
	}

	//printf("[value]iir_filter_multi_sum_w_noise_and_wind(): output = %f\n",output);

	for(f=wind_voices;f<total_filters;f++) {
		output += iir_array[f]->process(input) * mixing_volumes[f];
		//noise <<= 1;
		//noise |= f;
	}

	return output/(float)total_filters;
}
