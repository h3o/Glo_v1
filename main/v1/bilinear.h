#ifndef RTW_HEADER_bilinear_h_
#define RTW_HEADER_bilinear_h_

#ifdef __cplusplus
 extern "C" {
#endif

void prewarp(float *a0, float *a1, float *a2, float fc, float fs);

void bilinear(
    float a0, float a1, float a2,    /* numerator coefficients */
    float b0, float b1, float b2,    /* denominator coefficients */
    float *k,                                   /* overall gain factor */
    float fs,                                   /* sampling rate */
    float *coef);                         /* pointer to 4 iir coefficients */

void szxform(
    float *a0, float *a1, float *a2,     /* numerator coefficients */
    float *b0, float *b1, float *b2,   /* denominator coefficients */
    float fc,           /* Filter cutoff frequency */
    float fs,           /* sampling rate */
    float *k,           /* overall gain factor */
    float *coef);         /* pointer to 4 iir coefficients */

#ifdef __cplusplus
}
#endif
#endif

