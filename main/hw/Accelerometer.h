/*
 * Accelerometer.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 16 Jun 2018
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

#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

#include <stdint.h>
#include "hw/kx123.h"
#include "hw/kx123_registers.h"

/*
#include <board.h>

#ifdef BOARD_GECHO_V179
#define USE_LIS3DH_ACCELEROMETER
#define LIS3DH_I2C_ADDRESS 0x19
#else
#define USE_KIONIX_ACCELEROMETER
#endif
*/

//#define DEBUG_ACCELEROMETER
//#define ACCELEROMETER_TEST

//#include <stdint.h> //for uint16_t type

typedef struct {
	int measure_delay;
	//int cycle_delay;
	int startup_delay;
} AccelerometerParam_t;

#define ACC_RESULTS 3
#define ACC_X_AXIS acc_res[0]
#define ACC_Y_AXIS acc_res[1]
#define ACC_Z_AXIS acc_res[2]

extern float acc_res[ACC_RESULTS];//, acc_res1[ACC_RESULTS];//, acc_res2[ACC_RESULTS], acc_res3[ACC_RESULTS];
extern int acc_data[];
extern KX123 acc;

void init_accelerometer();
void process_accelerometer(void *pvParameters);
void accelerometer_test();

#endif /* ACCELEROMETER_H_ */
