/*
 * DrumKit.cpp
 *
 *  Created on: 26 Nov 2016
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

#include <DrumKit.h>
#include <Accelerometer.h>
#include <InitChannels.h>
#include <hw/leds.h>

#define DRUM_KIT_ECHO

int drum_samples_ptr[DRUM_CHANNELS_MAX]; 	//pointers for currently playing sample
int drum_trigger[DRUM_CHANNELS_MAX];		//triggers for execution of each drum
int drum_lengths[DRUM_SAMPLES];				//lengths of samples in bytes
int drum_bases[DRUM_SAMPLES];				//base addresses of samples

float DRUM_THRESHOLD_ON;
float DRUM_THRESHOLD_OFF;

int drum_condition(int drum_no, int direction)
{
	//printf("drum_condition(%d, %d): acc_res[0] = %f, acc_res[2] = %f\n", drum_no, direction, acc_res[0], acc_res[2]);

	#ifdef BOARD_WHALE
	//here is defined which drum is where
	if(drum_no==0) { if(direction) { return acc_res[0] < -DRUM_THRESHOLD_ON; } else { return acc_res[0] > -DRUM_THRESHOLD_OFF; } }
	if(drum_no==3) { if(direction) { return acc_res[0] >  DRUM_THRESHOLD_ON; } else { return acc_res[0] <  DRUM_THRESHOLD_OFF; } }
	if(drum_no==2) { if(direction) { return acc_res[2] < -DRUM_THRESHOLD_ON; } else { return acc_res[2] > -DRUM_THRESHOLD_OFF; } }
	if(drum_no==1) { if(direction) { return acc_res[2] >  DRUM_THRESHOLD_ON; } else { return acc_res[2] <  DRUM_THRESHOLD_OFF; } }
	#endif

	#ifdef BOARD_GECHO
	if(use_acc_or_ir_sensors==PARAMETER_CONTROL_SENSORS_ACCELEROMETER)
	{
		if(drum_no==0) { if(direction) { return acc_res[0] < -DRUM_THRESHOLD_ON; } else { return acc_res[0] > -DRUM_THRESHOLD_OFF; } }
		if(drum_no==3) { if(direction) { return acc_res[0] >  DRUM_THRESHOLD_ON; } else { return acc_res[0] <  DRUM_THRESHOLD_OFF; } }
		if(drum_no==2) { if(direction) { return acc_res[1] < -DRUM_THRESHOLD_ON; } else { return acc_res[1] > -DRUM_THRESHOLD_OFF; } }
		if(drum_no==1) { if(direction) { return acc_res[1] >  DRUM_THRESHOLD_ON; } else { return acc_res[1] <  DRUM_THRESHOLD_OFF; } }
	}
	else
	{
		if(drum_no==0) { if(direction) { return SENSOR_THRESHOLD_RED_5; } else { return !SENSOR_THRESHOLD_RED_3; } }
		if(drum_no==3) { if(direction) { return SENSOR_THRESHOLD_ORANGE_3; } else { return !SENSOR_THRESHOLD_ORANGE_1; } }
		if(drum_no==2) { if(direction) { return SENSOR_THRESHOLD_BLUE_3; } else { return !SENSOR_THRESHOLD_BLUE_5; } }
		if(drum_no==1) { if(direction) { return SENSOR_THRESHOLD_WHITE_4; } else { return !SENSOR_THRESHOLD_WHITE_6; } }
	}

	#endif

	return 0;
}

void channel_drum_kit()
{
	//--------------------------------------------------------------------------
	//defines, constants and variables

	DRUM_THRESHOLD_ON = global_settings.DRUM_THRESHOLD_ON;
	DRUM_THRESHOLD_OFF = global_settings.DRUM_THRESHOLD_OFF;

	//uint16_t t_TIMING_BY_SAMPLE_EVERY_250_MS; //optimization of timing counters

	//there is an external sampleCounter variable already, it will be used to timing and counting seconds
	sampleCounter = 0;
	seconds = 0;

	channel_init(9, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0); //map the drums sample from Flash to RAM

	//this overrides echo parameters set in channel_init()
	#ifdef DRUM_KIT_ECHO
	memset(echo_buffer,0,ECHO_BUFFER_LENGTH*sizeof(int16_t)); //clear memory
	echo_buffer_ptr0 = 0; //reset pointer
	echo_dynamic_loop_current_step = ECHO_DYNAMIC_LOOP_STEPS - 1; //last one = no echo
	echo_dynamic_loop_length = echo_dynamic_loop_steps[echo_dynamic_loop_current_step];
	printf("channel_drum_kit(): echo_dynamic_loop_current_step = %d, echo_dynamic_loop_length = %d\n", echo_dynamic_loop_current_step, echo_dynamic_loop_length);
	#endif

	#ifdef SWITCH_I2C_SPEED_MODES
	i2c_master_deinit();
    i2c_master_init(1); //fast mode
	#endif

	for (int i=0;i<DRUM_CHANNELS_MAX;i++)	//init all values
	{
		drum_samples_ptr[i] = -1;			//reset to "stopped" state
		drum_trigger[i] = 0;				//reset to "released" state
	}

	drum_lengths[0] = global_settings.DRUM_LENGTH1 / 2;		//each sample is 2 bytes
	drum_lengths[1] = global_settings.DRUM_LENGTH2 / 2;
	drum_lengths[2] = global_settings.DRUM_LENGTH3 / 2;
	drum_lengths[3] = global_settings.DRUM_LENGTH4 / 2;

	drum_bases[0] = 0;
	drum_bases[1] = drum_bases[0] + drum_lengths[0];
	drum_bases[2] = drum_bases[1] + drum_lengths[1];
	drum_bases[3] = drum_bases[2] + drum_lengths[2];

	#ifdef BOARD_WHALE
	RGB_LED_R_OFF;
	RGB_LED_G_OFF;
	RGB_LED_B_OFF;
	Delay(100);
	RGB_LED_R_ON;
	Delay(100);
	RGB_LED_R_OFF;
	RGB_LED_G_ON;
	Delay(100);
	RGB_LED_G_OFF;
	RGB_LED_B_ON;
	Delay(100);
	RGB_LED_B_OFF;
	#endif

	printf("DrumKit: function loop starting\n");

	channel_running = 1;
    volume_ramp = 1;

	while(!event_next_channel)
	{
		sampleCounter++;

		/*
		sample_f[0] = ( (float)(32768 - (int16_t)mixed_sample_buffer[mixed_sample_buffer_ptr_L++]) / 32768.0f) * mixed_sample_volume;// * noise_volume * noise_boost_by_sensor;

		if(mixed_sample_buffer_ptr_L == MIXED_SAMPLE_BUFFER_LENGTH)
		{
			mixed_sample_buffer_ptr_L = 0;
		}
		*/

		//sample_f[0] = 0;//(float)((int16_t)ADC_sample) * OpAmp_ADC12_signal_conversion_factor;

		/*
		sample_f[1] = ( (float)(32768 - (int16_t)mixed_sample_buffer[mixed_sample_buffer_ptr_R++]) / 32768.0f) * mixed_sample_volume;// * noise_volume * noise_boost_by_sensor;

		if(mixed_sample_buffer_ptr_R == MIXED_SAMPLE_BUFFER_LENGTH)
		{
			mixed_sample_buffer_ptr_R = 0;
		}
		*/

		//sample_f[1] = 0;//(float)((int16_t)(ADC_sample >> 16)) * OpAmp_ADC12_signal_conversion_factor;

		sample_mix = 0;

		for (int i=0;i<DRUM_CHANNELS_MAX;i++)
		{
			if (drum_samples_ptr[i] >= 0) //if playing
			{
				//translate from 16-bit binary format to float
				//sample_f[1] += ((float)(32768 - (int16_t)mixed_sample_buffer[drum_bases[i]+drum_samples_ptr[i]]) / 32768.0f) * SAMPLE_VOLUME;

				sample_mix += mixed_sample_buffer[drum_bases[i]+drum_samples_ptr[i]] / 4;

				drum_samples_ptr[i]++;						//move on to the next sample
				if (drum_samples_ptr[i]==drum_lengths[i])	//if reached the end of the sample
				{
					drum_samples_ptr[i] = -1;				//reset the pointer to "stopped" state
				}
			}
		}

		//sample_f[0] = sample_f[1];

		//t_TIMING_BY_SAMPLE_EVERY_250_MS = TIMING_EVERY_250_MS;

		//here actually two seconds as one increment of sampleCounter happens once per both channels
		if (TIMING_BY_SAMPLE_1_SEC) //one full second passed
		{
			//printf("TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION\n");
			seconds++;
			sampleCounter = 0;
		}

		if (TIMING_EVERY_4_MS == 13)
		{
			for (int i=0;i<DRUM_CHANNELS_MAX;i++)
			{
				if (!drum_trigger[i] && drum_condition(i,1)) //if not playing and 3D motion threshold exceeded, start playing
				{
					printf("trigger[%d]\n",i);
					drum_trigger[i] = 1;		//flip the trigger
					drum_samples_ptr[i] = 0;	//set pointer to the beginning of the sample

					#ifdef BOARD_WHALE
					if(i==0) RGB_LED_R_ON;
					if(i==3) RGB_LED_G_ON;
					if(i==2) RGB_LED_B_ON;
					if(i==1) { RGB_LED_R_ON;RGB_LED_G_ON;RGB_LED_B_ON;}
					#endif
				}
				if (drum_trigger[i] && drum_condition(i,0))	//if playing and 3D motion threshold lowered, allow restart
				{
					printf("release[%d]\n",i);
					drum_trigger[i] = 0;		//release trigger

					#ifdef BOARD_WHALE
					if(i==0) RGB_LED_R_OFF;
					if(i==3) RGB_LED_G_OFF;
					if(i==2) RGB_LED_B_OFF;
					if(i==1) { RGB_LED_R_OFF;RGB_LED_G_OFF;RGB_LED_B_OFF;}
					#endif
				}
			}

/*
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
*/		}

//		if (t_TIMING_BY_SAMPLE_EVERY_250_MS == 97) //4Hz
//		{
/*			if(acc_res[2] < -0.8f)
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
*/
//		}
/*
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
*/

/*
		if(short_press_volume_plus)
		{
			short_press_volume_plus = 0;
		}
		if(short_press_volume_minus)
		{
			short_press_volume_minus = 0;
		}
*/
		if(event_channel_options)
		{
			echo_dynamic_loop_current_step++;
			if(echo_dynamic_loop_current_step==ECHO_DYNAMIC_LOOP_STEPS)
			{
				echo_dynamic_loop_current_step = 0;
			}

			echo_dynamic_loop_length = echo_dynamic_loop_steps[echo_dynamic_loop_current_step];

			/*
			if(echo_dynamic_loop_length==0)
			{
				PROG_add_echo = false; //turn echo off altogether
			}
			else
			{
				PROG_add_echo = true; //turn echo back on
			}
			*/

			event_channel_options = 0;
		}


		//x-axis enables recording - when triggered, sample the data into the grain buffer (left channel first)
		//if (ADC_last_result[1] > IR_sensors_THRESHOLD_1)

		//always recording
		//if(acc_res[0] > 0.4f)
		{
			//new_mixing_vol = 1.0f + (acc_res[0] - 0.4f) * 1.6f;

			i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
//			grain_left[sampleCounter1] = (int16_t)ADC_sample;
        }

        //mix samples for all voices (left channel)
		//sample_mix = 0;
		//sample_mix = (int16_t)ADC_sample;
		//sample_mix += (int16_t)(sample_f[0]*MAIN_VOLUME*SAMPLE_VOLUME);

		//if (TIMING_BY_SAMPLE_EVERY_100_MS == 45) //10Hz
		//{
			//printf("Granular: TIMING_BY_SAMPLE_EVERY_100_MS == 45, acc_res[0] = %f, current_chord = %d\n", acc_res[0], current_chord);

		//}

		//sample_mix = sample_mix /* * SAMPLE_VOLUME / 12.0f*/ * chopperEnv1; //apply volume

		#ifdef DRUM_KIT_ECHO
		sample32 = echo_dynamic_loop_length ? ((add_echo((int16_t)(sample_mix))) << 16) : (((int16_t)(sample_mix/2)) << 16);
		#else
		sample32 = ((int16_t)(sample_mix)) << 16;
		#endif

		//sample data into the right channel grain buffer, when x-axis triggered
		//if (ADC_last_result[1] > IR_sensors_THRESHOLD_1)

		//always recording
		//if(acc_res[0] > 0.4f)
        //{
//			grain_right[sampleCounter2] = (int16_t)(ADC_sample>>16);
        //}

        //mix samples for all voices (right channel)
        //sample_mix = 0;
        //sample_mix = (int16_t)(ADC_sample>>16);
		//sample_mix += (int16_t)(sample_f[1]*MAIN_VOLUME*SAMPLE_VOLUME);

		//sample_mix = sample_mix /* * SAMPLE_VOLUME / 12.0f */ * chopperEnv2; //apply volume

        #ifdef DRUM_KIT_ECHO
        sample32 += echo_dynamic_loop_length ? (add_echo((int16_t)(sample_mix))) : (sample32 += (int16_t)(sample_mix/2));
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
