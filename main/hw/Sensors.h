/*
 * Sensors.h
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

#ifndef SENSORS_H_
#define SENSORS_H_

#include <hw/init.h>
#include <hw/gpio.h>

#ifdef BOARD_GECHO

#define IR_SENSORS 4
#define SENSORS_MEASURE_DELAY 		10		//10ms between changing state, which results by close to 50Hz frequency (at 0 cycle delay)
//#define SENSORS_MEASURE_DELAY 	50		//50ms between changing state, which results by close to 10Hz frequency (at 0 cycle delay)
#define SENSORS_CYCLE_DELAY			20		//delay between two measurements, saves energy

//for recalculation to 0..1 float
//#define SENSORS_MAX_VALUE		4096.0f
//#define SENSORS_MAX_VALUE		3333.3f
//#define SENSORS_MAX_VALUE		2907.0f

#ifdef BOARD_GECHO_V179 //also applies to v1.80 and v1.81
#define SENSORS_MAX_VALUE		3600.0f
#else
#define SENSORS_MAX_VALUE		2400.0f
#endif

#define IR_SENSOR_VALUE_S1	ir_res[0]
#define IR_SENSOR_VALUE_S2	ir_res[1]
#define IR_SENSOR_VALUE_S3	ir_res[2]
#define IR_SENSOR_VALUE_S4	ir_res[3]

void gecho_sensors_init();
void gecho_sensors_test();

void process_sensors(void *pvParameters);

#endif /* BOARD_GECHO */

#endif /* SENSORS_H_ */
