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
#include <Sensors.h>
#include <InitChannels.h>
#include <MusicBox.h>

#include "hw/midi.h"
#include "hw/sdcard.h"
#include "hw/leds.h"

//--------------------------------------------------------------------------
//defines, constants and variables

#ifdef BOARD_WHALE
#define GRAIN_LENGTH_MAX	(I2S_AUDIOFREQ/2)	//memory will be allocated for up to a half second grain
#define GRAIN_LENGTH_MIN	(I2S_AUDIOFREQ/16)	//one 16th of second will be shortest allowed grain length
#else
#define GRAIN_LENGTH_MAX	40000/2				//memory will be allocated for up to about 0.4 second grain
#define GRAIN_LENGTH_MIN	(I2S_AUDIOFREQ/40)	//one 40th of second will be shortest allowed grain length
#endif

#define GRAIN_VOICES_MIN	3		//one basic chord
#define GRAIN_VOICES_MAX	27		//voices per stereo channel

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

int g_current_chord = 1, chord_set = 0; //1=major, -1=minor chord
float detune_coeff = 0;

MusicBox *gran_chord = NULL;

#define GRANULAR_ECHO
//#define GRANULAR_MIX_EXT

void update_grain_freqs(float *freq, float *bases, int voices_used, float detune)
{
	//printf("update_grain_freqs(): bases ptr=0x%lx\n", (unsigned long)bases);
	//printf("update_grain_freqs(): bases={%.02f,%.02f,%.02f}\n", bases[0], bases[1], bases[2]);

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

	freq[21] = freq[3]*8;
	freq[22] = freq[4]*8;
	freq[23] = freq[5]*8;

	freq[24] = freq[6]/8;
	freq[25] = freq[7]/8;
	freq[26] = freq[8]/8;
}

void granular_sampler_simple()
{
	//--------------------------------------------------------------------------
	//defines, constants and variables

	int16_t *grain_left, *grain_right;	//two buffers for stereo samples

	int grain_voices = GRAIN_VOICES_MIN;	//voices currently active
	int grain_length = GRAIN_LENGTH_MAX;		//amount of samples to process from the grain
	int grain_start = 0;	//position where the grain starts within the buffer

	#define GRANULAR_SONGS 14
	int selected_song = 0;
	const int granular_songs[GRANULAR_SONGS] = {1,2,3,4,11,12,13,14,21,22,23,24,31,41};

	//there is an external sampleCounter variable already, it will be used to timing and counting seconds
	sampleCounter = 0;
	seconds = 0;

	//define frequencies for all voices

	float base_chord_33[3] = {FREQ_C5,FREQ_C5*MAJOR_THIRD,FREQ_C5*PERFECT_FIFTH};
	update_grain_freqs(freq, base_chord_33, GRAIN_VOICES_MAX, detune_coeff);

	printf("granular_sampler_simple(): allocating grain buffers, free mem = %u\n", xPortGetFreeHeapSize());
	//allocate memory for grain buffers
	grain_left = (int16_t*)malloc(GRAIN_LENGTH_MAX * sizeof(int16_t));
	printf("granular_sampler_simple(): grain_left buffer allocated at address %x\n", (unsigned int)grain_left);
	if(grain_left==NULL) { while(1) { error_blink(5,60,0,0,5,30); /*RGB:count,delay*/ }; }

	printf("granular_sampler_simple(): grain buffer #1 allocated, free mem = %u\n", xPortGetFreeHeapSize());

	grain_right = (int16_t*)malloc(GRAIN_LENGTH_MAX * sizeof(int16_t));
	printf("granular_sampler_simple(): grain_right buffer allocated at address %x\n", (unsigned int)grain_right);
	if(grain_right==NULL) { while(1){ error_blink(5,60,0,0,5,30); /*RGB:count,delay*/ }; }

	printf("granular_sampler_simple(): grain buffer #2 allocated, free mem = %u\n", xPortGetFreeHeapSize());

	memset(grain_left, 0, GRAIN_LENGTH_MAX * sizeof(int16_t));
	memset(grain_right, 0, GRAIN_LENGTH_MAX * sizeof(int16_t));

	//reset counters
	sampleCounter1 = 0;
	sampleCounter2 = 0;

	//reset pointers
	for(int i=1;i<GRAIN_VOICES_MAX;i++)
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

	//int freqs_updating = 0;

	//program_settings_reset();
	channel_init(0, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0); //init without any features

	#ifdef BOARD_WHALE
	BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_SHORT;
	#endif

	int midi_override = 0;
	//int midi_note_ptr = 0;
	//uint8_t midi_notes_active[3] = {0,0,0}, midi_notes_active_ptr = 0;

	/*
	if(use_midi)
	{
		gecho_init_MIDI(MIDI_UART);
		gecho_start_receive_MIDI();
	}
	*/

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
    int sample_hold = 1;//, sampling_l = 0, sampling_r = 0;
    float np;

    float sample_mix_l, sample_mix_r;

	#ifdef BOARD_WHALE
	//RGB_LED_R_OFF;
	RGB_LED_G_ON;
	//RGB_LED_B_OFF;
	#endif

	#ifdef BOARD_WHALE
	while(!event_next_channel || sample_hold) //in Whale, first press of "Play" button cancels sample_hold
	#else
	//while(!event_next_channel || midi_override) //in Gecho, sample_hold does not prevent from exiting the channel, but MIDI enabled does
	while(!event_next_channel) //in Gecho, sample_hold does not prevent from exiting the channel
	#endif
	{
		sampleCounter++;

		if (TIMING_BY_SAMPLE_1_SEC * 2) //two full seconds passed (one increment of sampleCounter happens once per both stereo channels)
		{
			//printf("Granular: TIMING_BY_SAMPLE_1_SEC * 2\n");
			seconds++;
			sampleCounter = 0;

			if(selected_song)
			{
				//printf("Granular: a song was selected\n");
				//replace the grain frequencies by chord
				seconds %= gran_chord->total_chords; //wrap around to total number of chords
				//freqs_updating = 2;

				//printf("Granular: update_grain_freqs(): seconds = %ld, bases_parsed ptr = 0x%lx\n", seconds, (unsigned long)(gran_chord->bases_parsed + seconds*3));
				update_grain_freqs(freq, gran_chord->bases_parsed + seconds*3, GRAIN_VOICES_MAX, detune_coeff); //update all at once
			}
		}

		//set offset of right stereo channel against left
		//if (TIMING_EVERY_100_MS == 43) //10Hz
		if (TIMING_EVERY_250_MS == 43) //4Hz
		{
			if(PARAMETER_CONTROL_SELECTED_IRS)
			{
				if(SENSOR_THRESHOLD_ORANGE_4) { sampleCounter2 = sampleCounter1 + GRAIN_LENGTH_MAX / 2; }
				else if(SENSOR_THRESHOLD_ORANGE_3) { sampleCounter2 = sampleCounter1 + GRAIN_LENGTH_MAX / 4; }
				else if(SENSOR_THRESHOLD_ORANGE_2) { sampleCounter2 = sampleCounter1 + GRAIN_LENGTH_MAX / 6; }
				else { sampleCounter2 = sampleCounter1; }
			}
			else
			{
				np = fabs(ACC_X_AXIS);
				if(np > 0.9f) { sampleCounter2 = sampleCounter1 + GRAIN_LENGTH_MAX / 2; }
				else if(np > 0.8f) { sampleCounter2 = sampleCounter1 + GRAIN_LENGTH_MAX / 3; }
				else if(np > 0.7f) { sampleCounter2 = sampleCounter1 + GRAIN_LENGTH_MAX / 4; }
				else if(np > 0.6f) { sampleCounter2 = sampleCounter1 + GRAIN_LENGTH_MAX / 6; }
				else { sampleCounter2 = sampleCounter1; }
			}
		}

		//increment counters and wrap around if completed full cycle
		if (++sampleCounter1 >= (unsigned int)grain_length)
    	{
        	//if(sample_hold)
        	//{
        	//	sampling_l = 0;
        	//}
			sampleCounter1 = 0;
    	}
		if (++sampleCounter2 >= (unsigned int)grain_length)
		{
        	//if(sample_hold)
        	//{
        	//	sampling_r = 0;
        	//}
			sampleCounter2 = 0;
		}

		i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
		//record only if not frozen
		#ifdef BOARD_WHALE
		if(!sample_hold)
		{
			grain_left[sampleCounter1] = (int16_t)ADC_sample;
			grain_right[sampleCounter2] = (int16_t)(ADC_sample>>16);
        }
		#else
		if(!sample_hold)
		//if(sampling_l)
		{
			grain_left[sampleCounter1] = (int16_t)ADC_sample;
        //}
		//if(sampling_r)
		//{
			grain_right[sampleCounter2] = (int16_t)(ADC_sample>>16);
        }
		#endif

		if(!selected_song)
		{
			if (TIMING_EVERY_40_MS == 45) //25Hz
			{
				if(!midi_override && MIDI_keys_pressed)
				{
					printf("Granular: MIDI active, will receive notes\n");
					midi_override = 1;
				}

				if(midi_override)
				{
					if(MIDI_notes_updated)
					{
						LED_W8_all_OFF();
						LED_B5_all_OFF();

						MIDI_to_LED(MIDI_last_chord[0], 1);
						MIDI_to_LED(MIDI_last_chord[1], 1);
						MIDI_to_LED(MIDI_last_chord[2], 1);

						base_chord_33[0] = MIDI_note_to_freq(MIDI_last_chord[0]);
						base_chord_33[1] = MIDI_note_to_freq(MIDI_last_chord[1]);
						base_chord_33[2] = MIDI_note_to_freq(MIDI_last_chord[2]);

						update_grain_freqs(freq, base_chord_33, GRAIN_VOICES_MAX, detune_coeff); //update all at once

						MIDI_notes_updated = 0;
					}
				}
				else
				{
					if(PARAMETER_CONTROL_SELECTED_IRS)
					{
						//switch between major and minor chord
						if(!chord_set && IR_SENSOR_VALUE_S3 > 0.5f)
						{
							if(g_current_chord < 1) //if minor chord at the moment and sensor S3 triggered
							{
								//printf("Granular: loading the major chord: FREQ_C5*MAJOR_THIRD\n");
								g_current_chord = 1; //load the major chord
								base_chord_33[1] = FREQ_C5*MAJOR_THIRD;
								update_grain_freqs(freq, base_chord_33, GRAIN_VOICES_MAX, detune_coeff); //update all at once
							}
							else if(g_current_chord > -1) //if major chord at the moment and sensor S3 triggered
							{
								//printf("Granular: loading the minor chord: FREQ_C5*MINOR_THIRD\n");
								g_current_chord = -1; //load the minor chord
								base_chord_33[1] = FREQ_C5*MINOR_THIRD;
								update_grain_freqs(freq, base_chord_33, GRAIN_VOICES_MAX, detune_coeff); //update all at once
							}
							chord_set = 1;
						}
						if(chord_set && IR_SENSOR_VALUE_S3 < 0.3f)
						{
							chord_set = 0;
						}
					}
					else
					{
						//switch between major and minor chord
						if(ACC_X_AXIS > 0.3f && g_current_chord < 1) //if minor chord at the moment and turning right
						//if((!use_acc_or_ir_sensors && normalized_params[0] > 0.65f && g_current_chord < 1) //if minor chord at the moment and turning right
						//|| (use_acc_or_ir_sensors && normalized_params[1] > 0.5f && g_current_chord < 1)) //or sensor S2 triggered
						{
							//load the major chord
							//printf("Granular: loading the major chord: FREQ_C5*MAJOR_THIRD\n");
							g_current_chord = 1;
							base_chord_33[1] = FREQ_C5*MAJOR_THIRD;
							update_grain_freqs(freq, base_chord_33, GRAIN_VOICES_MAX, detune_coeff); //update all at once

						}
						if(ACC_X_AXIS < -0.3f && g_current_chord > -1) //if major chord at the moment and turning left
						//if((!use_acc_or_ir_sensors && normalized_params[0] < 0.35f && g_current_chord > -1) //if major chord at the moment and turning left
						//|| (use_acc_or_ir_sensors && normalized_params[2] > 0.5f && g_current_chord > -1)) //or sensor S3 triggered
						{
							//printf("Granular: loading the minor chord: FREQ_C5*MINOR_THIRD\n");
							g_current_chord = -1;
							base_chord_33[1] = FREQ_C5*MINOR_THIRD;
							update_grain_freqs(freq, base_chord_33, GRAIN_VOICES_MAX, detune_coeff); //update all at once
						}
					}
				}
			}
		}


		#ifdef GRANULAR_MIX_EXT
		if(!sample_hold) //add to the output only if not frozen
		{
			//mix samples for all voices
			sample_mix_l = (int16_t)ADC_sample;
			sample_mix_r = (int16_t)(ADC_sample>>16);
		}
		else
		{
		#endif
			sample_mix_l = 0;
			sample_mix_r = 0;
		#ifdef GRANULAR_MIX_EXT
		}
		#endif

		for(int i=0;i<grain_voices;i++)
		{
			//if(isnan(freq[i]))
			//{
				//continue;
			//}
			step[i] += freq[i] / FREQ_C5;
			if ((int)step[i]>=grain_length)
			{
				step[i] = grain_start;
			}
			sample_mix_l += grain_left[(int)step[i]];
    		sample_mix_r += grain_right[(int)step[i]];
		}

		//sample_mix = sample_mix * SAMPLE_VOLUME / 12.0f; //apply volume

		#ifdef GRANULAR_ECHO
		sample32 = (add_echo((int16_t)(sample_mix_l))) << 16;
        sample32 += add_echo((int16_t)(sample_mix_r));
		#else
		sample32 = ((int16_t)(sample_mix_l)) << 16;
        sample32 += (int16_t)(sample_mix_r);
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
        sd_write_sample(&sample32);

		if (TIMING_EVERY_40_MS == 47) //25Hz
		{
			#ifdef BOARD_GECHO
			if(MIDI_controllers_active_CC) //override S4 controlling # of voices when MIDI controller CC wheel active
			{
				if(MIDI_controllers_updated==MIDI_WHEEL_CONTROLLER_CC_UPDATED)
				{
					//printf("MIDI_WHEEL_CONTROLLER_CC => %d\n", MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]);
					grain_voices = GRAIN_VOICES_MIN + (int)(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]/127.0f * (GRAIN_VOICES_MAX - GRAIN_VOICES_MIN));

					MIDI_controllers_updated = 0;
				}
			}
			else //manual control of voices only enabled until MIDI controller wheels active
			{
			#endif
				//map accelerometer values of y-axis (-1..1) to number of voices (min..max)
				if(PARAMETER_CONTROL_SELECTED_IRS)
				{
					grain_voices = GRAIN_VOICES_MIN + (int)(IR_SENSOR_VALUE_S4 * (GRAIN_VOICES_MAX - GRAIN_VOICES_MIN));
				}
				else
				{
					grain_voices = GRAIN_VOICES_MIN + (int)(((1.0f - ACC_Y_AXIS)/2.0f) * (GRAIN_VOICES_MAX - GRAIN_VOICES_MIN));
				}
				//grain_voices = GRAIN_VOICES_MIN + (int)((-normalized_params[3]) * (GRAIN_VOICES_MAX - GRAIN_VOICES_MIN));

			#ifdef BOARD_GECHO
			}
			#endif
			//printf("grain_voices = %d\n",grain_voices);
		}

		grain_length = GRAIN_LENGTH_MAX;
		grain_start = 0;

		if (TIMING_EVERY_20_MS == 33) //50Hz
		{
			if(limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT)
			{
				limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //limiter will fully recover within 0.4 second
				//printf("LC=%f",limiter_coeff);
			}
		}

		ui_command = 0;

		if (TIMING_EVERY_40_MS == 49) //25Hz
		{
			#ifdef BOARD_GECHO
			if(MIDI_controllers_active_PB) //override manual control of detune when MIDI controller PB wheel active
			{
				if(MIDI_controllers_updated==MIDI_WHEEL_CONTROLLER_PB_UPDATED)
				{
					//printf("MIDI_WHEEL_CONTROLLER_PB => %d\n", MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB]);

					//ideal: map from 64-127 MIDI values to GRANULAR_DETUNE_COEFF_SET-GRANULAR_DETUNE_COEFF_MAX, in log scale

					//simpler try first:

					//map from 64-127 MIDI values to 0-GRANULAR_DETUNE_COEFF_MAX, in linear scale
					if(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB]>=64)
					{
						detune_coeff = global_settings.GRANULAR_DETUNE_COEFF_MAX * ((float)MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB]-64.0f) / 64.0f;
					}
					//map from 0-63 MIDI values to GRANULAR_DETUNE_COEFF_MUL-0, in linear scale
					else //if(MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB]<64)
					{
						//detune_coeff = global_settings.GRANULAR_DETUNE_COEFF_MUL * (64.0f-(float)MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB]) / 64.0f;
						detune_coeff = -0.15f * global_settings.GRANULAR_DETUNE_COEFF_MAX * (64.0f-(float)MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB]) / 64.0f;
					}

					update_grain_freqs(freq, base_chord_33, GRAIN_VOICES_MAX, detune_coeff);
					MIDI_controllers_updated = 0;

					//printf("granular_sampler_simple(): detune_coeff=%f\n", detune_coeff);
				}
			}
			else //manual detune only enabled until MIDI controller wheels active
			{
			#endif
				#define GRANULAR_UI_CMD_DETUNE_DECREASE		1
				#define GRANULAR_UI_CMD_DETUNE_INCREASE		2
				#define GRANULAR_UI_CMD_NEXT_SONG			3
				#define GRANULAR_UI_CMD_LOAD_SONG			4

				//map UI commands
				#ifdef BOARD_WHALE
				if(short_press_volume_plus) { ui_command = GRANULAR_UI_CMD_DETUNE_DECREASE; short_press_volume_plus = 0; }
				if(short_press_volume_minus) { ui_command = GRANULAR_UI_CMD_DETUNE_INCREASE; short_press_volume_minus = 0; }
				if(short_press_sequence==2) { ui_command = GRANULAR_UI_CMD_NEXT_SONG; short_press_sequence = 0; }
				//if(short_press_sequence==-2) { ui_command = GRANULAR_UI_CMD_RESERVED; short_press_sequence = 0; }
				#endif

				#ifdef BOARD_GECHO
				//if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = GRANULAR_UI_CMD_DETUNE_INCREASE; btn_event_ext = 0; }
				//if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = GRANULAR_UI_CMD_DETUNE_DECREASE; btn_event_ext = 0; }
				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = GRANULAR_UI_CMD_NEXT_SONG; btn_event_ext = 0; }
				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = GRANULAR_UI_CMD_LOAD_SONG; btn_event_ext = 0; }

				//detune_coeff = global_settings.GRANULAR_DETUNE_COEFF_MAX * IR_SENSOR_VALUE_S1;
				if(SENSOR_THRESHOLD_RED_1)
				{
					//first 4 LEDs will only detune slightly
					detune_coeff = -(IR_SENSOR_VALUE_S1-IR_sensors_THRESHOLD_1) * 6; //1 note instead of global_settings.GRANULAR_DETUNE_COEFF_MUL

					if(SENSOR_THRESHOLD_RED_5)
					{
						detune_coeff += (IR_SENSOR_VALUE_S1-IR_sensors_THRESHOLD_1) * 12 * 2; //1 octave instead of global_settings.GRANULAR_DETUNE_COEFF_MUL
					}
				}
				else
				{
					detune_coeff = 0;
				}

				if(selected_song)
				{
					update_grain_freqs(freq, gran_chord->bases_parsed + seconds*3, GRAIN_VOICES_MAX, detune_coeff);

				}
				else
				{
					update_grain_freqs(freq, base_chord_33, GRAIN_VOICES_MAX, detune_coeff);
				}

				#endif

				if(ui_command==GRANULAR_UI_CMD_NEXT_SONG)
				{
					codec_set_mute(1); //this takes a while so will stop the sound so there is no buzzing
					selected_song++;
					#ifdef BOARD_WHALE
					RGB_LED_B_ON;
					#endif
					if(selected_song>=GRANULAR_SONGS)
					{
						selected_song = 0; //off again
						#ifdef BOARD_WHALE
						RGB_LED_B_OFF;
						#endif
					}
					//printf("granular_sampler_simple(): selected_song=%d\n", selected_song);
					load_song_for_granular(granular_songs[selected_song]);
					seconds = 0;
					sampleCounter = 1;
					codec_set_mute(0);
				}
				if(ui_command==GRANULAR_UI_CMD_LOAD_SONG)
				{
					codec_set_mute(1); //this takes a while so will stop the sound so there is no buzzing
					//printf("granular_sampler_simple(): load song from NVS, position 0\n");
					load_song_for_granular(granular_songs[0]); //for some reason this needs to be called first
					load_song_for_granular(LOAD_SONG_NVS);
					selected_song = LOAD_SONG_NVS;
					seconds = 0;
					sampleCounter = 1;
					codec_set_mute(0);
				}

				if(ui_command==GRANULAR_UI_CMD_DETUNE_INCREASE || ui_command==GRANULAR_UI_CMD_DETUNE_DECREASE)
				{
					if(ui_command==GRANULAR_UI_CMD_DETUNE_INCREASE)
					{
						detune_coeff *= global_settings.GRANULAR_DETUNE_COEFF_MUL;
					}
					else if(ui_command==GRANULAR_UI_CMD_DETUNE_DECREASE)
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

					//printf("granular_sampler_simple(): detune_coeff=%f\n", detune_coeff);
					if(!selected_song)
					{
						update_grain_freqs(freq, base_chord_33, GRAIN_VOICES_MAX, detune_coeff);
					}
				}
			#ifdef BOARD_GECHO
			}
			#endif

			#ifdef BOARD_WHALE
			if(event_channel_options)
			{
				event_channel_options = 0;
				sample_hold = 1;
				//#ifdef BOARD_WHALE
				RGB_LED_R_OFF;
				RGB_LED_G_ON;
				//#endif
			}
			if(event_next_channel && sample_hold)
			{
				event_next_channel = 0;
				sample_hold = 0;
				//#ifdef BOARD_WHALE
				RGB_LED_G_OFF;
				RGB_LED_R_ON;
				//#endif
			}
			#else

			/*
			//in Gecho, first press of RST cancels MIDI override (if enabled)
			if(event_next_channel && midi_override)
			{
				printf("Granular: MIDI deactivated, will use sensors again\n");
				event_next_channel = 0;
				midi_override = 0;
				MIDI_controllers_active_CC = 0;
				MIDI_controllers_active_PB = 0;
				MIDI_controllers_updated = 0;
			}
			*/

			//in Gecho, sample_hold is controlled either by accelerometer (z-axis) or S2
			if(PARAMETER_CONTROL_SELECTED_IRS)
			{
				sample_hold = !SENSOR_THRESHOLD_ORANGE_1;
			}
			else
			{
				sample_hold = ACC_Z_AXIS > 0.2f ? 1:0;
			}

			/*
			if(!sample_hold)
			{
				sampling_l = 1;
				sampling_r = 1;
			}
			*/
			#endif
		}

	}	//end while(1)

	printf("granular_sampler_simple(): freeing grain_buffers\n");
	free(grain_left);
	free(grain_right);
}


void load_song_for_granular(int song_id)
{
	printf("load_song_for_granular(): song_id = %d\n", song_id);

	if(song_id==-1) //deallocate memory
	{
		printf("load_song_for_granular(): deleting gran_chord\n");
		delete gran_chord;
		gran_chord = NULL;
	}
	else
	{
		if(gran_chord!=NULL)
		{
			printf("load_song_for_granular(): gran_chord not NULL\n");
			load_song_for_granular(-1); //deallocate
		}

		if(song_id>0)
		{
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
	}
}
