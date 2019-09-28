/*
 * Accelerometer.h
 *
 *  Created on: 16 Jun 2018
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

#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

#include <stdint.h>
#include "hw/kx123.h"
#include "hw/kx123_registers.h"

#define USE_ACCELEROMETER
//#define DEBUG_ACCELEROMETER
//#define ACCELEROMETER_TEST

//#include <stdint.h> //for uint16_t type

typedef struct {
	int measure_delay;
	//int cycle_delay;
} AccelerometerParam_t;

#define ACC_RESULTS 3

extern float acc_res[ACC_RESULTS], acc_res1[ACC_RESULTS], acc_res2[ACC_RESULTS], acc_res3[ACC_RESULTS];
extern KX123 acc;

void init_accelerometer();
void process_accelerometer(void *pvParameters);
//void accelerometer_test();

#endif /* ACCELEROMETER_H_ */
