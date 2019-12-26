// ----------------- file filterIIR00.c begin -----------------
/*
Resonant low pass filter source code.
By baltrax@hotmail.com (Zxform)
*/

#include <stdlib.h>
//#include <stdio.h>
//#include <math.h>
#include <string.h>
//#include <Arduino.h> //needed for Serial.println
#include "legacy.h"

/**************************************************************************

FILTER.C - Source code for filter functions

    iir_filter         IIR filter floats sample by sample (real time)

*************************************************************************/

#include "v1_FilterIIR00.h"
#include "bilinear.h"
//#include "codec.h"

v1_FilterIIR00::v1_FilterIIR00(void)
{

}

int v1_FilterIIR00::prepare_IIR(v1_FILTER *iir_dest, float fc, float Q, float k)
{

/*
 * --------------------------------------------------------------------
 *
 * main()
 *
 * Example main function to show how to update filter coefficients.
 * We create a 4th order filter (24 db/oct roloff), consisting
 * of two second order sections.
 * --------------------------------------------------------------------
 */
        float    *coef;
        //float   fs, fc;     /* Sampling frequency, cutoff frequency */
        //float   Q;     /* Resonance > 1.0 < 1000 */
        unsigned nInd;
        float   a0, a1, a2, b0, b1, b2;
        //float   k;           /* overall gain factor */
	    float fs = v1_I2S_AUDIOFREQ;

	    v1_FILTER iir;

        BIQUAD ProtoCoef[FILTER_SECTIONS];      /* Filter prototype coefficients, 1 for each filter section */


/*
 * Setup filter s-domain coefficients
 */
                 /* Section 1 */
        ProtoCoef[0].a0 = 1.0;
        ProtoCoef[0].a1 = 0;
        ProtoCoef[0].a2 = 0;
        ProtoCoef[0].b0 = 1.0;
        ProtoCoef[0].b1 = 0.765367;
        ProtoCoef[0].b2 = 1.0;

                 /* Section 2 */
        ProtoCoef[1].a0 = 1.0;
        ProtoCoef[1].a1 = 0;
        ProtoCoef[1].a2 = 0;
        ProtoCoef[1].b0 = 1.0;
        ProtoCoef[1].b1 = 1.847759;
        ProtoCoef[1].b2 = 1.0;

        iir.length = FILTER_SECTIONS;         /* Number of filter sections */

/*
 * Allocate array of z-domain coefficients for each filter section
 * plus filter gain variable
 */
        iir.coef = (float *) malloc((4 * iir.length + 1) * sizeof(float));
        if (!iir.coef)
        {
                 //printf("Unable to allocate coef array, exiting\n");
                 //exit(1);
                 //Serial.println(F("calloc[1]: coef"));
                 //delay(10000);
                 return 0;
        }

        //k = 0.05;          /* Set overall filter gain */
        coef = iir.coef + 1;     /* Skip k, or gain */

        //Q = 12;                         /* Resonance */
        //fc = 660;                  /* Filter cutoff (Hz) */
        //fs = 22050;                      /* Sampling frequency (Hz) */
        //fs = 48000;                      /* Sampling frequency (Hz) */

/*
 * Compute z-domain coefficients for each biquad section
 * for new Cutoff Frequency and Resonance
 */
        for (nInd = 0; nInd < iir.length; nInd++)
        {
                 a0 = ProtoCoef[nInd].a0;
                 a1 = ProtoCoef[nInd].a1;
                 a2 = ProtoCoef[nInd].a2;

                 b0 = ProtoCoef[nInd].b0;
                 b1 = ProtoCoef[nInd].b1 / Q;      /* Divide by resonance or Q
*/
                 b2 = ProtoCoef[nInd].b2;
                 szxform(&a0, &a1, &a2, &b0, &b1, &b2, fc, fs, &k, coef);
                 coef += 4;                       /* Point to next filter
section */
        }

        /* Update overall filter gain in coef array */
        iir.coef[0] = k;

        /* Display filter coefficients */
        //for (nInd = 0; nInd < (iir.length * 4 + 1); nInd++)
        //         printf("C[%d] = %15.10f\n", nInd, iir.coef[nInd]);
/*
 * To process audio samples, call function iir_filter()
 * for each audio sample
 */
        //return (0);
        iir.history = NULL;
        memcpy(iir_dest,&iir,sizeof(iir));
        return 1;
}


int v1_FilterIIR00::update_IIR(v1_FILTER *iir, float fc, float Q, float k)
{
        float    *coef;
        unsigned nInd;
        float   a0, a1, a2, b0, b1, b2;

	    float fs = v1_I2S_AUDIOFREQ;

        BIQUAD ProtoCoef[FILTER_SECTIONS];      /* Filter prototype coefficients, 1 for each filter section */
/*
 * Setup filter s-domain coefficients
 */
                 /* Section 1 */
        ProtoCoef[0].a0 = 1.0;
        ProtoCoef[0].a1 = 0;
        ProtoCoef[0].a2 = 0;
        ProtoCoef[0].b0 = 1.0;
        ProtoCoef[0].b1 = 0.765367;
        ProtoCoef[0].b2 = 1.0;

                 /* Section 2 */
        ProtoCoef[1].a0 = 1.0;
        ProtoCoef[1].a1 = 0;
        ProtoCoef[1].a2 = 0;
        ProtoCoef[1].b0 = 1.0;
        ProtoCoef[1].b1 = 1.847759;
        ProtoCoef[1].b2 = 1.0;

        //k = 0.05;          /* Set overall filter gain */
        coef = iir->coef + 1;     /* Skip k, or gain */

        //Q = 12;                         /* Resonance */
        //fc = 660;                  /* Filter cutoff (Hz) */
        //fs = 22050;                      /* Sampling frequency (Hz) */
        //fs = 48000;                      /* Sampling frequency (Hz) */

/*
 * Compute z-domain coefficients for each biquad section
 * for new Cutoff Frequency and Resonance
 */
        for (nInd = 0; nInd < iir->length; nInd++)
        {
                 a0 = ProtoCoef[nInd].a0;
                 a1 = ProtoCoef[nInd].a1;
                 a2 = ProtoCoef[nInd].a2;

                 b0 = ProtoCoef[nInd].b0;
                 b1 = ProtoCoef[nInd].b1 / Q;      /* Divide by resonance or Q
*/
                 b2 = ProtoCoef[nInd].b2;
                 szxform(&a0, &a1, &a2, &b0, &b1, &b2, fc, fs, &k, coef);
                 coef += 4;                       /* Point to next filter
section */
        }

        /* Update overall filter gain in coef array */
        iir->coef[0] = k;

        /* Display filter coefficients */
        //for (nInd = 0; nInd < (iir.length * 4 + 1); nInd++)
        //         printf("C[%d] = %15.10f\n", nInd, iir.coef[nInd]);
/*
 * To process audio samples, call function iir_filter()
 * for each audio sample
 */
        //return (0);
        //iir.history = NULL;
        //memcpy(iir_dest,&iir,sizeof(iir));
        return 1;
}

v1_FilterIIR00::~v1_FilterIIR00(void)
{

}

/*
 * --------------------------------------------------------------------
 *
 * iir_filter - Perform IIR filtering sample by sample on floats
 *
 * Implements cascaded direct form II second order sections.
 * Requires FILTER structure for history and coefficients.
 * The length in the filter structure specifies the number of sections.
 * The size of the history array is 2*iir->length.
 * The size of the coefficient array is 4*iir->length + 1 because
 * the first coefficient is the overall scale factor for the filter.
 * Returns one output sample for each input sample.  Allocates history
 * array if not previously allocated.
 *
 * float iir_filter(float input,FILTER *iir)
 *
 *     float input        new float input sample
 *     FILTER *iir        pointer to FILTER structure
 *
 * Returns float value giving the current output.
 *
 * Allocation errors cause an error message and a call to exit.
 * --------------------------------------------------------------------
 */
float v1_FilterIIR00::iir_filter(float input,v1_FILTER *iir)
    //float input;        /* new input sample */
    //FILTER *iir;        /* pointer to FILTER structure */
{
    float *hist1_ptr,*hist2_ptr,*coef_ptr;
    float output,new_hist,history1,history2;

/* allocate history array if different size than last call */

    if(!iir->history) {
        iir->history = (float *) calloc(2*iir->length,sizeof(float));
        if(!iir->history) {
            //printf("\nUnable to allocate history array in iir_filter\n");
            //exit(1);
            //Serial.println(F("calloc[2]: history"));
            //delay(10000);
            return 0;
        }
    }

    coef_ptr = iir->coef;                /* coefficient pointer */

    hist1_ptr = iir->history;            /* first history */
    hist2_ptr = hist1_ptr + 1;           /* next history */

        /* 1st number of coefficients array is overall input scale factor,
         * or filter gain */
    output = input * (*coef_ptr++);

    for (unsigned int i = 0 ; i < iir->length; i++)
        {
        history1 = *hist1_ptr;           /* history values */
        history2 = *hist2_ptr;

        output = output - history1 * (*coef_ptr++);
        new_hist = output - history2 * (*coef_ptr++);    /* poles */

        output = new_hist + history1 * (*coef_ptr++);
        output = output + history2 * (*coef_ptr++);      /* zeros */

        *hist2_ptr++ = *hist1_ptr;
        *hist1_ptr++ = new_hist;
        hist1_ptr++;
        hist2_ptr++;
    }

    return(output);
}

float v1_FilterIIR00::iir_filter_multi_sum(float input, v1_FILTER *iir_array, int total_filters)
{
    float *hist1_ptr,*hist2_ptr,*coef_ptr;
    float output,new_hist,history1,history2,mix_sum=0;

/* allocate history array if different size than last call */

    v1_FILTER *iir;

    for(int f=0;f<total_filters;f++){
    	iir = iir_array+f;

    if(!iir->history) {
        iir->history = (float *) calloc(2*iir->length,sizeof(float));
        if(!iir->history) {
            //printf("\nUnable to allocate history array in iir_filter\n");
            //exit(1);
            //Serial.println(F("calloc[2]: history"));
            //delay(10000);
            return 0;
        }
    }

    coef_ptr = iir->coef;                /* coefficient pointer */

    hist1_ptr = iir->history;            /* first history */
    hist2_ptr = hist1_ptr + 1;           /* next history */

        /* 1st number of coefficients array is overall input scale factor,
         * or filter gain */
    output = input * (*coef_ptr++);

    for (int i = 0 ; i < iir->length; i++)
        {
        history1 = *hist1_ptr;           /* history values */
        history2 = *hist2_ptr;

        output = output - history1 * (*coef_ptr++);
        new_hist = output - history2 * (*coef_ptr++);    /* poles */

        output = new_hist + history1 * (*coef_ptr++);
        output = output + history2 * (*coef_ptr++);      /* zeros */

        *hist2_ptr++ = *hist1_ptr;
        *hist1_ptr++ = new_hist;
        hist1_ptr++;
        hist2_ptr++;
    }

    mix_sum += output;
    iir++;
    }

    return(mix_sum);
}

float v1_FilterIIR00::iir_filter_multi_sum(float input, v1_FILTER *iir_array, int total_filters, float *mixing_volumes)
{
    float *hist1_ptr,*hist2_ptr,*coef_ptr;
    float output,new_hist,history1,history2,mix_sum=0;

/* allocate history array if different size than last call */

    v1_FILTER *iir;

    for(int f=0;f<total_filters;f++){
    	iir = iir_array+f;

    if(!iir->history) {
        iir->history = (float *) calloc(2*iir->length,sizeof(float));
        if(!iir->history) {
            //printf("\nUnable to allocate history array in iir_filter\n");
            //exit(1);
            //Serial.println(F("calloc[2]: history"));
            //delay(10000);
            return 0;
        }
    }

    coef_ptr = iir->coef;                /* coefficient pointer */

    hist1_ptr = iir->history;            /* first history */
    hist2_ptr = hist1_ptr + 1;           /* next history */

        /* 1st number of coefficients array is overall input scale factor,
         * or filter gain */
    output = input * (*coef_ptr++);

    for (int i = 0 ; i < iir->length; i++)
        {
        history1 = *hist1_ptr;           /* history values */
        history2 = *hist2_ptr;

        output = output - history1 * (*coef_ptr++);
        new_hist = output - history2 * (*coef_ptr++);    /* poles */

        output = new_hist + history1 * (*coef_ptr++);
        output = output + history2 * (*coef_ptr++);      /* zeros */

        *hist2_ptr++ = *hist1_ptr;
        *hist1_ptr++ = new_hist;
        hist1_ptr++;
        hist2_ptr++;
    }

    mix_sum += output*mixing_volumes[f];
    iir++;
    }

    return(mix_sum);
}

// ----------------- file filterIIR00.c end -----------------
