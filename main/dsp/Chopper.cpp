/*
 * Chopper.cpp
 *
 *  Created on: 24 Dec 2018
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

#include <Chopper.h>
#include <Accelerometer.h>
#include <InitChannels.h>
#include <MusicBox.h>

#define CHOPPER_ECHO

void channel_chopper()
{
	//--------------------------------------------------------------------------
	//defines, constants and variables

	uint16_t t_TIMING_BY_SAMPLE_EVERY_250_MS; //optimization of timing counters

	float chopperEnv1 = 0.0f;
	float chopperEnv2 = 1.0f;
	unsigned int chopperTimer = I2S_AUDIOFREQ / 16;
	unsigned int chopperCounter = 0;

	//there is an external sampleCounter variable already, it will be used to timing and counting seconds
	sampleCounter = 0;
	seconds = 0;

	#ifdef CHOPPER_ECHO
	//initialize echo without using init_echo_buffer() which allocates too much memory
	echo_dynamic_loop_length = ECHO_BUFFER_LENGTH_DEFAULT; //set default value (can be changed dynamically)
	memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	echo_buffer_ptr0 = 0; //reset pointer
	#endif

	program_settings_reset();

	#ifdef SWITCH_I2C_SPEED_MODES
	i2c_master_deinit();
    i2c_master_init(1); //fast mode
	#endif

    printf("Chopper: function loop starting\n");

	channel_running = 1;
    volume_ramp = 1;

	#ifdef BOARD_WHALE
    RGB_LED_G_ON;
    RGB_LED_B_ON;
	#endif

	while(!event_next_channel)
	{
		sampleCounter++;

		chopperCounter++;

		t_TIMING_BY_SAMPLE_EVERY_250_MS = TIMING_EVERY_250_MS;

		if(chopperCounter==chopperTimer)
		{
			chopperEnv1 = 1.0f;
			chopperEnv2 = 0.0f;
		}
		else if(chopperCounter>=chopperTimer * 2)
		{
			chopperEnv1 = 0.0f;
			chopperEnv2 = 1.0f;

			chopperCounter = 0;
		}

		//here actually two seconds as one increment of sampleCounter happens once per both channels
		if (TIMING_BY_SAMPLE_1_SEC) //one full second passed
		{
			seconds++;
			sampleCounter = 0;
		}

		if (TIMING_EVERY_100_MS == 43) //10Hz
		{
			//x-axis sets chopper timing

			if(acc_res[1] < -0.8f)
			{
				chopperTimer = I2S_AUDIOFREQ / 64;
			}
			else if(acc_res[1] < -0.65f)
			{
				chopperTimer = I2S_AUDIOFREQ / 32;
			}
			else if(acc_res[1] < -0.4f)
			{
				chopperTimer = I2S_AUDIOFREQ / 16;
			}
			else if(acc_res[1] < -0.2f)
			{
				chopperTimer = I2S_AUDIOFREQ / 8;
			}
			else if(acc_res[1] < 0.2f)
			{
				chopperTimer = I2S_AUDIOFREQ / 6;
			}
			else if(acc_res[1] < 0.4f)
			{
				chopperTimer = I2S_AUDIOFREQ / 4;
			}
			else if(acc_res[1] < 0.65f)
			{
				chopperTimer = I2S_AUDIOFREQ / 3;
			}
			else if(acc_res[1] < 0.8f)
			{
				chopperTimer = I2S_AUDIOFREQ / 2;
			}
			else
			{
				chopperTimer = I2S_AUDIOFREQ;
			}
		}

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 97) //4Hz
		{
			if(acc_res[2] < -0.8f)
			{
				echo_dynamic_loop_length = ECHO_BUFFER_LENGTH;
			}
			else if((acc_res[2] > -0.7f) && (acc_res[2] < -0.4f))
			{
				echo_dynamic_loop_length = I2S_AUDIOFREQ;
			}
			else if((acc_res[2] > -0.3f) && (acc_res[2] < 0.3f))
			{
				echo_dynamic_loop_length = I2S_AUDIOFREQ / 2;
			}
			else if((acc_res[2] > 0.4f) && (acc_res[2] < 0.8f))
			{
				echo_dynamic_loop_length = I2S_AUDIOFREQ / 4;
			}
			else if(acc_res[2] > 0.9f)
			{
				echo_dynamic_loop_length = I2S_AUDIOFREQ / 8;
				//echo_dynamic_loop_length = 0;
			}
		}
		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 99) //4Hz
		{
			if(echo_dynamic_loop_length0 < echo_dynamic_loop_length)
			{
				printf("Cleaning echo buffer from echo_dynamic_loop_length0 to echo_dynamic_loop_length\n");
				//echo_length_updated = echo_dynamic_loop_length0;
				//echo_dynamic_loop_length0 = echo_dynamic_loop_length;
			//}
			//if(echo_length_updated)
			//{
				//memset(echo_buffer+echo_dynamic_loop_length0*sizeof(int16_t),0,(echo_dynamic_loop_length-echo_dynamic_loop_length0)*sizeof(int16_t)); //clear memory
				//echo_length_updated = 0;
				echo_skip_samples = echo_dynamic_loop_length - echo_dynamic_loop_length0;
				echo_skip_samples_from = echo_dynamic_loop_length0;
			}
			echo_dynamic_loop_length0 = echo_dynamic_loop_length;
		}

		if (TIMING_EVERY_20_MS == 33) //50Hz
		{
			if(limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT)
			{
				//limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //timing @20Hz, limiter will fully recover within 1 second
				limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //timing @ 50Hz, limiter will fully recover within 0.4 second
			}
		}

		//always recording
		//if(acc_res[0] > 0.4f)
		{
			//new_mixing_vol = 1.0f + (acc_res[0] - 0.4f) * 1.6f;

			i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
        }

        //mix samples for all voices (left channel)
		//sample_mix = 0;
		sample_mix = (int16_t)ADC_sample;

		//if (TIMING_EVERY_100_MS == 45) //10Hz
		//{

		//}

		sample_mix = sample_mix /* * SAMPLE_VOLUME / 12.0f*/ * chopperEnv1; //apply volume

		#ifdef CHOPPER_ECHO
		sample32 = (add_echo((int16_t)(sample_mix))) << 16;
		#else
		sample32 = ((int16_t)(sample_mix)) << 16;
		#endif

        //mix samples for all voices (right channel)
        //sample_mix = 0;
        //sample_mix = (int16_t)(ADC_sample>>16);

        sample_mix = sample_mix /* * SAMPLE_VOLUME / 12.0f */ * chopperEnv2; //apply volume

        #ifdef CHOPPER_ECHO
        sample32 += add_echo((int16_t)(sample_mix));
		#else
        sample32 += (int16_t)(sample_mix);
		#endif

		#ifdef BOARD_WHALE
		if(sampleCounter & (1<<add_beep))
		{
			if(add_beep)
			{
				sample32 += (100 + (100<<16));
			}
		}
		#endif

        //sample32 = 0; //ADC_sample; //test bypass all effects
        i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
        //i2s_push_sample(I2S_NUM, (char *)&ADC_sample, portMAX_DELAY); //test bypass

	}	//end while(1)
}
