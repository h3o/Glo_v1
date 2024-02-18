/*
 * ChannelsDef.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Feb 02, 2019
 *      Author: mario
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of GNU GPLv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/
 *
 */

#include "ChannelsDef.h"
#include "FilteredChannels.h"
#include "InitChannels.h"
#include "Interface.h"
#include "hw/codec.h"
#include "hw/midi.h"
#include "hw/sync.h"
#include "hw/leds.h"
#include "hw/sdcard.h"
#include "hw/Accelerometer.h"

//#include "dsp/FreqDetect.h"
#include "dsp/Filters.h"
#include "dsp/Granular.h"
#include "dsp/Reverb.h"
#include "dsp/Antarctica.h"
//#include "dsp/Bytebeat.h"
#include "dsp/DCO_Synth.h"
#include "v1/v1_DCO_Synth.h"
//#include "dsp/Chopper.h"
//#include "dsp/Dekrispator.h"
#include "dsp/KarplusStrong.h"

#include "mi/clouds/clouds.h"

void channel_high_pass(int song, int melody, int sample, float sample_volume)
{
	//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
	channel_init(sample, song, melody, FILTERS_TYPE_HIGH_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_ALL, 0, 1);
	filtered_channel(sample, 0, 0, sample_volume, use_acc_or_ir_sensors); //params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef, use_acc_or_ir_sensors
}

void channel_high_pass_high_reso(int song, int melody, int sample)
{
	//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
	channel_init(sample, song, melody, FILTERS_TYPE_HIGH_PASS, FILTERS_RESONANCE_HIGHER, 1, 0, ACTIVE_FILTER_PAIRS_ALL, 0, 1);
	filtered_channel(4, 0, 0, 0.5f, use_acc_or_ir_sensors); //params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef, use_acc_or_ir_sensors
}

void channel_low_pass_high_reso(int song, int melody, int sample)
{
	//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
	channel_init(sample, song, melody, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_HIGHER, 1, 0, ACTIVE_FILTER_PAIRS_ALL, 0, 1);
	filtered_channel(4, 0, 0, 1.0f, use_acc_or_ir_sensors); //params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef, use_acc_or_ir_sensors
}

void channel_low_pass(int song, int melody)
{
	//#define OpAmp_ADC12_signal_conversion_factor  (OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT*2) //more sensitive
	//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices...
	channel_init(0, song, melody, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 0, DEFAULT_WIND_VOICES, ACTIVE_FILTER_PAIRS_ALL, 0, 1);
	filtered_channel(0, 0, 0, 1.0f, use_acc_or_ir_sensors); //params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef, use_acc_or_ir_sensors
}

void channel_direct_waves(int use_reverb)
{
	//selected_song = 41; //simple chord in A key over more octaves (minor/major)
	if(use_reverb)
	{
		//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
		channel_init(0, 41, 0, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_ONE_LESS, 0, use_reverb);
		//channel_init(0, 41, 0, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_TWO_LESS);
		//params: use_bg_sample, use_direct_waves_filters, use_reverb, mixed_sample_volume_coef, use_acc_or_ir_sensors
		filtered_channel(0, 1, use_reverb, 0.5f, use_acc_or_ir_sensors);
	}
	else
	{
		//params: bg_sample,song,melody,filters_type,resonance,enable_mclk,wind_voices,active_filter_pairs...
	    //printf("----START-----[channel_direct_waves()] Free heap: %u\n", xPortGetFreeHeapSize());
		channel_init(0, 41, 0, FILTERS_TYPE_LOW_PASS, FILTERS_RESONANCE_DEFAULT, 1, 0, ACTIVE_FILTER_PAIRS_ALL, 0, use_reverb);
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
			new_random_value();

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
			i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
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
	}
}

void channel_antarctica()
{
	channel_running = 1;
    volume_ramp = 1;

	#ifdef BOARD_WHALE
	//glow the LED in white
	RGB_LED_R_ON;
	RGB_LED_G_ON;
	RGB_LED_B_ON;
	#endif

	song_of_wind_and_ice();
}

void DCO_play(void *pvParameters)
{
	printf("DCO_play(): task running on core ID=%d\n",xPortGetCoreID());

	#ifdef BOARD_WHALE
	BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_SHORT;
	#endif

	int mode = ((int*)pvParameters)[0];
	if(mode==1)
	{
		v1_DCO_Synth *dco = new v1_DCO_Synth();
		dco->v1_init();
		dco->v1_play_loop();

		channel_running = 0;

		printf("DCO_play(): task done, freeing DCO_Synth object\n");
		delete(dco);
	}
	else if(mode==2)
	{
		DCO_Synth *dco = new DCO_Synth();
		dco->v1_init();
		dco->v1_play_loop();

		channel_running = 0;

		printf("DCO_play(): task done, freeing DCO_Synth object\n");
		delete(dco);
	}

    printf("DCO_play(): deleting task...\n");
	vTaskDelete(NULL);
    printf("DCO_play(): task deleted\n");
}

void channel_dco(int mode)
{
	channel_init(0, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0, 0, 0); //init without any features

	channel_running = 1;
    volume_ramp = 1;

    int dco_params[1];
    dco_params[0] = mode;

	//DCO runs on core 1
	xTaskCreatePinnedToCore((TaskFunction_t)&DCO_play, "dco_play_task", 2048, &dco_params, PRIORITY_CHANNEL_DCO_TASK, NULL, CPU_CORE_CHANNEL_DCO_TASK);

	while(channel_running) //wait till channel ends
	{
		Delay(10); //free some CPU for tasks on core 0
	}
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

void channel_reverb(int extended_buffers)
{
	//no init, happens inside of the function
	#ifdef BOARD_WHALE
	start_MCLK();
	#endif
	decaying_reverb(extended_buffers);
	#ifdef BOARD_WHALE
	stop_MCLK();
	#endif
}

void channel_mi_clouds(int mode)
{
	printf("channel_mi_clouds()[1] Free heap: %u\n", xPortGetFreeHeapSize());
	clear_unallocated_memory();
	printf("channel_mi_clouds()[2] Free heap: %u\n", xPortGetFreeHeapSize());
	i2s_driver_uninstall(I2S_NUM);
	//printf("channel_mi_clouds()[3] Free heap: %u\n", xPortGetFreeHeapSize());
	init_i2s_and_gpio(CODEC_BUF_COUNT_CLOUDS, CODEC_BUF_LEN_CLOUDS, SAMPLE_RATE_DEFAULT); //Clouds ignores user-set sampling rate
	//printf("channel_mi_clouds()[4] Free heap: %u\n", xPortGetFreeHeapSize());
	clouds_main(0, mode); //no sample rate switching
	i2s_driver_uninstall(I2S_NUM);
	init_i2s_and_gpio(CODEC_BUF_COUNT_DEFAULT, CODEC_BUF_LEN_DEFAULT, persistent_settings.SAMPLING_RATE);
}

void pass_through()
{
	#define MIN_V 32000		//initial minimum value, should be higher than any possible value
	#define MAX_V -32000	//initial maximum value, should be lower than any possible value

	int16_t min_lr=MIN_V, min_rr=MIN_V, max_lr=MAX_V, max_rr=MAX_V; //raw
	float min_lf=MIN_V, min_rf=MIN_V, max_lf=MAX_V, max_rf=MAX_V; //converted
	int32_t min_l=MIN_V, min_r=MIN_V, max_l=MAX_V, max_r=MAX_V; //final

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
			//sample32 += sample_i16 << 16;
			sample32 += add_echo(sample_i16) << 16;
		}
		else
		{
			//sample32 = sample_i16;
			sample32 = add_echo(sample_i16);
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
			sd_write_sample(&sample32);

			i2s_read(I2S_NUM, (void*)&ADC_sample, 4, &i2s_bytes_rw, portMAX_DELAY);
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

		if (TIMING_EVERY_20_MS == 33) //50Hz
		{
			if(limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT)
			{
				limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //limiter will fully recover within 0.4 second
			}
		}

    }

	//return input select back to previous user-defined value
	codec_select_input(ADC_input_select);
}

void channel_just_water()
{
	channel_init(0, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0, 0, 0); //init without any features

    int samples_map[2*SAMPLES_MAX+2]; //0th element holds number of found samples
    get_samples_map(samples_map);

	printf("channel_just_water(): get_samples_map() returned %d samples, last one = %d\n", samples_map[0], samples_map[1]);
    if(samples_map[0]>0)
    {
    	for(int i=1;i<samples_map[0]+1;i++)
    	{
    		printf("%d-%d,",samples_map[2*i],samples_map[2*i+1]);
    	}
    	printf("\n");
    }

	//#define BG_SAMPLES_TO_MAP	2 //won't work with 4 separate samples in DATA memory (SPI_FLASH_MMAP_DATA), works with SPI_FLASH_MMAP_INST
	#define BG_SAMPLES_TO_MAP	4 //works with SPI_FLASH_MMAP_INST but memory reads as 0xbad00bad for 3rd and 4th sample

    #if BG_SAMPLES_TO_MAP==2
    spi_flash_mmap_handle_t mmap_handle_samples[BG_SAMPLES_TO_MAP] = {0,0};
    int32_t *mixed_sample_buffer[BG_SAMPLES_TO_MAP] = {NULL,NULL};
    int bg_sample_nums[BG_SAMPLES_TO_MAP] = {1,4}; //#2 is actually combined from #1 and #3
	#else
    spi_flash_mmap_handle_t mmap_handle_samples[BG_SAMPLES_TO_MAP] = {0,0,0,0};
    int32_t *mixed_sample_buffer[BG_SAMPLES_TO_MAP] = {NULL,NULL,NULL,NULL};
    int bg_sample_nums[BG_SAMPLES_TO_MAP] = {1,3,4,6};
	#endif

    unsigned int mixed_sample_buffer_adr[BG_SAMPLES_TO_MAP], mixed_sample_buffer_adr_aligned_64k[BG_SAMPLES_TO_MAP], mixed_sample_buffer_align_offset[BG_SAMPLES_TO_MAP];
    int mixed_sample_buffer_ptr_L[BG_SAMPLES_TO_MAP], mixed_sample_buffer_ptr_R[BG_SAMPLES_TO_MAP];
    int MIXED_SAMPLE_BUFFER_LENGTH[BG_SAMPLES_TO_MAP];
    int bg_sample;
    const void *ptr1[BG_SAMPLES_TO_MAP];

    for(int s=0;s<BG_SAMPLES_TO_MAP;s++)
    {
    	bg_sample = bg_sample_nums[s];
    	mixed_sample_buffer_adr[s] = SAMPLES_BASE + samples_map[2*bg_sample];//SAMPLES_BASE;
    	MIXED_SAMPLE_BUFFER_LENGTH[s] = samples_map[2*bg_sample+1] / 4; //size in 32-bit samples, not bytes!
		#if BG_SAMPLES_TO_MAP==2
    	MIXED_SAMPLE_BUFFER_LENGTH[s] += samples_map[2*bg_sample+3] / 4; //add the subsequent sample, we need 2 pairs
		#endif

    	printf("calculated position for sample #%d (%d) = 0x%x (%d), length = %d (in 32-bit samples)\n", s, bg_sample, mixed_sample_buffer_adr[s], mixed_sample_buffer_adr[s], MIXED_SAMPLE_BUFFER_LENGTH[s]);
    	printf("mapping bg sample #%d (n = %d)\n", s, bg_sample);

    	//mapping address must be aligned to 64kB blocks (0x10000)
    	mixed_sample_buffer_adr_aligned_64k[s] = mixed_sample_buffer_adr[s] & 0xFFFF0000;
    	mixed_sample_buffer_align_offset[s] = mixed_sample_buffer_adr[s] - mixed_sample_buffer_adr_aligned_64k[s];

    	printf("Mapping address aligned to %x (%d), offset = %x (%d)\n", mixed_sample_buffer_adr_aligned_64k[s], mixed_sample_buffer_adr_aligned_64k[s], mixed_sample_buffer_align_offset[s], mixed_sample_buffer_align_offset[s]);
    	printf("Mapping flash memory at %x (+%x length)\n", mixed_sample_buffer_adr_aligned_64k[s], mixed_sample_buffer_align_offset[s] + MIXED_SAMPLE_BUFFER_LENGTH[s]*4);

    	int esp_result;

    	if(s==0)
    	{
    		esp_result = spi_flash_mmap(mixed_sample_buffer_adr_aligned_64k[s],
    										(size_t)(mixed_sample_buffer_align_offset[s]+MIXED_SAMPLE_BUFFER_LENGTH[s]*4),
											SPI_FLASH_MMAP_INST, &ptr1[s], &mmap_handle_samples[s]);
    	}
    	else
    	{
    		esp_result = spi_flash_mmap(mixed_sample_buffer_adr_aligned_64k[s],
    										(size_t)(mixed_sample_buffer_align_offset[s]+MIXED_SAMPLE_BUFFER_LENGTH[s]*4),
											SPI_FLASH_MMAP_DATA, &ptr1[s], &mmap_handle_samples[s]);
    	}

    	printf("spi_flash_mmap() result = %d\n",esp_result);
    	printf("spi_flash_mmap() mapped destination #%d = %x\n", s, (unsigned int)ptr1[s]);

    	if(ptr1[s]==NULL)
    	{
    		printf("spi_flash_mmap() returned NULL pointer\n");
    		while(1);
    	}

    	printf("mmap_res: handle=%d ptr=%p\n", mmap_handle_samples[s], ptr1[s]);
    	printf("1st dword of %p: 0x%08x\n", ptr1[s], ((uint32_t*)(ptr1[s]))[mixed_sample_buffer_align_offset[s]/4]);

    	mixed_sample_buffer[s] = (/*short*/ int*)(ptr1[s]);
    	mixed_sample_buffer[s] += mixed_sample_buffer_align_offset[s] / 4; //as it is in samples, not bytes

    	//spi_flash_mmap_dump();
    	//printf("[spi_flash_mmap]Free heap: %u\n", xPortGetFreeHeapSize());
    	//if(!heap_caps_check_integrity_all(true)) { printf("---> HEAP CORRUPT!\n"); }

    	mixed_sample_buffer_ptr_L[s] = 0;
    	mixed_sample_buffer_ptr_R[s] = MIXED_SAMPLE_BUFFER_LENGTH[s] / 2; //set second channel to mid of the buffer for better stereo effect
    }

	float mixed_sample_volume[BG_SAMPLES_TO_MAP] = {2.0f,2.0f,1.0f,1.0f}, mixed_sample_volume0[BG_SAMPLES_TO_MAP];

	uint32_t read_dword;
	int wordCounter = 0;
	#define PANNING_COEFF	0.4f

	#define S_LPF_ALPHA_SETTINGS 3//5
	float s_lpf_alphas[S_LPF_ALPHA_SETTINGS] = {0.06f, 0.12f, 1.0f};//, 0.0005f, 0.005f};
	int s_lpf_alpha_setting = 0;
	float S_LPF_ALPHA = s_lpf_alphas[s_lpf_alpha_setting];
	float s_lpf[4] = {0,0,0,0};

	while(!event_next_channel)
    {
		//printf("sampleCounter=%d\n",sampleCounter);

		if (!(sampleCounter & 0x00000001)) //right channel
		{
			sample_f[0] = (float)((int16_t)(ADC_sample >> 16)) * OpAmp_ADC12_signal_conversion_factor;

			if(wordCounter & 0x00000001)
			{
				sample_f[0] += ((float)(32768 - ((int16_t)(mixed_sample_buffer[0][mixed_sample_buffer_ptr_L[0]] >> 16))) / 32768.0f) * mixed_sample_volume[0];
				sample_f[0] += ((float)(32768 - ((int16_t)(mixed_sample_buffer[2][mixed_sample_buffer_ptr_L[2]] >> 16))) / 32768.0f) * mixed_sample_volume[2];
			}
			else
			{
				sample_f[0] += ((float)(32768 - ((int16_t)(mixed_sample_buffer[0][mixed_sample_buffer_ptr_L[0]]))) / 32768.0f) * mixed_sample_volume[0];
				sample_f[0] += ((float)(32768 - ((int16_t)(mixed_sample_buffer[2][mixed_sample_buffer_ptr_L[2]]))) / 32768.0f) * mixed_sample_volume[2];
			}
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			sample_f[1] = (float)((int16_t)ADC_sample) * OpAmp_ADC12_signal_conversion_factor;

			if(wordCounter & 0x00000001)
			{
				sample_f[1] += ((float)(32768 - ((int16_t)(mixed_sample_buffer[1][mixed_sample_buffer_ptr_L[1]] >> 16))) / 32768.0f) * mixed_sample_volume[1];
				sample_f[1] += ((float)(32768 - ((int16_t)(mixed_sample_buffer[3][mixed_sample_buffer_ptr_L[3]] >> 16))) / 32768.0f) * mixed_sample_volume[3];
			}
			else
			{
				sample_f[1] += ((float)(32768 - ((int16_t)(mixed_sample_buffer[1][mixed_sample_buffer_ptr_L[1]]))) / 32768.0f) * mixed_sample_volume[1];
				sample_f[1] += ((float)(32768 - ((int16_t)(mixed_sample_buffer[3][mixed_sample_buffer_ptr_L[3]]))) / 32768.0f) * mixed_sample_volume[3];
			}

			sample_mix = (sample_f[1] + sample_f[0] * PANNING_COEFF) * MAIN_VOLUME;
			sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);
		}
		else
		{
			sample_mix = (sample_f[0] + sample_f[1] * PANNING_COEFF) * MAIN_VOLUME;
			sample_i16 = (int16_t)(sample_mix * SAMPLE_VOLUME);
		}

		if(sampleCounter % 4 == 0)
		{
		    for(int s=0;s<BG_SAMPLES_TO_MAP;s++)
		    {
		    	mixed_sample_buffer_ptr_L[s]++;
		    	if(mixed_sample_buffer_ptr_L[s] >= MIXED_SAMPLE_BUFFER_LENGTH[s])
		    	{
		    		mixed_sample_buffer_ptr_L[s] = 0;
		    	}
		    }
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			//sample32 += sample_i16 << 16;
			sample32 += add_echo(sample_i16) << 16;
		}
		else
		{
			//sample32 = sample_i16;
			sample32 = add_echo(sample_i16);
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			//printf("w=%x\n", sample32);
			i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
			sd_write_sample(&sample32);

			i2s_read(I2S_NUM, (void*)&ADC_sample, 4, &i2s_bytes_rw, portMAX_DELAY);
			//printf("r=%x\n", ADC_sample);
		}

		sampleCounter++;
		wordCounter++;

		if (TIMING_BY_SAMPLE_1_SEC) //one full second passed
    	{
    		seconds++;
    		sampleCounter = 0;
    	}

		ui_command = 0;

		if (TIMING_EVERY_20_MS==31) //50Hz
		{
			#define W_UI_CMD_SET_ALPHA			1
			//#define W_UI_CMD_RESERVED			2

			if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = W_UI_CMD_SET_ALPHA; btn_event_ext = 0; }
			//if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = W_UI_CMD_RESERVED; btn_event_ext = 0; }

			if(ui_command==W_UI_CMD_SET_ALPHA)
			{
				s_lpf_alpha_setting++;
				if(s_lpf_alpha_setting==S_LPF_ALPHA_SETTINGS)
				{
					s_lpf_alpha_setting = 0;
				}
				S_LPF_ALPHA = s_lpf_alphas[s_lpf_alpha_setting];
				printf("channel_just_water(): S_LPF_ALPHA = %f\n", S_LPF_ALPHA);
			}

			ui_command = 0;
		}

		if (TIMING_EVERY_20_MS == 19) //50Hz
		{
			if(use_acc_or_ir_sensors==PARAMETER_CONTROL_SENSORS_ACCELEROMETER)
			{
				if(acc_res[1] > 0.0f)
				{
					mixed_sample_volume0[0] = -acc_res[2]*2;
					mixed_sample_volume0[1] = -acc_res[2]*2;
					mixed_sample_volume0[2] = (acc_res[1] + acc_res[0]) * 0.75f;
					mixed_sample_volume0[3] = (acc_res[1] - acc_res[0]) * 0.75f;
				}
				else if(acc_res[1] < 0.0f)
				{
					mixed_sample_volume0[0] = (-acc_res[1] + acc_res[0]) * 1.5f;
					mixed_sample_volume0[1] = (-acc_res[1] - acc_res[0]) * 1.5f;
					mixed_sample_volume0[2] = -acc_res[2];
					mixed_sample_volume0[3] = -acc_res[2];
				}
				if(mixed_sample_volume0[0] < 0.0f) mixed_sample_volume0[0] = 0.0f;
				if(mixed_sample_volume0[1] < 0.0f) mixed_sample_volume0[1] = 0.0f;
				if(mixed_sample_volume0[2] < 0.0f) mixed_sample_volume0[2] = 0.0f;
				if(mixed_sample_volume0[3] < 0.0f) mixed_sample_volume0[3] = 0.0f;

				for(int s=0;s<4;s++) //variable name s is used for both sample counter and sensor counter
				{
					mixed_sample_volume[s] = mixed_sample_volume[s] + S_LPF_ALPHA * (mixed_sample_volume0[s] - mixed_sample_volume[s]);
				}
			}
			else //PARAMETER_CONTROL_SENSORS_IRS
			{
				for(int s=0;s<4;s++) //variable name s is used for both sample counter and sensor counter
				{
					s_lpf[s] = s_lpf[s] + S_LPF_ALPHA * (ir_res[s] - s_lpf[s]);
				}

				mixed_sample_volume[0] = s_lpf[0] * 2;
				mixed_sample_volume[1] = s_lpf[1] * 2;
				mixed_sample_volume[2] = s_lpf[2];
				mixed_sample_volume[3] = s_lpf[3];
			}
		}
    }

    for(int s=0;s<BG_SAMPLES_TO_MAP;s++)
    {
    	if(mmap_handle_samples[s])
    	{
    		printf("spi_flash_munmap(mmap_handle_samples[%d]) ... ", s);
    		spi_flash_munmap(mmap_handle_samples[s]);
    		mmap_handle_samples[s] = 0;
    		printf("unmapped!\n");
    	}
    }
}

void channel_karplus_strong()
{
	KarplusStrong *ks_engine;

	ks_engine = new KarplusStrong(KARPLUS_STRONG_VOICES);

	int midi_override = 0;

	float volume_ramp_ks[2] = {0.0f, 0.0f};

	channel_init(0, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0, 0, 0); //init without any features
	program_settings_reset();
	channel_running = 1;
	volume_ramp = 1;

	LEDs_all_OFF();

	while(!event_next_channel)
	{
		if (!(sampleCounter & 0x00000001)) //right channel
		{
			ks_engine->process_all(sample_f); //contains both L & R samples: sample_f[0], sample_f[1]

			//sample_f[0] = ks_engine->ks_remove_DC_offset_IIR(sample_f[0], 0);
			//sample_f[1] = ks_engine->ks_remove_DC_offset_IIR(sample_f[1], 1);
			sample_f[0] = ks_engine->ks_remove_DC_offset_IIR(sample_f[0] + volume_ramp_ks[0], 0);
			sample_f[1] = ks_engine->ks_remove_DC_offset_IIR(sample_f[1] + volume_ramp_ks[1], 1);
			//sample_f[0] += volume_ramp_ks[0];
			//sample_f[1] += volume_ramp_ks[1];

			#ifdef USE_KS_REVERB
			sample_mix = ks_engine->rebL->Sample(sample_f[0], 3, 0, 2) * ENGINE_KS_RELATIVE_GAIN;
			#else //USE_KS_REVERB
			sample_mix = sample_f[0] * ENGINE_KS_RELATIVE_GAIN;
			#endif //USE_KS_REVERB

			//sample_mix = sample_f[0] * COMPUTED_SAMPLES_MIXING_VOLUME;
			sample_i16 = (int16_t)(sample_mix);// * SAMPLE_VOLUME);
		}
		else
		{
			//check for MIDI activity
			if (TIMING_EVERY_100_MS == 613) //10Hz periodically, at given sample
			{
				if(!midi_override && MIDI_keys_pressed)
				{
					printf("MIDI active, will receive chords\n");
					midi_override = 1;
				}
			}

			//process MIDI messages
			if (TIMING_EVERY_20_MS == 47) //50Hz
			{
				if(midi_override && MIDI_notes_updated)
				{
					MIDI_notes_updated = 0;

					LED_W8_all_OFF();
					LED_B5_all_OFF();

					MIDI_to_LED(MIDI_last_chord[0], 1);
					MIDI_to_LED(MIDI_last_chord[1], 1);
					MIDI_to_LED(MIDI_last_chord[2], 1);
					MIDI_to_LED(MIDI_last_chord[3], 1);

					if(MIDI_last_melody_note)
					{
						printf("MIDI_last_melody_note=%d, MIDI_last_melody_note_velocity=%d\n", MIDI_last_melody_note, MIDI_last_melody_note_velocity);
						ks_engine->new_note(MIDI_note_to_freq(MIDI_last_melody_note), MIDI_last_melody_note_velocity, volume_ramp_ks);

						MIDI_last_melody_note = 0;
					}
				}
			}

			#ifdef USE_KS_REVERB
			sample_mix = ks_engine->rebR->Sample(sample_f[1], 2, 1, 2) * ENGINE_KS_RELATIVE_GAIN;
			#else //USE_KS_REVERB
			sample_mix = sample_f[1] * ENGINE_KS_RELATIVE_GAIN;
			#endif //USE_KS_REVERB

			//sample_mix = sample_f[1] * COMPUTED_SAMPLES_MIXING_VOLUME;
			sample_i16 = (int16_t)(sample_mix);// * SAMPLE_VOLUME);
		}

		if (sampleCounter & 0x00000001) //left channel
		{
			sample32 += sample_i16 << 16;
			i2s_write(I2S_NUM, (void*)&sample32, 4, &i2s_bytes_rw, portMAX_DELAY);
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

	} //event_next_channel

	delete ks_engine;
}
