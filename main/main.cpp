/*
 * main.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Jan 23, 2018
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

//#define RGB_AND_BUTTONS_TEST
//#define I2C_SCANNER
#define SD_CARD_ENABLED
//#define SD_CARD_SPEED_TEST

//#define ENABLE_MCLK				//disable when debugging
#ifdef ENABLE_MCLK
#define ENABLE_MCLK_ON_START
#define ENABLE_MCLK_ON_CHANNEL_LOOP
#define DISABLE_MCLK_1_SEC
#endif

#define LISTEN_FOR_TEMPO_AT_IDLE

#include "board.h"
#include "main.h"

extern void legacy_main();

extern "C" void app_main(void)
{
	//printf("Running on core ID=%d\n", xPortGetCoreID());

	//uint8_t factory_MAC[8];
	//esp_efuse_mac_get_default(factory_MAC);
	//printf("Factory MAC = %02x:%02x:%02x:%02x:%02x:%02x\n", factory_MAC[0],factory_MAC[1],factory_MAC[2],factory_MAC[3],factory_MAC[4],factory_MAC[5]);
	//Delay(200);

    //printf("spi_flash_cache_enabled() returns %u\n", spi_flash_cache_enabled());

	init_deinit_TWDT();

	/*
	//random generator functions test
	for(int i=0;i<10000;i++)
	{
		new_random_value();
		printf("%d	%f	%f\n", random_value, PseudoRNG1a_next_float_new(), PseudoRNG1a_next_float_new01());
	}
	*/

	//float_speed_test();
	//double_speed_test();
	//esp_restart();

#ifdef BOARD_WHALE
    //hold the power on
	whale_init_power();
#endif

	codec_reset();

    i2c_master_init(0);

	Delay(5);
	gpio_set_level(CODEC_RST_PIN, 1);  //release the codec reset
	uint16_t hw_check_result = hardware_check();
	codec_reset();

#ifdef I2C_SCANNER
    while(1)
    {
    	codec_reset();
    	Delay(100);
    	gpio_set_level(CODEC_RST_PIN, 1);  //release the codec reset

    	//look for i2c devices
    	printf("looking for i2c devices...\n");
    	//i2c_init(26, 27); //pin numbers: SDA,SCL
    	i2c_scan_for_devices(1,NULL,NULL);
    	printf("finished...\n");

    	codec_reset();

    	Delay(1000);
    }
#endif

glo_run = 1;

int res = nvs_flash_init();
if(res!=ESP_OK)
{
	printf("nvs_flash_init(); returned %d\n",res);
}

#ifdef BOARD_WHALE
	printf("whale_init_RGB_LED();\n");
	whale_init_RGB_LED();

	printf("whale_init_buttons_GPIO();\n");
	whale_init_buttons_GPIO();

#ifdef RGB_AND_BUTTONS_TEST
	//RGB LED test on Whale board
	printf("whale_test_RGB();\n");
	whale_test_RGB();
	printf("[end]\n");
	while(1);
#endif

	RGB_LEDs_blink(3,20);
#endif

#ifdef SD_CARD_ENABLED

	#ifdef SD_CARD_SPEED_TEST
		sd_card_init();
		sd_card_speed_test();
	#endif

	sd_card_check(FW_UPDATE_LINK_FILE, 0);
	printf("sd_card_check(): result = %d\n", sd_card_present);

	verify_sd_counter_overlap(); //to not overwrite existing wav files if counter set too low (i.e. card swapped from another unit)

#endif

	init_echo_buffer();

#ifdef BOARD_GECHO
    gecho_init_buttons_GPIO();
	//gecho_test_LEDs();
	//gecho_test_buttons();
    //gecho_sensors_test();
	gecho_sensors_init();
	gecho_LED_expander_init();
	//gecho_LED_expander_test();

	//leave time for expanders to activate
	LED_SIG_ON;
	Delay(200);
	LED_SIG_OFF;

	if(hw_check_result) //i2c_errors + found << 8;
	{
		printf("I2C errors found: %d\n", hw_check_result);
		if(hw_check_result>>8)
		{
			LED_O4_set_byte(0x0f>>(hw_check_result>>8));
			indicate_error(0x33cc, 10, 80);
		}
		else if(hw_check_result&0xff)
		{
			LED_B5_set_byte(~(hw_check_result&0xff));
			indicate_error(0xcc33, 10, 80);
		}
		Delay(200);
	}

	int reason = rtc_get_reset_reason(0);
	printf("esp_reset_reason() returned %d\n", reason);

	if(reason==POWERON_RESET)
	{
		service_menu();
	}
#endif

	//while(BUTTON_U1_ON); //wait till PWR button released
	//spdif_transceiver_init();
	//i2c_master_deinit();

	//codec_init(); //audio codec - init but keep off (RESET -> LOW)
	//codec_comm_blink_test(1);
	//codec_ctrl_init(); //audio codec - release reset, init controller
	//codec_comm_blink_test(2);

	init_accelerometer();

	#ifdef BOARD_WHALE
	xTaskCreatePinnedToCore((TaskFunction_t)&process_buttons_controls_whale, "process_buttons_controls_task", 4096, NULL, 10, NULL, 1);
	#endif

	#ifdef BOARD_GECHO
	xTaskCreatePinnedToCore((TaskFunction_t)&process_buttons_controls_gecho, "process_buttons_controls_task", 2048, NULL, PRIORITY_BUTTONS_CONTROLS_TASK, NULL, CPU_CORE_BUTTONS_CONTROLS_TASK);
	#endif

	//heap_trace_start();
    //printf("[select channel loop] Free heap: %u\n", xPortGetFreeHeapSize());
    //if(!heap_caps_check_integrity_all(true)) { printf("---> HEAP CORRUPT!\n"); }

    #ifdef BOARD_WHALE

	#ifdef ENABLE_MCLK_ON_START
    init_i2s_and_gpio(CODEC_BUF_COUNT_DEFAULT, CODEC_BUF_LEN_DEFAULT, SAMPLE_RATE_DEFAULT);
	start_MCLK();
	#endif

	channels_map_t channels_map[MAIN_CHANNELS+1];
    channels_found = get_channels_map(channels_map, 0); //all channels, no filtering
    printf("get_channels_map(): found %d channels\n", channels_found);
    //for(int i=0;i<channels_found;i++)
    //{
	//    printf("[%s] with %d parameters: %d %d %d %d\n", channels_map[i].name, channels_map[i].p_count, channels_map[i].p1, channels_map[i].p2, channels_map[i].p3, channels_map[i].p4);
    //}

    load_all_settings(); //this can modify # of channels (channels_found)
    codec_init();

    unlocked_channels_found = channels_found;

    remapped_channels_found = get_remapped_channels(remapped_channels, channels_found);
    printf("get_remapped_channels(): found %d channels\n", remapped_channels_found);
    if(remapped_channels_found!=channels_found)
    {
    	//number of channels changed, need to reset
        printf("get_remapped_channels(): number of channels has changed, resetting to default order\n");
        remapped_channels_found = channels_found;
        for(int i=0;i<remapped_channels_found;i++)
    	{
    		remapped_channels[i] = i;
    	}
    }
    printf("get_remapped_channels(): new order = ");
    for(int i=0;i<remapped_channels_found;i++)
    {
    	printf("%d ", remapped_channels[i]);
    }
    printf("\n");

    while(BUTTON_U1_ON); //wait till PWR button released

    #else //!BOARD_WHALE

    channels_map_t channels_map[1];
    int set = load_all_settings();
    printf("load_all_settings(): block found = %d\n", set);

    if(set==-1)
    {
        printf("looks like a new chip, trying to load factory data from SD card\n");
        factory_data_load_SD();
    	printf("factory_data_load_SD() done, restarting\n");
        esp_restart();
    }

    if(!flashed_config_new_enough())
    {
		printf("looks like the config is too old for this firmware, trying to reload from SD card\n");
		int reloaded = reload_config();
		if(reloaded)
		{
			printf("reload_config() done, restarting\n");
			esp_restart();
		}
		else
		{
			printf("reload_config() failed!\n");
		}
    }

	start_MCLK();
	init_i2s_and_gpio(CODEC_BUF_COUNT_DEFAULT, CODEC_BUF_LEN_DEFAULT, persistent_settings.SAMPLING_RATE);
	codec_init();
	if(persistent_settings.AGC_ENABLED_OR_PGA==0)
	{
		//disable AGC
		codec_configure_AGC(0, global_settings.AGC_MAX_GAIN, global_settings.AGC_TARGET_LEVEL);
	}
    if(ADC_input_select != ADC_INPUT_DEFAULT)
	{
    	codec_select_input(ADC_input_select);
	}
    #endif

	//MIDI_SYNC_MODE_MIDI_IN_OUT	//clock derived and chords captured from MIDI if signal present, chords transmitted while playing
	//MIDI_SYNC_MODE_MIDI_IN		//clock derived and chords captured from MIDI if signal present, nothing transmitted
	//MIDI_SYNC_MODE_MIDI_OUT		//chords transmitted via MIDI while playing, no clock sent, incoming signal ignored
	//MIDI_SYNC_MODE_MIDI_CLOCK_OUT	//chords and clock transmitted via MIDI while playing

	//sync and MIDI clock capture at idle, need to init here

	printf("midi_sync_mode = %d\n", midi_sync_mode);

    if(midi_sync_mode==MIDI_SYNC_MODE_MIDI_IN_OUT
    || midi_sync_mode==MIDI_SYNC_MODE_MIDI_IN
	|| midi_sync_mode==MIDI_SYNC_MODE_MIDI_OUT
	|| midi_sync_mode==MIDI_SYNC_MODE_MIDI_CLOCK_OUT)
    {
    	printf("midi_sync_mode: enabling MIDI, gecho_init_MIDI(%d,%d)\n", MIDI_UART, midi_sync_mode==MIDI_SYNC_MODE_MIDI_IN?0:1);
    	gecho_init_MIDI(MIDI_UART, midi_sync_mode==MIDI_SYNC_MODE_MIDI_IN?0:1);
        if(midi_sync_mode==MIDI_SYNC_MODE_MIDI_IN || midi_sync_mode==MIDI_SYNC_MODE_MIDI_IN_OUT)
        {
        	printf("midi_sync_mode: enabling MIDI In: gecho_start_receive_MIDI()\n");
        	gecho_start_receive_MIDI();
        }
    }
    else if(midi_sync_mode==MIDI_SYNC_MODE_SYNC_IN || midi_sync_mode==MIDI_SYNC_MODE_SYNC_CV_IN)
    {
    	printf("midi_sync_mode: enabling Sync In: gecho_start_receive_sync()\n");
    	gecho_start_receive_sync();
    }

	printf("load_midi_controller_configuration() returned 0x%04x\n", load_midi_controller_configuration());
	printf("midi_ctrl_keys = %d, midi_ctrl_cc = %d\n", midi_ctrl_keys, midi_ctrl_cc);
	printf("keys=(");
	for(int i=0;i<midi_ctrl_keys;i++)
	{
		printf("%02x ",midi_ctrl_keys_map[i]);
	}
	printf("), cc=(");

	for(int i=0;i<midi_ctrl_cc;i++)
	{
		printf("%02x ",midi_ctrl_cc_map[i]);
	}
	printf("), cc_size=(");

	for(int i=0;i<midi_ctrl_cc;i++)
	{
		printf("%02x ",midi_ctrl_cc_msg_size[i]);
	}
	printf(")\n");

    int memory_loss;

	#ifdef SD_WRITE_BUFFER_PERMANENT
    sd_write_buf = (uint32_t*)malloc(SD_WRITE_BUFFER);
	#endif

    init_free_mem = xPortGetFreeHeapSize();

    while(glo_run) //select channel loop
	{
	    printf("----START-----[select channel loop] Free heap: %u\n", memory_loss = xPortGetFreeHeapSize());

		#ifdef LISTEN_FOR_TEMPO_AT_IDLE
	    gecho_start_listen_for_tempo();
		#endif

		#ifdef BOARD_GECHO

	    LED_O4_all_OFF();

	    uint64_t channel = select_channel();
    	printf("selected channel = %llu\n", channel);
    	channel_loop_remapped = 0;

    	t_start_channel = micros();

		#ifdef LISTEN_FOR_TEMPO_AT_IDLE
    	//if(!start_channel_by_sync_pulse)
    	//{
    		gecho_stop_listen_for_tempo();
    	//}
		#endif

    	int channel_found = get_channels_map(channels_map, channel);
    	if(!channel_found)
    	{
    		printf("get_channels_map(%llu): Channel not found\n", channel);
    		continue;
    	}
    	else
    	{
    		//service channels do not need codec, those ending with:
    		if(channel_name_ends_with("_test", channels_map[channel_loop_remapped].name)
			|| channel_name_ends_with("_info", channels_map[channel_loop_remapped].name)
			|| channel_name_ends_with("_load", channels_map[channel_loop_remapped].name)
			|| channel_name_ends_with("_download", channels_map[channel_loop_remapped].name)
			|| channel_name_ends_with("_reset", channels_map[channel_loop_remapped].name)
			|| channel_name_ends_with("_update", channels_map[channel_loop_remapped].name)
			|| channel_name_ends_with("_set", channels_map[channel_loop_remapped].name)
        	|| channel_name_ends_with("_write", channels_map[channel_loop_remapped].name)
			|| channel_name_ends_with("_program", channels_map[channel_loop_remapped].name)
    		|| channel_name_ends_with("_sender", channels_map[channel_loop_remapped].name))
    		{
    			printf("This channel does not use codec\n");
    			channel_uses_codec = 0;
    		}
    		else
    		{
    			printf("This channel uses codec\n");
    			channel_uses_codec = 1;
    			codec_silence(200);
    		}
    	}
		#endif

		#ifdef BOARD_WHALE
		if(channel_loop==channels_found)
		{
			channel_loop = 0;
		}

		channel_loop_remapped = remapped_channels[channel_loop];
		#endif

		#ifdef ENABLE_MCLK_ON_CHANNEL_LOOP
		start_MCLK();
		#endif

		printf("Running channel[%s] with %d parameters (%d,%d,%d,%f,settings=%d,binaural=%d)\n",
				channels_map[channel_loop_remapped].name,
				channels_map[channel_loop_remapped].p_count,
				channels_map[channel_loop_remapped].i1,
				channels_map[channel_loop_remapped].i2,
				channels_map[channel_loop_remapped].i3,
				channels_map[channel_loop_remapped].f4,
				channels_map[channel_loop_remapped].settings,
				channels_map[channel_loop_remapped].binaural);

		use_alt_settings = channels_map[channel_loop_remapped].settings - 1; //default settings are at zero
		use_binaural = channels_map[channel_loop_remapped].binaural;

		if(0==strcmp((const char*)channels_map[channel_loop_remapped].name,"high_pass"))
		{
			channel_high_pass(channels_map[channel_loop_remapped].i1,channels_map[channel_loop_remapped].i2,channels_map[channel_loop_remapped].i3,channels_map[channel_loop_remapped].f4);
		}
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"low_pass"))
		{
			channel_low_pass(channels_map[channel_loop_remapped].i1,channels_map[channel_loop_remapped].i2);
		}
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"low_pass_high_reso"))
		{
			channel_low_pass_high_reso(channels_map[channel_loop_remapped].i1,channels_map[channel_loop_remapped].i2,channels_map[channel_loop_remapped].i3);
		}
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"direct_waves"))
		{
			channel_direct_waves(channels_map[channel_loop_remapped].i1);
		}

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"granular")) { channel_granular(); }
		//else if(0==strcmp(channels_map[channel_loop_remapped].name,"sampler")) { channel_sampler(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"reverb")) { channel_reverb(use_alt_settings); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"antarctica")) { channel_antarctica(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"bytebeat")) { channel_bytebeat(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"dco")) { channel_dco(2); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"dco_legacy")) { channel_dco(1); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"chopper")) { channel_chopper(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"dekrispator")) { channel_dekrispator(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"drum_kit")) { channel_drum_kit(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"clouds")) { channel_mi_clouds(use_alt_settings); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sampler_looper")) { channel_sampler_looper(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sampled_drum_seq")) { channel_sampled_drum_sequencer(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"white_noise")) { channel_white_noise(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"original_channel_one")) { legacy_main(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"looper_shifter")) { channel_looper_shifter(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"infinite_looper")) { channel_infinite_looper(); }
		//else if(0==strcmp(channels_map[channel_loop_remapped].name,"all_the_sounds")) { all_the_sounds(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"filter_flanger")) { channel_filter_flanger(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"just_water")) { channel_just_water(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"pass_through_mics")) { codec_select_input(ADC_INPUT_MIC); pass_through(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"pass_through_linein")) { codec_select_input(ADC_INPUT_LINE_IN); pass_through(); }

		#ifdef BOARD_GECHO

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"karplus_strong")) { channel_karplus_strong(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sync_in_test")) { sync_in_test(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sync_out_test")) { sync_out_init(); sync_out_test(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"cv_in_test")) { cv_in_test(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"cv_out_test")) { cv_out_init(); cv_out_test(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sd_card_info")) { sd_card_info(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sd_card_speed_test")) { sd_card_speed_test(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sd_card_files_info")) { sd_card_file_list(NULL, 0, NULL, NULL, 0); printf("\nContent of /rec directory:\n"); sd_card_file_list("rec", 0, NULL, NULL, 0); /*sd_card_file_list(NULL, 1, NULL, NULL, 0);*/ }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sd_card_download")) { sd_card_download(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sd_card_play")) { sd_card_play(channels_map[channel_loop_remapped].str_param,channels_map[channel_loop_remapped].i1); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"all_leds_test")) { all_LEDs_test(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"accelerometer_test")) { accelerometer_test(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sensors_test")) { gecho_sensors_test(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"memory_info")) { free_memory_info(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"board_sn_info")) { show_board_serial_no(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"fw_version_info")) { show_fw_version(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"song_program")) { program_song(use_alt_settings+1); } //not using codec
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"program_song")) { program_song(use_alt_settings+1); } //using codec

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"config_write")) { write_config(); }
		//change sensors configuration and update NVS immediately
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"controls_set")) { set_controls(use_alt_settings+1, 1); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"mic_bias_set")) { set_mic_bias(use_alt_settings+1); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"midi_out_test")) { gecho_test_MIDI_output(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"midi_in_test")) { gecho_test_MIDI_input(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"midi_in_hw_test")) { gecho_test_MIDI_input_HW(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sysex_sender")) { MIDI_sysex_sender(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"settings_reset")) { settings_reset(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"play_last_recording")) { channel_SD_playback(SD_RECORDING_LAST, 0); } //no slicer

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"recorded_sample_slicer")) { channel_SD_playback(SD_RECORDING_LAST, 1); } //slicer on
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"mellotron")) { channel_SD_mellotron(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"factory_data_load")) { factory_data_load_SD(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sd_firmware_update")) { firmware_update_SD(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"factory_reset")) { factory_reset_firmware(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"running_partition_info")) { configured_and_running_partition_info(); }
		#endif

		channel_deinit();
		free(channels_map[channel_loop_remapped].name);
		if(channels_map[channel_loop_remapped].str_param)
		{
			free(channels_map[channel_loop_remapped].str_param);
			//channels_map[channel_loop_remapped].str_param = NULL;
		}

	    printf("----END-------[select channel loop] Free heap after deinit: %u\n", xPortGetFreeHeapSize());
	    memory_loss -= xPortGetFreeHeapSize();
	    printf("----END-------[select channel loop] Memory loss = %d\n", memory_loss);

		#ifdef BOARD_WHALE
	    //if(long_press_button_play_and_plus) //if channel ended due to selecting settings menu, do not skip to next channel
	    //{
	    //	voice_settings_menu();
	    //	long_press_button_play_and_plus = 0;
	    //}
	    //else //otherwise will move on to next channel
	    //{
	    	channel_loop++;
	    //}
		#endif

	} //select program loop

	while(1) //test loop
	{
		use_binaural = 1;
		channel_high_pass(31,0,5,1.0); //Notjustmoreidlechatter, voiceover
		channel_deinit();
		use_binaural = 0;
		//channel_mi_clouds();
		//channel_deinit();
		channel_antarctica(); //fallback for corrupt binary
		channel_deinit();
		//channel_dco();
		//channel_deinit();
		channel_reverb(0);
		channel_deinit();
		//pass_through(); //audio in directly to out with no effects
		//channel_deinit();
	}
}
