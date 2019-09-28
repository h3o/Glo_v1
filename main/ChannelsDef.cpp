/*
 * ChannelsDef.cpp
 *
 *  Created on: Feb 02, 2019
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

#include <ChannelsDef.h>
#include <InitChannels.h>
#include <FilteredChannels.h>

#include <Interface.h>
#include <hw/codec.h>
//#include <dsp/FreqDetect.h>
#include <dsp/Filters.h>

#include <dsp/Granular.h>
#include <dsp/Reverb.h>
#include <dsp/Antarctica.h>
//#include <dsp/Bytebeat.h>
#include <dsp/DCO_Synth.h>
//#include <dsp/Chopper.h>
//#include <dsp/Dekrispator.h>

#include <mi/clouds/clouds.h>

void channel_high_pass(int song, int melody, int sample, float sample_volume)
{
	//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
	channel_init(sample, song, melody, FILTERS_TYPE_HIGH_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_ALL);
	filtered_channel(sample, 0, 0, sample_volume); //params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef
}

void channel_high_pass_high_reso(int song, int melody, int sample)
{
	//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
	channel_init(sample, song, melody, FILTERS_TYPE_HIGH_PASS, FILTERS_RESONANCE_HIGHER, 1, 0, ACTIVE_FILTER_PAIRS_ALL);
	filtered_channel(4, 0, 0, 0.5f); //params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef
}

void channel_low_pass_high_reso(int song, int melody, int sample)
{
	//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
	channel_init(sample, song, melody, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_HIGHER, 1, 0, ACTIVE_FILTER_PAIRS_ALL);
	filtered_channel(4, 0, 0, 1.0f); //params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef
}

void channel_low_pass(int song, int melody)
{
	//#define OpAmp_ADC12_signal_conversion_factor  (OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT*2) //more sensitive
	//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices...
	channel_init(0, song, melody, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 0, DEFAULT_WIND_VOICES, ACTIVE_FILTER_PAIRS_ALL);
	filtered_channel(0, 0, 0, 1.0f); //params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef
}

void channel_direct_waves(int use_reverb)
{
	//selected_song = 41; //simple chord in A key over more octaves (minor/major)
	if(use_reverb)
	{
		//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
		channel_init(0, 41, 0, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_ONE_LESS);
		//channel_init(0, 41, 0, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_TWO_LESS);
		//params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef
		filtered_channel(0, 1, use_reverb, 0.5f);
	}
	else
	{
		//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
	    printf("----START-----[channel_direct_waves()] Free heap: %u\n", xPortGetFreeHeapSize());
		channel_init(0, 41, 0, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_ALL);
	    printf("----START-----[after channel_init()] Free heap: %u\n", xPortGetFreeHeapSize());
		//params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef
		filtered_channel(0, 1, use_reverb, 1.0f);
	    printf("----START-----[after filtered_channel()] Free heap: %u\n", xPortGetFreeHeapSize());
	}
}

void channel_antarctica()
{
	printf("ARCTIC_WIND_TEST: start_codec()\n");
	//start_MCLK();
	//codec_init(); //i2c init
	channel_running = 1;
    volume_ramp = 1;
	printf("ARCTIC_WIND_TEST: codec started\n");

	#ifdef BOARD_WHALE
	//glow the LED in white
	RGB_LED_R_ON;
	RGB_LED_G_ON;
	RGB_LED_B_ON;
	#endif

	song_of_wind_and_ice();
	//stop_MCLK();
}

void DCO_play(void *pvParameters)
{
	printf("DCO_play(): task running on core ID=%d\n",xPortGetCoreID());

    BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_SHORT;

	DCO_Synth *dco = new DCO_Synth();
	dco->v1_init();
	dco->v1_play_loop();

    channel_running = 0;

	printf("DCO_play(): task done, freeing DCO_Synth object\n");
	delete(dco);

    printf("DCO_play(): deleting task...\n");
	vTaskDelete(NULL);
    printf("DCO_play(): task deleted\n");
}

void channel_dco()
{
	channel_running = 1;
    volume_ramp = 1;

	//DCO runs on core 1
	xTaskCreatePinnedToCore((TaskFunction_t)&DCO_play, "dco_play_task", 2048, NULL, 24, NULL, 1);

	while(channel_running) //wait till channel ends
	{
		Delay(10); //free some CPU for tasks on core 0
	}

	//else if (prog == CHANNEL_34_DCO_SYNTH_PROGRAMMABLE)
	//dco->v2_init();
	//dco->v2_play_loop();
	//dco->v3_play_loop();
	//dco->v4_play_loop();
}

void channel_granular()
{
	//no init, happens inside of the function
	//granular_sampler(31); //select song #31
	granular_sampler_simple(); //no song, direct control
}

//void channel_sampler()
//{
//	//no init, happens inside of the function
//	granular_sampler_segmented();
//}

void channel_reverb()
{
	//no init, happens inside of the function
	start_MCLK();
	decaying_reverb();
	stop_MCLK();
}

void channel_mi_clouds()
{
	//deinit_echo_buffer();
	clear_unallocated_memory();
	i2s_driver_uninstall(I2S_NUM);
	init_i2s_and_gpio(CODEC_BUF_COUNT_CLOUDS, CODEC_BUF_LEN_CLOUDS);
	#ifdef CLOUDS_FAUX_LOW_SAMPLE_RATE
	clouds_main(0);
	#else
	clouds_main(1);
	#endif
	i2s_driver_uninstall(I2S_NUM);
	init_i2s_and_gpio(CODEC_BUF_COUNT_DEFAULT, CODEC_BUF_LEN_DEFAULT);
	//init_echo_buffer();
}

void pass_through()
{
	#define MIN_V 32000		//initial minimum value, should be higher than any possible value
	#define MAX_V -32000	//initial maximum value, should be lower than any possible value

	int16_t min_lr, min_rr, max_lr, max_rr; //raw
	float min_lf, min_rf, max_lf, max_rf; //converted
	int32_t min_l, min_r, max_l, max_r; //final

	program_settings_reset();
	channel_running = 1;
	volume_ramp = 1;

    RGB_LED_R_ON;

    while(!event_next_channel)
    {
		if (!(sampleCounter & 0x00000001)) //right channel
		{
			//r = PseudoRNG1a_next_float();
			//memcpy(&random_value, &r, sizeof(random_value));

			sample_f[0] = (float)((int16_t)(ADC_sample >> 16)) * OpAmp_ADC12_signal_conversion_factor;

			if((int16_t)(ADC_sample >> 16)<min_rr) { min_rr = (int16_t)(ADC_sample >> 16); }
			if((int16_t)(ADC_sample >> 16)>max_rr) { max_rr = (int16_t)(ADC_sample >> 16); }

			if(sample_f[0]<min_rf) { min_rf = sample_f[0]; }
			if(sample_f[0]>max_rf) { max_rf = sample_f[0]; }
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			sample_f[1] = (float)((int16_t)ADC_sample) * OpAmp_ADC12_signal_conversion_factor;

			if((int16_t)ADC_sample<min_lr) { min_lr = (int16_t)ADC_sample; }
			if((int16_t)ADC_sample>max_lr) { max_lr = (int16_t)ADC_sample; }

			if(sample_f[1]<min_lf) { min_lf = sample_f[1]; }
			if(sample_f[1]>max_lf) { max_lf = sample_f[1]; }

			sample_mix = sample_f[1] * MAIN_VOLUME;

			sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);

			if(sample_i16<min_l) { min_l = sample_i16; }
			if(sample_i16>max_l) { max_l = sample_i16; }
		}
		else
		{
			sample_mix = sample_f[0] * MAIN_VOLUME;

			sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);

			if(sample_i16<min_r) { min_r = sample_i16; }
			if(sample_i16>max_r) { max_r = sample_i16; }
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			sample32 += sample_i16 << 16;
		}
		else
		{
			sample32 = sample_i16;
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			n_bytes = i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);

			ADC_result = i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
		}

		sampleCounter++;

    	if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION) //one full second passed
    	{
    		seconds++;
    		sampleCounter = 0;

    		printf("LEFT: %d - %d (raw), %f - %f / %d - %d (out), RIGHT: %d - %d (raw), %f - %f / %d - %d (out)\n", min_lr, max_lr, min_lf, max_lf, min_l, max_l, min_rr, max_rr, min_rf, max_rf, min_r, max_r);

    		min_l=MIN_V;
    		min_r=MIN_V;
    		max_l=MAX_V;
    		max_r=MAX_V;
    		min_lf=MIN_V;
    		min_rf=MIN_V;
    		max_lf=MAX_V;
    		max_rf=MAX_V;
    		min_lr=MIN_V;
    		min_rr=MIN_V;
    		max_lr=MAX_V;
    		max_rr=MAX_V;
    	}

    } //end skip channel cnt
}

