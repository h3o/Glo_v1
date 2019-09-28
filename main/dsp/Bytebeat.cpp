/*
 * Bytebeat.cpp
 *
 *  Created on: Jul 14 2018
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

#include <Bytebeat.h>
#include <Interface.h>
#include <InitChannels.h>
//#include <hw/codec.h>
//#include <hw/signals.h>
#include <hw/init.h>
#include <hw/gpio.h>
#include <string.h>

int bytebeat_song_ptr;
int bytebeat_song;
#define BYTEBEAT_SONGS 8
//#define BYTEBEAT_MCLK

int bytebeat_speed = 2;
int bytebeat_speed_div = 0;
#define BYTEBEAT_SPEED_MAX 1
#define BYTEBEAT_SPEED_MIN 128

unsigned char s[4];
float mix1, mix2;

//int t[4];

//int tt;

#define STEREO_MIXING_STEPS 4
float stereo_mixing[STEREO_MIXING_STEPS] = {0.5f,0.4f,0.2f,0}; //50, 60, 80 and 100% mixing ratio
int stereo_mixing_step = 1; //default mixing 60:40
int bytebeat_echo_on = 1;
int bytebeat_echo_cycle = 4;

int tt[4];

#define BYTEBEAT_MIXING_VOLUME 2.5f

uint16_t sample1, sample2;

uint32_t bb_sample;

void channel_bytebeat()
{
	#ifdef BYTEBEAT_MCLK
	start_MCLK();
	#endif
	//codec_init(); //i2c init

	/*
	echo_dynamic_loop_length = I2S_AUDIOFREQ; //set default value (can be changed dynamically)
	//echo_buffer = (int16_t*)malloc(echo_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	echo_buffer_ptr0 = 0; //reset pointer

    ECHO_MIXING_GAIN_MUL = 1; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 4; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in
    */

	/*
    unsigned char s[4];
	float mix1, mix2;

	int t[4];
	for(int tt=0;;tt++)
	{
		t[0] = tt/8;
		t[1] = t[0]/2;
		t[2] = t[1]/2;
		t[3] = t[2]/2;

		s[0] = t[0]*((t[0]>>12|t[0]>>8)&63&t[0]>>4);
		s[1] = t[1]*((t[1]>>12|t[1]>>8)&63&t[1]>>4);
		s[2] = t[2]*((t[2]>>12|t[2]>>8)&63&t[2]>>4);
		s[3] = t[3]*((t[3]>>12|t[3]>>8)&63&t[3]>>4);

		//mix1 = (float)bytebeat_echo(s1);
		//mix2 = (float)bytebeat_echo(s2);
		mix1 = (float)(s[0]+s[2]);
		mix2 = (float)(s[1]+s[3]);

		s[0] = (int16_t)(mix1*4.5f+mix2*1.5f);
		s[1] = (int16_t)(mix2*4.5f+mix1*1.5f);

		sample32 = s[0]|(s[1]<<16);

		i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
	}
	*/
	bytebeat_init();
    RGB_LED_B_ON;

	sample32 = 0;
	for(int i=0;i<I2S_AUDIOFREQ/2;i++)
	{
		i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
	}

	channel_running = 1;
    volume_ramp = 1;

	while(!event_next_channel)
	{
		sample32 = bytebeat_next_sample();
		i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);

		if(short_press_volume_plus)
		{
			short_press_volume_plus = 0;
			bytebeat_next_song();
		}
		if(short_press_volume_minus)
		{
			short_press_volume_minus = 0;
			bytebeat_stereo_paning();
			bytebeat_echo_on = (bytebeat_echo_cycle%(2*STEREO_MIXING_STEPS)) / STEREO_MIXING_STEPS;
			bytebeat_echo_cycle++;
			printf("channel_bytebeat(): Pannig step, echo = %d (ec=%d), paning step = %d\n", bytebeat_echo_on, bytebeat_echo_cycle, stereo_mixing_step);
		}
		if(short_press_sequence==2)
		{
			short_press_sequence = 0;
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
		if(short_press_sequence==-2)
		{
			short_press_sequence = 0;
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

	#ifdef BYTEBEAT_MCLK
	stop_MCLK();
	#endif

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
}

void bytebeat_stereo_paning()
{
	stereo_mixing_step++;
	if(stereo_mixing_step==STEREO_MIXING_STEPS)
	{
		stereo_mixing_step = 0;
	}
}

int16_t bytebeat_echo(int16_t sample)
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

	return sample;
}

void bytebeat_init()
{
	program_settings_reset();

	//echo_dynamic_loop_length = I2S_AUDIOFREQ / 128; //set default value (can be changed dynamically)
	echo_dynamic_loop_length = ECHO_BUFFER_LENGTH_DEFAULT; //set default value (can be changed dynamically)
	//echo_buffer = (int16_t*)malloc(echo_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	echo_buffer_ptr0 = 0; //reset pointer

    ECHO_MIXING_GAIN_MUL = 1; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 2; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

    //tt=0;
    bytebeat_song_ptr = 0;
    bytebeat_song = 0;
    bytebeat_speed_div = 0;

    BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_SHORT;
}

uint32_t bytebeat_next_sample()
{
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

		s[0] = tt[0]*(tt[0]>>11&tt[0]>>8&123&tt[0]>>3);
		s[1] = tt[1]*(tt[1]>>11&tt[1]>>8&123&tt[1]>>3);

		mix1 = (float)s[0];
		mix2 = (float)s[1];
	}
	else if(bytebeat_song==1)
	{
		tt[0] = bytebeat_song_ptr/6;

		//t[1] = t[0];
		tt[1] = tt[0]/2;
		//t[1] = t[0]/4;

		tt[2] = bytebeat_song_ptr/3;
		tt[3] = tt[1]/2;

		s[0] = (tt[0]*(tt[0]>>5|tt[0]>>8))>>(tt[0]>>16);
		s[1] = (tt[1]*(tt[1]>>5|tt[1]>>8))>>(tt[1]>>16);
		s[2] = (tt[2]*(tt[2]>>5|tt[2]>>8))>>(tt[2]>>16);
		s[3] = (tt[3]*(tt[3]>>5|tt[3]>>8))>>(tt[3]>>16);

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

		s[0] = tt[0]*((tt[0]>>12|tt[0]>>8)&63&tt[0]>>4);
		s[1] = tt[1]*((tt[1]>>12|tt[1]>>8)&63&tt[1]>>4);
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

		s[0] = tt[0]*((tt[0]>>9|tt[0]>>13)&25&tt[0]>>6);
		s[1] = tt[1]*((tt[1]>>9|tt[1]>>13)&25&tt[1]>>6);

		mix1 = (float)s[0];
		mix2 = (float)s[1];
	}
	else if(bytebeat_song==4)
	{
		tt[0] = bytebeat_song_ptr/3;

		tt[1] = tt[0]/2;

		#define t tt[0]
		s[0] = ((-t&4095)*(255&t*(t&t>>13))>>12)+(127&t*(234&t>>8&t>>3)>>(3&t>>14));
		#define t tt[1]
		s[1] = ((-t&4095)*(255&t*(t&t>>13))>>12)+(127&t*(234&t>>8&t>>3)>>(3&t>>14));
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
		s[0] = (t*(t>>(((t>>9)|(t>>8)))&63&(t>>4))) + ((t^(t>>8))|(((t<<3)&56)^t));
		#define t tt[1]
		s[1] = (t*(t>>(((t>>9)|(t>>8)))&63&(t>>4))) + ((t^(t>>8))|(((t<<3)&56)^t));
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
		s[0] = t*t*~((t>>16|t>>12)&215&~t>>8);
		#define t tt[1]
		s[1] = t*t*~((t>>16|t>>12)&215&~t>>8);
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
		s[0] = t*((t>>14|t>>9)&92&t>>5);
		#define t tt[1]
		s[1] = t*((t>>14|t>>9)&92&t>>5);
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

	if(bytebeat_echo_on)
	{
		sample1 = bytebeat_echo(sample1);
		sample2 = bytebeat_echo(sample2);
	}

	bb_sample = sample1;
	bb_sample <<= 16;
	bb_sample |= sample2;

	//return ((uint32_t)s[0])<<7|(((uint32_t)s[1])<<23);
	return bb_sample << 1;
}

