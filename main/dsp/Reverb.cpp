/*
 * Reverb.cpp
 *
 *  Created on: 16 Jul 2018
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

#include <Reverb.h>
#include <Accelerometer.h>
#include <Interface.h>
#include <InitChannels.h>
#include <FilteredChannels.h>
#include <hw/signals.h>
#include <hw/gpio.h>
#include <string.h>


float ADC_LPF_ALPHA = 0.2f; //up to 1.0 for max feedback = no filtering

void decaying_reverb()
{
	program_settings_reset();

	//echo_dynamic_loop_length = I2S_AUDIOFREQ; //set default value (can be changed dynamically)
	#define ECHO_LENGTH_DEFAULT ECHO_BUFFER_LENGTH
    echo_dynamic_loop_length = ECHO_LENGTH_DEFAULT;
	//echo_buffer = (int16_t*)malloc(echo_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	echo_buffer_ptr0 = 0; //reset pointer

    ECHO_MIXING_GAIN_MUL = 9; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 10; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

	#ifdef ENABLE_MCLK_AT_CHANNEL_INIT
    enable_out_clock(); //better always enabled here, otherwise getting bad glitch
	mclk_enabled = 1;
	#endif

	//codec_init(); //i2c init

	#ifdef SWITCH_I2C_SPEED_MODES
	i2c_master_deinit();
    i2c_master_init(1); //fast mode
	#endif

    //int bit_crusher_div = BIT_CRUSHER_DIV_DEFAULT;
	int bit_crusher_reverb_d = BIT_CRUSHER_REVERB_DEFAULT;

	#define DECAY_DIR_FROM		-20
	#define DECAY_DIR_TO		20
	#define DECAY_DIR_DEFAULT	1
	int decay_direction = DECAY_DIR_DEFAULT;

    REVERB_MIXING_GAIN_MUL = 9; //amount of signal to feed back to reverb loop, expressed as a fragment
    REVERB_MIXING_GAIN_DIV = 10; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

    reverb_dynamic_loop_length = /* I2S_AUDIOFREQ / */ BIT_CRUSHER_REVERB_DEFAULT; //set default value (can be changed dynamically)
	//reverb_buffer = (int16_t*)malloc(reverb_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	memset(reverb_buffer,0,REVERB_BUFFER_LENGTH*sizeof(int16_t)); //clear memory
	reverb_buffer_ptr0 = 0; //reset pointer

	channel_running = 1;
    volume_ramp = 1;

    RGB_LED_R_ON;

	while(!event_next_channel)
	{
		sampleCounter++;

		t_TIMING_BY_SAMPLE_EVERY_250_MS = TIMING_BY_SAMPLE_EVERY_250_MS;

		//if (TIMING_BY_SAMPLE_EVERY_125_MS == 24) //8Hz
		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 31) //4Hz
		{
			/*
			//printf("250-31ms\n");
			if(acc_res[1] < -0.3f)
			{
				//printf("[1]<-0.3\n");
				bit_crusher_reverb_d+=3;

				if(bit_crusher_reverb_d>BIT_CRUSHER_REVERB_MAX)
				{
					bit_crusher_reverb_d = BIT_CRUSHER_REVERB_MAX;
				}
			}
			else if(acc_res[1] < 0.1f)
			{
				//printf("[1]<-0.1\n");
				bit_crusher_reverb_d++;

				if(bit_crusher_reverb_d>BIT_CRUSHER_REVERB_MAX)
				{
					bit_crusher_reverb_d = BIT_CRUSHER_REVERB_MAX;
				}
			}
			else if(acc_res[1] < 0.4f)
			{
				//printf("[1]<0.4\n");
			}
			else
			{
				//printf("[1]else\n");
				bit_crusher_reverb_d--;

				if(bit_crusher_reverb_d<BIT_CRUSHER_REVERB_MIN)
				{
					bit_crusher_reverb_d = BIT_CRUSHER_REVERB_MIN;
				}
			}

			reverb_dynamic_loop_length = bit_crusher_reverb_d;
			//reverb_dynamic_loop_length = I2S_AUDIOFREQ / bit_crusher_reverb_d;
			*/

			bit_crusher_reverb_d += decay_direction;

			if(bit_crusher_reverb_d>BIT_CRUSHER_REVERB_MAX)
			{
				bit_crusher_reverb_d = BIT_CRUSHER_REVERB_MAX;
			}
			if(bit_crusher_reverb_d<BIT_CRUSHER_REVERB_MIN)
			{
				bit_crusher_reverb_d = BIT_CRUSHER_REVERB_MIN;
			}

			reverb_dynamic_loop_length = bit_crusher_reverb_d;
		}

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 29) //4Hz
		{
			if(event_channel_options) //stops decay
			{
				decay_direction = 0; //DECAY_DIR_DEFAULT;
				event_channel_options = 0;
			}
			if(short_press_volume_plus) //as delay length decreases, perceived tone goes up
			{
				decay_direction--;
				if(decay_direction < DECAY_DIR_FROM)
				{
					decay_direction = DECAY_DIR_FROM;
				}
				short_press_volume_plus = 0;
			}
			if(short_press_volume_minus) //as delay length increases, perceived tone goes low
			{
				decay_direction++;
				if(decay_direction > DECAY_DIR_TO)
				{
					decay_direction = DECAY_DIR_TO;
				}
				short_press_volume_minus = 0;
			}
		}

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 27) //4Hz
		{
			if(acc_res[1]>0)
			{
				ADC_LPF_ALPHA = acc_res[1] / 2 + 0.5f;
			}
			else if(acc_res[1]>-0.2f)  { ADC_LPF_ALPHA = 0.4f; }
			else if(acc_res[1]>-0.3f)  { ADC_LPF_ALPHA = 0.3f; }
			else if(acc_res[1]>-0.45f) { ADC_LPF_ALPHA = 0.2f; }
			else if(acc_res[1]>-0.65f) { ADC_LPF_ALPHA = 0.1f; }
			else if(acc_res[1]>-0.85f) { ADC_LPF_ALPHA = 0.05f;}
		}

		/*
		//if (TIMING_BY_SAMPLE_EVERY_125_MS == 26) //8Hz
		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 26) //4Hz
		{
			printf("bit_crusher_reverb_d=%d,len=%d\n",bit_crusher_reverb_d,reverb_dynamic_loop_length);
		}
		*/

		i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);

		/*
		if(sampleCounter%bit_crusher_div==0)
		{
			ADC_sample0 = ADC_sample;
		}
		else
		{
			ADC_sample = ADC_sample0;
		}
		*/

		//sample_mix = (int16_t)ADC_sample;
		//sample_mix = sample_mix * SAMPLE_VOLUME_REVERB; //apply volume
		//LPF enabled
		sample_lpf[0] = sample_lpf[0] + ADC_LPF_ALPHA * ((float)((int16_t)ADC_sample) - sample_lpf[0]);
		sample_mix = sample_lpf[0] * SAMPLE_VOLUME_REVERB; //apply volume


		//sample32 = (add_echo((int16_t)(sample_mix))) << 16;
		//sample32 = (add_reverb((int16_t)(sample_mix))) << 16;
		sample32 = (add_echo(add_reverb((int16_t)(sample_mix)))) << 16;

        //sample_mix = (int16_t)(ADC_sample>>16);
        //sample_mix = sample_mix * SAMPLE_VOLUME_REVERB; //apply volume
		//LPF enabled
		sample_lpf[1] = sample_lpf[1] + ADC_LPF_ALPHA * ((float)((int16_t)(ADC_sample>>16)) - sample_lpf[1]);
		sample_mix = sample_lpf[1] * SAMPLE_VOLUME_REVERB; //apply volume

        //sample32 += add_echo((int16_t)(sample_mix));
        //sample32 += add_reverb((int16_t)(sample_mix));
        sample32 += add_echo(add_reverb((int16_t)(sample_mix)));

		if(add_beep)
		{
			if(sampleCounter & (1<<add_beep))
			{
				sample32 += (100 + (100<<16));
			}
		}

		i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);

        /*
		//if (TIMING_BY_SAMPLE_EVERY_50_MS == 33) //20Hz
		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 31) //4Hz
		{
			acc.getresults_g(acc_res1);
			//printf("acc_res=%f,%f,%f\n",acc_res1[0],acc_res1[1],acc_res1[2]);
		}
		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 32) //4Hz
		{
			acc.getresults_g(acc_res2);
			//printf("acc_res=%f,%f,%f\n",acc_res2[0],acc_res2[1],acc_res2[2]);
		}
		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 33) //4Hz
		{
			acc.getresults_g(acc_res3);
			//printf("acc_res=%f,%f,%f\n",acc_res3[0],acc_res3[1],acc_res3[2]);
		}

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 34) //4Hz
		{
			acc_stable_cnt = 0;
			for(acc_i=0;acc_i<3;acc_i++)
			{
				if(fabs(acc_res1[acc_i]-acc_res2[acc_i])<0.1
				&& fabs(acc_res2[acc_i]-acc_res3[acc_i])<0.1
				&& fabs(acc_res1[acc_i]-acc_res3[acc_i])<0.1)
				{
					//acc_res[acc_i] = acc_res3[acc_i];
					acc_stable_cnt++;
					//printf("acc.param[%d] stable... ", acc_i);
				}
				//else
				//{
				//	printf("acc.param[%d] NOT STABLE... ", acc_i);
				//}
			}

			if(acc_stable_cnt==3)
			{
				//acc_res[0] = acc_res2[0];
				//acc_res[1] = acc_res2[1];
				//acc_res[2] = acc_res2[2];
				memcpy(acc_res,acc_res2,sizeof(acc_res2));
			}

			//printf("\n");
		}
		*/

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 35) //4Hz
		{
			if(!echo_dynamic_loop_current_step) //if a fixed delay is not set, use accelerometer
			{
				if(acc_res[2] < -0.8f)
				{
					echo_dynamic_loop_length = ECHO_LENGTH_DEFAULT;
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
		}

		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 37) //4Hz
		{
			if(echo_dynamic_loop_length0 < echo_dynamic_loop_length)
			{
				//printf("Cleaning echo buffer from echo_dynamic_loop_length0 to echo_dynamic_loop_length\n");
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

		//if (TIMING_BY_SAMPLE_EVERY_50_MS == 33) //20Hz
		if (TIMING_BY_SAMPLE_EVERY_10_MS == 33) //100Hz
		{
			if(limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT)
			{
				//limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //timing @20Hz, limiter will fully recover within 1 second
				limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //timing @ 100Hz, limiter will fully recover within 0.2 second
			}
		}
	}
}

