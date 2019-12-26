/*
 * DCO_synth.cpp
 *
 *  Created on: 11 May 2017
 *      Author: mayo
 *
 * Based on "The Tiny-TS Touch Synthesizer" by Janost 2016, Sweden
 * https://janostman.wordpress.com/the-tiny-ts-diy-touch-synthesizer/
 *
 */

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "v1_DCO_Synth.h"

#define USE_FAUX_22k_MODE

const uint8_t v1_DCO_Synth::sinetable256[256] = {
	0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,16,18,20,21,23,25,27,29,31,
	33,35,37,39,42,44,46,49,51,54,56,59,62,64,67,70,73,76,78,81,84,87,90,93,96,99,102,105,108,111,115,118,121,124,
	127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,176,178,181,184,187,190,192,195,198,200,203,205,208,210,212,215,217,219,221,223,225,227,229,231,233,234,236,238,239,240,
	242,243,244,245,247,248,249,249,250,251,252,252,253,253,253,254,254,254,254,254,254,254,253,253,253,252,252,251,250,249,249,248,247,245,244,243,242,240,239,238,236,234,233,231,229,227,225,223,
	221,219,217,215,212,210,208,205,203,200,198,195,192,190,187,184,181,178,176,173,170,167,164,161,158,155,152,149,146,143,139,136,133,130,127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,78,
	76,73,70,67,64,62,59,56,54,51,49,46,44,42,39,37,35,33,31,29,27,25,23,21,20,18,16,15,14,12,11,10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0
};

v1_DCO_Synth::v1_DCO_Synth()
{
	envtick = 549;
	DOUBLE = 0;
	resonant_peak_mod_volume = 0;

	//-------- Synth parameters --------------
	//volatile uint8_t VCA=255;         //VCA level 0-255
	ATTACK = 1;        // ENV Attack rate 0-255
	RELEASE = 1;       // ENV Release rate 0-255
	//ENVELOPE=0;      // ENV Shape
	//TRIG=0;          //MIDItrig 1=note ON
	//-----------------------------------------

	FREQ = 0;         //DCO pitch

	DCO_BLOCKS_ACTIVE = DCO_BLOCKS_MAX;

	use_table256 = (uint8_t*)sinetable256;
}

//---------------------------------------------------------------------------------------------------------------------
// Old, simpler version (no controls during play, apart from IR sensors) but keeping here as it is fun to play
// Some leftovers from the original Tiny-TS code
//---------------------------------------------------------------------------------------------------------------------

void v1_DCO_Synth::v1_init()
{
	sample_i16 = 128;
	FREQ=440;

	DOUBLE = 12;
	PHASEamt = 9;
	ENVamt = 123;
	ATTACK = 64;
	RELEASE = 64;

	init_echo_buffer();

	ECHO_MIXING_GAIN_MUL = 1; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 4; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in
}

void v1_DCO_Synth::v1_play_loop()
{
	while(1)
	{
		//-------------------- DCO block ------------------------------------------

		DCO_output[0] = 0;
		DCO_output[1] = 0;

		for (int b=0; b<DCO_BLOCKS_ACTIVE; b++)
		{
			phacc[b] += FREQ + (b * DOUBLE);
		    if (phacc[b] & 0x8000) {
				phacc[b] &= 0x7FFF;
				otone1[b] += 2;
				pdacc[b] += PDmod;
		    }
		    if (!otone1[b]) pdacc[b] = 0;
		    otone2[b] = (pdacc[b] >> 3) & 255;
		    uint8_t env = (255-otone1[b]);
		    DCO_output[b%2] += (use_table256[otone2[b]] + use_table256[(otone2[b] + (127 - env)) & 255]);
		}
		//---------------------------------------------------------------------------

		if (!(envtick--)) {
			envtick=582;

		    //--------- Resonant Peak ENV modulation ----------------------------------------

			PDmod=((resonant_peak_mod_volume*ENVamt)>>8)+PHASEamt;
			if (PDmod>255) PDmod=255;
			//if (PDmod>8192) PDmod=8192;
		    //-----------------------------------------------------------------
		}

		//-------------------------------------------------------------------------
		// send sample to codec
		//-------------------------------------------------------------------------

		sample_i16 = (int16_t)DCO_output[0];
		sample_i16 += echo_buffer[echo_buffer_ptr];

		//while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		//SPI_I2S_SendData(CODEC_I2S, sample_i16);
		sample32 = sample_i16 << 16;

		sample_i16 = (int16_t)DCO_output[1];

		//if(PROG_add_echo)
		//{
			//wrap the echo loop
			echo_buffer_ptr0++;
			if(echo_buffer_ptr0 >= echo_dynamic_loop_length)
			{
				echo_buffer_ptr0 = 0;
			}

			echo_buffer_ptr = echo_buffer_ptr0 + 1;
			if(echo_buffer_ptr >= echo_dynamic_loop_length)
			{
				echo_buffer_ptr = 0;
			}

			//add echo from the loop
			sample_i16 += echo_buffer[echo_buffer_ptr];

			//store result to echo, the amount defined by a fragment
			echo_buffer[echo_buffer_ptr0] = sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
		//}

		//while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		//SPI_I2S_SendData(CODEC_I2S, sample_i16);
		sample32 += sample_i16;
		i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);

		#ifdef USE_FAUX_22k_MODE
		i2s_pop_sample(I2S_NUM, (char*)&ADC_sample0, portMAX_DELAY);
		i2s_push_sample(I2S_NUM, (char*)&sample32, portMAX_DELAY);
		#endif

		sampleCounter++;

		if (TIMING_BY_SAMPLE_1_SEC)
		{
			sampleCounter = 0;
			seconds++;
		}

		if (TIMING_EVERY_10_MS==0) //100Hz
		//if (TIMING_EVERY_250_MS==0) //4Hz
		{
			/*
			FREQ = ir_res[0] * 4;

			//if (MUX==1) DOUBLE=(ADCL+(ADCH<<8))>>2;

			//DOUBLE = ADC_last_result[1] / 8;
			DOUBLE = ir_res[1] / 12;

			//if (MUX==2) PHASEamt=(((ADCL+(ADCH<<8)))>>3);

			//PHASEamt = ADC_last_result[2] / 8;
			PHASEamt = ir_res[2] / 16;

			ENVamt = ir_res[3] * 16; /// 4;
			*/

			#define DCO_FREQ_DEFAULT 32000
			FREQ = (uint16_t)(ir_res[0] * DCO_FREQ_DEFAULT * 2);
			//printf("FREQ=%d\n",FREQ);
			DOUBLE = (uint8_t)(ir_res[1] * 256);
			//printf("DOUBLE=%d\n",DOUBLE);
			PHASEamt = (uint8_t)(ir_res[2] * 256);
			//printf("PHASEamt=%d\n",PHASEamt);
			ENVamt = (uint8_t)(ir_res[3] * 32);
			//printf("ENVamt=%d\n",ENVamt);
			resonant_peak_mod_volume = (uint16_t)(ir_res[3] * 4096);
		}
	}
}
