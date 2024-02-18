/*
 * Sensors.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 13 Mar 2019
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

#include "Sensors.h"

#include "hw/leds.h"
#include "hw/ui.h"

#ifdef BOARD_GECHO
//#define DEBUG_IR_SENSORS
#define DISPLAY_IR_SENSOR_LEVELS

uint16_t ir_val[IR_SENSORS*2];
float ir_res[IR_SENSORS] = {0,0,0,0};
TaskHandle_t sensors_task_handle;

void gecho_sensors_init()
{
	printf("gecho_sensors_init()\n");

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); //GPI36 is connected to ADC1 channel #0
	adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_11); //GPI37 is connected to ADC1 channel #1
	adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11); //GPI38 is connected to ADC1 channel #2
	adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11); //GPI39 is connected to ADC1 channel #3

	//static AccelerometerParam_t AccelerometerParam;
	//AccelerometerParam.measure_delay = 5;//works with dev prototype
	//AccelerometerParam.cycle_delay = 10;
	printf("gecho_sensors_init(): starting process_sensors task\n");
	xTaskCreatePinnedToCore((TaskFunction_t)&process_sensors, "process_sensors_task", 2048, NULL, PRIORITY_SENSORS_TASK, &sensors_task_handle, CPU_CORE_SENSORS_TASK);
}

void process_sensors(void *pvParameters)
{
	//int measure_delay = SENSORS_MEASURE_DELAY;
	//int cycle_delay = SENSORS_CYCLE_DELAY;

	printf("process_sensors(): task running on core ID=%d\n",xPortGetCoreID());
	//printf("process_sensors(): measure_delay = %d\n",measure_delay);
	//printf("process_sensors(): cycle_delay = %d\n",cycle_delay);

	while(1)
	{
		if(sensors_active)
		{
			IR_DRV1_ON;
			Delay(SENSORS_MEASURE_DELAY);

			ir_val[4] = adc1_get_raw(ADC1_CHANNEL_0);
			ir_val[5] = adc1_get_raw(ADC1_CHANNEL_1);
			ir_val[3] = adc1_get_raw(ADC1_CHANNEL_2);
			ir_val[2] = adc1_get_raw(ADC1_CHANNEL_3);

			#ifdef DEBUG_IR_SENSORS_X
			printf("GPI36-39 analog values: %d %d %d %d\n", ir_val[0], ir_val[1], ir_val[2], ir_val[3]);
			#endif

			IR_DRV1_OFF;
			IR_DRV2_ON;
			Delay(SENSORS_MEASURE_DELAY);

			ir_val[0] = adc1_get_raw(ADC1_CHANNEL_0);
			ir_val[1] = adc1_get_raw(ADC1_CHANNEL_1);
			ir_val[7] = adc1_get_raw(ADC1_CHANNEL_2);
			ir_val[6] = adc1_get_raw(ADC1_CHANNEL_3);

			IR_DRV2_OFF;

			#ifdef DEBUG_IR_SENSORS_X
			printf("GPI36-39 analog values: %d %d %d %d\n", ir_val[4], ir_val[5], ir_val[6], ir_val[7]);
			#endif

			#ifdef DEBUG_IR_SENSORS_X
			printf("process_sensors(): off=%d,%d,%d,%d on=%d,%d,%d,%d\n",ir_val[0],ir_val[1],ir_val[2],ir_val[3],ir_val[4],ir_val[5],ir_val[6],ir_val[7]);
			#endif

			for(int i=0;i<4;i++)
			{
				ir_res[i] = ((float)ir_val[i+4] - (float)ir_val[i]) / SENSORS_MAX_VALUE;
			}

			//memcpy(normalized_params, ir_res, sizeof(float)*4);

			#ifdef DEBUG_IR_SENSORS
			printf("process_sensors(): diff=%f,%f,%f,%f\n",ir_res[0],ir_res[1],ir_res[2],ir_res[3]);
			#endif

			if(sensors_override)
			{
				//red
				if(sensors_override&0x00000080) { ir_res[0] = IR_sensors_THRESHOLD_8 + 0.01f; }
				else if(sensors_override&0x00000040) { ir_res[0] = IR_sensors_THRESHOLD_7 + 0.01f; }
				else if(sensors_override&0x00000020) { ir_res[0] = IR_sensors_THRESHOLD_6 + 0.01f; }
				else if(sensors_override&0x00000010) { ir_res[0] = IR_sensors_THRESHOLD_5 + 0.01f; }
				else if(sensors_override&0x00000008) { ir_res[0] = IR_sensors_THRESHOLD_4 + 0.01f; }
				else if(sensors_override&0x00000004) { ir_res[0] = IR_sensors_THRESHOLD_3 + 0.01f; }
				else if(sensors_override&0x00000002) { ir_res[0] = IR_sensors_THRESHOLD_2 + 0.01f; }
				else if(sensors_override&0x00000001) { ir_res[0] = IR_sensors_THRESHOLD_1 + 0.01f; }

				//orange
				if(sensors_override&0x00000800) { ir_res[1] = IR_sensors_THRESHOLD_7 + 0.01f; }
				else if(sensors_override&0x00000400) { ir_res[1] = IR_sensors_THRESHOLD_5 + 0.01f; }
				else if(sensors_override&0x00000200) { ir_res[1] = IR_sensors_THRESHOLD_3 + 0.01f; }
				else if(sensors_override&0x00000100) { ir_res[1] = IR_sensors_THRESHOLD_1 + 0.01f; }

				//blue
				if(sensors_override&0x00100000) { ir_res[2] = IR_sensors_THRESHOLD_8 + 0.01f; }
				else if(sensors_override&0x00080000) { ir_res[2] = IR_sensors_THRESHOLD_6 + 0.01f; }
				else if(sensors_override&0x00040000) { ir_res[2] = IR_sensors_THRESHOLD_4 + 0.01f; }
				else if(sensors_override&0x00020000) { ir_res[2] = IR_sensors_THRESHOLD_2 + 0.01f; }
				else if(sensors_override&0x00010000) { ir_res[2] = IR_sensors_THRESHOLD_1 + 0.01f; }

				//white
				if(sensors_override&0x80000000) { ir_res[3] = IR_sensors_THRESHOLD_8 + 0.01f; }
				else if(sensors_override&0x40000000) { ir_res[3] = IR_sensors_THRESHOLD_7 + 0.01f; }
				else if(sensors_override&0x20000000) { ir_res[3] = IR_sensors_THRESHOLD_6 + 0.01f; }
				else if(sensors_override&0x10000000) { ir_res[3] = IR_sensors_THRESHOLD_5 + 0.01f; }
				else if(sensors_override&0x08000000) { ir_res[3] = IR_sensors_THRESHOLD_4 + 0.01f; }
				else if(sensors_override&0x04000000) { ir_res[3] = IR_sensors_THRESHOLD_3 + 0.01f; }
				else if(sensors_override&0x02000000) { ir_res[3] = IR_sensors_THRESHOLD_2 + 0.01f; }
				else if(sensors_override&0x01000000) { ir_res[3] = IR_sensors_THRESHOLD_1 + 0.01f; }
			}

			#ifdef DISPLAY_IR_SENSOR_LEVELS
			if(SENSORS_LEDS_indication_enabled)
			{
				display_IR_sensors_levels();
			}
			#endif
		}
		else
		{
			IR_DRV1_OFF;
			IR_DRV2_OFF;
		}

		Delay(SENSORS_CYCLE_DELAY);
	}
}

void gecho_sensors_test()
{
	vTaskDelete(sensors_task_handle);

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); //GPI36 is connected to ADC1 channel #0
	adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_11); //GPI37 is connected to ADC1 channel #1
	adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11); //GPI38 is connected to ADC1 channel #2
	adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11); //GPI39 is connected to ADC1 channel #3

	int val[8];

	channel_running = 1;

	while(1)
	{
		IR_DRV1_ON;
		Delay(50);

		val[0] = adc1_get_raw(ADC1_CHANNEL_0);
		val[1] = adc1_get_raw(ADC1_CHANNEL_1);
		val[2] = adc1_get_raw(ADC1_CHANNEL_2);
		val[3] = adc1_get_raw(ADC1_CHANNEL_3);
		//printf("GPI36-39 analog values: %d %d %d %d\n", val[0], val[1], val[2], val[3]);

		IR_DRV1_OFF;
		IR_DRV2_ON;
		Delay(50);

		val[4] = adc1_get_raw(ADC1_CHANNEL_0);
		val[5] = adc1_get_raw(ADC1_CHANNEL_1);
		val[6] = adc1_get_raw(ADC1_CHANNEL_2);
		val[7] = adc1_get_raw(ADC1_CHANNEL_3);
		//printf("GPI36-39 analog values: %d %d %d %d\n", val[4], val[5], val[6], val[7]);

		IR_DRV2_OFF;

		printf("GPI36-39 analog values: %d	%d	%d	%d	%d	%d	%d	%d,	differences: %d	%d	%d	%d\n",
			val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7],
			val[4] - val[0],
			val[5] - val[1],
			val[6] - val[2],
			val[7] - val[3]);
	}
}

#endif /* BOARD_GECHO */
