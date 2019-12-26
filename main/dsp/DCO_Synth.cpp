/*
 * DCO_synth.cpp
 *
 *  Created on: 11 May 2017
 *      Author: mario
 *
 * Based on "The Tiny-TS Touch Synthesizer" by Janost 2016, Sweden
 * https://janostman.wordpress.com/the-tiny-ts-diy-touch-synthesizer/
 * https://www.kickstarter.com/projects/732508269/the-tiny-ts-an-open-sourced-diy-touch-synthesizer/
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

#include <DCO_Synth.h>
#include <Accelerometer.h>
//#include <stdbool.h>
//#include <string.h>
//#include <stddef.h>
//#include <stdlib.h>
//#include <math.h>
//#include <InitChannels.h>
#include <Interface.h>
//#include <MusicBox.h>
#include <hw/codec.h>
#include <hw/signals.h>
#include <hw/ui.h>
#include <notes.h>
#include <FreqDetect.h>
//#include <hw/init.h>
#include <hw/gpio.h>
#include <hw/sdcard.h>
#include <hw/midi.h>
#include <hw/leds.h>
//#include <hw/sha1.h>
//#include <stdio.h>

//#define DEBUG_DCO
#define MIX_INPUT
#define MIX_ECHO
//#define USE_AUTOCORRELATION
//#define LPF_ENABLED
#define ADC_LPF_ALPHA_DCO 0.2f //up to 1.0 for max feedback = no filtering
#define ADC_SAMPLE_GAIN 4 //integer value
#define DYNAMIC_LIMITER
//#define STATIC_LIMITER

extern float ADC_LPF_ALPHA;

const uint8_t DCO_Synth::sinetable256[256] = {
	0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,16,18,20,21,23,25,27,29,31,
	33,35,37,39,42,44,46,49,51,54,56,59,62,64,67,70,73,76,78,81,84,87,90,93,96,99,102,105,108,111,115,118,121,124,
	127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,176,178,181,184,187,190,192,195,198,200,203,205,208,210,212,215,217,219,221,223,225,227,229,231,233,234,236,238,239,240,
	242,243,244,245,247,248,249,249,250,251,252,252,253,253,253,254,254,254,254,254,254,254,253,253,253,252,252,251,250,249,249,248,247,245,244,243,242,240,239,238,236,234,233,231,229,227,225,223,
	221,219,217,215,212,210,208,205,203,200,198,195,192,190,187,184,181,178,176,173,170,167,164,161,158,155,152,149,146,143,139,136,133,130,127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,78,
	76,73,70,67,64,62,59,56,54,51,49,46,44,42,39,37,35,33,31,29,27,25,23,21,20,18,16,15,14,12,11,10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0
};

//uint8_t DCO_Synth::*sawtable256;
//uint8_t DCO_Synth::*squaretable256;

DCO_Synth::DCO_Synth()
{
	envtick = 549;
	DOUBLE = 0;
	RESO = 64;	//resonant peak mod volume

	//-------- Synth parameters --------------
	//volatile uint8_t VCA=255;         //VCA level 0-255
	ATTACK = 1;        // ENV Attack rate 0-255
	RELEASE = 1;       // ENV Release rate 0-255
	//ENVELOPE=0;      // ENV Shape
	//TRIG=0;          //MIDItrig 1=note ON
	//-----------------------------------------

	FREQ = 0;         //DCO pitch

	DCO_BLOCKS_ACTIVE = DCO_BLOCKS_MAX;// / 2;

	DCO_mode = 0;
	SET_mode = 0;
	button_set_held = 0;
	MIDI_note_set = 0;//, MIDI_note_on = 0;

	use_table256 = (uint8_t*)sinetable256;

	sawtable256 = (uint8_t*)malloc(256*sizeof(uint8_t));
	squaretable256 = (uint8_t*)malloc(256*sizeof(uint8_t));

	for(int i=0;i<256;i++)
	{
		sawtable256[i] = i;
		squaretable256[i] = (i<128)?0:255;
	}
}

DCO_Synth::~DCO_Synth(void)
{
	printf("DCO_Synth::~DCO_Synth(void)\n");
	free(sawtable256);
	free(squaretable256);

	#ifdef USE_AUTOCORRELATION
	stop_autocorrelation();
	#endif
}

//---------------------------------------------------------------------------------------------------------------------
// Old, simpler version (no controls during play, apart from IR sensors) but keeping here as it is fun to play
// Some leftovers from the original Tiny-TS code
//---------------------------------------------------------------------------------------------------------------------

//mixed sound output variable
//int16_t OutSample = 128;
//uint8_t GATEIO=0; //0=Gate is output, 1=Gate is input
//uint16_t COARSEPITCH;
//uint8_t ENVsmoothing;
//uint8_t envcnt=10;
//int16_t ENV;
//uint8_t k=0;
//uint8_t z;
//int8_t MUX=0;

void DCO_Synth::v1_init()//int mode)
{
	//-----------------------------------------------------------------------------------
	// Tiny-TS variables init
	//-----------------------------------------------------------------------------------

	sample_i16 = 128;

	FREQ=DCO_FREQ_DEFAULT;//440;//MIDI2FREQ(key+12+(COARSEPITCH/21.3125));
	//uint16_t CVout=COARSEPITCH+(key*21.33);
	//OCR1AH = (CVout>>8);
	//OCR1AL = CVout&255;

	//---------------------------------------------------------------

	//--------------- ADC block -------------------------------------

	//if (MUX==0) COARSEPITCH=(ADCL+(ADCH<<8));
	//COARSEPITCH=0;//(uint16_t)123;//45;

	//if (MUX==1) DOUBLE=(ADCL+(ADCH<<8))>>2;
	DOUBLE = 12;
	//if (MUX==2) PHASEamt=(((ADCL+(ADCH<<8)))>>3);
	PHASEamt = 9;
	//if (MUX==3) ENVamt=((ADCL+(ADCH<<8))>>3);
	ENVamt = 123;
	//if (MUX==4) ATTACK=ATTrates[15-((ADCL+(ADCH<<8))>>6)];
	ATTACK = 64;//ATTrates[12];
	//if (MUX==5) RELEASE=RELrates[15-((ADCL+(ADCH<<8))>>6)];
	RELEASE = 64;//RELrates[6];

	ECHO_MIXING_GAIN_MUL = 3; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 4; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

	ADC_LPF_ALPHA = ADC_LPF_ALPHA_DCO;

	#ifdef USE_AUTOCORRELATION
	start_autocorrelation(0); //start at core 0 as the DCO is running at core 1
	#endif
}

void DCO_Synth::v1_play_loop()
{
	int i;

	int midi_override = 0;

	while(!event_next_channel)
	{
		if(waveform)
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

				DCO_output[b%2] +=
				//DCO_output[b/(DCO_BLOCKS_ACTIVE/2)] +=
						(use_table256[otone2[b]] + use_table256[(otone2[b] + (127 - env)) & 255]);
			}
			//---------------------------------------------------------------------------

			//------------------ VCA block ------------------------------------
			/*
				if ((ATTACK==255)&&(TRIG==1)) VCA=255;
				if (!(envcnt--)) {
					envcnt=20;
					if (VCA<volume) VCA++;
					if (VCA>volume) VCA--;
				}
			*/
			/*
				#define M(MX, MX1, MX2) \
				  asm volatile ( \
					"clr r26 \n\t"\
					"mulsu %B1, %A2 \n\t"\
					"movw %A0, r0 \n\t"\
					"mul %A1, %A2 \n\t"\
					"add %A0, r1 \n\t"\
					"adc %B0, r26 \n\t"\
					"clr r1 \n\t"\
					: \
					"=&r" (MX) \
					: \
					"a" (MX1), \
					"a" (MX2) \
					:\
					"r26"\
				  )
			*/
			//DCO>>=1;
			//DCO>>=2;
			//DCO_output>>=4;
			////M(ENV, (int16_t)DCO, VCA);
			//ENV = (int16_t)DCO * VCA;
			//OCR2A = ENV>>1;

			//OutSample = (int16_t)DCO * VCA;
			//sample_i16 = (int16_t)DCO_output * VCA;

			//sample_i16 = (int16_t)DCO_output;

			//-----------------------------------------------------------------

			if (!(envtick--)) {
				envtick=582;

				//--------------------- ENV block ---------------------------------

				/*
				if ((TRIG==1)&&(volume<255)) {
					volume+=ATTACK;
					if (volume>255) volume=255;
				}
				if ((TRIG==0)&&(volume>0)) {
					volume-=RELEASE;
					if (volume<0) volume=0;
				}
				*/
				//-----------------------------------------------------------------

				//--------- Resonant Peak ENV modulation ----------------------------------------

				//if(!DCO_mode)
				{
					PDmod=((RESO*ENVamt)>>8)+PHASEamt;
					if (PDmod>255) PDmod=255;
					//if (PDmod>8192) PDmod=8192;
				}
				//-----------------------------------------------------------------
			}

			//-------------------------------------------------------------------------
			// listen and analyse
			//-------------------------------------------------------------------------
			#ifdef USE_AUTOCORRELATION
			if(dco_control_mode==2)
			{
				if(autocorrelation_rec_sample)
				{
					autocorrelation_buffer[--autocorrelation_rec_sample] = (int16_t)ADC_sample;
				}
			}
			#endif

			//-------------------------------------------------------------------------
			// send sample to codec
			//-------------------------------------------------------------------------

			sample_i16 = (int16_t)DCO_output[0];
		}
		else
		{
			sample_i16 = 0;
		}

		#ifdef MIX_INPUT
		#ifdef LPF_ENABLED
		sample_lpf[0] = sample_lpf[0] + ADC_LPF_ALPHA * ((float)((int16_t)(ADC_sample>>16)) - sample_lpf[0]);
		sample_i16 += (int16_t)(sample_lpf[0] * ADC_SAMPLE_GAIN); //low-pass
		//sample_i16 += (int16_t)(ADC_sample>>16) - (int16_t)(sample_lpf[0] * ADC_SAMPLE_GAIN); //high-pass
		#else
		sample_i16 += ((int16_t)(ADC_sample>>16)) * ADC_SAMPLE_GAIN;
		#endif
		#endif

		#ifdef MIX_ECHO
		//add echo from the loop
		sample_i16 += echo_buffer[echo_buffer_ptr];
		#endif

		#ifdef DYNAMIC_LIMITER
		//apply dynamic limiter
		echo_mix_f = (float)sample_i16 * limiter_coeff;

		if(echo_mix_f < -DYNAMIC_LIMITER_THRESHOLD_ECHO_DCO && limiter_coeff > 0)
		{
			limiter_coeff *= DYNAMIC_LIMITER_COEFF_MUL_DCO;
		}

		sample_i16 = (int16_t)echo_mix_f;
		#endif

		#ifdef STATIC_LIMITER
		while(sample_i16 > 16000 || sample_i16 < -16000)
		{
			sample_i16 /= 2;
		}
		#endif

		sample32 = sample_i16 << 16;

		if(waveform)
		{
			sample_i16 = (int16_t)DCO_output[1];
		}
		else
		{
			sample_i16 = 0;
		}

		#ifdef MIX_INPUT
		#ifdef LPF_ENABLED
		sample_lpf[1] = sample_lpf[1] + ADC_LPF_ALPHA * ((float)((int16_t)ADC_sample) - sample_lpf[1]);
		sample_i16 += (int16_t)(sample_lpf[1] * ADC_SAMPLE_GAIN); //low-pass
		//sample_i16 += (int16_t)ADC_sample - (int16_t)(sample_lpf[1] * ADC_SAMPLE_GAIN); //high-pass
		#else
		sample_i16 += ((int16_t)(ADC_sample)) * ADC_SAMPLE_GAIN;
		#endif
		#endif

		#ifdef MIX_ECHO
		//---------------------------------------------------------------------------------------
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

		#ifdef DYNAMIC_LIMITER
		//apply dynamic limiter
		echo_mix_f = (float)sample_i16 * limiter_coeff;

		if(echo_mix_f < -DYNAMIC_LIMITER_THRESHOLD_ECHO_DCO && limiter_coeff > 0)
		{
			limiter_coeff *= DYNAMIC_LIMITER_COEFF_MUL_DCO;
		}

		sample_i16 = (int16_t)echo_mix_f;
		#endif

		#ifdef STATIC_LIMITER
		while(sample_i16 > 16000 || sample_i16 < -16000)
		{
			sample_i16 /= 2;
		}
		#endif

		#ifdef DYNAMIC_LIMITER
		if(erase_echo==1 || (limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT))
		{
			ECHO_MIXING_GAIN_MUL = 1.0f;
		}
		else
		{
			ECHO_MIXING_GAIN_MUL = 3.0f;
		}
		#endif

		if(erase_echo!=2)
		{
			//store result to echo, the amount defined by a fragment
			echo_buffer[echo_buffer_ptr0] = (int16_t)((float)sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV);
		}

		#endif

		/*
		if(erase_echo)
		{
			echo_buffer[echo_buffer_ptr0] /= 2;
		}
		*/

		//---------------------------------------------------------------------------------------
		sample32 += sample_i16;

		#ifdef BOARD_WHALE
		if(add_beep)
		{
			if(sampleCounter & (1<<add_beep))
			{
				sample32 += (100 + (100<<16));
			}
		}
		#endif

		for(i=0;i<dco_oversample;i++)
		{
			i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
			sd_write_sample(&sample32);
		}

		#if defined(MIX_INPUT) || defined(USE_AUTOCORRELATION)
		for(i=0;i<dco_oversample;i++)
		{
			/*ADC_result = */i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
		}
		#endif

		//---------------------------------------------------------------------------------------

		sampleCounter++;

		if(TIMING_BY_SAMPLE_1_SEC)
		{
			sampleCounter = 0;
			seconds++;
		}

		#ifdef BOARD_WHALE
		if (TIMING_BY_SAMPLE_EVERY_100_MS==0)
		{
			RGB_LED_R_OFF;
			RGB_LED_G_OFF;
			RGB_LED_B_OFF;
		}

		if (TIMING_BY_SAMPLE_EVERY_100_MS==1000 && waveform)
		{
			RGB_LED_R_OFF;
			RGB_LED_B_OFF;
			if(param==0) { RGB_LED_R_ON; RGB_LED_G_ON; RGB_LED_B_ON; }
			if(param==1) { RGB_LED_R_ON; RGB_LED_G_ON; }
			if(param==2) { RGB_LED_R_ON; RGB_LED_B_ON; }
			if(param==3) { RGB_LED_G_ON; }
			if(param==4) { RGB_LED_R_ON; }
		}
		#endif

		ui_command = 0;

		if (TIMING_EVERY_20_MS==31) //50Hz
		{
			#define DCO_UI_CMD_NEXT_PARAM			1
			#define DCO_UI_CMD_PREV_PARAM			2
			#define DCO_UI_CMD_OVERSAMPLE_UP		3
			#define DCO_UI_CMD_OVERSAMPLE_DOWN		4
			#define DCO_UI_CMD_NEXT_WAVEFORM		5
			#define DCO_UI_CMD_NEXT_DELAY			6
			#define DCO_UI_CMD_RANDOMIZE_PARAMS		7
			#define DCO_UI_CMD_DRIFT_PARAMS			8

			//map UI commands
			#ifdef BOARD_WHALE
			if(short_press_volume_plus) { ui_command = DCO_UI_CMD_NEXT_PARAM; short_press_volume_plus = 0; }
			if(short_press_volume_minus) { ui_command = DCO_UI_CMD_PREV_PARAM; short_press_volume_minus = 0; }
			if(short_press_sequence==2) { ui_command =  DCO_UI_CMD_OVERSAMPLE_UP; short_press_sequence = 0; }
			if(short_press_sequence==-2) { ui_command = DCO_UI_CMD_OVERSAMPLE_DOWN; short_press_sequence = 0; }
			if(short_press_sequence==3) { ui_command = DCO_UI_CMD_NEXT_WAVEFORM; short_press_sequence = 0; }
			if(short_press_sequence==-3) { ui_command = DCO_UI_CMD_NEXT_DELAY; short_press_sequence = 0; }
			if(short_press_sequence==SEQ_PLUS_MINUS) { ui_command = DCO_UI_CMD_RANDOMIZE_PARAMS; short_press_sequence = 0; }
			if(short_press_sequence==SEQ_MINUS_PLUS) { ui_command = DCO_UI_CMD_DRIFT_PARAMS; short_press_sequence = 0; }
			#endif

			#ifdef BOARD_GECHO
			if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = DCO_UI_CMD_DRIFT_PARAMS; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = DCO_UI_CMD_NEXT_WAVEFORM; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_2) { ui_command =  DCO_UI_CMD_OVERSAMPLE_UP; btn_event_ext = 0; }
			if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_1) { ui_command = DCO_UI_CMD_OVERSAMPLE_DOWN; btn_event_ext = 0; }
			//if(event_channel_options) { ui_command = DCO_UI_CMD_RANDOMIZE_PARAMS; event_channel_options = 0; }
			#endif

			//FREQ = ADC_last_result[0] * 4;
			//FREQ = (uint16_t)((acc_res[0] + 1.0f) * 440.0f * 16);

			#ifdef BOARD_WHALE
			if(event_channel_options)
			{
				event_channel_options = 0;

				dco_control_mode++;
				if(dco_control_mode==DCO_CONTROL_MODES)
				{
					dco_control_mode = 0;
				}
				printf("DCO Control mode = %d\n", dco_control_mode);
			}

			if(ui_command==DCO_UI_CMD_NEXT_PARAM)
			{
				if(dco_control_mode==1 || dco_control_mode==2)
				{
					param++;
					if(param==DCO_MAX_PARAMS)
					{
						param = 0;
					}
				}
			}

			if(ui_command==DCO_UI_CMD_PREV_PARAM)
			{
				if(dco_control_mode==1 || dco_control_mode==2)
				{
					param--;
					if(param==0)
					{
						param = DCO_MAX_PARAMS-1;
					}
				}
			}
			#endif

			if(ui_command==DCO_UI_CMD_OVERSAMPLE_DOWN)
			{
				if(dco_oversample<8)
				{
					//dco_oversample*=2;
					dco_oversample++;
				}
				printf("DCO Oversample = %d\n", dco_oversample);
			}

			if(ui_command==DCO_UI_CMD_OVERSAMPLE_UP)
			{
				if(dco_oversample>1)
				{
					//dco_oversample/=2;
					dco_oversample--;
					printf("DCO Oversample = %d\n", dco_oversample);
				}
			}

			if(ui_command==DCO_UI_CMD_NEXT_WAVEFORM)
			{
				waveform++;
				if(waveform==1)
				{
					use_table256 = (uint8_t*)sinetable256;
				}
				else if(waveform==2)
				{
					use_table256 = squaretable256;
				}
				else if(waveform==3)
				{
					use_table256 = sawtable256;
				}
				else
				{
					waveform = 0;
				}
				printf("waveform set to type #%d\n",waveform);
			}

			#ifdef BOARD_WHALE
			if(ui_command==DCO_UI_CMD_NEXT_DELAY)
			{
				if(echo_dynamic_loop_current_step!=ECHO_DYNAMIC_LOOP_LENGTH_ECHO_OFF)
				{
					echo_dynamic_loop_current_step = ECHO_DYNAMIC_LOOP_LENGTH_ECHO_OFF;
				}
				else
				{
					echo_dynamic_loop_current_step = ECHO_DYNAMIC_LOOP_LENGTH_DEFAULT_STEP;
				}
				echo_dynamic_loop_length = echo_dynamic_loop_steps[echo_dynamic_loop_current_step];
				printf("echo set to step #%d, length = %d\n",echo_dynamic_loop_current_step,echo_dynamic_loop_length);
			}
			#endif

			if(ui_command==DCO_UI_CMD_RANDOMIZE_PARAMS)
			{
				//randomize all params
				if(!midi_override)
				{
					new_random_value();
					FREQ = random_value;
				}
				new_random_value();
				DOUBLE = random_value;
				PHASEamt = random_value<<8;
				new_random_value();
				ENVamt = random_value;
				new_random_value();
				RESO = random_value;
			}
			if(ui_command==DCO_UI_CMD_DRIFT_PARAMS)
			{
				drift_params++;
				if(drift_params==DRIFT_PARAMS_MAX+1)
				{
					drift_params = 0;
				}
				drift_params_cnt = 0;
				printf("drift_params = %d\n", drift_params);
			}

			if(!drift_params)
			{
				if(use_acc_or_ir_sensors==PARAMETER_CONTROL_SENSORS_ACCELEROMETER)
				{
					if(dco_control_mode==0)
					{
						if(!shift_param && (acc_res[0] > 0.3f))
						{
							printf("shift_param = 1, param++\n");
							shift_param = 1;
							param++;
							if(param==DCO_MAX_PARAMS)
							{
								param = 0;
							}
							sampleCounter = 0;
						}
						else if(!shift_param && (acc_res[0] < -0.3f))
						{
							printf("shift_param = 1, param--\n");
							shift_param = 1;
							param--;
							if(param<0)
							{
								param = DCO_MAX_PARAMS - 1;
							}
							sampleCounter = 0;
						}
						else if(shift_param && (fabs(acc_res[0]) < 0.2f))
						{
							shift_param = 0;
							printf("shift_param = 0\n");
						}
					}

					//printf("acc_res[0-2]==%f,\t%f,\t%f...\t", acc_res[0],acc_res[1],acc_res[2]);
					if(param==0) //FREQ
					{
						FREQ = (uint16_t)(acc_res[1] * DCO_FREQ_DEFAULT * 2);
						//printf("FREQ=%d\n",FREQ);
					}
					if(param==1) //DOUBLE
					{
						DOUBLE = (uint8_t)(acc_res[1] * 256);
						//printf("DOUBLE=%d\n",DOUBLE);
					}
					if(param==2) //PHASEamt
					{
						PHASEamt = (uint8_t)(acc_res[1] * 256);
						//printf("PHASEamt=%d\n",PHASEamt);
					}
					if(param==3) //ENVamt
					{
						ENVamt = (uint8_t)(acc_res[1] * 32);
						//printf("ENVamt=%d\n",ENVamt);
					}
					if(param==4) //RESO
					{
						RESO = (uint16_t)(acc_res[1] * 4096);
						//printf("RESO=%d\n",RESO);
					}
					/*
					if(param==5) //DCO_BLOCKS_ACTIVE
					{
						DCO_BLOCKS_ACTIVE = (uint8_t)(((acc_res[1] + 1.0f) * (DCO_BLOCKS_MAX-1))/2);
						printf("DCO_BLOCKS_ACTIVE=%d\n",DCO_BLOCKS_ACTIVE);
					}
					*/
					/*
					if(!erase_echo && acc_res[2] > 0.7f)
					{
						erase_echo = 2;
					}
					else
					{
						erase_echo = 0;
					}
					*/
					if(acc_res[2] > 0.7f)
					{
						//ECHO_MIXING_GAIN_MUL = 1.0f;
						//if(erase_echo!=1) printf("setting erase_echo => 1\n");
						erase_echo = 1;
					}
					else if(acc_res[0] < -0.7f)
					{
						//ECHO_MIXING_GAIN_MUL = 1.0f;
						//if(erase_echo!=2) printf("setting erase_echo => 2\n");
						erase_echo = 2;
					}
					else
					{
						//ECHO_MIXING_GAIN_MUL = 3.0f;
						//if(erase_echo!=0) printf("setting erase_echo => 0\n");
						erase_echo = 0;
					}
				}
				else //if sensors used
				{
					if(!midi_override)
					{
						FREQ = (uint16_t)(ir_res[0] * DCO_FREQ_DEFAULT * 2);
						//printf("FREQ=%d\n",FREQ);
					}

					DOUBLE = (uint8_t)(ir_res[1] * 256);
					//printf("DOUBLE=%d\n",DOUBLE);
					PHASEamt = (uint8_t)(ir_res[2] * 256);
					//printf("PHASEamt=%d\n",PHASEamt);
					ENVamt = (uint8_t)(ir_res[3] * 32);
					//printf("ENVamt=%d\n",ENVamt);
					RESO = (uint16_t)(ir_res[3] * 4096);
				}
			}

			#ifdef USE_AUTOCORRELATION
			if(autocorrelation_result_rdy && dco_control_mode==2)
			{
				//printf("ac res = %d,%d,%d,%d\n", autocorrelation_result[0], autocorrelation_result[1], autocorrelation_result[2], autocorrelation_result[3]);

				autocorrelation_result_rdy = 0;
				//FREQ = (256-autocorrelation_result[0])<<8;//(uint16_t)((acc_res[0] + 1.0f) * 440.0f * 16);
				//FREQ = (autocorrelation_result[0])<<8;//(uint16_t)((acc_res[0] + 1.0f) * 440.0f * 16);
				//int x = ac_to_freq(autocorrelation_result[0]);
				int x = (uint16_t)((acc_res[0] + 1.0f) * DCO_FREQ_DEFAULT/2 );
				if(x>100)
				{
					FREQ = x;
					printf("%d => %d\n", autocorrelation_result[0], FREQ);
				}
			}

			//DOUBLE = ADC_last_result[1] / 8;
			//DOUBLE = ADC_last_result[1] / 12;
			//DOUBLE = 220;//(uint8_t)((acc_res[1] + 1.0f) * 120); //6; //default 12?

			//PHASEamt = ADC_last_result[2] / 8;
			//PHASEamt = ADC_last_result[2] / 16;
			//PHASEamt = 110;//(uint8_t)((acc_res[2] + 1.0f) * 120); //4; //default 9?

			//ENVamt = ADC_last_result[3] * 16; /// 4;
			//ENVamt = 123;//(acc_res[2] + 1.0f) * 128;//64; //default 123
			#endif

			#ifdef DIRECT_PARAM_CONTROL
			//direct controlls by buttons
			if(event_channel_options)
			{
				event_channel_options = 0;
				param++;
				if(param==1)
				{
					printf("updating DOUBLE\n");
				}
				if(param==2)
				{
					printf("updating PHASEamt\n");
				}
				if(param==3)
				{
					printf("updating ENVamt\n");
				}
				if(param==DCO_MAX_PARAMS)
				{
					param = 0;
					printf("updating FREQ\n");
				}
				printf("FREQ=%d,DOUBLE=%d,PHASEamt=%d,ENVamt=%d\n",FREQ,DOUBLE,PHASEamt,ENVamt);
			}
			if(short_press_sequence)
			{
				if(param==0) //FREQ (16-bit)
				{
					if(short_press_volume_minus) {FREQ--; short_press_volume_minus = 0; }
					if(short_press_volume_plus) {FREQ++; short_press_volume_plus = 0; }
					if(abs(short_press_sequence)==2) FREQ += short_press_sequence*10/2;
					if(abs(short_press_sequence)==3) FREQ += short_press_sequence*100/3;
					if(abs(short_press_sequence)==4) FREQ += short_press_sequence*1000/4;
					printf("FREQ updated to %d\n", FREQ);
				}

				if(param==1) //DOUBLE (8-bit)
				{
					if(short_press_volume_minus) {DOUBLE--; short_press_volume_minus = 0; }
					if(short_press_volume_plus) {DOUBLE++; short_press_volume_plus = 0; }
					if(abs(short_press_sequence)==2) DOUBLE += short_press_sequence*10/2;
					if(abs(short_press_sequence)==3) DOUBLE += short_press_sequence*100/3;
					printf("DOUBLE updated to %d\n", DOUBLE);
				}

				if(param==2) //PHASE (8-bit)
				{
					if(short_press_volume_minus) {PHASEamt--; short_press_volume_minus = 0; }
					if(short_press_volume_plus) {PHASEamt++; short_press_volume_plus = 0; }
					if(abs(short_press_sequence)==2) PHASEamt += short_press_sequence*10/2;
					if(abs(short_press_sequence)==3) PHASEamt += short_press_sequence*100/3;
					printf("PHASEamt updated to %d\n", PHASEamt);
				}

				if(param==3) //ENVamt (8-bit)
				{
					if(short_press_volume_minus) {ENVamt--; short_press_volume_minus = 0; }
					if(short_press_volume_plus) {ENVamt++; short_press_volume_plus = 0; }
					if(abs(short_press_sequence)==2) ENVamt += short_press_sequence*10/2;
					if(abs(short_press_sequence)==3) ENVamt += short_press_sequence*100/3;
					printf("ENVamt updated to %d\n", ENVamt);
				}
				short_press_sequence = 0;
			}
			#endif

			#ifdef DEBUG_DCO
			printf("FREQ=%d,DOUBLE=%d,PHASEamt=%d,ENVamt=%d\n",FREQ,DOUBLE,PHASEamt,ENVamt);
			#endif
		}

		if (TIMING_EVERY_20_MS == 47) //50Hz
		{
			if(!midi_override && MIDI_keys_pressed)
			{
				printf("MIDI active, will receive chords\n");
				midi_override = 1;
			}

			if(midi_override && MIDI_notes_updated)
			{
				MIDI_notes_updated = 0;

				LED_W8_all_OFF();
				LED_B5_all_OFF();

				MIDI_to_LED(MIDI_last_chord[0], 1);

				FREQ = MIDI_note_to_freq(MIDI_last_chord[0]+36); //shift a couple octaves up as the FREQ parameter here works differently
			}
		}

		//if (TIMING_EVERY_250_MS == 39) //4Hz
		if (TIMING_EVERY_125_MS == 39) //8Hz
		{
			if(drift_params==4
		   || (drift_params==3 && drift_params_cnt%2==0)
		   || (drift_params==2 && drift_params_cnt%4==0)
		   || (drift_params==1 && drift_params_cnt%8==0))
			{
				//adjust all params by small random value
				new_random_value();
				if(!midi_override)
				{
					FREQ += ((int8_t)random_value)/8;
				}
				DOUBLE += ((int8_t)(random_value>>8))/8;
				PHASEamt += ((int8_t)(random_value>>16))/8;
				ENVamt += ((int8_t)(random_value>>24))/8;
				new_random_value();
				RESO += ((int16_t)random_value/128);
			}
			else if(drift_params==8
				|| (drift_params==7 && drift_params_cnt%2==0)
				|| (drift_params==6 && drift_params_cnt%4==0)
				|| (drift_params==5 && drift_params_cnt%8==0))
			{
				//randomize all params
				if(!midi_override)
				{
					new_random_value();
					FREQ = random_value;
				}
				new_random_value();
				DOUBLE = random_value;
				PHASEamt = random_value<<8;
				new_random_value();
				ENVamt = random_value;
				new_random_value();
				RESO = random_value;
			}
			#if 0
			else if(drift_params==9 || (drift_params==8 && drift_params_cnt%2==0) || (drift_params==7 && drift_params_cnt%4==0))
			{
				//randomize all params
				new_random_value();
				FREQ = random_value;
				new_random_value();
				DOUBLE = random_value;
				PHASEamt = random_value<<8;
				new_random_value();
				ENVamt = random_value;
				new_random_value();
				RESO = random_value;
			}
			else if(drift_params==12 || (drift_params==11 && drift_params_cnt%2==0) || (drift_params==10 && drift_params_cnt%4==0))
			{
				//randomize all params
				new_random_value();
				FREQ = random_value;
				new_random_value();
				DOUBLE = random_value;
				PHASEamt = random_value<<8;
				new_random_value();
				ENVamt = random_value;
				new_random_value();
				RESO = random_value;
			}
			else if(drift_params==15 || (drift_params==14 && drift_params_cnt%2==0) || (drift_params==13 && drift_params_cnt%4==0))
			{
				//randomize all params
				new_random_value();
				FREQ = random_value;
				new_random_value();
				DOUBLE = random_value;
				PHASEamt = random_value<<8;
				new_random_value();
				ENVamt = random_value;
				new_random_value();
				RESO = random_value;
			}
			else if(drift_params==18 || (drift_params==17 && drift_params_cnt%2==0) || (drift_params==16 && drift_params_cnt%4==0))
			{
				//randomize all params
				new_random_value();
				FREQ = random_value;
				new_random_value();
				DOUBLE = random_value;
				PHASEamt = random_value<<8;
				new_random_value();
				ENVamt = random_value;
				new_random_value();
				RESO = random_value;
			}
			/*else if(drift_params==21 || (drift_params==20 && drift_params_cnt%2==0) || (drift_params==19 && drift_params_cnt%4==0))
			{
				//randomize all params
				new_random_value();
				FREQ = random_value;
				new_random_value();
				DOUBLE = random_value;
				PHASEamt = random_value<<8;
				new_random_value();
				ENVamt = random_value;
				new_random_value();
				RESO = random_value;
			}*/
			#endif
			drift_params_cnt++;
		}

		if (TIMING_EVERY_20_MS == 33) //50Hz
		{
			if(limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT)
			{
				//limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //timing @20Hz, limiter will fully recover within 1 second
				limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //timing @ 50Hz, limiter will fully recover within 0.4 second

				printf("limiter_coeff=%f\n",limiter_coeff);
			}
		}

	}
}

#if 0
void DCO_Synth::v2_init()
{
	init_codec();

	//-----------------------------------------------------------------------------------
	// Gecho variables and peripherals init
	//-----------------------------------------------------------------------------------

	GPIO_LEDs_Buttons_Reset();
	//ADC_DeInit(); //reset ADC module
	ADC_configure_SENSORS(ADCConvertedValues);

	//ADC_configure_CV_GATE(CV_GATE_PC0_PB0); //PC0 stays the same as if configured for IRS1
	//int calibration_success = Calibrate_CV(); //if successful, array with results is stored in *calibrated_cv_values

	//-----------------------------------------------------------------------------------
	// Tiny-TS variables init
	//-----------------------------------------------------------------------------------

	sample_i16 = 128;
	FREQ=440;//MIDI2FREQ(key+12+(COARSEPITCH/21.3125));
	DOUBLE=12;
	PHASEamt=9;
	ENVamt=123;
	ATTACK=64;//ATTrates[12];
	RELEASE=64;//RELrates[6];
	DCO_BLOCKS_ACTIVE = 24;
	RESO = 64;

	//if(mode==1)
	//{
		//load the default sequence
		//char *seq = "a3a2a3a2c3a2d3c3a2a2a3a2g3c3f#3e3";
		//parse_notes(seq, sequence, NULL);
		//SEQUENCER_THRESHOLD = 200;
	//}

	//echo loop
	init_echo_buffer();

	ECHO_MIXING_GAIN_MUL = 1; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 4; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

	for(int i=0;i<128;i++)
	{
		MIDI_notes_to_freq[i] = NOTE_FREQ_A4 * pow(HALFTONE_STEP_COEF, i - 33);
	}
}

void DCO_Synth::v2_play_loop()
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

		if (!(envtick--)) {
			envtick=582;

			//--------- Resonant Peak ENV modulation ----------------------------------------

			//if(!DCO_mode)
			{
				PDmod=((RESO*ENVamt)>>8)+PHASEamt;
				if (PDmod>255) PDmod=255;
			}
		}

		//-------------------------------------------------------------------------
		// send sample to codec
		//-------------------------------------------------------------------------

		sample_i16 = (int16_t)DCO_output[0];
		sample_i16 += echo_buffer[echo_buffer_ptr];

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sample_i16 = (int16_t)DCO_output[1];

		//wrap in the echo loop
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

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sampleCounter++;

		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION)
		{
			sampleCounter = 0;
			seconds++;
		}


		if (TIMING_BY_SAMPLE_EVERY_10_MS==0) //100Hz
		{
			if(ADC_process_sensors()==1)
			{
				ADC_last_result[2] = -ADC_last_result[2];
				ADC_last_result[3] = -ADC_last_result[3];
				CLEAR_ADC_RESULT_RDY_FLAG;

				sensors_loop++;

				IR_sensors_LED_indicators(ADC_last_result);

				if(SET_mode==0)
				{
					FREQ = ADC_last_result[0] * 4;

					//DOUBLE = ADC_last_result[1] / 8;
					DOUBLE = ADC_last_result[1] / 12;

					//PHASEamt = ADC_last_result[2] / 8;
					PHASEamt = ADC_last_result[2] / 16;

					//if(!DCO_mode)
						ENVamt = ADC_last_result[3] * 16; /// 4;
					//}
					//else
					//{
					//	if(ADC_last_result[3] > 200)
					//	{
					//		//PDmod = ADC_last_result[3] / 2; /// 4;
					//		ENVamt = ADC_last_result[3] * 32; /// 4;
					//	}
					//	else
					//	{
					//		PDmod = 255;
					//	}
					//}
				}
				else
				{
					if(BUTTON_U1_ON)
					{
						FREQ = ADC_last_result[0] * 8;
					}
					if(BUTTON_U2_ON)
					{
						DOUBLE = ADC_last_result[1] / 12;
					}
					if(BUTTON_U3_ON)
					{
						PHASEamt = ADC_last_result[2] / 16;
					}
					if(BUTTON_U4_ON)
					{
						ENVamt = ADC_last_result[3] * 16;
					}
				}
			}

			if(BUTTON_SET_ON)
			{
				if(!button_set_held)
				{
					if(!SET_mode)
					{
						LED_SIG_ON;
						SET_mode = 1;
						button_set_held = 1;
					}
					else
					{
						LED_SIG_OFF;
						SET_mode = 0;
						button_set_held = 1;
					}
				}
			}
			else
			{
				button_set_held = 0;
			}
		}
	}
}
#endif

#if 0
//---------------------------------------------------------------------------------
// version with sequencer
//---------------------------------------------------------------------------------

void DCO_Synth::v3_play_loop()
{
	#define SEQ_MAX_NOTES 32		//maximum length = 32 notes
	#define SEQ_NOTE_DIVISION 8		//steps per note
	float sequence_notes[SEQ_MAX_NOTES * SEQ_NOTE_DIVISION];
	int seq_ptr = 0, seq_length;
	const char *sequence = "a3a3a3a3 c4a3d4c4 a3a3a3a3 d4c4e4d4 a4g4e4a4 g4e4a4g4 f#4d4a3f#4 d4a3c4b3";

	//MusicBox *chord = new MusicBox();

	seq_length = MusicBox::get_song_total_melody_notes((char*)sequence);
	parse_notes((char*)sequence, sequence_notes, NULL);

	#define SEQ_SPREAD 1
	spread_notes(sequence_notes, seq_length, SEQ_SPREAD); //spread the sequence 8 times
	seq_length *= SEQ_SPREAD;

	//FREQ = MIDI_notes_to_freq[data];

	DOUBLE=8;
	PHASEamt=217;
	ENVamt=144;
	SET_mode = 1;
	DCO_BLOCKS_ACTIVE = 2;//16;

	#define FREQ_MULTIPLIER 12

	//use_table256 = (uint8_t*)sawtable256;
	use_table256 = (uint8_t*)squaretable256;

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

		if (!(envtick--)) {
			envtick=582;

			//--------- Resonant Peak ENV modulation ----------------------------------------

			//if(!DCO_mode)
			{
				PDmod=((RESO*ENVamt)>>8)+PHASEamt;
				if (PDmod>255) PDmod=255;
			}
		}

		//-------------------------------------------------------------------------
		// send sample to codec
		//-------------------------------------------------------------------------

		sample_i16 = (int16_t)DCO_output[0];
		sample_i16 += echo_buffer[echo_buffer_ptr];

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sample_i16 = (int16_t)DCO_output[1];

		//wrap in the echo loop
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

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sampleCounter++;

		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION)
		{
			sampleCounter = 0;
			seconds++;
		}

		if (TIMING_BY_SAMPLE_EVERY_10_MS==0) //100Hz
		{
			if(ADC_process_sensors()==1)
			{
				ADC_last_result[2] = -ADC_last_result[2];
				ADC_last_result[3] = -ADC_last_result[3];
				CLEAR_ADC_RESULT_RDY_FLAG;

				sensors_loop++;

				IR_sensors_LED_indicators(ADC_last_result);

				if(SET_mode==0)
				{
					FREQ = ADC_last_result[0] * 4;

					//DOUBLE = ADC_last_result[1] / 8;
					DOUBLE = ADC_last_result[1] / 12;

					//PHASEamt = ADC_last_result[2] / 8;
					PHASEamt = ADC_last_result[2] / 16;

					//if(!DCO_mode)
						ENVamt = ADC_last_result[3] * 16; /// 4;
					//}
					//else
					//{
					//	if(ADC_last_result[3] > 200)
					//	{
					//		//PDmod = ADC_last_result[3] / 2; /// 4;
					//		ENVamt = ADC_last_result[3] * 32; /// 4;
					//	}
					//	else
					//	{
					//		PDmod = 255;
					//	}
					//}
				}
				else
				{
					if(BUTTON_U1_ON)
					{
						FREQ = ADC_last_result[0] * 8;
					}
					if(BUTTON_U2_ON)
					{
						DOUBLE = ADC_last_result[1] / 12;
					}
					if(BUTTON_U3_ON)
					{
						PHASEamt = ADC_last_result[2] / 16;
					}
					if(BUTTON_U4_ON)
					{
						ENVamt = ADC_last_result[3] * 16;
					}
				}
			}

			if(BUTTON_SET_ON)
			{
				if(!button_set_held)
				{
					if(!SET_mode)
					{
						LED_SIG_ON;
						SET_mode = 1;
						button_set_held = 1;
					}
					else
					{
						LED_SIG_OFF;
						SET_mode = 0;
						button_set_held = 1;
					}
				}
			}
			else
			{
				button_set_held = 0;
			}


		}

		//if (TIMING_BY_SAMPLE_EVERY_250_MS==0) //4Hz
		if (TIMING_BY_SAMPLE_EVERY_125_MS==0) //8Hz
		{
			//if(ADC_last_result[0] < SEQUENCER_THRESHOLD)
			//{
				if(sequence_notes[seq_ptr] > 0)
				{
					FREQ = sequence_notes[seq_ptr] * FREQ_MULTIPLIER;
				}

				if (++seq_ptr == seq_length)
				{
					seq_ptr = 0;
				}
			//}
		}

	}
}

void DCO_Synth::v4_play_loop()
//SIGNAL(PWM_INTERRUPT)
{

  uint8_t value;
  uint16_t output;

  // incorporating DuaneB's volatile fix
  uint16_t syncPhaseAcc;
  volatile uint16_t syncPhaseInc;
  uint16_t grainPhaseAcc;
  volatile uint16_t grainPhaseInc;
  uint16_t grainAmp;
  volatile uint8_t grainDecay;
  uint16_t grain2PhaseAcc;
  volatile uint16_t grain2PhaseInc;
  uint16_t grain2Amp;
  volatile uint8_t grain2Decay;

  int led_status = 0;

  while(1)
  {
	  syncPhaseAcc += syncPhaseInc;
	  if (syncPhaseAcc < syncPhaseInc) {
		// Time to start the next grain
		grainPhaseAcc = 0;
		grainAmp = 0x7fff;
		grain2PhaseAcc = 0;
		grain2Amp = 0x7fff;
		//LED_PORT ^= 1 << LED_BIT; // Faster than using digitalWrite
		led_status?LED_SIG_ON:LED_SIG_OFF;
		led_status=led_status?0:1;
	  }

	  // Increment the phase of the grain oscillators
	  grainPhaseAcc += grainPhaseInc;
	  grain2PhaseAcc += grain2PhaseInc;

	  // Convert phase into a triangle wave
	  value = (grainPhaseAcc >> 7) & 0xff;
	  if (grainPhaseAcc & 0x8000) value = ~value;
	  // Multiply by current grain amplitude to get sample
	  output = value * (grainAmp >> 8);

	  // Repeat for second grain
	  value = (grain2PhaseAcc >> 7) & 0xff;
	  if (grain2PhaseAcc & 0x8000) value = ~value;
	  output += value * (grain2Amp >> 8);

	  // Make the grain amplitudes decay by a factor every sample (exponential decay)
	  grainAmp -= (grainAmp >> 8) * grainDecay;
	  grain2Amp -= (grain2Amp >> 8) * grain2Decay;

	  // Scale output to the available range, clipping if necessary
	  output >>= 9;
	  if (output > 255) output = 255;

	  // Output to PWM (this is faster than using analogWrite)
	  //PWM_VALUE = output;

		//-------------------------------------------------------------------------
		// send sample to codec
		//-------------------------------------------------------------------------

		sample_i16 = (int16_t)output;//[0];
		sample_i16 += echo_buffer[echo_buffer_ptr];

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sample_i16 = (int16_t)output;//[1];

		//wrap in the echo loop
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

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sampleCounter++;

		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION)
		{
			sampleCounter = 0;
			seconds++;
		}

  }
}
#endif
