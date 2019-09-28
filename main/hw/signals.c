/*
 * signals.c
 *
 *  Created on: Apr 27, 2016
 *      Author: mario
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

#include "signals.h"
#include "init.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//-------------------------- sample and echo buffers -------------------

uint32_t ADC_sample;
uint32_t sample32;

volatile int16_t sample_i16 = 0;
volatile uint16_t sample_u16 = 0;

float sample_f[2],sample_mix,sample_lpf[2];

int n_bytes, ADC_result;
uint32_t ADC_sample, ADC_sample0;
//int ADC_sample_valid = 0;
uint32_t sample32;

int autocorrelation_rec_sample;
uint16_t *autocorrelation_buffer;
int autocorrelation_result[4], autocorrelation_result_rdy;
//int autocorrelation_buffer_full;

#ifdef ECHO_BUFFER_STATIC
int16_t echo_buffer[ECHO_BUFFER_LENGTH];	//the buffer is allocated statically
//int16_t echo_buffer[184192/2];	//184192 bytes required for clouds -> region `dram0_0_seg' overflowed by 34384 bytes
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

const int echo_dynamic_loop_steps[ECHO_DYNAMIC_LOOP_STEPS] = {
	ECHO_BUFFER_LENGTH_DEFAULT,	//default
	//(I2S_AUDIOFREQ * 3 / 2),	//best one
	(I2S_AUDIOFREQ * 4 / 3),	//
	(13000),					//as in Dekrispator
	(I2S_AUDIOFREQ),			//one second
	//(I2S_AUDIOFREQ * 2), 		//interesting
	//(I2S_AUDIOFREQ * 5 / 2),	//not bad either
	(I2S_AUDIOFREQ / 3 * 2),	//short delay
	(I2S_AUDIOFREQ / 2),		//short delay
	(I2S_AUDIOFREQ / 3),		//short delay
	(I2S_AUDIOFREQ / 4),		//short delay
	(I2S_AUDIOFREQ / 6),		//short delay
	(I2S_AUDIOFREQ / 8),		//short delay
	(I2S_AUDIOFREQ / 10),		//short delay
	0							//delay off
};

//#define ECHO_BUFFER_LENGTH ((int)(I2S_AUDIOFREQ * 1.23456789f)) //-> nonsense
//#define ECHO_BUFFER_LENGTH ((int)(I2S_AUDIOFREQ * 2.54321f)) //-> max fitting to ram
//#define ECHO_BUFFER_LENGTH (I2S_AUDIOFREQ * 9 / 4) //-> hard to follow
//#define ECHO_BUFFER_LENGTH (I2S_AUDIOFREQ * 4 / 3) //-> not too interesting

int fixed_arp_level = 0;

int16_t reverb_buffer[REVERB_BUFFER_LENGTH];	//the buffer is allocated statically
int reverb_buffer_ptr0, reverb_buffer_ptr;		//pointers for reverb buffer
int reverb_dynamic_loop_length;
float REVERB_MIXING_GAIN_MUL, REVERB_MIXING_GAIN_DIV;
float reverb_mix_f;

float SAMPLE_VOLUME = SAMPLE_VOLUME_DEFAULT; //9.0f; //12.0f; //0.375f;
float limiter_coeff = DYNAMIC_LIMITER_COEFF_DEFAULT;

//---------------------------------------------------------------------------

double static b_noise = NOISE_SEED;
uint32_t random_value;

void reset_pseudo_random_seed()
{
	b_noise = NOISE_SEED;
}

void set_pseudo_random_seed(double new_value)
{
	printf("set_pseudo_random_seed(%f)\n",new_value);
	b_noise = new_value;
}

float PseudoRNG1a_next_float() //returns random float between -0.5f and +0.5f
{
/*
	b_noise = b_noise * b_noise;
	int i_noise = b_noise;
	b_noise = b_noise - i_noise;

	float b_noiseout;
	b_noiseout = b_noise - 0.5;

	b_noise = b_noise + 19;

	return b_noiseout;
*/
	b_noise = b_noise + 19;
	b_noise = b_noise * b_noise;
	int i_noise = b_noise;
	b_noise = b_noise - i_noise;
	return b_noise - 0.5;
}

/*
float PseudoRNG1b_next_float()
{
	double b_noiselast = b_noise;
	b_noise = b_noise + 19;
	b_noise = b_noise * b_noise;
	b_noise = (b_noise + b_noiselast) * 0.5;
	b_noise = b_noise - (int)b_noise;

	return b_noise - 0.5;
}

uint32_t PseudoRNG2_next_int32()
{
	//http://musicdsp.org/showone.php?id=59
	//Type : Linear Congruential, 32bit
	//References : Hal Chamberlain, "Musical Applications of Microprocessors" (Posted by Phil Burk)
	//Notes :
	//This can be used to generate random numeric sequences or to synthesise a white noise audio signal.
	//If you only use some of the bits, use the most significant bits by shifting right.
	//Do not just mask off the low bits.

	//Calculate pseudo-random 32 bit number based on linear congruential method.

	//Change this for different random sequences.
	static unsigned long randSeed = 22222;
	randSeed = (randSeed * 196314165) + 907633515;
	return randSeed;
}
*/

void new_random_value()
{
	float r = PseudoRNG1a_next_float();
	memcpy(&random_value, &r, sizeof(random_value));
}

int fill_with_random_value(char *buffer)
{
	float r = PseudoRNG1a_next_float();
	memcpy(buffer, &r, sizeof(r));
	return sizeof(r);
}

//this seems to be NOT a more optimal way of passing the value
void PseudoRNG_next_value(uint32_t *buffer) //load next random value to the variable
{
	b_noise = b_noise * b_noise;
	int i_noise = b_noise;
	b_noise = b_noise - i_noise;
	float b_noiseout = b_noise - 0.5;
	b_noise = b_noise + NOISE_SEED;
	memcpy(buffer, &b_noiseout, sizeof(b_noiseout));
}

//uint8_t test[10000]; //test for how much static memory is left

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

int16_t add_reverb(int16_t sample)
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

	if (reverb_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER)
	{
		reverb_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER;
	}

	if (reverb_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER)
	{
		reverb_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER;
	}

	sample = (int16_t)reverb_mix_f;

	//store result to reverb, the amount defined by a fragment
	//reverb_mix_f = ((float)sample_i16 * REVERB_MIXING_GAIN_MUL / REVERB_MIXING_GAIN_DIV);
	//reverb_mix_f *= REVERB_MIXING_GAIN_MUL / REVERB_MIXING_GAIN_DIV;
	//reverb_buffer[reverb_buffer_ptr0] = (int16_t)reverb_mix_f;
	reverb_buffer[reverb_buffer_ptr0] = sample;

	return sample;
}

int16_t add_echo(int16_t sample)
{
//	if(!PROG_add_echo)
//	{
//		return sample;
//	}

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

	/*
	if (echo_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER)
	{
		echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER;
	}

	if (echo_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER)
	{
		echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER;
	}
	*/
	sample = (int16_t)echo_mix_f;

	//store result to echo, the amount defined by a fragment
	//echo_mix_f = ((float)sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV);
	//echo_mix_f *= ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
	//echo_buffer[echo_buffer_ptr0] = (int16_t)echo_mix_f;
	echo_buffer[echo_buffer_ptr0] = sample;

	return sample;
}

