/*
 * InitChannels.cpp
 *
 *  Created on: Nov 26, 2016
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

#include <InitChannels.h>
#include <Accelerometer.h>
#include <Interface.h>
#include <hw/init.h>
#include <hw/gpio.h>
#include <hw/leds.h>
#include <hw/signals.h>
#include <hw/sdcard.h>
#include "glo_config.h"
#include "binaural.h"
#include <string.h>

//#define ENABLE_MCLK_AT_CHANNEL_INIT
#define SEND_MIDI_NOTES
//#define ENABLE_SD_RECORDING

uint64_t channel;

float chaotic_coef = 1.0f;
float dc_offset_sum = 0.0f, dc_offset = 0.0f;
int dc_offset_cnt = 0;

int /*base_freq,*/ param_i;
//float param_f;
//int waves_freqs[WAVES_FILTERS];

//char *melody_str = NULL;
//char *progression_str = NULL;
//int progression_str_length;

int mixed_sample_buffer_ptr_L, mixed_sample_buffer_ptr_R;
//int mixed_sample_buffer_ptr_L2, mixed_sample_buffer_ptr_R2;
//int mixed_sample_buffer_ptr_L3, mixed_sample_buffer_ptr_R3;
//int mixed_sample_buffer_ptr_L4, mixed_sample_buffer_ptr_R4;

int16_t *mixed_sample_buffer = NULL;
int MIXED_SAMPLE_BUFFER_LENGTH;
unsigned int mixed_sample_buffer_adr, mixed_sample_buffer_adr_aligned_64k, mixed_sample_buffer_align_offset;

int bit_crusher_reverb;

uint16_t t_TIMING_BY_SAMPLE_EVERY_250_MS; //optimization of timing counters
uint16_t t_TIMING_BY_SAMPLE_EVERY_125_MS; //optimization of timing counters

extern int wind_voices;

spi_flash_mmap_handle_t mmap_handle_samples;

void channel_init(int bg_sample, int song_id, int melody_id, int filters_type, float resonance, int use_mclk, int set_wind_voices, int set_active_filter_pairs)
{
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

    /*
	codec_init(); //i2c init
	*/

    //song_of_wind_and_ice();

	//channel = DIRECT_PROGRAM_START;

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

	//printf("channel_init(): [1] total_chords = %d\n", fil->chord->total_chords);

	//----------------------------------------------------------------

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

		//----------------------------------------------------------------

		mixed_sample_buffer_ptr_L = 0;
		mixed_sample_buffer_ptr_R = MIXED_SAMPLE_BUFFER_LENGTH / 2; //set second channel to mid of the buffer for better stereo effect
		//mixed_sample_buffer_ptr_L2 = MIXED_SAMPLE_BUFFER_LENGTH / 4;
		//mixed_sample_buffer_ptr_R2 = MIXED_SAMPLE_BUFFER_LENGTH * 3 / 4;

		//mixed_sample_buffer_ptr_L3 = MIXED_SAMPLE_BUFFER_LENGTH / 8;
		//mixed_sample_buffer_ptr_R3 = MIXED_SAMPLE_BUFFER_LENGTH * 3 / 8;
		//mixed_sample_buffer_ptr_L4 = MIXED_SAMPLE_BUFFER_LENGTH * 5 / 8;
		//mixed_sample_buffer_ptr_R4 = MIXED_SAMPLE_BUFFER_LENGTH * 7 / 8;

		//----------------------------------------------------------------

		//PROG_enable_rhythm = false;

		noise_volume_max = 0;//1.0f;
		noise_volume = 0;//1.0f;
		noise_boost_by_sensor = 0;
		//PROG_add_plain_noise = false;

		//OpAmp_ADC12_signal_conversion_factor = OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT;
	}
	else
	{
		//PROG_enable_rhythm = false;//true;

		printf("[FILTERS_TYPE_LOW_PASS+FILTERS_ORDER_4]\n");
		//printf("[FILTERS_TYPE_LOW_PASS+FILTERS_ORDER_8]\n");
		//printf("[FILTERS_TYPE_LOW_PASS+FILTERS_ORDER_2]\n");

		noise_volume_max = 1.0f;
		noise_volume = 1.0f;
		//noise_boost_by_sensor = 1;
		//PROG_add_plain_noise = true;

		//OpAmp_ADC12_signal_conversion_factor = OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT * 2;
	}
	//----------------------------------------------------------------

	//set echo to 3/2
	echo_dynamic_loop_current_step = ECHO_DYNAMIC_LOOP_LENGTH_DEFAULT_STEP;
	//echo_dynamic_loop_length = echo_dynamic_loop_steps[echo_dynamic_loop_current_step];

	//echo_dynamic_loop_length = 69677;//69997; //prime numbers close to 72k (1.5x48k)

	echo_dynamic_loop_length = ECHO_BUFFER_LENGTH_DEFAULT; //I2S_AUDIOFREQ * 3 / 2;
	//echo_dynamic_loop_length = I2S_AUDIOFREQ;
	//echo_dynamic_loop_length = I2S_AUDIOFREQ / 2;

    //if(PROG_add_echo)
    //{
    	//printf("clear echo buffer (size=%d)...",sizeof(echo_buffer));
    	//memset(echo_buffer,0,sizeof(echo_buffer));
    	printf("clear echo buffer (size=%d)...",ECHO_BUFFER_LENGTH*sizeof(int16_t));
    	memset(echo_buffer,0,ECHO_BUFFER_LENGTH*sizeof(int16_t));
    	echo_buffer_ptr0 = 0;
    	printf("done!\n");
    //}

    //----------------------------------------------------------------

	bit_crusher_reverb = BIT_CRUSHER_REVERB_DEFAULT;

    REVERB_MIXING_GAIN_MUL = 9; //amount of signal to feed back to reverb loop, expressed as a fragment
    REVERB_MIXING_GAIN_DIV = 10; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

	//printf("channel_init(): [2] total_chords = %d\n", fil->chord->total_chords);

	reverb_dynamic_loop_length = /* I2S_AUDIOFREQ / */ BIT_CRUSHER_REVERB_DEFAULT; //set default value (can be changed dynamically)
	//reverb_buffer = (int16_t*)malloc(reverb_dynamic_loop_length*sizeof(int16_t)); //allocate memory
    reverb_buffer = (int16_t*)malloc(REVERB_BUFFER_LENGTH*sizeof(int16_t));

    //printf("channel_init(): [3] total_chords = %d\n", fil->chord->total_chords);

    memset(reverb_buffer,0,REVERB_BUFFER_LENGTH*sizeof(int16_t)); //clear memory
	reverb_buffer_ptr0 = 0; //reset pointer

	//printf("channel_init(): [4] total_chords = %d\n", fil->chord->total_chords);

	//----------------------------------------------------------------

	fixed_arp_level = 0;

	SAMPLE_VOLUME = SAMPLE_VOLUME_DEFAULT;
	limiter_coeff = DYNAMIC_LIMITER_COEFF_DEFAULT;

	//----------------------------------------------------------------

	#ifdef BOARD_WHALE
	play_button_cnt = 0; //reset to default value
	#endif

    ms10_counter = 0;

    channel_running = 1;
    volume_ramp = 1;

    ui_button3_enabled = 1;
    ui_button4_enabled = 1;

    //mics_off = 0; //let this persist across channels, but not stored in flash settings

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
		xTaskCreatePinnedToCore((TaskFunction_t)&binaural_program, "process_binaural_program", 2048, NULL, 12, NULL, 1);
	}
    //this is done in volume ramp
    //codec_set_mute(0); //un-mute the codec

	/*
	//init MIDI if needed
	#ifdef SEND_MIDI_NOTES
	if(use_midi)
	{
		gecho_init_MIDI(MIDI_UART);
	}
	#endif
	*/

	if(use_acc_or_ir_sensors==PARAMETER_CONTROL_SENSORS_IRS)
	{
		sensors_active = 1;
	}
	if(use_acc_or_ir_sensors==PARAMETER_CONTROL_SENSORS_ACCELEROMETER)
	{
		accelerometer_active = 1;
	}

	//#ifdef ENABLE_SD_RECORDING
	//xTaskCreatePinnedToCore((TaskFunction_t)&sd_recording, "sd_recording_task", 2048, NULL, 12, NULL, 1);
	//sd_write_buf = (uint32_t*)malloc(SD_WRITE_BUFFER);
	//#endif

	//----------------------------------------------------------------

	LEDs_all_OFF();
}

/*
	if (prog==CHANNEL_21_INTERACTIVE_NOISE_EFFECT)
		base_freq = 400; //base frequency
		param_f = 0.0f; //no base resonance
	else if (prog==CHANNEL_22_INTERACTIVE_NOISE_EFFECT)
		base_freq = 150; //base frequency
		param_f = 0.9f; //some base resonance
	else if (prog==CHANNEL_23_INTERACTIVE_NOISE_EFFECT)
		base_freq = 0; //base frequency
		param_f = 0.995f; //maximum base resonance
	else if (prog==CHANNEL_31_THEREMIN_BY_MAGNETIC_RING || prog == CHANNEL_32_THEREMIN_BY_IR_SENSORS || prog==1211)
		base_freq = 0; //base frequency
		param_f = 0.998f; //maximum base resonance
		mixing_volumes_default = 1.0f;
*/

void channel_deinit()
{
	//printf("heap_trace_dump(void)\n");
	//heap_trace_dump();

	//printf("codec_reset()\n");
	//codec_reset();

    /*
	if(volume_ramp)
    {
    	codec_digital_volume = codec_volume_user;
    	codec_set_digital_volume();

    	for(int i=0;i<SAMPLE_RATE;i++)
    	{
    		//if(sampleCounter & 0x10)
    		if(sampleCounter & (1<<add_beep))
    		{
    			sample32 = (100 + (100<<16));
    		}
    		i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);

    		sampleCounter++;
    	}
    }
    */

	//printf("[codec_reset]Free heap: %u\n", xPortGetFreeHeapSize());
	//if(!heap_caps_check_integrity_all(true)) { printf("---> HEAP CORRUPT!\n"); }

	channel_running = 0;
	sd_playback_channel = 0;
    volume_ramp = 0;
	select_channel_RDY_idle_blink = 1;
	sensors_active = 0;
	accelerometer_active = 0;
	ui_button3_enabled = 1;
	ui_button4_enabled = 1;

    if(channel_uses_codec)
    {
    	codec_set_mute(1); //mute the codec
    	if(persistent_settings.SAMPLING_RATE != SAMPLE_RATE_DEFAULT)
    	{
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

    /*
    if(volume_ramp)
    {
    	codec_digital_volume = codec_volume_user;
    	codec_set_digital_volume();
    }
    else
    {
    */
    	//codec_analog_volume = CODEC_ANALOG_VOLUME_MUTE;
    	//codec_set_analog_volume();
    	//codec_digital_volume = CODEC_DIGITAL_VOLUME_MUTE;
    	//codec_set_digital_volume();
    //}

	//printf("skipping channel\n");

	//printf("delete(fil) ... if(fil != NULL)\n");

	//release some allocated memory
	if(fil != NULL)
	{
	    //chord is allocated first
		//printf("DELETING! delete(fil->chord); ... \n");
		//delete(fil->chord);
	    //printf("Deleted!\n");

	    //for some reason this causes reboot
		/*
		//iir2 is allocated second
	    printf("DELETING! delete(fil->iir2); ... \n");
	    delete(fil->iir2);
	    printf("Deleted!\n");
	    */

	    printf("DELETING! delete(fil) ... \n");
		delete(fil);
	    printf("Deleted!\n");

	    printf("setting fil = NULL; ... \n");
		fil = NULL;
	    printf("done!\n");
	}

	if(mixed_sample_buffer!=NULL)
	{
		printf("spi_flash_munmap(mmap_handle_samples) ... ");
		spi_flash_munmap(mmap_handle_samples);
		printf("unmapped!\n");

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

    printf("Channel finished.\n");
    //Delay(500);

    /*
    printf("codec_reset()...\n");
    //Delay(500);

    codec_reset();
    */

	#ifdef SWITCH_I2C_SPEED_MODES
    printf("I2C back to normal mode...\n");
    i2c_master_deinit();
    i2c_master_init(0); //normal mode
	#endif

    //printf("done\n");
    //Delay(500);

    /*
    printf("i2c_master_deinit()...");
    //Delay(500);
    i2c_master_deinit();
    printf("done\n");
    //Delay(500);
    */

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

    /*
    printf("while(BUTTON_U1_ON); //wait till PWR button released... ");
    while(BUTTON_U1_ON) //wait till PWR button released
    {
		play_button_cnt++;

		if(play_button_cnt == HIDDEN_MENU_TIMEOUT)
    	{
    		hidden_menu = 1;
    		break;
    	}

    	Delay(10);
    }
    */

	#ifdef BOARD_WHALE
    play_button_cnt = 0; //reset to default value
	#endif

    indicate_binaural(NULL); //turn off LED

	/*
    //deinit MIDI if needed
	#ifdef SEND_MIDI_NOTES
	if(use_midi)
	{
		gecho_deinit_MIDI(MIDI_UART);
	}
	#endif
	*/

	LEDs_all_OFF();

	printf("done\n");
    //Delay(500);
}
/*
void voice_settings_menu()
{
	printf("voice_settings_menu(): channel_init(10,...)\n");

	//channel_init(10, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0); //map the voice menu sample from Flash to RAM
	channel_init(5, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0);

	#if SAMPLE_RATE_VOICE_MENU != SAMPLE_RATE_DEFAULT
	set_sampling_rate(SAMPLE_RATE_VOICE_MENU);
	#endif
    volume_ramp = 0; //to not interfere with codec_set_mute() in voice_menu_say()

	settings_menu_active = 1;

	voice_menu_t voice_menu_items[VOICE_MENU_ITEMS_MAX];
	int items_found = get_voice_menu_items(voice_menu_items);

	printf("get_voice_menu_items(): %d items found:\n", items_found);
	for(int i=0;i<items_found;i++)
	{
		printf("%s -> %f / %d - %d\n", voice_menu_items[i].name, voice_menu_items[i].position_f, voice_menu_items[i].position_s, voice_menu_items[i].length_s);
	}

	//voice_menu_say("settings_menu", voice_menu_items, items_found);
	//voice_menu_say("this_is_glo", voice_menu_items, items_found);

	while(settings_menu_active)
	{
		//

		if(long_press_volume_plus)
		{
			printf("voice_settings_menu(): save settings\n");
			settings_menu_active = 0;
			//voice_menu_say("settings_saved", voice_menu_items, items_found);
			//voice_menu_say("440hz", voice_menu_items, items_found);
			voice_menu_say("upgrade_your_perception", voice_menu_items, items_found);
		}
		if(long_press_volume_minus)
		{
			printf("voice_settings_menu(): cancel saving settings\n");
			settings_menu_active = 0;
			//voice_menu_say("settings_not_saved", voice_menu_items, items_found);
			//voice_menu_say("zero", voice_menu_items, items_found);
			voice_menu_say("pitch_and_roll", voice_menu_items, items_found);
		}
	}

	volume_ramp = 1; //to not overwrite stored user volume setting
	printf("voice_settings_menu(): channel_deinit()\n");
	channel_deinit();
	#if SAMPLE_RATE_VOICE_MENU != SAMPLE_RATE_DEFAULT
	set_sampling_rate(SAMPLE_RATE_DEFAULT);
	#endif

	//release memory
	for(int i=0;i<items_found;i++)
	{
		free(voice_menu_items[i].name);
	}
}
*/

