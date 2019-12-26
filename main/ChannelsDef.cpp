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
#include <hw/midi.h>
#include <hw/leds.h>
#include <hw/sdcard.h>
//#include <dsp/FreqDetect.h>
#include <dsp/Filters.h>

#include <dsp/Granular.h>
#include <dsp/Reverb.h>
#include <dsp/Antarctica.h>
//#include <dsp/Bytebeat.h>
#include <dsp/DCO_Synth.h>
#include <v1/v1_DCO_Synth.h>
//#include <dsp/Chopper.h>
//#include <dsp/Dekrispator.h>

#include <mi/clouds/clouds.h>

void channel_high_pass(int song, int melody, int sample, float sample_volume)
{
	//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
	channel_init(sample, song, melody, FILTERS_TYPE_HIGH_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_ALL);
	filtered_channel(sample, 0, 0, sample_volume, use_acc_or_ir_sensors); //params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef, use_acc_or_ir_sensors
}

void channel_high_pass_high_reso(int song, int melody, int sample)
{
	//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
	channel_init(sample, song, melody, FILTERS_TYPE_HIGH_PASS, FILTERS_RESONANCE_HIGHER, 1, 0, ACTIVE_FILTER_PAIRS_ALL);
	filtered_channel(4, 0, 0, 0.5f, use_acc_or_ir_sensors); //params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef, use_acc_or_ir_sensors
}

void channel_low_pass_high_reso(int song, int melody, int sample)
{
	//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
	channel_init(sample, song, melody, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_HIGHER, 1, 0, ACTIVE_FILTER_PAIRS_ALL);
	filtered_channel(4, 0, 0, 1.0f, use_acc_or_ir_sensors); //params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef, use_acc_or_ir_sensors
}

void channel_low_pass(int song, int melody)
{
	//#define OpAmp_ADC12_signal_conversion_factor  (OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT*2) //more sensitive
	//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices...
	channel_init(0, song, melody, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 0, DEFAULT_WIND_VOICES, ACTIVE_FILTER_PAIRS_ALL);
	filtered_channel(0, 0, 0, 1.0f, use_acc_or_ir_sensors); //params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef, use_acc_or_ir_sensors
}

void channel_direct_waves(int use_reverb)
{
	//selected_song = 41; //simple chord in A key over more octaves (minor/major)
	if(use_reverb)
	{
		//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
		channel_init(0, 41, 0, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_ONE_LESS);
		//channel_init(0, 41, 0, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_TWO_LESS);
		//params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef, use_acc_or_ir_sensors
		filtered_channel(0, 1, use_reverb, 0.5f, use_acc_or_ir_sensors);
	}
	else
	{
		//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
	    //printf("----START-----[channel_direct_waves()] Free heap: %u\n", xPortGetFreeHeapSize());
		channel_init(0, 41, 0, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_ALL);
	    //printf("----START-----[after channel_init()] Free heap: %u\n", xPortGetFreeHeapSize());
		//params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef, use_acc_or_ir_sensors
		filtered_channel(0, 1, use_reverb, 1.0f, use_acc_or_ir_sensors);
	    //printf("----START-----[after filtered_channel()] Free heap: %u\n", xPortGetFreeHeapSize());
	}
}

void channel_white_noise()
{
	program_settings_reset();
	channel_running = 1;
	volume_ramp = 1;

	#ifdef BOARD_WHALE
	RGB_LED_R_ON;
	RGB_LED_G_ON;
	RGB_LED_B_ON;
	#endif

	float r;

	while(!event_next_channel)
	{
		if (!(sampleCounter & 0x00000001)) //right channel
		{
			r = PseudoRNG1a_next_float();
			memcpy(&random_value, &r, sizeof(random_value));

			sample_f[0] = (float)((int16_t)(random_value >> 16)) * OpAmp_ADC12_signal_conversion_factor;
			sample_mix = sample_f[0] * COMPUTED_SAMPLES_MIXING_VOLUME;
			sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);
		}
		else
		{
			sample_f[1] = (float)((int16_t)random_value) * OpAmp_ADC12_signal_conversion_factor;
			sample_mix = sample_f[1] * COMPUTED_SAMPLES_MIXING_VOLUME;
			sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			sample32 += sample_i16 << 16;
			/*n_bytes = */i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
			sd_write_sample(&sample32);
		}
		else
		{
			sample32 = sample_i16;
		}

		sampleCounter++;

		if (TIMING_BY_SAMPLE_1_SEC) //one full second passed
		{
			seconds++;
			sampleCounter = 0;
		}
	} //end skip channel cnt
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

	#ifdef BOARD_WHALE
	BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_SHORT;
	#endif

	//DCO_Synth *dco = new DCO_Synth();
	v1_DCO_Synth *dco = new v1_DCO_Synth();
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
	channel_init(0, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0); //init without any features

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
	#ifdef BOARD_WHALE
	start_MCLK();
	#endif
	decaying_reverb();
	#ifdef BOARD_WHALE
	stop_MCLK();
	#endif
}

void channel_mi_clouds()
{
    /*
	int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
    printf("channel_mi_clouds(): after sd_card_init(), free mem = %u\n", xPortGetFreeHeapSize());

    if(result!=ESP_OK)
	{
		printf("channel_mi_clouds(): problem with SD Card Init, code=%d\n", result);
	}
    */

	//deinit_echo_buffer();
	clear_unallocated_memory();
	i2s_driver_uninstall(I2S_NUM);
	#ifdef CLOUDS_FAUX_LOW_SAMPLE_RATE
	init_i2s_and_gpio(CODEC_BUF_COUNT_CLOUDS, CODEC_BUF_LEN_CLOUDS, SAMPLE_RATE_DEFAULT); //Clouds ignores user-set sampling rate
	clouds_main(0);
	#else
	init_i2s_and_gpio(CODEC_BUF_COUNT_CLOUDS, CODEC_BUF_LEN_CLOUDS, SAMPLE_RATE_CLOUDS);
	clouds_main(1);
	#endif
	i2s_driver_uninstall(I2S_NUM);
	init_i2s_and_gpio(CODEC_BUF_COUNT_DEFAULT, CODEC_BUF_LEN_DEFAULT, persistent_settings.SAMPLING_RATE);
	//init_echo_buffer();
}

void pass_through()
{
	#define MIN_V 32000		//initial minimum value, should be higher than any possible value
	#define MAX_V -32000	//initial maximum value, should be lower than any possible value

	int16_t min_lr, min_rr, max_lr, max_rr; //raw
	float min_lf, min_rf, max_lf, max_rf; //converted
	int32_t min_l, min_r, max_l, max_r; //final

	/*
	//test for powered mics - voltage taken from CV out
	gecho_deinit_MIDI();
	cv_out_init();

	//DAC output is 8-bit. Maximum (255) corresponds to VDD.

	//dac_output_voltage(DAC_CHANNEL_1, 150); //cca 1.76V
	//dac_output_voltage(DAC_CHANNEL_2, 150); //cca 1.76V
	dac_output_voltage(DAC_CHANNEL_1, 200); //cca 2.35V
	dac_output_voltage(DAC_CHANNEL_2, 200); //cca 2.35V
	*/

	program_settings_reset();
	channel_running = 1;
	volume_ramp = 1;

	#ifdef BOARD_WHALE
	RGB_LED_R_ON;
	#endif

	uint8_t l,r,clipping_l=0,clipping_r=0,clipping_restore=0;

	LEDs_all_OFF();

	#define MAX_UV_RANGE	32000.0f

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
			/*n_bytes = */i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
			sd_write_sample(&sample32);

			/*ADC_result = */i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
		}

		sampleCounter++;

		#ifdef PASS_THROUGH_LEVELS_DEBUG
		if (TIMING_BY_SAMPLE_1_SEC) //one full second passed
    	{
    		seconds++;
    		printf("LEFT: %d - %d (raw), %f - %f / %d - %d (out), RIGHT: %d - %d (raw), %f - %f / %d - %d (out)\n", min_lr, max_lr, min_lf, max_lf, min_l, max_l, min_rr, max_rr, min_rf, max_rf, min_r, max_r);
		#else
    	if (TIMING_EVERY_50_MS==0)
        {
    		clipping_restore++;
    		if(clipping_restore==20) //every 1 second
    		{
    			clipping_restore = 0;
    			if(clipping_r>1) clipping_r--;
    			if(clipping_l>1) clipping_l--;
    			LED_B5_set_byte(0x1f >> (5-clipping_r));
    			LED_O4_set_byte(0x0f << (4-clipping_l));
    		}

    		l = 9 - ((float)max_l / MAX_UV_RANGE)*9.0f;
    		r = 9 - ((float)max_r / MAX_UV_RANGE)*9.0f;
    		LED_R8_set_byte( 0xff << l);
    		LED_W8_set_byte( 0xff >> r);

    		if(max_r > MAX_UV_RANGE || min_r < -MAX_UV_RANGE)
    		{
    			if(clipping_r<5)
    			{
    				clipping_r++;
        			LED_B5_set_byte(0x1f >> (5-clipping_r));
    			}
    		}
    		if(max_l > MAX_UV_RANGE || min_l < -MAX_UV_RANGE)
    		{
    			if(clipping_l<4)
    			{
    				clipping_l++;
    				LED_O4_set_byte(0x0f << (4-clipping_l));
    			}
    		}
    	#endif
    		sampleCounter = 0;

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

	//return input select back to previous user-defined value
	codec_select_input(ADC_input_select);
}
