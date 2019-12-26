/*
 * sync.c
 *
 *  Created on: Jul 7, 2019
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

#include "sync.h"
#include "init.h"
#include "gpio.h"
#include "ui.h"
#include "signals.h"
#include "leds.h"

#define SYNC_DIVIDER	4

void process_receive_sync(void *pvParameters)
{
	printf("process_receive_sync(): task running on core ID=%d\n",xPortGetCoreID());

	int sync_signal;
	uint8_t sync_edge = 0, sync_counter = 0;
	//channel_running = 1;

	while(1)
	{
		adc2_get_raw(ADC2_CHANNEL_8, ADC_WIDTH_12Bit, &sync_signal);
		//printf("%d\n", sync_signal);

		if(sync_signal & 0xfff0) //detect threshold = 16
		{
			if(!sync_edge)	//trigger on raising edge
			{
				sync_edge = 1;
				sync_counter++;
				LED_SIG_ON;
			}
		}
		else if(sync_edge)
		{
			sync_edge = 0;
			LED_SIG_OFF;
		}

		//derive the clock from SYNC
		if(sync_counter%SYNC_DIVIDER==1)
		{
			select_channel_RDY_idle_blink = 0;
			LED_RDY_ON;
		}
		else if(sync_counter%SYNC_DIVIDER==1+SYNC_DIVIDER/2)
		{
			LED_RDY_OFF;
		}

		vTaskDelay(5);
	}
}

TaskHandle_t receive_sync_task = NULL;

void gecho_start_receive_sync()
{
	printf("gecho_start_receive_sync()\n");

	MIDI_SYNC_derived_tempo = 0;

	//the range is 5.8V over 4096 values
	adc2_config_channel_atten(ADC2_CHANNEL_8, ADC_ATTEN_DB_6); //GPIO25 is connected to ADC2 channel #8

	printf("gecho_start_receive_sync(): starting process_receive_sync task\n");
	xTaskCreatePinnedToCore((TaskFunction_t)&process_receive_sync, "receive_sync_task", 2048, NULL, 9, &receive_sync_task, 1);
}

void gecho_stop_receive_sync()
{
	MIDI_SYNC_derived_tempo = 0;

	if(receive_sync_task==NULL)
	{
		printf("gecho_stop_receive_sync(): task not running\n");
		return;
	}

	printf("gecho_stop_receive_sync(): deleting task receive_MIDI_task\n");
	vTaskDelete(receive_sync_task);
	receive_sync_task = NULL;
	printf("gecho_stop_receive_sync(): task deleted\n");
}

void process_listen_for_tempo(void *pvParameters)
{
	printf("process_listen_for_tempo(): task running on core ID=%d\n",xPortGetCoreID());

	sample32 = 0;
	sampleCounter = 0;

	//float avg1 = 0, avg2 = 0;

	int tempo_counter = 0;
	int edge_detect = 0;

	//#define TEMPO_DETECT_SLICE 				500
	//#define TEMPO_DETECT_THRESHOLD_HIGH		25.0f
	//#define TEMPO_DETECT_THRESHOLD_LOW		2.0f

	#define TEMPO_DETECT_THRESHOLD_HIGH		1.0f
	#define TEMPO_DETECT_THRESHOLD_LOW		0.1f

	//#define SIGNAL_DETECT_TIMING			100 //every 100 samples = cca 500Hz
	#define SIGNAL_DETECT_TIMING			50 //every 50 samples = cca 1000Hz
	#define SIGNAL_STABLE_COUNT				5

	int signal_history[2] = {0,0};

	#define MILLIS_HISTORY_LENGTH 8 //16
	uint32_t millis_history[MILLIS_HISTORY_LENGTH];

	while(1)
	{
		//i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
		i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);

		if(sampleCounter % SIGNAL_DETECT_TIMING == 0)
		{
			sample_f[0] = (float)((int16_t)(ADC_sample >> 16)) * OpAmp_ADC12_signal_conversion_factor;
			sample_f[1] = (float)((int16_t)ADC_sample) * OpAmp_ADC12_signal_conversion_factor;

			//printf("sample_f[0,1]=%f,%f\n",sample_f[0],sample_f[1]);

			//if(sample_f[0]>TEMPO_DETECT_THRESHOLD_HIGH)// || sample_f[1]>TEMPO_DETECT_THRESHOLD_HIGH)
			if(sample_f[1]>TEMPO_DETECT_THRESHOLD_HIGH)
			{
				signal_history[1]++;
				signal_history[0] = 0;
			}
			//else if(sample_f[0]<TEMPO_DETECT_THRESHOLD_LOW)// && sample_f[1]<TEMPO_DETECT_THRESHOLD_LOW)
			else if(sample_f[1]<TEMPO_DETECT_THRESHOLD_LOW)
			{
				signal_history[0]++;
				signal_history[1] = 0;
			}
			else
			{
				//vTaskDelay(1);
				continue;
			}

			if(!edge_detect && signal_history[1] == SIGNAL_STABLE_COUNT)
			{
				edge_detect = 1;
				//vTaskDelay(10);

				LED_R8_set_byte(0x01<<(tempo_counter%8));
				millis_history[tempo_counter] = millis();
				//millis_history[tempo_counter] = sampleCounter;

				tempo_counter++;

				if(tempo_counter==MILLIS_HISTORY_LENGTH)
				{
					//check if timing looks stable enough to derive tempo
					printf("timing history: ");
					int avg = 0;
					for(int i=1;i<MILLIS_HISTORY_LENGTH;i++)
					{
						millis_history[i-1] = millis_history[i] - millis_history[i-1];
						avg += millis_history[i-1];
						printf("%d,",millis_history[i-1]);
					}
					avg /= MILLIS_HISTORY_LENGTH-1;

					int valid = 1;
					for(int i=1;i<MILLIS_HISTORY_LENGTH;i++)
					{
						if(abs(avg - millis_history[i-1])>2)
						{
							valid = 0;
						}
					}

					//MIDI_SYNC_derived_tempo =

					//int BPM = (float)avg / (float)current_sampling_rate * 120.0 * 4.0f;
					//printf(", avg=%d, Fs=%d, ", avg, current_sampling_rate);
					//int BPM = (float)avg / 250.0f * 120.0 * 4.0f;

					int BPM = 250.0f / (float)avg * 120.0;
					printf(", avg=%d, ", avg);

					printf("BPM=%d, valid=%d\n", BPM, valid);

					if(valid)
					{
						if(BPM>=79&&BPM<=81)
						{
							BPM=80;
						}
						if(BPM>=99&&BPM<=101)
						{
							BPM=100;
						}
						if(BPM>=119&&BPM<=121)
						{
							BPM=120;
						}
						if(BPM>=138&&BPM<=142)
						{
							BPM=140;
						}
						if(BPM>=148&&BPM<=152)
						{
							BPM=150;
						}
						if(BPM>=158&&BPM<=162)
						{
							BPM=160;
						}
						MIDI_SYNC_derived_tempo = BPM;
						LED_R8_all_OFF();
						LED_O4_set_byte(0x0f);

						Delay(1000);
						LED_O4_all_OFF();

						tempo_bpm = MIDI_SYNC_derived_tempo;
						LED_W8_set_byte(0xff);
						LED_B5_set_byte(0x1f);

						gecho_stop_listen_for_tempo();
						while(1);
					}

					tempo_counter = 0;
					sampleCounter = 0;
				}

			}
			else if(edge_detect && signal_history[0] == SIGNAL_STABLE_COUNT)
			{
				edge_detect = 0;
				//vTaskDelay(10);
				LED_R8_all_OFF();
			}
		}


		/*
		//if(sample_f[0]>0)
		{
			avg1 += sample_f[0];
		}
		//if(sample_f[1]>0)
		{
			avg2 += sample_f[1];
		}

		if(sampleCounter%TEMPO_DETECT_SLICE==0)
		{
			avg1/=TEMPO_DETECT_SLICE;
			avg2/=TEMPO_DETECT_SLICE;

			if(avg1>TEMPO_DETECT_THRESHOLD_HIGH || avg2>TEMPO_DETECT_THRESHOLD_HIGH)
			{
				if(!edge_detect)
				{
					edge_detect = 1;

					printf("avg1=%f,avg2=%f\n",avg1,avg2);
					//LED_R8_1_ON;
					LED_R8_set_byte(0x01<<tempo_counter);
					millis_history[tempo_counter] = millis();
					//vTaskDelay(10);

					tempo_counter++;

					if(tempo_counter==8)
					{
						//check if timing looks stable enough to derive tempo
						printf("timing history: ");
						int avg = 0;
						for(int i=1;i<8;i++)
						{
							millis_history[i-1] = millis_history[i] - millis_history[i-1];
							avg += millis_history[i-1];
							printf("%d,",millis_history[i-1]);
						}
						avg /= 7;
						printf(", avg=%d\n", avg);

						//MIDI_SYNC_derived_tempo =

						tempo_counter = 0;
					}
				}
			}
			else if(avg1<TEMPO_DETECT_THRESHOLD_LOW && avg2<TEMPO_DETECT_THRESHOLD_LOW)
			{
				edge_detect = 0;
				//vTaskDelay(10);
				LED_R8_all_OFF();
			}

			avg1 = 0;
			avg2 = 0;
		}
		//if(sampleCounter%TEMPO_DETECT_SLICE==TEMPO_DETECT_SLICE*0.8)
		//{
			//LED_R8_all_OFF();
		//}
		//if(sampleCounter%200==23) //cca 250Hz
		if(sampleCounter%2000==23) //cca 25Hz
		{
			vTaskDelay(2);
		}
		*/
		sampleCounter++;
	}
}

TaskHandle_t listen_for_tempo_task = NULL;
uint8_t listen_for_tempo_task_running = 0;

void gecho_start_listen_for_tempo()
{
	printf("gecho_start_listen_for_tempo()\n");

	codec_select_input(ADC_INPUT_LINE_IN);
	//codec_select_input(ADC_INPUT_BOTH_MIXED);
	//codec_select_input(ADC_INPUT_MIC);

	printf("gecho_start_listen_for_tempo(): starting listen_for_tempo_task\n");
	xTaskCreatePinnedToCore((TaskFunction_t)&process_listen_for_tempo, "listen_tempo_task", 2048, NULL, 9, &listen_for_tempo_task, 1);
	listen_for_tempo_task_running = 1;
}

void gecho_stop_listen_for_tempo()
{
	if(!listen_for_tempo_task_running)//listen_for_tempo_task==NULL)
	{
		printf("gecho_stop_listen_for_tempo(): task not running\n");
		return;
	}

	printf("gecho_stop_listen_for_tempo(): deleting listen_for_tempo_task\n");
	listen_for_tempo_task_running = 0;
	vTaskDelete(listen_for_tempo_task);
	Delay(10);
	//listen_for_tempo_task = NULL;
	printf("gecho_stop_listen_for_tempo(): task deleted\n");

	//return input select back to previous user-defined value
	codec_select_input(ADC_input_select);
}
