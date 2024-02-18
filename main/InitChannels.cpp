/*
 * InitChannels.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Nov 26, 2016
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

#include <string.h>

#include "InitChannels.h"
#include "Accelerometer.h"
#include "Interface.h"
#include "hw/init.h"
#include "hw/gpio.h"
#include "hw/leds.h"
#include "hw/signals.h"
#include "hw/sdcard.h"
#include "hw/midi.h"
#include "hw/sync.h"
#include "glo_config.h"
#include "binaural.h"

//#define ENABLE_MCLK_AT_CHANNEL_INIT
#define SEND_MIDI_NOTES
//#define ENABLE_SD_RECORDING

uint64_t channel;

float chaotic_coef = 1.0f;
float dc_offset_sum = 0.0f, dc_offset = 0.0f;
int dc_offset_cnt = 0;

int param_i;

int mixed_sample_buffer_ptr_L, mixed_sample_buffer_ptr_R;

int16_t *mixed_sample_buffer = NULL;
int MIXED_SAMPLE_BUFFER_LENGTH;
unsigned int mixed_sample_buffer_adr, mixed_sample_buffer_adr_aligned_64k, mixed_sample_buffer_align_offset;

uint16_t t_TIMING_BY_SAMPLE_EVERY_250_MS; //optimization of timing counters
uint16_t t_TIMING_BY_SAMPLE_EVERY_125_MS; //optimization of timing counters

extern int wind_voices;

float new_mixing_vol; //tmp var for accelerometer-driven ARP voice volume calculation

Filters *fil = NULL;

spi_flash_mmap_handle_t mmap_handle_samples = 0;//NULL;

void channel_init(int bg_sample, int song_id, int melody_id, int filters_type, float resonance, int use_mclk, int set_wind_voices, int set_active_filter_pairs, int reverb_ext, int use_reverb)
{
	printf("channel_init(): Free heap[start]: %u\n", xPortGetFreeHeapSize());

	//GPIO_LEDs_Buttons_Reset();
	program_settings_reset();

	ACTIVE_FILTERS_PAIRS = set_active_filter_pairs; //override value set in program_settings_reset();

	selected_song = song_id;
	selected_melody = melody_id;
	wind_voices = set_wind_voices;

	//RGB_LED_OFF;

	if(use_binaural)
	{
		//load binaural profile
    	printf("channel_init(): loading new binaural_profile\n");
		binaural_profile = new(binaural_profile_t);
		load_binaural_profile(binaural_profile, use_binaural);
    	printf("channel_init(): binaural_profile loaded, total length = %d sec, coords = ", binaural_profile->total_length);
    	for(int i=0;i<binaural_profile->total_coords;i++)
    	{
    		printf("%d/%f ", binaural_profile->coord_x[i], binaural_profile->coord_y[i]);
    	}
    	printf("\n");
	}
	else
	{
		indicate_binaural(NULL);
	}

    //printf("[program_settings_reset]Free heap: %u\n", xPortGetFreeHeapSize());
    //if(!heap_caps_check_integrity_all(true)) { printf("---> HEAP CORRUPT!\n"); }

	#ifdef ENABLE_MCLK_AT_CHANNEL_INIT
	if(use_mclk)
	{
		enable_out_clock();
		mclk_enabled = 1;
	}
	#endif

	#ifdef SWITCH_I2C_SPEED_MODES
	i2c_master_deinit();
    i2c_master_init(1); //fast mode
	#endif

	printf("channel_init(song_id = %d, melody_id = %d, filters_type = %d, resonance = %f)\n", song_id, melody_id, filters_type, resonance);

	if(filters_type!=FILTERS_TYPE_NO_FILTERS)
	{
		if(filters_type == FILTERS_TYPE_HIGH_PASS)
		{
			FILTERS_TYPE_AND_ORDER = FILTERS_TYPE_HIGH_PASS + FILTERS_ORDER_4;
		}
		else
		{
			FILTERS_TYPE_AND_ORDER = FILTERS_TYPE_LOW_PASS + FILTERS_ORDER_4;
			//FILTERS_TYPE_AND_ORDER = FILTERS_TYPE_LOW_PASS + FILTERS_ORDER_8;
			//FILTERS_TYPE_AND_ORDER = FILTERS_TYPE_LOW_PASS + FILTERS_ORDER_2;
		}

		filters_init(resonance);
		//printf("[filters_and_signals_init]Free heap: %u\n", xPortGetFreeHeapSize());
		//if(!heap_caps_check_integrity_all(true)) { printf("---> HEAP CORRUPT!\n"); }
	}

	printf("channel_init(bg_sample = %d)\n",bg_sample);

	if(bg_sample)
	{
	    int samples_map[2*SAMPLES_MAX+2]; //0th element holds number of found samples
	    get_samples_map(samples_map);

	    printf("get_samples_map() returned %d samples, last one = %d\n", samples_map[0], samples_map[1]);
	    if(samples_map[0]>0)
	    {
	    	for(int i=1;i<samples_map[0]+1;i++)
	    	{
	    		printf("%d-%d,",samples_map[2*i],samples_map[2*i+1]);
	    	}
	    	printf("\n");
	    }

		if(bg_sample > samples_map[0]) //not enough samples
		{
			printf("channel_init(): requested sample #%d not found in the samples map, there is only %d samples\n", bg_sample, samples_map[0]);
			while(1) { error_blink(15,60,5,30,5,30); /*RGB:count,delay*/ }; //halt
		}

		mixed_sample_buffer_adr = SAMPLES_BASE + samples_map[2*bg_sample];//SAMPLES_BASE;
		MIXED_SAMPLE_BUFFER_LENGTH = samples_map[2*bg_sample+1] / 2; //samples_map[bg_sample] / 2; //size in 16-bit samples, not bytes!

		printf("calculated position for sample %d = 0x%x (%d), length = %d (in 16-bit samples)\n", bg_sample, mixed_sample_buffer_adr, mixed_sample_buffer_adr, MIXED_SAMPLE_BUFFER_LENGTH);

		printf("mapping bg sample = %d\n", bg_sample);

		//mapping address must be aligned to 64kB blocks (0x10000)
		mixed_sample_buffer_adr_aligned_64k = mixed_sample_buffer_adr & 0xFFFF0000;
		mixed_sample_buffer_align_offset = mixed_sample_buffer_adr - mixed_sample_buffer_adr_aligned_64k;

		printf("Mapping address aligned to %x (%d), offset = %x (%d)\n", mixed_sample_buffer_adr_aligned_64k, mixed_sample_buffer_adr_aligned_64k, mixed_sample_buffer_align_offset, mixed_sample_buffer_align_offset);

		printf("Mapping flash memory at %x (+%x length)\n", mixed_sample_buffer_adr_aligned_64k, mixed_sample_buffer_align_offset + MIXED_SAMPLE_BUFFER_LENGTH*2);

		const void *ptr1;
		int esp_result = spi_flash_mmap(mixed_sample_buffer_adr_aligned_64k, (size_t)(mixed_sample_buffer_align_offset+MIXED_SAMPLE_BUFFER_LENGTH*2), SPI_FLASH_MMAP_DATA, &ptr1, &mmap_handle_samples);
		printf("spi_flash_mmap() result = %d\n",esp_result);
		printf("spi_flash_mmap() mapped destination = %x\n",(unsigned int)ptr1);

		if(ptr1==NULL)
		{
			printf("spi_flash_mmap() returned NULL pointer\n");
			while(1);
		}

		printf("mmap_res: handle=%d ptr=%p\n", mmap_handle_samples, ptr1);

		mixed_sample_buffer = (short int*)ptr1;
		mixed_sample_buffer += mixed_sample_buffer_align_offset / 2; //as it is in samples, not bytes

		//spi_flash_mmap_dump();
		//printf("[spi_flash_mmap]Free heap: %u\n", xPortGetFreeHeapSize());
		//if(!heap_caps_check_integrity_all(true)) { printf("---> HEAP CORRUPT!\n"); }

		mixed_sample_buffer_ptr_L = 0;
		mixed_sample_buffer_ptr_R = MIXED_SAMPLE_BUFFER_LENGTH / 2; //set second channel to mid of the buffer for better stereo effect

		noise_volume_max = 0;
		noise_volume = 0;
		noise_boost_by_sensor = 0;

		//OpAmp_ADC12_signal_conversion_factor = OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT;
	}
	else
	{
		noise_volume_max = 1.0f;
		noise_volume = 1.0f;
	}

	echo_dynamic_loop_current_step = ECHO_DYNAMIC_LOOP_LENGTH_DEFAULT_STEP;
	echo_dynamic_loop_length = ECHO_BUFFER_LENGTH_DEFAULT;

	//printf("clear echo buffer (size=%d)...",sizeof(echo_buffer));
	//memset(echo_buffer,0,sizeof(echo_buffer));
	printf("clear echo buffer (size=%d)...",ECHO_BUFFER_LENGTH*sizeof(int16_t));
	memset(echo_buffer,0,ECHO_BUFFER_LENGTH*sizeof(int16_t));
	echo_buffer_ptr0 = 0;
	printf("done!\n");

    if(use_reverb)
    {
    	printf("channel_init(): Free heap[allocating reverb]: %u\n", xPortGetFreeHeapSize());

    	bit_crusher_reverb = BIT_CRUSHER_REVERB_DEFAULT;

    	REVERB_MIXING_GAIN_MUL = REVERB_MIXING_GAIN_MUL_DEFAULT; //amount of signal to feed back to reverb loop, expressed as a fragment
    	REVERB_MIXING_GAIN_DIV = REVERB_MIXING_GAIN_DIV_DEFAULT; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

    	reverb_dynamic_loop_length = BIT_CRUSHER_REVERB_DEFAULT; //set default value (can be changed dynamically)
    	reverb_buffer = (int16_t*)malloc(REVERB_BUFFER_LENGTH*sizeof(int16_t));

    	memset(reverb_buffer,0,REVERB_BUFFER_LENGTH*sizeof(int16_t)); //clear memory
    	reverb_buffer_ptr0 = 0; //reset pointer

    	printf("channel_init(): Free heap[reverb allocated]: %u\n", xPortGetFreeHeapSize());
    }

    if(reverb_ext)
    {
    	for(int r=0;r<REVERB_BUFFERS_EXT;r++)
    	{
    		reverb_dynamic_loop_length_ext[r] = BIT_CRUSHER_REVERB_DEFAULT_EXT(r);
    		reverb_buffer_ext[r] = (int16_t*)malloc(BIT_CRUSHER_REVERB_MAX_EXT(r)*sizeof(int16_t)); //allocate memory

    		memset(reverb_buffer_ext[r],0,BIT_CRUSHER_REVERB_MAX_EXT(r)*sizeof(int16_t)); //clear memory
    		reverb_buffer_ptr0_ext[r] = 0; //reset pointer

    		printf("channel_init(): Free heap[extended reverb #%d allocated]: %u\n", r, xPortGetFreeHeapSize());
    	}

    	REVERB_MIXING_GAIN_MUL_EXT = REVERB_MIXING_GAIN_MUL_EXT_DEFAULT; //amount of signal to feed back to reverb loop, expressed as a fragment
    	REVERB_MIXING_GAIN_DIV_EXT = REVERB_MIXING_GAIN_DIV_EXT_DEFAULT; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in
    }

	fixed_arp_level = 0;

	SAMPLE_VOLUME = SAMPLE_VOLUME_DEFAULT;
	limiter_coeff = DYNAMIC_LIMITER_COEFF_DEFAULT;

	memset(ADC_sampleA,0,4*ADC_SAMPLE_BUFFER);
	ADC_sample_ptr = 0;

	#ifdef BOARD_WHALE
	play_button_cnt = 0; //reset to default value
	#endif

    ms10_counter = 0;

    channel_running = 1;
    volume_ramp = 1;

    ui_button3_enabled = 1;
    ui_button4_enabled = 1;

    SENSORS_LEDS_indication_enabled = 1;

    seconds = 0;
    auto_power_off = persistent_settings.AUTO_POWER_OFF*600;//global_settings.AUTO_POWER_OFF_TIMEOUT;
    //printf("channel_init(): setting initial value for [auto_power_off] by global_settings.AUTO_POWER_OFF_TIMEOUT = %d\n", global_settings.AUTO_POWER_OFF_TIMEOUT);

	TEMPO_BY_SAMPLE = get_tempo_by_BPM(tempo_bpm);
	MELODY_BY_SAMPLE = TEMPO_BY_SAMPLE / 4;

	tempoCounter = SHIFT_CHORD_TIMING_BY_SAMPLE;
	melodyCounter = SHIFT_MELODY_NOTE_TIMING_BY_SAMPLE;

	DELAY_BY_TEMPO = get_delay_by_BPM(tempo_bpm);

	sequencer_LEDs_timing_seq = 0;

	#ifdef BOARD_WHALE
	BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_DEFAULT;
	#endif

	if(use_binaural)
	{
		xTaskCreatePinnedToCore((TaskFunction_t)&binaural_program, "process_binaural_program", 2048, NULL, PRIORITY_BINAURAL_PROGRAM_TASK, NULL, CPU_CORE_BINAURAL_PROGRAM_TASK);
	}

	if(use_acc_or_ir_sensors==PARAMETER_CONTROL_SENSORS_IRS)
	{
		sensors_active = 1;
	}
	if(use_acc_or_ir_sensors==PARAMETER_CONTROL_SENSORS_ACCELEROMETER)
	{
		accelerometer_active = 1;
	}

	MIDI_parser_reset();

	LEDs_all_OFF();

    printf("channel_init(): Free heap[end]: %u\n", xPortGetFreeHeapSize());

    t_start_channel = micros() - t_start_channel;
    printf("channel_init(): finished at %f micros from start\n", t_start_channel);

    printf("channel_init(): tempo_bpm = %d, TEMPO_BY_SAMPLE = %d, start_channel_by_sync_pulse = %d\n", tempo_bpm, TEMPO_BY_SAMPLE, start_channel_by_sync_pulse);

    if(start_channel_by_sync_pulse)
    {
    	uint32_t delayed_start = ((2.0f - t_start_channel) * TEMPO_BY_SAMPLE * 1000) / SAMPLE_RATE_DEFAULT;
        printf("channel_init(): codec_silence(%u)\n", delayed_start);
    	codec_silence(delayed_start); //length in ms
    }
}

void channel_deinit()
{
	//printf("heap_trace_dump(void)\n");
	//heap_trace_dump();
	//if(!heap_caps_check_integrity_all(true)) { printf("---> HEAP CORRUPT!\n"); }

	channel_running = 0;
	sd_playback_channel = 0;
    volume_ramp = 0;
	select_channel_RDY_idle_blink = 1;
	sensors_active = 0;
	accelerometer_active = 0;
	ui_button3_enabled = 1;
	ui_button4_enabled = 1;
	SENSORS_LEDS_indication_enabled = 1;
	context_menu_enabled = 1;

    if(channel_uses_codec)
    {
    	codec_set_mute(1); //mute the codec
    	if(persistent_settings.SAMPLING_RATE != SAMPLE_RATE_DEFAULT)
    	{
    		printf("channel_deinit(): changing sample rate from %d to %d\n", persistent_settings.SAMPLING_RATE, SAMPLE_RATE_DEFAULT);
    		set_sampling_rate(persistent_settings.SAMPLING_RATE);
    	}
    }

    Delay(10); //wait till IR sensors / LEDs indication stopped

    if(use_binaural)
    {
    	//delete binaural profile
    	printf("channel_deinit(): freeing memory and deleting binaural_profile\n");
    	free(binaural_profile->coord_x);
    	free(binaural_profile->coord_y);
    	free(binaural_profile->name);
    	delete(binaural_profile);
    	use_binaural = 0;
    }

	//release some allocated memory
	if(fil != NULL)
	{
		delete(fil);
		fil = NULL;
	}

	if(mixed_sample_buffer!=NULL)
	{
		unload_recorded_sample();

		if(mmap_handle_samples)//!=NULL)
		{
			printf("spi_flash_munmap(mmap_handle_samples) ... ");
			spi_flash_munmap(mmap_handle_samples);
			mmap_handle_samples = 0;//NULL;
			printf("unmapped!\n");
		}

		mixed_sample_buffer = NULL;

		//printf("[spi_flash_munmap]Free heap: %u\n", xPortGetFreeHeapSize());
		//if(!heap_caps_check_integrity_all(true)) { printf("---> HEAP CORRUPT!\n"); }
	}

	if(reverb_buffer!=NULL)
	{
		printf("free(reverb_buffer)\n");
		free(reverb_buffer);
		reverb_buffer=NULL;
	}

	for(int r=0;r<REVERB_BUFFERS_EXT;r++)
	{
		if(reverb_buffer_ext[r]!=NULL)
		{
			printf("free(reverb_buffer_ext[%d])\n", r);
			free(reverb_buffer_ext[r]);
			reverb_buffer_ext[r]=NULL;
		}
	}

    printf("Channel finished.\n");

	#ifdef SWITCH_I2C_SPEED_MODES
    printf("I2C back to normal mode...\n");
    i2c_master_deinit();
    i2c_master_init(0); //normal mode
	#endif

	#ifdef BOARD_WHALE
    //RGB_LEDs_blink(2,10);
	RGB_LED_OFF;

    //Delay(500);

    //the channel probably ended by triggering one of these events
    short_press_button_play = 0;
    long_press_button_play = 0;
	#endif

    //also reset the depending variables
    event_next_channel = 0;
    event_channel_options = 0;
    ui_ignore_events = 0;
    context_menu_active = 0;
    settings_menu_active = 0;

	#ifdef BOARD_WHALE
    BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_DEFAULT;
	#endif

	#ifdef BOARD_WHALE
    play_button_cnt = 0; //reset to default value
	#endif

    indicate_binaural(NULL); //turn off LED

	LEDs_all_OFF();

	printf("done\n");
}
