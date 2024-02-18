/*
 * signals.c
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "signals.h"

#include "init.h"
#include "dsp/reverb.h"

//-------------------------- sample and echo buffers -------------------

volatile int16_t sample_i16 = 0;
volatile uint16_t sample_u16 = 0;

float sample_f[2],sample_mix,sample_lpf[4],sample_hpf[4];

uint32_t ADC_sample, ADC_sample0;
uint32_t ADC_sampleA[ADC_SAMPLE_BUFFER];
int ADC_sample_ptr;
uint32_t sample32;

int autocorrelation_rec_sample;
uint16_t *autocorrelation_buffer;
int autocorrelation_result[4], autocorrelation_result_rdy;
//int autocorrelation_buffer_full;

#ifdef ECHO_BUFFER_STATIC
int16_t echo_buffer[ECHO_BUFFER_LENGTH];	//the buffer is allocated statically
#else
int16_t *echo_buffer = NULL;	//the buffer is allocated dynamically
#endif

int echo_buffer_ptr0, echo_buffer_ptr;
int echo_dynamic_loop_length = ECHO_BUFFER_LENGTH_DEFAULT; //default
int echo_dynamic_loop_length0;//, echo_length_updated = 0;
int echo_skip_samples = 0, echo_skip_samples_from = 0;
int echo_dynamic_loop_current_step = 0;
float ECHO_MIXING_GAIN_MUL, ECHO_MIXING_GAIN_DIV; //todo replace with one fragment value
float echo_mix_f;

const uint8_t echo_dynamic_loop_indicator[ECHO_DYNAMIC_LOOP_STEPS] = {0x11,0x01,0x06,0x02,0x04,0x08,0x20,0x80,0x21,0x06,0x28,0x0c,0x18,0x05,0x00};
const int echo_dynamic_loop_steps[ECHO_DYNAMIC_LOOP_STEPS] = {
	ECHO_BUFFER_LENGTH_DEFAULT,	//default, same as (I2S_AUDIOFREQ * 3 / 2)
	(I2S_AUDIOFREQ),			//one second
	(I2S_AUDIOFREQ / 3 * 2),	//short delay
	(I2S_AUDIOFREQ / 2),		//short delay
	(I2S_AUDIOFREQ / 3),		//short delay
	(I2S_AUDIOFREQ / 4),		//short delay
	(I2S_AUDIOFREQ / 6),		//short delay
	(I2S_AUDIOFREQ / 8),		//short delay
	(I2S_AUDIOFREQ / 16),		//short delay
	(I2S_AUDIOFREQ / 32),		//short delay
	(I2S_AUDIOFREQ / 64),		//short delay
	(I2S_AUDIOFREQ * 4 / 3),	//
	(I2S_AUDIOFREQ * 5 / 4),	//
	(13000),					//as in Dekrispator
	//(I2S_AUDIOFREQ * 2), 		//interesting
	//(I2S_AUDIOFREQ * 5 / 2),	//not bad either
	0							//delay off
};

int fixed_arp_level = 0;

//int16_t reverb_buffer[REVERB_BUFFER_LENGTH];	//the buffer is allocated statically
int16_t *reverb_buffer = NULL; 					//the buffer is allocated dynamically
int reverb_buffer_ptr0, reverb_buffer_ptr;		//pointers for reverb buffer
int reverb_dynamic_loop_length;
float REVERB_MIXING_GAIN_MUL, REVERB_MIXING_GAIN_DIV;
float reverb_mix_f;

int16_t *reverb_buffer_ext[REVERB_BUFFERS_EXT] = {NULL,NULL};	//extra reverb buffers for higher and lower octaves
int reverb_buffer_ptr0_ext[REVERB_BUFFERS_EXT], reverb_buffer_ptr_ext[REVERB_BUFFERS_EXT];	//pointers for reverb buffer
int reverb_dynamic_loop_length_ext[REVERB_BUFFERS_EXT];
float REVERB_MIXING_GAIN_MUL_EXT, REVERB_MIXING_GAIN_DIV_EXT;

float SAMPLE_VOLUME = SAMPLE_VOLUME_DEFAULT;
float limiter_coeff = DYNAMIC_LIMITER_COEFF_DEFAULT;

float ADC_LPF_ALPHA = 0.2f; //up to 1.0 for max feedback = no filtering
float ADC_HPF_ALPHA = 0.2f;

//---------------------------------------------------------------------------
uint32_t random_value;
double r_rand = NOISE_SEED;
int i_rand;

void new_random_value()
{
	//random_value = esp_random();
	r_rand = r_rand + 19;
	r_rand = r_rand * r_rand;
	i_rand = r_rand;
	r_rand = r_rand - i_rand;
	//return r_rand - 0.5f;
	memcpy(&random_value,&r_rand,4);
}

int fill_with_random_value(char *buffer)
{
	new_random_value();
	memcpy(buffer, &random_value, sizeof(random_value));
	return sizeof(random_value);
}

float PseudoRNG1a_next_float_new() //returns random float between -0.5f and +0.5f
{
	r_rand = r_rand + 19;
	r_rand = r_rand * r_rand;
	i_rand = r_rand;
	r_rand = r_rand - i_rand;
	return r_rand - 0.5f;
}

float PseudoRNG1a_next_float_new01() //returns random float between 0.0f and +1.0f
{
	r_rand = r_rand + 19;
	r_rand = r_rand * r_rand;
	i_rand = r_rand;
	r_rand = r_rand - i_rand;
	return r_rand;
}

void init_echo_buffer()
{
	#ifdef ECHO_BUFFER_STATIC
	//printf("init_echo_buffer(): buffer allocated as static array\n");
	#else
	printf("init_echo_buffer(): free heap = %u, allocating = %u\n", xPortGetFreeHeapSize(), ECHO_BUFFER_LENGTH * sizeof(int16_t));
	echo_buffer = (int16_t*)malloc(ECHO_BUFFER_LENGTH * sizeof(int16_t));
	printf("init_echo_buffer(): echo_buffer = %x\n", (unsigned int)echo_buffer);
	if(echo_buffer==NULL)
	{
		printf("init_echo_buffer(): could not allocate buffer of size %d bytes\n", ECHO_BUFFER_LENGTH * sizeof(int16_t));
		while(1);
	}
	#endif
}

void deinit_echo_buffer()
{
	#ifdef ECHO_BUFFER_STATIC
	//printf("deinit_echo_buffer(): buffer allocated as static array\n");
	#else
	if(echo_buffer!=NULL)
	{
		free(echo_buffer);
		echo_buffer = NULL;
		printf("deinit_echo_buffer(): memory freed\n");
	}
	else
	{
		printf("deinit_echo_buffer(): buffer not allocated\n");
	}
	#endif
}

IRAM_ATTR int16_t add_reverb(int16_t sample)
{
	//wrap the reverb loop
	reverb_buffer_ptr0++;
	if (reverb_buffer_ptr0 >= reverb_dynamic_loop_length)
	{
		reverb_buffer_ptr0 = 0;
	}

	reverb_buffer_ptr = reverb_buffer_ptr0 + 1;
	if (reverb_buffer_ptr >= reverb_dynamic_loop_length)
	{
		reverb_buffer_ptr = 0;
	}

	//add reverb from the loop
	reverb_mix_f = (float)sample + (float)reverb_buffer[reverb_buffer_ptr] * REVERB_MIXING_GAIN_MUL / REVERB_MIXING_GAIN_DIV;

	if (reverb_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER) { reverb_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER; }
	if (reverb_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER) { reverb_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER; }

	sample = (int16_t)reverb_mix_f;

	//store result to reverb, the amount defined by a fragment
	//reverb_mix_f = ((float)sample_i16 * REVERB_MIXING_GAIN_MUL / REVERB_MIXING_GAIN_DIV);
	//reverb_mix_f *= REVERB_MIXING_GAIN_MUL / REVERB_MIXING_GAIN_DIV;
	//reverb_buffer[reverb_buffer_ptr0] = (int16_t)reverb_mix_f;

	reverb_buffer[reverb_buffer_ptr0] = sample;
	return sample;
}

/*
IRAM_ATTR int16_t add_reverb_ext(int16_t sample)
{
	int16_t out_sample = 0;

	for(int r=0;r<REVERB_BUFFERS_EXT;r++)
	{
		reverb_buffer_ptr0_ext[r]++;
		if (reverb_buffer_ptr0_ext[r] >= reverb_dynamic_loop_length_ext[r])
		{
			reverb_buffer_ptr0_ext[r] = 0;
		}

		reverb_buffer_ptr_ext[r] = reverb_buffer_ptr0_ext[r] + 1;
		if (reverb_buffer_ptr_ext[r] >= reverb_dynamic_loop_length_ext[r])
		{
			reverb_buffer_ptr_ext[r] = 0;
		}

		//add reverb from the loop
		reverb_mix_f = (float)sample*REVERB_EXT_SAMPLE_INPUT_COEFF + (float)reverb_buffer_ext[r][reverb_buffer_ptr_ext[r]] * REVERB_MIXING_GAIN_MUL_EXT / REVERB_MIXING_GAIN_DIV_EXT;
		reverb_mix_f *= REVERB_EXT_SAMPLE_OUTPUT_COEFF;

		if (reverb_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER) { reverb_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER; }
		if (reverb_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER) { reverb_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER; }

		sample = (int16_t)reverb_mix_f;
		reverb_buffer_ext[r][reverb_buffer_ptr0_ext[r]] = sample;

		out_sample += sample;
	}

	return out_sample;
}
*/

IRAM_ATTR int16_t add_reverb_ext_all3(int16_t sample)
{
	int32_t out_sample = 0;
	float sample_f = ((float)sample) * REVERB_EXT_SAMPLE_INPUT_COEFF;

	//wrap the reverb loop
	reverb_buffer_ptr0++;
	if (reverb_buffer_ptr0 >= reverb_dynamic_loop_length)
	{
		reverb_buffer_ptr0 = 0;
	}

	reverb_buffer_ptr = reverb_buffer_ptr0 + 1;
	if (reverb_buffer_ptr >= reverb_dynamic_loop_length)
	{
		reverb_buffer_ptr = 0;
	}

	//add reverb from the loop
	reverb_mix_f = sample_f + (float)reverb_buffer[reverb_buffer_ptr] * REVERB_MIXING_GAIN_MUL / REVERB_MIXING_GAIN_DIV;
	reverb_mix_f *= REVERB_EXT_SAMPLE_OUTPUT_COEFF;

	if (reverb_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER) { reverb_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER; }
	if (reverb_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER) { reverb_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER; }

	reverb_buffer[reverb_buffer_ptr0] = (out_sample = (int16_t)reverb_mix_f);

	//add 2nd and 3rd buffer
	for(int r=0;r<REVERB_BUFFERS_EXT;r++)
	{
		reverb_buffer_ptr0_ext[r]++;
		if (reverb_buffer_ptr0_ext[r] >= reverb_dynamic_loop_length_ext[r])
		{
			reverb_buffer_ptr0_ext[r] = 0;
		}

		reverb_buffer_ptr_ext[r] = reverb_buffer_ptr0_ext[r] + 1;
		if (reverb_buffer_ptr_ext[r] >= reverb_dynamic_loop_length_ext[r])
		{
			reverb_buffer_ptr_ext[r] = 0;
		}

		//add reverb from the loop
		reverb_mix_f = sample_f + (float)reverb_buffer_ext[r][reverb_buffer_ptr_ext[r]] * REVERB_MIXING_GAIN_MUL_EXT / REVERB_MIXING_GAIN_DIV_EXT;
		reverb_mix_f *= REVERB_EXT_SAMPLE_OUTPUT_COEFF;

		if (reverb_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER) { reverb_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER; }
		if (reverb_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER) { reverb_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER; }

		out_sample += (reverb_buffer_ext[r][reverb_buffer_ptr0_ext[r]] = (int16_t)reverb_mix_f);
	}

	if (out_sample > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER_INT) { out_sample = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER_INT; }
	if (out_sample < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER_INT) { out_sample = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER_INT; }

	return (int16_t)out_sample;
}

IRAM_ATTR int16_t add_echo(int16_t sample)
{
	if(echo_dynamic_loop_length)
	{
		//wrap the echo loop
		echo_buffer_ptr0++;
		if (echo_buffer_ptr0 >= echo_dynamic_loop_length)
		{
			echo_buffer_ptr0 = 0;
		}

		echo_buffer_ptr = echo_buffer_ptr0 + 1;
		if (echo_buffer_ptr >= echo_dynamic_loop_length)
		{
			echo_buffer_ptr = 0;
		}

		if(echo_skip_samples)
		{
			if(echo_skip_samples_from && (echo_skip_samples_from == echo_buffer_ptr))
			{
				echo_skip_samples_from = 0;
			}
			if(echo_skip_samples_from==0)
			{
				echo_buffer[echo_buffer_ptr] = 0;
				echo_skip_samples--;
			}
		}

		//add echo from the loop
		echo_mix_f = (float)sample + (float)echo_buffer[echo_buffer_ptr] * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
	}
	else
	{
		echo_mix_f = (float)sample;
	}

	echo_mix_f *= limiter_coeff;

	if(echo_mix_f < -DYNAMIC_LIMITER_THRESHOLD_ECHO && limiter_coeff > 0)
	{
		limiter_coeff *= DYNAMIC_LIMITER_COEFF_MUL;
	}

	sample = (int16_t)echo_mix_f;

	//store result to echo, the amount defined by a fragment
	//echo_mix_f = ((float)sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV);
	//echo_mix_f *= ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
	//echo_buffer[echo_buffer_ptr0] = (int16_t)echo_mix_f;

	echo_buffer[echo_buffer_ptr0] = sample;

	return sample;
}

IRAM_ATTR int16_t reversible_echo(int16_t sample, int direction)
{
	if(echo_dynamic_loop_length)
	{
		//wrap the echo loop
		if(direction)
		{
			echo_buffer_ptr0++;
			if (echo_buffer_ptr0 >= echo_dynamic_loop_length)
			{
				echo_buffer_ptr0 = 0;
			}

			echo_buffer_ptr = echo_buffer_ptr0 + 1;
			if (echo_buffer_ptr >= echo_dynamic_loop_length)
			{
				echo_buffer_ptr = 0;
			}
		}
		else
		{
			if (echo_buffer_ptr0 > 0)
			{
				echo_buffer_ptr0--;
			}
			else
			{
				echo_buffer_ptr0 = echo_dynamic_loop_length;
			}

			echo_buffer_ptr = echo_buffer_ptr0 - 1;
			if (echo_buffer_ptr < 0)
			{
				echo_buffer_ptr = echo_dynamic_loop_length;
			}
		}

		//add echo from the loop
		echo_mix_f = (float)sample + (float)echo_buffer[echo_buffer_ptr] * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
	}
	else
	{
		echo_mix_f = (float)sample;
	}

	echo_mix_f *= limiter_coeff;

	if(echo_mix_f < -DYNAMIC_LIMITER_THRESHOLD_ECHO && limiter_coeff > 0)
	{
		limiter_coeff *= DYNAMIC_LIMITER_COEFF_MUL;
	}

	sample = (int16_t)echo_mix_f;

	echo_buffer[echo_buffer_ptr0] = sample;

	return sample;
}
