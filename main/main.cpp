/*
 * main.cpp
 *
 *  Created on: Jan 23, 2018
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

//#define RGB_AND_BUTTONS_TEST
//#define I2C_SCANNER
//#define ROTARY_ENCODER_TEST
//#define SD_CARD_ENABLED
//#define SD_CARD_SPEED_TEST

//#define ENABLE_MCLK				//disable when debugging
#ifdef ENABLE_MCLK
#define ENABLE_MCLK_ON_START
#define ENABLE_MCLK_ON_CHANNEL_LOOP
#define DISABLE_MCLK_1_SEC
#endif

#include "board.h"
#include "main.h"

extern "C" void app_main(void)
{
	printf("The mighty Glo firmware starting...\n");
	printf("Running on core ID=%d\n", xPortGetCoreID());

	codec_reset();

    printf("spi_flash_cache_enabled() returns %u\n", spi_flash_cache_enabled());

#ifdef BOARD_WHALE
    //esp_brownout_init();

    //hold the power on
	whale_init_power();
#endif

    i2c_master_init(0);

#ifdef I2C_SCANNER
    //look for i2c devices
	printf("looking for i2c devices...\n");
	//i2c_init(26, 27); //pin numbers: SDA,SCL
	i2c_scan_for_devices();
	printf("finished...\n");
#endif

	init_deinit_TWDT();

	generate_random_seed();

	int res = nvs_flash_init();
	if(res!=ESP_OK)
	{
		printf("nvs_flash_init(); returned %d\n",res);
	}

#ifdef ROTARY_ENCODER_TEST
	rotary_encoder_init();
	rotary_encoder_test();
#endif

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
    sd_card_init();
	#ifdef SD_CARD_SPEED_TEST
		sd_card_speed_test();
	#endif
#endif

    init_echo_buffer();

#ifdef BOARD_GECHO
    gecho_init_buttons_GPIO();
	//gecho_test_LEDs();
	//gecho_test_buttons();
	//gecho_test_MIDI_input();

	LED_SIG_ON;
	Delay(200);
	LED_SIG_OFF;

	gecho_sensors_init();
	//gecho_sensors_test();

	gecho_LED_expander_init();
	//gecho_LED_expander_test();
#endif

	#ifdef USE_ACCELEROMETER
	init_accelerometer();
	#endif

	#ifdef BOARD_WHALE
	xTaskCreatePinnedToCore((TaskFunction_t)&process_buttons_controls_whale, "process_buttons_controls_task", 4096, NULL, 10, NULL, 1);
	#endif

	#ifdef BOARD_GECHO
	xTaskCreatePinnedToCore((TaskFunction_t)&process_buttons_controls_gecho, "process_buttons_controls_task", 2048, NULL, 10, NULL, 1);
	#endif

    init_i2s_and_gpio(CODEC_BUF_COUNT_DEFAULT, CODEC_BUF_LEN_DEFAULT);

	//heap_trace_start();
    //printf("[select channel loop] Free heap: %u\n", xPortGetFreeHeapSize());
    //if(!heap_caps_check_integrity_all(true)) { printf("---> HEAP CORRUPT!\n"); }

    #ifdef BOARD_WHALE

	#ifdef ENABLE_MCLK_ON_START
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

    #else
    channels_map_t channels_map[1];
    #endif

    int memory_loss;

	while(glo_run) //select channel loop
	{
	    printf("----START-----[select channel loop] Free heap: %u\n", memory_loss = xPortGetFreeHeapSize());

		#ifdef BOARD_GECHO

	    LED_O4_all_OFF();

	    uint64_t channel = select_channel();
    	printf("selected channel = %llu\n", channel);
    	channel_loop_remapped = 0;

    	int channel_found = get_channels_map(channels_map, channel);
    	if(!channel_found)
    	{
    		printf("get_channels_map(%llu): Channel not found\n", channel);
    		continue;
    	}
    	else
    	{
    		start_MCLK();
    		codec_init();
    		//codec_set_mute(0); //unmute the codec
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
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"reverb")) { channel_reverb(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"antarctica")) { channel_antarctica(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"bytebeat")) { channel_bytebeat(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"dco")) { channel_dco(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"chopper")) { channel_chopper(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"dekrispator")) { channel_dekrispator(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"drum_kit")) { channel_drum_kit(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"clouds")) { channel_mi_clouds(); }

		else if(0==strcmp(channels_map[channel_loop_remapped].name,"pass_through_mics")) { codec_select_input(ADC_INPUT_MIC); pass_through(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"pass_through_linein")) { codec_select_input(ADC_INPUT_LINE_IN); pass_through(); }

		#ifdef BOARD_GECHO
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sync_out_test")) { sync_out_init(); sync_out_test(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sd_card_info")) { sd_card_info(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sd_card_speed_test")) { sd_card_speed_test(); }
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"sd_card_file_list")) { sd_card_file_list(); }
		#endif
		else if(0==strcmp(channels_map[channel_loop_remapped].name,"midi_in_test")) { gecho_test_MIDI_input(); }

		channel_deinit();

	    printf("----END-------[select channel loop] Free heap after deinit: %u\n", xPortGetFreeHeapSize());
	    memory_loss -= xPortGetFreeHeapSize();
	    printf("----END-------[select channel loop] Memory loss = %d\n", memory_loss);

		#ifdef BOARD_WHALE
	    channel_loop++;
		#endif

	} //select program loop

	while(1)
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
		channel_reverb();
		channel_deinit();
		//pass_through(); //audio in directly to out with no effects
		//channel_deinit();
	}
}


