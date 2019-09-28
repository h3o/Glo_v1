/*
 * Sensors.cpp
 *
 *  Created on: 13 Mar 2019
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

#include <Sensors.h>

#include "hw/leds.h"

#ifdef BOARD_GECHO
//#define DEBUG_IR_SENSORS
#define DISPLAY_IR_SENSOR_LEVELS

uint16_t ir_val[IR_SENSORS*2];
float ir_res[IR_SENSORS] = {0,0,0,0};

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
	xTaskCreatePinnedToCore((TaskFunction_t)&process_sensors, "process_sensors_task", 2048, NULL, 10, NULL, 1);
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

		#ifdef DEBUG_IR_SENSORS
		printf("process_sensors(): diff=%f,%f,%f,%f\n",ir_res[0],ir_res[1],ir_res[2],ir_res[3]);
		#endif

		#ifdef DISPLAY_IR_SENSOR_LEVELS
		display_IR_sensors_levels();
		#endif

		Delay(SENSORS_CYCLE_DELAY);
	}
}

/*
void gecho_sensors_test()
{
	int val[8];
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

		printf("GPI36-39 analog value differences: %d %d %d %d\n",
			val[4] - val[0],
			val[5] - val[1],
			val[6] - val[2],
			val[7] - val[3]);
	}
}
*/

#endif /* BOARD_GECHO */
