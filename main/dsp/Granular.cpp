/*
 * Granular.cpp
 *
 *  Created on: 19 Jan 2018
 *      Author: mario
 *
 *  As explained in the coding tutorial: http://gechologic.com/granular-sampler
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

#include <Granular.h>
#include <Accelerometer.h>
#include <InitChannels.h>
#include <MusicBox.h>

#define GRANULAR_ECHO

//--------------------------------------------------------------------------
//defines, constants and variables

#define GRAIN_SAMPLES     (I2S_AUDIOFREQ/2)	 //memory will be allocated for up to half second grain
#define GRAIN_LENGTH_MIN  (I2S_AUDIOFREQ/16) //one 16th of second will be shortest allowed grain length

#define GRAIN_VOICES_MIN  3		//one basic chord
#define GRAIN_VOICES_MAX  33	//voices per stereo channel (42 looks like maximum)
#define GRAIN_VOICES_MAX_S  24	//for simple version of granular

//define some handy constants
#define FREQ_C5 523.25f	//frequency of note C5
#define FREQ_Eb5 622.25f //frequency of note D#5
#define FREQ_E5 659.25f //frequency of note E5
#define FREQ_G5 783.99f //frequency of note G5

#define MAJOR_THIRD (FREQ_E5/FREQ_C5)	//ratio between notes C and E
#define MINOR_THIRD (FREQ_Eb5/FREQ_C5)	//ratio between notes C and Eb/D#
#define PERFECT_FIFTH (FREQ_G5/FREQ_C5)	//ratio between notes C and G

//we need a separate counter for right channel to process it independently from the left
unsigned int sampleCounter1;
unsigned int sampleCounter2;

//variables to hold grain playback pitch values
float freq[GRAIN_VOICES_MAX], step[GRAIN_VOICES_MAX];

int current_chord = 1; //1=major, -1=minor chord
float detune_coeff = 0;

MusicBox *gran_chord = NULL;

#if 0
void granular_sampler(int selected_song)
{

	int16_t *grain_left, *grain_right;	//two buffers for stereo samples

	int grain_voices = GRAIN_VOICES_MIN;	//voices currently active
	int grain_length = GRAIN_SAMPLES;		//amount of samples to process from the grain
	int grain_start = 0;	//position where the grain starts within the buffer
	int grain_voices_used = GRAIN_VOICES_MAX;

	//there is an external sampleCounter variable already, it will be used to timing and counting seconds
	sampleCounter = 0;
	seconds = 0;

	//define frequencies for all voices

	if(selected_song)
	{
		//#define GRAIN_VOICES_MAX  27
		grain_voices_used = 27;
		float base_chord_27[3] = {FREQ_C5,FREQ_C5*MAJOR_THIRD,FREQ_C5*PERFECT_FIFTH};
		update_grain_freqs(freq, base_chord_27, grain_voices_used, 0); //update all at once

		//grain_voices_used = 24;
		//float base_chord_24[3] = {FREQ_C5,FREQ_C5*MAJOR_THIRD,FREQ_C5*PERFECT_FIFTH};
		//update_grain_freqs(freq, base_chord_24, grain_voices_used, 0); //update all at once
	}
	else
	{
		//#define GRAIN_VOICES_MAX  33
		grain_voices_used = 33;
		float base_chord_33[3] = {FREQ_C5,FREQ_C5*MAJOR_THIRD,FREQ_C5*PERFECT_FIFTH};
		update_grain_freqs(freq, base_chord_33, grain_voices_used, 0); //update all at once
	}

	//-------------------------------------------------------------------
	MusicBox *chord = NULL;
	if(selected_song>=1)
	{
		//generate chords for the selected song
		chord = new MusicBox(selected_song);

		int notes_parsed = parse_notes(chord->base_notes, chord->bases_parsed, chord->led_indicators, chord->midi_notes);

		if(notes_parsed != chord->total_chords * NOTES_PER_CHORD) //integrity check
		{
			//we don't have correct amount of notes
			if(notes_parsed < chord->total_chords * NOTES_PER_CHORD) //some notes missing
			{
				do //let's add zeros, so it won't break the playback
				{
					chord->bases_parsed[notes_parsed++] = 0;
				}
				while(notes_parsed < chord->total_chords * NOTES_PER_CHORD);
			}
		}
		//chord->generate(CHORD_MAX_VOICES); //no need to expand bases
	}
	//-------------------------------------------------------------------

	//allocate memory for grain buffers
	grain_left = (int16_t*)malloc(GRAIN_SAMPLES * sizeof(int16_t));
	grain_right = (int16_t*)malloc(GRAIN_SAMPLES * sizeof(int16_t));
	memset(grain_left, 0, GRAIN_SAMPLES * sizeof(int16_t));
	memset(grain_right, 0, GRAIN_SAMPLES * sizeof(int16_t));

	//reset counters
	sampleCounter1 = 0;
	sampleCounter2 = 0;

	//reset pointers
	for(int i=1;i<grain_voices_used;i++)
	{
		step[i] = 0;
	}

	//initialize echo without using init_echo_buffer() which allocates too much memory
	#define GRANULAR_SAMPLER_ECHO_BUFFER_SIZE I2S_AUDIOFREQ //half second (buffer is shared for both stereo channels)
	echo_dynamic_loop_length = GRANULAR_SAMPLER_ECHO_BUFFER_SIZE; //set default value (can be changed dynamically)
	//echo_buffer = (int16_t*)malloc(echo_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	echo_buffer_ptr0 = 0; //reset pointer
	int freqs_updating = 0;

	program_settings_reset();

	enable_I2S_MCLK_clock();
	codec_init(); //i2c init

	printf("Granular: function loop starting\n");

	channel_running = 1;
    volume_ramp = 1;

	while(!event_next_channel)
	{
		sampleCounter++;

		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION) //one full second passed
		{
			printf("Granular: TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION\n");
			seconds++;
			sampleCounter = 0;

			if(selected_song)
			{
				//replace the grain frequencies by chord
				seconds %= chord->total_chords; //wrap around to total number of chords
				freqs_updating = 2;
			}
		}
// *
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 		if(TIMING_BY_SAMPLE_EVERY_1_MS)
		{
			if(freqs_updating>0)
			{
				update_grain_freqs(freq, chord->bases_parsed + seconds*3, grain_voices_used, 3-freqs_updating); //update one group only
				freqs_updating--;
			}
		}
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//* /

		//x-axis sets offset of right stereo channel against left
		//if (ADC_last_result[1] > IR_sensors_THRESHOLD_9)
		if(acc_res[0] > 0.9f)
		{
			sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 2;
		}
		else //if (ADC_last_result[1] > IR_sensors_THRESHOLD_8)
		if(acc_res[0] > 0.8f)
		{
			sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 3;
		}
		else //if (ADC_last_result[1] > IR_sensors_THRESHOLD_6)
		if(acc_res[0] > 0.7f)
		{
			sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 4;
		}
		else //if (ADC_last_result[1] > IR_sensors_THRESHOLD_4)
		if(acc_res[0] > 0.6f)
		{
			sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 6;
		}
		else //if (ADC_last_result[1] > IR_sensors_THRESHOLD_2)
		{
			sampleCounter2 = sampleCounter1;
		}

// *
 	//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
		//increment counters and wrap around if completed full cycle
		if (++sampleCounter1 >= (unsigned int)grain_length)
    	{
        	sampleCounter1 = 0;
    	}
		if (++sampleCounter2 >= (unsigned int)grain_length)
		{
			sampleCounter2 = 0;
		}
 	//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * /

		//x-axis enables recording - when triggered, sample the data into the grain buffer (left channel first)
		//if (ADC_last_result[1] > IR_sensors_THRESHOLD_1)
		if(acc_res[0] > 0.4f)
		{
			//new_mixing_vol = 1.0f + (acc_res[0] - 0.4f) * 1.6f;

			i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
			grain_left[sampleCounter1] = (int16_t)ADC_sample;
        }

        //mix samples for all voices (left channel)
		//sample_mix = 0;
		sample_mix = (int16_t)ADC_sample;

// *
//		 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
		for(int i=0;i<grain_voices;i++)
		{
			step[i] += freq[i] / FREQ_C5;
			if ((int)step[i]>=grain_length)
			{
				step[i] = grain_start;
			}
			sample_mix += grain_left[(int)step[i]];
		}
//		 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//* /
		//sample_mix = sample_mix * SAMPLE_VOLUME / 12.0f; //apply volume

		sample32 = (add_echo((int16_t)(sample_mix))) << 16;

		//sample data into the right channel grain buffer, when x-axis triggered
		//if (ADC_last_result[1] > IR_sensors_THRESHOLD_1)
		if(acc_res[0] > 0.4f)
        {
			grain_right[sampleCounter2] = (int16_t)(ADC_sample>>16);
        }

        //mix samples for all voices (right channel)
        //sample_mix = 0;
        sample_mix = (int16_t)(ADC_sample>>16);

// *
//   		 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
        for(int i=0;i<grain_voices;i++)
    	{
    		sample_mix += grain_right[(int)step[i]];
    	}
//		 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//* /

        //sample_mix = sample_mix * SAMPLE_VOLUME / 12.0f; //apply volume
        sample32 += add_echo((int16_t)(sample_mix));

		if(sampleCounter & (1<<add_beep))
		{
			if(add_beep)
			{
				sample32 += (100 + (100<<16));
			}
		}

		//sample32 = 0; //ADC_sample; //test bypass all effects
        i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
        //i2s_push_sample(I2S_NUM, (char *)&ADC_sample, portMAX_DELAY); //test bypass

		if (TIMING_BY_SAMPLE_EVERY_100_MS == 47) //10Hz
		{
			//map accelerometer values of y-axis (-1..1) to number of voices (min..max)
			grain_voices = GRAIN_VOICES_MIN + (int)(((1.0f - acc_res[1])/2.0f) * (GRAIN_VOICES_MAX - GRAIN_VOICES_MIN));
			printf("grain_voices = %d\n",grain_voices);
		}

		grain_length = GRAIN_SAMPLES;
		grain_start = 0;

		//if (TIMING_BY_SAMPLE_EVERY_50_MS == 33) //20Hz
		if (TIMING_BY_SAMPLE_EVERY_10_MS == 33) //100Hz
		{
			if(limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT)
			{
				//limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //timing @20Hz, limiter will fully recover within 1 second
				limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //timing @ 100Hz, limiter will fully recover within 0.2 second
				//printf("LC=%f",limiter_coeff);
			}
		}

	}	//end while(1)

	printf("granular_sampler(): freeing grain_buffers\n");
	free(grain_left);
	free(grain_right);
}

void update_grain_freqs0(float *freq, float *bases, int voices_used, int part) //part: 0=all
{
//#if GRAIN_VOICES_MAX == 33
	if(voices_used==33)
	{
	if(part!=2)
	{
	freq[0] = bases[0];//FREQ_C5;
	freq[1] = bases[1];//freq[0]*MAJOR_THIRD;
	freq[2] = bases[2];//freq[0]*PERFECT_FIFTH;
	freq[3] = freq[0]*0.99;
	freq[4] = freq[1]*0.99;
	freq[5] = freq[2]*0.99;
	freq[6] = freq[0]*1.01;
	freq[7] = freq[1]*1.01;
	freq[8] = freq[2]*1.01;
	freq[9] = freq[0]*0.98/2;
	freq[10] = freq[1]*0.98/2;
	freq[11] = freq[2]*0.98/2;
	freq[12] = freq[0]*1.02/2;
	freq[13] = freq[1]*1.02/2;
	freq[14] = freq[2]*1.02/2;
	freq[15] = freq[0]*0.97*2;
	}
	if(part!=1)
	{
	freq[16] = freq[1]*0.97*2;
	freq[17] = freq[2]*0.97*2;
	freq[18] = freq[0]*1.03*2;
	freq[19] = freq[1]*1.03*2;
	freq[20] = freq[2]*1.03*2;
	freq[21] = freq[0]*0.96/4;
	freq[22] = freq[1]*0.96/4;
	freq[23] = freq[2]*0.96/4;
	freq[24] = freq[0]*1.04*4;
	freq[25] = freq[1]*1.04*4;
	freq[26] = freq[2]*1.04*4;
	freq[27] = freq[0]*0.95/8;
	freq[28] = freq[1]*0.95/8;
	freq[29] = freq[2]*0.95/8;
	freq[30] = freq[0]*1.05*8;
	freq[31] = freq[1]*1.05*8;
	freq[32] = freq[2]*1.05*8;
	}
//#endif
	}
	else if(voices_used>=24)
	{
/*
#if GRAIN_VOICES_MAX == 27
	freq[0] = bases[0];//FREQ0;
	freq[1] = freq[0]*0.97;
	freq[2] = freq[0]*0.93;
	freq[3] = bases[1];//freq[0]*MAJOR_THIRD;
	freq[4] = bases[2];//freq[0]*PERFECT_FIFTH;
	freq[5] = freq[3]*0.96;
	freq[6] = freq[4]*0.94;
	freq[7] = freq[0]/2;
	freq[8] = freq[1]/2;
	freq[9] = freq[2]/2;
	freq[10] = freq[3]/2;
	freq[11] = freq[4]/2;
	freq[12] = freq[5]/2;
	freq[13] = freq[6]/2;
	freq[14] = freq[0]/4;
	freq[15] = freq[1]/4;
	freq[16] = freq[2]/4;
	freq[17] = freq[3]/4;
	freq[18] = freq[4]/4;
	freq[19] = freq[5]/4;
	freq[20] = freq[6]/4;
	freq[21] = freq[0]/8;
	freq[22] = freq[1]/8;
	freq[23] = freq[2]/8;
	freq[24] = freq[3]/8;
	freq[25] = freq[4]/8;
	freq[26] = freq[5]/8;
#endif
*/

//#if GRAIN_VOICES_MAX == 27
	if(part!=2)
	{
	freq[0] = bases[0];//FREQ_C5;
	freq[1] = bases[1];//freq[0]*MAJOR_THIRD;
	freq[2] = bases[2];//freq[0]*PERFECT_FIFTH;
	freq[3] = freq[0]*0.99*2;
	freq[4] = freq[1]*0.99*2;
	freq[5] = freq[2]*0.99*2;
	freq[6] = freq[0]*1.01/2;
	freq[7] = freq[1]*1.01/2;
	freq[8] = freq[2]*1.01/2;
	freq[9] = freq[0]*0.98/4;
	freq[10] = freq[1]*0.98/4;
	freq[11] = freq[2]*0.98/4;
	freq[12] = freq[0]*1.02/8;
	freq[13] = freq[1]*1.02/8;
	}
	if(part!=1)
	{
	freq[14] = freq[2]*1.02/8;
	freq[15] = freq[0]*0.995*4;
	freq[16] = freq[1]*0.995*4;
	freq[17] = freq[2]*0.995*4;
	freq[18] = freq[0]*1.005*8;
	freq[19] = freq[1]*1.005*8;
	freq[20] = freq[2]*1.005*8;
	freq[21] = freq[0]*1.012*16;
	freq[22] = freq[1]*1.012*16;
	freq[23] = freq[2]*1.012*16;
	if(voices_used>24)
	{
		freq[24] = freq[0]*0.987/16;
		freq[25] = freq[1]*0.987/16;
		freq[26] = freq[2]*0.987/16;
	}
	}
//#endif
	}
}
#endif
/*
void update_grain_freqs_chords_octaves(float *freq, float *bases, int voices_used, int part) //part: 0=all
{
	freq[0] = bases[0];
	freq[1] = bases[1];
	freq[2] = bases[2];

	freq[3] = freq[0]/2;
	freq[4] = freq[1]/2;
	freq[5] = freq[2]/2;

	freq[6] = freq[3]/2;
	freq[7] = freq[4]/2;
	freq[8] = freq[5]/2;

	freq[9] = freq[6]/2;

	freq[10] = freq[0]*2;
	freq[11] = freq[1]*2;
	freq[12] = freq[2]*2;

	freq[13] = freq[10]*2;
	freq[14] = freq[11]*2;
	freq[15] = freq[12]*2;

	freq[16] = freq[13]*2;
	freq[17] = freq[14]*2;
	freq[18] = freq[15]*2;

	freq[19] = freq[16]*2;
	freq[20] = freq[17]*2;
	freq[21] = freq[18]*2;

	freq[22] = freq[7]/2;
	freq[23] = freq[8]/2;
}
*/
void update_grain_freqs(float *freq, float *bases, int voices_used, float detune)
{
	freq[0] = bases[0];
	freq[1] = bases[1];
	freq[2] = bases[2];

	freq[3] = freq[0] * (2.0f+detune*GRANULAR_DETUNE_A1);
	freq[4] = freq[1] * (2.0f+detune*GRANULAR_DETUNE_A2);
	freq[5] = freq[2] * (2.0f+detune*GRANULAR_DETUNE_A3);

	freq[6] = freq[0] * (0.5f+detune*GRANULAR_DETUNE_B1);
	freq[7] = freq[1] * (0.5f+detune*GRANULAR_DETUNE_B2);
	freq[8] = freq[2] * (0.5f+detune*GRANULAR_DETUNE_B3);

	freq[9] = freq[3]*2;
	freq[10] = freq[4]*2;
	freq[11] = freq[5]*2;

	freq[12] = freq[6]/2;
	freq[13] = freq[7]/2;
	freq[14] = freq[8]/2;

	freq[15] = freq[3]*4;
	freq[16] = freq[4]*4;
	freq[17] = freq[5]*4;

	freq[18] = freq[6]/4;
	freq[19] = freq[7]/4;
	freq[20] = freq[8]/4;

	freq[21] = freq[0]*8;
	freq[22] = freq[1]*8;
	freq[23] = freq[2]*8;
}

void granular_sampler_simple()
{
	//--------------------------------------------------------------------------
	//defines, constants and variables

	int16_t *grain_left, *grain_right;	//two buffers for stereo samples

	int grain_voices = GRAIN_VOICES_MIN;	//voices currently active
	int grain_length = GRAIN_SAMPLES;		//amount of samples to process from the grain
	int grain_start = 0;	//position where the grain starts within the buffer

	int grain_voices_used = GRAIN_VOICES_MAX_S;
	//#define grain_voices_used GRAIN_VOICES_MAX_S

	#define GRANULAR_SONGS 14
	int selected_song = 0;
	const int granular_songs[GRANULAR_SONGS] = {1,2,3,4,11,12,13,14,21,22,23,24,31,41};

	//there is an external sampleCounter variable already, it will be used to timing and counting seconds
	sampleCounter = 0;
	seconds = 0;

	//define frequencies for all voices

	//grain_voices_used = 33;//12; //33;
	float base_chord_33[3] = {FREQ_C5,FREQ_C5*MAJOR_THIRD,FREQ_C5*PERFECT_FIFTH};
	update_grain_freqs(freq, base_chord_33, grain_voices_used, detune_coeff);

	//allocate memory for grain buffers
	grain_left = (int16_t*)malloc(GRAIN_SAMPLES * sizeof(int16_t));
	printf("granular_sampler_simple(): grain_left buffer allocated at address %x\n", (unsigned int)grain_left);
	if(grain_left==NULL) { while(1) { error_blink(5,60,0,0,5,30); /*RGB:count,delay*/ }; }
	grain_right = (int16_t*)malloc(GRAIN_SAMPLES * sizeof(int16_t));
	printf("granular_sampler_simple(): grain_right buffer allocated at address %x\n", (unsigned int)grain_right);
	if(grain_right==NULL) { while(1){ error_blink(5,60,0,0,5,30); /*RGB:count,delay*/ }; }

	memset(grain_left, 0, GRAIN_SAMPLES * sizeof(int16_t));
	memset(grain_right, 0, GRAIN_SAMPLES * sizeof(int16_t));

	//reset counters
	sampleCounter1 = 0;
	sampleCounter2 = 0;

	//reset pointers
	for(int i=1;i<grain_voices_used;i++)
	{
		step[i] = 0;
	}

	#ifdef GRANULAR_ECHO
	//initialize echo without using init_echo_buffer() which allocates too much memory
	#define GRANULAR_SAMPLER_ECHO_BUFFER_SIZE I2S_AUDIOFREQ //half second (buffer is shared for both stereo channels)
	echo_dynamic_loop_length = GRANULAR_SAMPLER_ECHO_BUFFER_SIZE; //set default value (can be changed dynamically)
	//echo_buffer = (int16_t*)malloc(echo_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	echo_buffer_ptr0 = 0; //reset pointer
	#endif

	int freqs_updating = 0;

	program_settings_reset();

	BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_SHORT;

	/*
	enable_out_clock();
	mclk_enabled = 1;
	codec_init(); //i2c init
	*/

	#ifdef SWITCH_I2C_SPEED_MODES
	i2c_master_deinit();
    i2c_master_init(1); //fast mode
	#endif

    printf("Granular: function loop starting\n");

	channel_running = 1;
    volume_ramp = 1;
    int sample_hold = 0;

	//RGB_LED_R_OFF;
	RGB_LED_G_ON;
	//RGB_LED_B_OFF;

	while(!event_next_channel || sample_hold)
	{
		sampleCounter++;

		//here actually two seconds as one increment of sampleCounter happens once per both channels
		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION) //one full second passed
		{
			//printf("Granular: TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION\n");
			seconds++;
			sampleCounter = 0;

			if(selected_song)
			{
				//replace the grain frequencies by chord
				seconds %= gran_chord->total_chords; //wrap around to total number of chords
				//freqs_updating = 2;

				update_grain_freqs(freq, gran_chord->bases_parsed + seconds*3, grain_voices_used, 0); //update all at once
			}
		}
// *
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
/*
		if(TIMING_BY_SAMPLE_EVERY_1_MS)
		{
			if(freqs_updating>0)
			{
				//update_grain_freqs(freq, gran_chord->bases_parsed + seconds*3, grain_voices_used, 3-freqs_updating); //update one group only
				//freqs_updating--;

				update_grain_freqs(freq, gran_chord->bases_parsed + seconds*3, grain_voices_used, 0); //update all at once
				freqs_updating=0;
			}
		}
*/
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//* /

		if (TIMING_BY_SAMPLE_EVERY_100_MS == 43) //10Hz
		{
			//x-axis sets offset of right stereo channel against left
			//if (ADC_last_result[1] > IR_sensors_THRESHOLD_9)
			if(fabs(acc_res[0]) > 0.9f)
			{
				sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 2;
			}
			else //if (ADC_last_result[1] > IR_sensors_THRESHOLD_8)
			if(fabs(acc_res[0]) > 0.8f)
			{
				sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 3;
			}
			else //if (ADC_last_result[1] > IR_sensors_THRESHOLD_6)
			if(fabs(acc_res[0]) > 0.7f)
			{
				sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 4;
			}
			else //if (ADC_last_result[1] > IR_sensors_THRESHOLD_4)
			if(fabs(acc_res[0]) > 0.6f)
			{
				sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 6;
			}
			else //if (ADC_last_result[1] > IR_sensors_THRESHOLD_2)
			{
				sampleCounter2 = sampleCounter1;
			}
		}

// *
 	//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
		//increment counters and wrap around if completed full cycle
		if (++sampleCounter1 >= (unsigned int)grain_length)
    	{
        	sampleCounter1 = 0;
    	}
		if (++sampleCounter2 >= (unsigned int)grain_length)
		{
			sampleCounter2 = 0;
		}
 	//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * /

		//x-axis enables recording - when triggered, sample the data into the grain buffer (left channel first)
		//if (ADC_last_result[1] > IR_sensors_THRESHOLD_1)

		//always recording
		//if(acc_res[0] > 0.4f)
		//record only if not frozen
		if(!sample_hold)
		{
			//new_mixing_vol = 1.0f + (acc_res[0] - 0.4f) * 1.6f;

			i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
			grain_left[sampleCounter1] = (int16_t)ADC_sample;
        }

        //mix samples for all voices (left channel)
		//sample_mix = 0;
		sample_mix = (int16_t)ADC_sample;

		if(!selected_song)
		{
			if (TIMING_BY_SAMPLE_EVERY_100_MS == 45) //10Hz
			{
				//printf("Granular: TIMING_BY_SAMPLE_EVERY_100_MS == 45, acc_res[0] = %f, current_chord = %d\n", acc_res[0], current_chord);

				//switch between major and minor chord
				if(acc_res[0] > 0.3f && current_chord < 1) //if minor chord at the moment and turning right
				{
					//load the major chord
					//printf("Granular: loading the major chord: FREQ_C5*MAJOR_THIRD\n");
					current_chord = 1;
					base_chord_33[1] = FREQ_C5*MAJOR_THIRD;
					update_grain_freqs(freq, base_chord_33, grain_voices_used, 0); //update all at once

				}
				if(acc_res[0] < -0.3f && current_chord > -1) //if major chord at the moment and turning left
				{
					//printf("Granular: loading the minor chord: FREQ_C5*MINOR_THIRD\n");
					current_chord = -1;
					base_chord_33[1] = FREQ_C5*MINOR_THIRD;
					update_grain_freqs(freq, base_chord_33, grain_voices_used, 0); //update all at once
				}
			}
		}

// *
//		 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
		for(int i=0;i<grain_voices;i++)
		{
			step[i] += freq[i] / FREQ_C5;
			if ((int)step[i]>=grain_length)
			{
				step[i] = grain_start;
			}
			sample_mix += grain_left[(int)step[i]];
		}
//		 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//* /
		//sample_mix = sample_mix * SAMPLE_VOLUME / 12.0f; //apply volume

		#ifdef GRANULAR_ECHO
		sample32 = (add_echo((int16_t)(sample_mix))) << 16;
		#else
		sample32 = ((int16_t)(sample_mix)) << 16;
		#endif

		//sample data into the right channel grain buffer, when x-axis triggered
		//if (ADC_last_result[1] > IR_sensors_THRESHOLD_1)

		//always recording
		//if(acc_res[0] > 0.4f)

		//record only if not frozen
		if(!sample_hold)
        {
			grain_right[sampleCounter2] = (int16_t)(ADC_sample>>16);
        }

        //mix samples for all voices (right channel)
        //sample_mix = 0;
        sample_mix = (int16_t)(ADC_sample>>16);

// *
//   		 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
        for(int i=0;i<grain_voices;i++)
    	{
    		sample_mix += grain_right[(int)step[i]];
    	}
//		 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//* /

        //sample_mix = sample_mix * SAMPLE_VOLUME / 12.0f; //apply volume

        #ifdef GRANULAR_ECHO
        sample32 += add_echo((int16_t)(sample_mix));
		#else
        sample32 += (int16_t)(sample_mix);
		#endif

		if(sampleCounter & (1<<add_beep))
		{
			if(add_beep)
			{
				sample32 += (100 + (100<<16));
			}
		}

		//sample32 = 0; //ADC_sample; //test bypass all effects
        i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
        //i2s_push_sample(I2S_NUM, (char *)&ADC_sample, portMAX_DELAY); //test bypass

		if (TIMING_BY_SAMPLE_EVERY_100_MS == 47) //10Hz
		{
			//map accelerometer values of y-axis (-1..1) to number of voices (min..max)

			//grain_voices = GRAIN_VOICES_MIN + (int)(((1.0f - acc_res[1])/2.0f) * (GRAIN_VOICES_MAX - GRAIN_VOICES_MIN));
			grain_voices = GRAIN_VOICES_MIN + (int)(((1.0f - acc_res[1])/2.0f) * (grain_voices_used - GRAIN_VOICES_MIN));

			//printf("grain_voices = %d\n",grain_voices);
		}

		grain_length = GRAIN_SAMPLES;
		grain_start = 0;

		//if (TIMING_BY_SAMPLE_EVERY_50_MS == 33) //20Hz
		if (TIMING_BY_SAMPLE_EVERY_10_MS == 33) //100Hz
		{
			if(limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT)
			{
				//limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //limiter will fully recover within 1 second
				limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //limiter will fully recover within 0.2 second
				//printf("LC=%f",limiter_coeff);
			}
		}

		if (TIMING_BY_SAMPLE_EVERY_100_MS == 49) //10Hz
		{
			if(short_press_sequence==2)
			{
				short_press_sequence = 0;
				codec_set_mute(1); //this takes a while so will stop the sound so there is no buzzing
				selected_song++;
				RGB_LED_B_ON;
				if(selected_song==GRANULAR_SONGS)
				{
					selected_song = 0; //off again
					RGB_LED_B_OFF;
				}
				printf("granular_sampler_simple(): selected_song=%d\n", selected_song);
				load_song_for_granular(granular_songs[selected_song]);
				codec_set_mute(0);
			}
			if(short_press_sequence==-2)
			{
				short_press_sequence = 0;
			}

			if(short_press_volume_plus || short_press_volume_minus)
			{
				if(short_press_volume_minus)
				{
					detune_coeff *= global_settings.GRANULAR_DETUNE_COEFF_MUL;
				}
				else if(short_press_volume_plus)
				{
					detune_coeff /= global_settings.GRANULAR_DETUNE_COEFF_MUL;
				}

				if(detune_coeff < global_settings.GRANULAR_DETUNE_COEFF_SET)
				{
					detune_coeff = global_settings.GRANULAR_DETUNE_COEFF_SET;
				}
				else if(detune_coeff > global_settings.GRANULAR_DETUNE_COEFF_MAX)
				{
					detune_coeff = global_settings.GRANULAR_DETUNE_COEFF_MAX;
				}

				printf("granular_sampler_simple(): detune_coeff=%f\n", detune_coeff);
				if(!selected_song)
				{
					update_grain_freqs(freq, base_chord_33, grain_voices_used, detune_coeff);
				}
				short_press_volume_plus = 0;
				short_press_volume_minus = 0;
			}

			if(event_channel_options)
			{
				event_channel_options = 0;
				sample_hold = 1;
				RGB_LED_R_OFF;
				RGB_LED_G_ON;
			}
			if(event_next_channel && sample_hold)
			{
				event_next_channel = 0;
				sample_hold = 0;
				RGB_LED_G_OFF;
				RGB_LED_R_ON;
			}
		}

	}	//end while(1)

	printf("granular_sampler_simple(): freeing grain_buffers\n");
	free(grain_left);
	free(grain_right);
}


void load_song_for_granular(int song_id)
{
	printf("load_song_for_granular(): song_id = %d\n", song_id);
	if(song_id)
	{
		if(gran_chord!=NULL)
		{
			printf("load_song_for_granular(): gran_chord not NULL\n");
			load_song_for_granular(0); //deallocate
		}

		printf("load_song_for_granular(): allocating new MusicBox with song_id = %d\n", song_id);
		//generate chords for the selected song
		gran_chord = new MusicBox(song_id);
		printf("load_song_for_granular(): new MusicBox loaded with song_id = %d, total chords = %d\n", song_id, gran_chord->total_chords);

		/*
		//this will access unallocated memory
		for(int c=0;c<gran_chord->total_chords;c++)
		{
			printf("load_song_for_granular() chords[%d].freqs content = %f,%f,%f\n", c, gran_chord->chords[c].freqs[0], gran_chord->chords[c].freqs[1], gran_chord->chords[c].freqs[2]);
		}
		delete gran_chord;
		gran_chord = new MusicBox(song_id);
		*/

		int notes_parsed = parse_notes(gran_chord->base_notes, gran_chord->bases_parsed, NULL, NULL);

		if(notes_parsed != gran_chord->total_chords * NOTES_PER_CHORD) //integrity check
		{
			//we don't have correct amount of notes
			if(notes_parsed < gran_chord->total_chords * NOTES_PER_CHORD) //some notes missing
			{
				do //let's add zeros, so it won't break the playback
				{
					gran_chord->bases_parsed[notes_parsed++] = 0;
				}
				while(notes_parsed < gran_chord->total_chords * NOTES_PER_CHORD);
			}
		}
		//chord->generate(CHORD_MAX_VOICES); //no need to expand bases
		printf("load_song_for_granular(): %d notes parsed, total chords = %d\n", notes_parsed, gran_chord->total_chords);
	}
	else //deallocate memory
	{
		printf("load_song_for_granular(): deleting gran_chord\n");
		delete gran_chord;
		gran_chord = NULL;
	}
}

#if 0
void granular_sampler_segmented()
{
	//--------------------------------------------------------------------------
	//defines, constants and variables

	printf("granular_sampler_segmented()\n");

	#define GRAIN_BUFFERS 5

	int16_t *grain_buffer[GRAIN_BUFFERS];	//buffers for samples (recorded in mono)

	int active_sampling_buffer = 0;

	int grain_voices = GRAIN_VOICES_MIN;	//voices currently active
	int grain_length = GRAIN_SAMPLES;		//amount of samples to process from the grain
	int grain_start = 0;	//position where the grain starts within the buffer

	//#define grain_voices_used 16 //voices at the same moment
	int grain_voices_used = 16; //voices at the same moment

	//there is an external sampleCounter variable already, it will be used to timing and counting seconds
	sampleCounter = 0;
	seconds = 0;

	//define frequencies for all voices

	//grain_voices_used = 33;//12; //33;
	float base_chord_33[3] = {FREQ_C5,FREQ_C5*MAJOR_THIRD,FREQ_C5*PERFECT_FIFTH};
	update_grain_freqs(freq, base_chord_33, grain_voices_used, 0); //update all at once

	//allocate memory for grain buffers

	printf("granular_sampler_segmented(): allocating memory (%d buffers x %d samples)\n",GRAIN_BUFFERS,GRAIN_SAMPLES);
	for(int i=0;i<GRAIN_BUFFERS;i++)
	{
		printf("granular_sampler_segmented(): allocating grain_buffer[%d]\n",i);
		grain_buffer[i] = (int16_t*)malloc(GRAIN_SAMPLES * sizeof(int16_t));

		if(grain_buffer[i]==0) { printf("out of memory!\n"); while(1); }

		printf("granular_sampler_segmented(): clearing grain_buffer[%d], address = %x\n",i,(unsigned int)grain_buffer[i]);
		memset(grain_buffer[i], 0, GRAIN_SAMPLES * sizeof(int16_t));
	}
	printf("granular_sampler_segmented(): buffers allocated and cleared\n");

	//reset counters
	sampleCounter1 = 0;
	sampleCounter2 = 0;

	printf("granular_sampler_segmented(): reset pointers...");
	//reset pointers
	for(int i=1;i<grain_voices_used;i++)
	{
		step[i] = 0;
	}
	printf(" done.\n");

	#ifdef GRANULAR_ECHO
	//initialize echo without using init_echo_buffer() which allocates too much memory
	#define GRANULAR_SAMPLER_ECHO_BUFFER_SIZE I2S_AUDIOFREQ //half second (buffer is shared for both stereo channels)
	echo_dynamic_loop_length = GRANULAR_SAMPLER_ECHO_BUFFER_SIZE; //set default value (can be changed dynamically)
	//echo_buffer = (int16_t*)malloc(echo_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	echo_buffer_ptr0 = 0; //reset pointer
	#endif

	//int freqs_updating = 0;

	printf("granular_sampler_segmented(): program_settings_reset()\n");
	program_settings_reset();

	/*
	printf("granular_sampler_segmented(): enable_out_clock()\n");
	enable_out_clock();
	mclk_enabled = 1;
	*/

	printf("granular_sampler_segmented(): codec_init()\n");
	codec_init(); //i2c init

	printf("granular_sampler_segmented(): i2c master re-init\n");
	i2c_master_deinit();
    i2c_master_init(1); //fast mode

    printf("Granular: function loop starting\n");

	channel_running = 1;
    volume_ramp = 1;

	while(!event_next_channel)
	{
		sampleCounter++;

		//printf("acc_res[0]=%f,acc_res[1]=%f,acc_res[2]=%f\n",acc_res[0],acc_res[1],acc_res[2]);

		//here actually two seconds as one increment of sampleCounter happens once per both channels
		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION) //one full second passed
		{
			//printf("Granular: TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION\n");
			seconds++;
			sampleCounter = 0;
			/*
			if(selected_song)
			{
				//replace the grain frequencies by chord
				seconds %= chord->total_chords; //wrap around to total number of chords
				freqs_updating = 2;
			}
			*/
		}
// *
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 		/*
		if(TIMING_BY_SAMPLE_EVERY_1_MS)
		{
			if(freqs_updating>0)
			{
				update_grain_freqs(freq, chord->bases_parsed + seconds*3, grain_voices_used, 3-freqs_updating); //update one group only
				freqs_updating--;
			}
		}
		*/
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//* /

		if (TIMING_BY_SAMPLE_EVERY_100_MS == 43) //10Hz
		{
			//x-axis sets offset of right stereo channel against left
			//if (ADC_last_result[1] > IR_sensors_THRESHOLD_9)
			if(fabs(acc_res[0]) > 0.9f)
			{
				sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 2;
			}
			else //if (ADC_last_result[1] > IR_sensors_THRESHOLD_8)
			if(fabs(acc_res[0]) > 0.8f)
			{
				sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 3;
			}
			else //if (ADC_last_result[1] > IR_sensors_THRESHOLD_6)
			if(fabs(acc_res[0]) > 0.7f)
			{
				sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 4;
			}
			else //if (ADC_last_result[1] > IR_sensors_THRESHOLD_4)
			if(fabs(acc_res[0]) > 0.6f)
			{
				sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 6;
			}
			else //if (ADC_last_result[1] > IR_sensors_THRESHOLD_2)
			{
				sampleCounter2 = sampleCounter1;
			}
		}

// *
 	//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
		//increment counters and wrap around if completed full cycle
		if (++sampleCounter1 >= (unsigned int)grain_length)
    	{
        	sampleCounter1 = 0;
    	}
		if (++sampleCounter2 >= (unsigned int)grain_length)
		{
			sampleCounter2 = 0;
		}
 	//* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
// * /

		//x-axis enables recording - when triggered, sample the data into the grain buffer (left channel first)
		//if (ADC_last_result[1] > IR_sensors_THRESHOLD_1)

		if(acc_res[0] > 0.4f)
		{
			//new_mixing_vol = 1.0f + (acc_res[0] - 0.4f) * 1.6f;

			i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
			grain_buffer[active_sampling_buffer][sampleCounter1] = (int16_t)ADC_sample;
        }

        //mix samples for all voices (left channel)
		//sample_mix = 0;
		sample_mix = (int16_t)ADC_sample;

		if (TIMING_BY_SAMPLE_EVERY_100_MS == 45) //10Hz
		{
			//printf("Granular: TIMING_BY_SAMPLE_EVERY_100_MS == 45, acc_res[0] = %f, current_chord = %d\n", acc_res[0], current_chord);

			//switch between major and minor chord
			if(acc_res[0] > 0.3f && current_chord < 1) //if minor chord at the moment and turning right
			{
				//load the major chord
				//printf("Granular: loading the major chord: FREQ_C5*MAJOR_THIRD\n");
				current_chord = 1;
				base_chord_33[1] = FREQ_C5*MAJOR_THIRD;
				update_grain_freqs(freq, base_chord_33, grain_voices_used, 0); //update all at once

			}
			if(acc_res[0] < -0.3f && current_chord > -1) //if major chord at the moment and turning left
			{
				//printf("Granular: loading the minor chord: FREQ_C5*MINOR_THIRD\n");
				current_chord = -1;
				base_chord_33[1] = FREQ_C5*MINOR_THIRD;
				update_grain_freqs(freq, base_chord_33, grain_voices_used, 0); //update all at once
			}
		}

// *
//		 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
		for(int i=0;i<grain_voices;i++)
		{
			step[i] += freq[i] / FREQ_C5;
			if ((int)step[i]>=grain_length)
			{
				step[i] = grain_start;
			}
			sample_mix += grain_buffer[active_sampling_buffer][(int)step[i]];
		}
//		 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//* /
		//sample_mix = sample_mix * SAMPLE_VOLUME / 12.0f; //apply volume

		#ifdef GRANULAR_ECHO
		sample32 = (add_echo((int16_t)(sample_mix))) << 16;
		#else
		sample32 = ((int16_t)(sample_mix)) << 16;
		#endif

		//sample data into the right channel grain buffer, when x-axis triggered
		//if (ADC_last_result[1] > IR_sensors_THRESHOLD_1)

		if(acc_res[0] < -0.4f)
        {
			grain_buffer[active_sampling_buffer][sampleCounter2] = (int16_t)(ADC_sample>>16);
        }

        //mix samples for all voices (right channel)
        //sample_mix = 0;
        sample_mix = (int16_t)(ADC_sample>>16);

// *
//   		 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
        for(int i=0;i<grain_voices;i++)
    	{
    		sample_mix += grain_buffer[active_sampling_buffer][(int)step[i]];
    	}
//		 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//* /

        //sample_mix = sample_mix * SAMPLE_VOLUME / 12.0f; //apply volume

        #ifdef GRANULAR_ECHO
        sample32 += add_echo((int16_t)(sample_mix));
		#else
        sample32 += (int16_t)(sample_mix);
		#endif

		if(sampleCounter & (1<<add_beep))
		{
			if(add_beep)
			{
				sample32 += (100 + (100<<16));
			}
		}

		//sample32 = 0; //ADC_sample; //test bypass all effects
        i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
        //i2s_push_sample(I2S_NUM, (char *)&ADC_sample, portMAX_DELAY); //test bypass

		if (TIMING_BY_SAMPLE_EVERY_100_MS == 47) //10Hz
		{
			//map accelerometer values of y-axis (-1..1) to number of voices (min..max)

			//grain_voices = GRAIN_VOICES_MIN + (int)(((1.0f - acc_res[1])/2.0f) * (GRAIN_VOICES_MAX - GRAIN_VOICES_MIN));
			grain_voices = GRAIN_VOICES_MIN + (int)(((1.0f - acc_res[1])/2.0f) * (grain_voices_used - GRAIN_VOICES_MIN));

			printf("grain_voices = %d\n",grain_voices);
		}

		grain_length = GRAIN_SAMPLES;
		grain_start = 0;

		//if (TIMING_BY_SAMPLE_EVERY_50_MS == 33) //20Hz
		if (TIMING_BY_SAMPLE_EVERY_10_MS == 33) //100Hz
		{
			if(limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT)
			{
				//limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //limiter will fully recover within 1 second
				limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //limiter will fully recover within 0.2 second
				//printf("LC=%f",limiter_coeff);
			}
		}

	}	//end while(1)

	for(int i=0;i<GRAIN_BUFFERS;i++)
	{
		printf("granular_sampler_segmented(): freeing grain_buffer[%d]\n",i);
		free(grain_buffer[i]);
	}
}
#endif
