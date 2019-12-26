/* FILTER INFORMATION STRUCTURE FOR FILTER ROUTINES */

#pragma once

#define FILTER_SECTIONS   2   /* 2 filter sections for 24 db/oct filter */

typedef struct {
    unsigned int length;       /* size of filter */
    float *history;            /* pointer to history in filter */
    float *coef;               /* pointer to coefficients of filter */
} v1_FILTER;

class v1_FilterIIR00
{

public:
v1_FilterIIR00(void);
~v1_FilterIIR00(void);
//int prepare_IIR(FILTER *iir_dest);
static int prepare_IIR(v1_FILTER *iir_dest, float fc, float Q, float k);
static int update_IIR(v1_FILTER *iir_dest, float fc, float Q, float k);
static float iir_filter(float input,v1_FILTER *iir);
static float iir_filter_multi_sum(float input, v1_FILTER *iir_array, int total_filters);
static float iir_filter_multi_sum(float input, v1_FILTER *iir_array, int total_filters, float *mixing_volumes);

private:

typedef struct {
        float a0, a1, a2;       /* numerator coefficients */
        float b0, b1, b2;       /* denominator coefficients */
} BIQUAD;

};


