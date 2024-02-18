/*
 * Bytebeat.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Jul 14 2018
 *      Author: mario
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of GNU GPLv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/
 *
 *  Bytebeat uses math formulas discovered by Viznut a.k.a. Ville-Matias Heikkilä and his friends.
 *
 *  Sources and more information: http://viznut.fi/en/
 *                                http://viznut.fi/texts-en/bytebeat_algorithmic_symphonies.html
 *
 */

#include <string.h>

#include "Bytebeat.h"
#include "Interface.h"
#include "InitChannels.h"
#include "hw/init.h"
#include "hw/gpio.h"
#include "hw/sdcard.h"
#include "hw/leds.h"

#define BYTEBEAT_SONGS 8
uint8_t bytebeat_song;
int bytebeat_song_ptr;
uint8_t bytebeat_speed_div = 0;

uint8_t bytebeat_speed = 2;
#define BYTEBEAT_SPEED_MAX 1
#define BYTEBEAT_SPEED_MIN 128

#define STEREO_MIXING_STEPS 4
uint8_t stereo_mixing_step = 1; //default mixing 60:40
#ifdef BOARD_WHALE
uint8_t bytebeat_echo_on = 1;
#endif

IRAM_ATTR void channel_bytebeat()
{
	#ifdef BOARD_WHALE
	int bytebeat_echo_cycle = 4;
	#endif

	#define BYTEBEAT_MIXING_VOLUME 4.5f

	bytebeat_init();
	#ifdef BOARD_WHALE
	RGB_LED_B_ON;
	#endif

	sample32 = 0;
	for(int i=0;i<I2S_AUDIOFREQ/2;i++)
	{
		i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
	}

	channel_running = 1;
    //volume_ramp = 1;
	//instead of volume ramp, set the codec volume instantly to un-mute the codec
	codec_digital_volume=codec_volume_user;
	codec_set_digital_volume();
	noise_volume_max = 1.0f;

	sensors_active = 1;

	while(!event_next_channel)
	{
		sample32 = bytebeat_next_sample();
		i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
		sd_write_sample(&sample32);

		sampleCounter++;

		ui_command = 0;

		#define BYTEBEAT_UI_CMD_NEXT_SONG		1
		#define BYTEBEAT_UI_CMD_STEREO_PANNING	2
		#define BYTEBEAT_UI_CMD_SPEED_INCREASE	3
		#define BYTEBEAT_UI_CMD_SPEED_DECREASE	4

		//map UI commands
		#ifdef BOARD_WHALE
		if(short_press_volume_plus) { ui_command = BYTEBEAT_UI_CMD_NEXT_SONG; short_press_volume_plus = 0; }
		if(short_press_volume_minus) { ui_command = BYTEBEAT_UI_CMD_STEREO_PANNING; short_press_volume_minus = 0; }
		if(short_press_sequence==2) { ui_command = BYTEBEAT_UI_CMD_SPEED_INCREASE; short_press_sequence = 0; }
		if(short_press_sequence==-2) { ui_command = BYTEBEAT_UI_CMD_SPEED_DECREASE; short_press_sequence = 0; }
		#endif

		#ifdef BOARD_GECHO
		if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = BYTEBEAT_UI_CMD_NEXT_SONG; btn_event_ext = 0; }
		if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = BYTEBEAT_UI_CMD_STEREO_PANNING; btn_event_ext = 0; }
		if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_1) { ui_command = BYTEBEAT_UI_CMD_SPEED_DECREASE; btn_event_ext = 0; }
		if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_2) { ui_command = BYTEBEAT_UI_CMD_SPEED_INCREASE; btn_event_ext = 0; }
		#endif

		if(ui_command==BYTEBEAT_UI_CMD_NEXT_SONG)
		{
			bytebeat_next_song();
		}
		if(ui_command==BYTEBEAT_UI_CMD_STEREO_PANNING)
		{
			bytebeat_stereo_paning();
			#ifdef BOARD_WHALE
			bytebeat_echo_on = (bytebeat_echo_cycle%(2*STEREO_MIXING_STEPS)) / STEREO_MIXING_STEPS;
			bytebeat_echo_cycle++;
			printf("channel_bytebeat(): Pannig step, echo = %d (ec=%d), paning step = %d\n", bytebeat_echo_on, bytebeat_echo_cycle, stereo_mixing_step);
			#else
			printf("channel_bytebeat(): Pannig step = %d\n", stereo_mixing_step);
			#endif
		}
		if(ui_command==BYTEBEAT_UI_CMD_SPEED_INCREASE)
		{
			if(bytebeat_speed>BYTEBEAT_SPEED_MAX)
			{
				if(bytebeat_speed>=16)
				{
					bytebeat_speed/=2;
				}
				else
				{
					bytebeat_speed--;
				}
				bytebeat_speed_div = 0;
			}
			printf("channel_bytebeat(): Speed up => %d\n", bytebeat_speed);
		}
		if(ui_command==BYTEBEAT_UI_CMD_SPEED_DECREASE)
		{
			if(bytebeat_speed<BYTEBEAT_SPEED_MIN)
			{
				if(bytebeat_speed>=8)
				{
					bytebeat_speed*=2;
				}
				else
				{
					bytebeat_speed++;
				}
				bytebeat_speed_div = 0;
			}
			printf("channel_bytebeat(): Slow down => %d\n", bytebeat_speed);
		}
	}

	channel_running = 0;
}

void bytebeat_next_song()
{
	bytebeat_song_ptr = 0;
	bytebeat_speed_div = 0;
	bytebeat_song++;
	if(bytebeat_song==BYTEBEAT_SONGS)
	{
		bytebeat_song = 0;
	}
	LED_R8_set_byte(1<<bytebeat_song);
}

void bytebeat_stereo_paning()
{
	uint8_t stereo_mixing_step_indication[STEREO_MIXING_STEPS] = {0x18,0x24,0x42,0x81};

	stereo_mixing_step++;
	if(stereo_mixing_step==STEREO_MIXING_STEPS)
	{
		stereo_mixing_step = 0;
	}
	LED_R8_set_byte(stereo_mixing_step_indication[stereo_mixing_step]);
}

IRAM_ATTR int16_t bytebeat_echo(int16_t sample)
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

		//add echo from the loop
		echo_mix_f = float(sample) + float(echo_buffer[echo_buffer_ptr]) * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;

		if (echo_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER)
		{
			echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER;
		}

		if (echo_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER)
		{
			echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER;
		}

		sample = (int16_t)echo_mix_f;

		//store result to echo, the amount defined by a fragment
		//echo_mix_f = ((float)sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV);
		//echo_mix_f *= ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
		//echo_buffer[echo_buffer_ptr0] = (int16_t)echo_mix_f;
		echo_buffer[echo_buffer_ptr0] = sample;
	}

	return sample;
}

void bytebeat_init()
{
	program_settings_reset();

	echo_dynamic_loop_length = ECHO_BUFFER_LENGTH_DEFAULT; //set default value (can be changed dynamically)
	memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	echo_buffer_ptr0 = 0; //reset pointer

    ECHO_MIXING_GAIN_MUL = 1; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 2; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

    //tt=0;
    bytebeat_song_ptr = 0;
    bytebeat_song = 0;
    bytebeat_speed_div = 0;

    LED_R8_set_byte(1<<bytebeat_song);

	#ifdef BOARD_WHALE
    BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_SHORT;
	#endif
}

IRAM_ATTR uint32_t bytebeat_next_sample()
{
	int tt[4];
	unsigned char s[4];
	float mix1 = 0, mix2 = 0;
	uint16_t sample1, sample2;
	uint32_t bb_sample;
	static unsigned char sensor_p[4] = {0,0,0,0};
	static float s_lpf[4] = {0,0,0,0};
	#define S_LPF_ALPHA	0.06f

	float stereo_mixing[STEREO_MIXING_STEPS] = {0.5f,0.4f,0.2f,0}; //50, 60, 80 and 100% mixing ratio

	if (TIMING_EVERY_100_MS == 43) //10Hz
	//if (TIMING_EVERY_250_MS == 43) //4Hz
	{
		for(int i=0;i<4;i++)
		{
			s_lpf[i] = s_lpf[i] + S_LPF_ALPHA * (ir_res[i] - s_lpf[i]);

			//if(PARAMETER_CONTROL_SELECTED_IRS)
			//{
			if(ir_res[i] > IR_sensors_THRESHOLD_1)
			{
				sensor_p[i] = s_lpf[i]*100;
				//printf("SENSOR_THRESHOLD_ORANGE_1, ir_res[1] = %f, sensor_p[1] = %d\n", ir_res[1], sensor_p[1]);
			}
			else
			{
				sensor_p[i] = 0;
				//printf("SENSOR_THRESHOLD_ORANGE_1 NOT, sensor_p[1] = 0\n");
			}
			//}
		}
	}

	/*	tt++;

	t[0] = tt/6;
	//t[0] = tt/12;
	//t[0] = tt/24;

	//t[1] = t[0];
	t[1] = t[0]/2;
	//t[1] = t[0]/4;

	//t[2] = t[1]/4;
	//t[3] = t[2]/4;

	//s[0] = t[0]*((t[0]>>12|t[0]>>8)&63&t[0]>>4);
	//s[1] = t[1]*((t[1]>>12|t[1]>>8)&63&t[1]>>4);
	//s[2] = t[2]*((t[2]>>12|t[2]>>8)&63&t[2]>>4);
	//s[3] = t[3]*((t[3]>>12|t[3]>>8)&63&t[3]>>4);

	//s[0] = (t[0]*(t[0]>>5|t[0]>>8))>>(t[0]>>16);
	//s[1] = (t[1]*(t[1]>>5|t[1]>>8))>>(t[1]>>16);
	//s[2] = (t[2]*(t[2]>>5|t[2]>>8))>>(t[2]>>16);
	//s[3] = (t[3]*(t[3]>>5|t[3]>>8))>>(t[3]>>16);

	s[0] = t[0]*(t[0]>>11&t[0]>>8&123&t[0]>>3);
	s[1] = t[1]*(t[1]>>11&t[1]>>8&123&t[1]>>3);

	//mix1 = (float)bytebeat_echo(s[0]+s[2]);
	//mix2 = (float)bytebeat_echo(s[1]+s[3]);
	//mix1 = (float)(s[0]+s[2]);
	//mix2 = (float)(s[1]+s[3]);
	mix1 = (float)s[0];
	mix2 = (float)s[1];
*/

	bytebeat_speed_div++;
	if(bytebeat_speed_div==bytebeat_speed)
	{
		bytebeat_speed_div = 0;
	    bytebeat_song_ptr++;
	}

	if(bytebeat_song==0)
	{
		tt[0] = bytebeat_song_ptr/6;

		tt[1] = tt[0]/4;

		s[0] = tt[0]*(tt[0]>>(11)&tt[0]>>(8-sensor_p[0]/5)&(123-sensor_p[2])&tt[0]>>(3+sensor_p[3]/10))*(sensor_p[1]+1);
		s[1] = tt[1]*(tt[1]>>(11)&tt[1]>>(8-sensor_p[0]/5)&(123-sensor_p[2])&tt[1]>>(3+sensor_p[3]/10))*(sensor_p[1]+1);

		mix1 = (float)s[0];
		mix2 = (float)s[1];
	}
	else if(bytebeat_song==1)
	{
		tt[0] = bytebeat_song_ptr/6;

		//t[1] = t[0];
		tt[1] = tt[0]/2;
		//t[1] = t[0]/4;

		tt[2] = bytebeat_song_ptr/(3+sensor_p[2]/10);
		tt[3] = tt[1]/2;

		s[0] = (tt[0]*(tt[0]>>(5-sensor_p[0]/20)|tt[0]>>(8-sensor_p[1]/20)))>>(tt[0]>>(16-sensor_p[3]/10));
		s[1] = (tt[1]*(tt[1]>>(5-sensor_p[0]/20)|tt[1]>>(8-sensor_p[1]/20)))>>(tt[1]>>(16-sensor_p[3]/10));
		s[2] = (tt[2]*(tt[2]>>(5+sensor_p[0]/10)|tt[2]>>(8+sensor_p[1]/10)))>>(tt[2]>>(16-sensor_p[3]/10));
		s[3] = (tt[3]*(tt[3]>>(5+sensor_p[0]/10)|tt[3]>>(8+sensor_p[1]/10)))>>(tt[3]>>(16-sensor_p[3]/10));

		mix1 = (float)(s[0]+s[2]);
		mix2 = (float)(s[1]+s[3]);
	}
	else if(bytebeat_song==2)
	{
		tt[0] = bytebeat_song_ptr/6;

		//t[1] = t[0];
		tt[1] = tt[0]/2;
		//t[1] = t[0]/4;

		//t[2] = t[1]/2;
		//t[3] = t[2]/2;

		s[0] = tt[0]*((tt[0]>>(12-sensor_p[0]/10)|tt[0]>>(8-sensor_p[1]/2))&(63-sensor_p[2])&tt[0]>>(4+sensor_p[3]/8));
		s[1] = tt[1]*((tt[1]>>(12-sensor_p[0]/10)|tt[1]>>(8+sensor_p[1]/2))&(63-sensor_p[2])&tt[1]>>(4+sensor_p[3]/8));
		//s[2] = t[2]*((t[2]>>12|t[2]>>8)&63&t[2]>>4);
		//s[3] = t[3]*((t[3]>>12|t[3]>>8)&63&t[3]>>4);

		mix1 = (float)s[0];
		mix2 = (float)s[1];
		//mix1 = (float)(s[0]+s[2]);
		//mix2 = (float)(s[1]+s[3]);
	}
	else if(bytebeat_song==3)
	{
		tt[0] = bytebeat_song_ptr;

		tt[1] = tt[0]/2;

		s[0] = tt[0]*((tt[0]>>(9-sensor_p[0]/10)|tt[0]>>(13-sensor_p[1]/8))&(25-sensor_p[3]/4)&tt[0]>>(6-sensor_p[2]/20));
		s[1] = tt[1]*((tt[1]>>(9-sensor_p[0]/10)|tt[1]>>(13-sensor_p[1]/8))&(25-sensor_p[3]/4)&tt[1]>>(6-sensor_p[2]/20));

		mix1 = (float)s[0];
		mix2 = (float)s[1];
	}
	else if(bytebeat_song==4)
	{
		tt[0] = bytebeat_song_ptr/3;

		tt[1] = tt[0]/2;

		#define t tt[0]
		s[0] = ((-t&4095)*(255&t*(t&t>>(13-sensor_p[0]/10)))>>(12-sensor_p[1]/10))+(127&t*(234&t>>8&t>>(3+sensor_p[2]/10))>>(3&t>>(14-sensor_p[3]/10)));
		#define t tt[1]
		s[1] = ((-t&4095)*(255&t*(t&t>>(13-sensor_p[0]/10)))>>(12-sensor_p[1]/10))+(127&t*(234&t>>8&t>>(3+sensor_p[2]/10))>>(3&t>>(14-sensor_p[3]/10)));
		#undef t

		//s[0] = (t[0]*(t[0]>>8*(t[0]>>15|t[0]>>8)&(20|(t[0]>>19)*5>>t[0]|t[0]>>3));
		//s[1] = (t[1]*(t[1]>>8*(t[1]>>15|t[1]>>8)&(20|(t[1]>>19)*5>>t[1]|t[1]>>3));

		//s[0] = ((((5&((3 *(23*(4^t[0])))+t[0]))*(9*((15>>((9&((t[0]&12)^15))>>5))*2)))*((((t[0]*(t[0]*8))>>10)&t[0])>>42))^15);
		//s[1] = ((((5&((3 *(23*(4^t[1])))+t[1]))*(9*((15>>((9&((t[1]&12)^15))>>5))*2)))*((((t[1]*(t[1]*8))>>10)&t[1])>>42))^15);

		mix1 = (float)s[0];
		mix2 = (float)s[1];
	}
	else if(bytebeat_song==5)
	{
		tt[0] = bytebeat_song_ptr/3;
		tt[1] = tt[0]/2;

		/*
		//long gaps of silence but very interesting bits!
		#define t tt[0]
		s[0] = t*(t>>((t>>9|t>>8))&63&t>>4);
		#define t tt[1]
		s[1] = t*(t>>((t>>9|t>>8))&63&t>>4);
		#undef t
		*/

		/*
		//(t^t>>8)|t<<3&56^t
		//that one seems to produce something interesting :D

		//low tone motorboating with quieter high buzzing
		#define t tt[0]
		s[0] = (t^t>>8)|t<<3&56^t;
		#define t tt[1]
		s[1] = (t^t>>8)|t<<3&56^t;
		#undef t
		*/

		//combined:
		#define t tt[0]
		s[0] = (t*(t>>(((t>>(9-sensor_p[1]/10))|(t>>8)))&(63-sensor_p[2]/2)&(t>>(4+sensor_p[3]/10)))) + ((t^(t>>8))|(((t<<3)&(56-sensor_p[0]/2))^t));
		#define t tt[1]
		s[1] = (t*(t>>(((t>>(9-sensor_p[1]/10))|(t>>8)))&(63-sensor_p[2]/2)&(t>>(4+sensor_p[3]/10)))) + ((t^(t>>8))|(((t<<3)&(56-sensor_p[0]/2))^t));
		#undef t

		/*
		//not fast enough within this sampling rate timing

		//long gaps of silence but very interesting bits - now compressed
		s[0] = 0;
		s[1] = 0;
		while(s[0]==0 && s[1]==0)
		{
			tt[0] = bytebeat_song_ptr/3;
			tt[1] = tt[0]/2;
			#define t tt[0]
			s[0] = t*(t>>((t>>9|t>>8))&63&t>>4);
			#define t tt[1]
			s[1] = t*(t>>((t>>9|t>>8))&63&t>>4);
			#undef t
			bytebeat_song_ptr++;
		}
		bytebeat_song_ptr--;
		*/

		mix1 = (float)s[0];
		mix2 = (float)s[1];
	}
	/*
	else if(bytebeat_song==6)
	{
		tt[0] = bytebeat_song_ptr;///3;
		tt[1] = tt[0]/2;

		//put (t>>1)+(sin(t))*(t>>4)  at 44,100 sample rate
		//it sounds like voices and static

		#define t tt[0]
		s[0] = (t>>1)+(sin(t))*(t>>4);
		#define t tt[1]
		s[1] = (t>>1)+(sin(t))*(t>>4);
		#undef t

		mix1 = (float)s[0];
		mix2 = (float)s[1];
	}
	else if(bytebeat_song==7)
	{
		tt[0] = bytebeat_song_ptr;///3;
		tt[1] = tt[0]/2;

		//try this at 44,100 rate (t>>1)+(cos(t<<1))*(t>>4)

		#define t tt[0]
		s[0] = (t>>1)+(cos(t<<1))*(t>>4);
		#define t tt[1]
		s[1] = (t>>1)+(cos(t<<1))*(t>>4);
		#undef t

		mix1 = (float)s[0];
		mix2 = (float)s[1];
	}
	*/
	else if(bytebeat_song==6)
	{
		tt[0] = bytebeat_song_ptr/3;
		tt[1] = tt[0]/2;

		/*
		//nothing?
		#define t tt[0]
		s[0] = (t>>((t>>(3)|t>>(5)))&(63)&t>>7)*t;
		#define t tt[1]
		s[1] = (t>>((t>>(3)|t>>(5)))&(63)&t>>7)*t;
		#undef t
		*/

		//cool bagpipe-like sound with rhythm
		#define t tt[0]
		s[0] = (sensor_p[0]+1)*t*t*~((t>>(16)|t>>(12))/(sensor_p[2]+1)&215&~t>>(8+sensor_p[1]/2))/(sensor_p[3]+1);
		#define t tt[1]
		s[1] = (sensor_p[0]+1)*t*t*~((t>>(16)|t>>(12))/(sensor_p[2]+1)&215&~t>>(8-sensor_p[1]/2))/(sensor_p[3]+1);
		#undef t

		mix1 = (float)s[0];
		mix2 = (float)s[1];
	}
	else if(bytebeat_song==7)
	{
		tt[0] = bytebeat_song_ptr/3;
		tt[1] = tt[0]/2;

		//Here's one that sounds exactly like music.
		//Hasn't repeated once at all during 66 seconds.
		//t * ((t>>14|t>>9)&92&t>>5)
		//even has an ending! :D SWEET!

		#define t tt[0]
		s[0] = t*((t>>(14-sensor_p[1]/7)|t>>(9-sensor_p[0]/11))&(92-sensor_p[3])&t>>(5+sensor_p[2]/10));
		#define t tt[1]
		s[1] = t*((t>>(14-sensor_p[1]/7)|t>>(9-sensor_p[0]/11))&(92-sensor_p[3])&t>>(5+sensor_p[2]/10));
		#undef t

		mix1 = (float)s[0];
		mix2 = (float)s[1];
	}

	//------------------------------------------------------------------------------------------------------

	//s[0] = (int16_t)(mix1*0.7f+mix2*0.3f);
	//s[1] = (int16_t)(mix2*0.7f+mix1*0.3f);
	//sample1 = (int16_t)(mix1*0.6f*BYTEBEAT_MIXING_VOLUME+mix2*0.4f*BYTEBEAT_MIXING_VOLUME);
	//sample2 = (int16_t)(mix1*0.4f*BYTEBEAT_MIXING_VOLUME+mix2*0.6f*BYTEBEAT_MIXING_VOLUME);
	sample1 = (mix1*stereo_mixing[stereo_mixing_step]+mix2*(1.0f-stereo_mixing[stereo_mixing_step])) * BYTEBEAT_MIXING_VOLUME;
	sample2 = (mix2*stereo_mixing[stereo_mixing_step]+mix1*(1.0f-stereo_mixing[stereo_mixing_step])) * BYTEBEAT_MIXING_VOLUME;

	#ifdef BOARD_WHALE
	if(bytebeat_echo_on)
	{
	#endif
		sample1 = bytebeat_echo(sample1);
		sample2 = bytebeat_echo(sample2);
	#ifdef BOARD_WHALE
	}
	#endif

	bb_sample = sample1;
	bb_sample <<= 16;
	bb_sample |= sample2;

	//return ((uint32_t)s[0])<<7|(((uint32_t)s[1])<<23);
	return bb_sample << 1;
}
