/*
 * Dekrispator.cpp
 *
 *  Created on: 29 Jan 2019
 *      Author: mario
 *
 *  Port & integration of Dekrispator FM Synth by Xavier Halgand (https://github.com/MrBlueXav/Dekrispator)
 *  Please find the source code and license information within the files located in ../dkr/ subdirectory
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

#include <Dekrispator.h>
#include <Interface.h>
//#include <hw/codec.h>
#include <hw/signals.h>
//#include <hw/init.h>
//#include <string.h>
#include <InitChannels.h>
#include <string.h>

dekrispator_patch_t *dkr_patch;

void channel_dekrispator()
{
	#ifdef USE_LOW_SAMPLE_RATE
	set_sampling_rate(SAMPLE_RATE_LOW);
	#endif

	#ifdef USE_LARGER_BUFFERS
	i2s_driver_uninstall(I2S_NUM);
	init_i2s_and_gpio(CODEC_BUF_COUNT_DKR, CODEC_BUF_LEN_DKR);
	#endif

	//-------------------------- DEKRISPATOR TEST -------------------------------------------------
	//dekrispator_main();
	dekrispator_synth_init();
	//int bufs = 0;

	//xTaskCreatePinnedToCore((TaskFunction_t)&process_dekrispator_render, "process_dekrispator_render_task", 2048, NULL, 3, NULL, 1);
	//xTaskCreatePinnedToCore((TaskFunction_t)&process_dekrispator_play, "process_dekrispator_play_task", 2048, NULL, 5, NULL, 0);

	#ifdef DEKRISPATOR_ECHO
	//initialize echo without using init_echo_buffer() which allocates too much memory
	echo_dynamic_loop_length = 13000; //ECHO_BUFFER_LENGTH_DEFAULT; //set default value (can be changed dynamically)
	echo_dynamic_loop_current_step = 13;
	//memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	memset(echo_buffer,0,ECHO_BUFFER_LENGTH*sizeof(int16_t));
	echo_buffer_ptr0 = 0; //reset pointer
	#endif

	program_settings_reset();

	channel_running = 1;
    volume_ramp = 1;

	#ifdef BOARD_WHALE
    BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_SHORT;
	#endif

	/*
    #ifdef SEND_MIDI_NOTES
	gecho_init_MIDI(MIDI_UART);
	#endif
	*/

	dkr_patch = new(dekrispator_patch_t);
	for(int i=0;i<MP_PARAMS;i++) { dkr_patch->patch[i]=0; }
	for(int i=0;i<MF_PARAMS;i++) { dkr_patch->fx[i]=0; }

	//while(skip_channel_cnt)//bufs<1000)
	//{
		//printf("make_sound(%x, %d) buf=%d\n", (unsigned int)audiobuff, BUFF_LEN_DIV2, bufs++);
		//make_sound(audiobuff, BUFF_LEN_DIV2);
		dekrispator_make_sound();//BUFF_LEN);
	//}
	dekrispator_synth_deinit();

	delete(dkr_patch);

	//deinit MIDI if needed
	#ifdef SEND_MIDI_NOTES
	gecho_deinit_MIDI(MIDI_UART);
	#endif

	//-------------------------- DEKRISPATOR TEST -------------------------------------------------
	#ifdef USE_LOW_SAMPLE_RATE
	set_sampling_rate(SAMPLE_RATE_DEFAULT);
	#endif

	#ifdef USE_LARGER_BUFFERS
	i2s_driver_uninstall(I2S_NUM);
	init_i2s_and_gpio(CODEC_BUF_COUNT_DEFAULT, CODEC_BUF_LEN_DEFAULT);
	#endif

}

//void process_dekrispator_render(void *pvParameters);
//void process_dekrispator_play(void *pvParameters);

/*
int dekrispator_render_half = 0;
int dekrispator_data_ready = 0;

void process_dekrispator_render(void *pvParameters)
{
	printf("process_dekrispator_render(): started on core ID=%d\n", xPortGetCoreID());

	while(1)
	{
		while(dekrispator_data_ready) { vTaskDelay(500); } //wait till the other process resets this flag

		printf("make_sound(%x, %d): buffer=%d half=%d\n", (unsigned int)(audiobuff+dekrispator_render_half*BUFF_LEN_DIV2), BUFF_LEN_DIV2, bufs++, dekrispator_render_half);
		make_sound(audiobuff+dekrispator_render_half*BUFF_LEN_DIV2, BUFF_LEN_DIV2);
		dekrispator_data_ready = 1;
	}
}

void process_dekrispator_play(void *pvParameters)
{
	printf("process_dekrispator_play(): started on core ID=%d\n", xPortGetCoreID());

	int play_from;

	while(1)
	{
		while(!dekrispator_data_ready) { vTaskDelay(500); } //wait till the other process sets this flag

		if(dekrispator_render_half)
		{
			play_from = BUFF_LEN_DIV2;
		}
		else
		{
			play_from = 0;
		}

		//move on the 2nd half of the buffer
		dekrispator_render_half++;
		dekrispator_render_half%=2;

		//let the other process continues rendering
		dekrispator_data_ready = 0;

		printf("playing from sample %d\n", play_from);

		for(int i=play_from;i<play_from+BUFF_LEN_DIV2;i+=2)
		{
			//sample32 = (int32_t)(audiobuff[i]/4) + (int32_t)(((audiobuff[i+1]/4)<<16)&0xffff0000);
			sample32 = (((int32_t)(audiobuff[i]) + (int32_t)(((audiobuff[i+1])<<16)&0xffff0000)) >> 8) & 0x00ff00ff;
			i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
		}
	}
}
*/
