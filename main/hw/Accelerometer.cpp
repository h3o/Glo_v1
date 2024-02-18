/*
 * Accelerometer.cpp
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

#include "Accelerometer.h"
#include "board.h"
#include "init.h"
#include "gpio.h"
#include "ui.h"

#ifdef BOARD_GECHO_V179
#define USE_LIS3DH_ACCELEROMETER
#define LIS3DH_I2C_ADDRESS 0x19
#else
#define USE_KIONIX_ACCELEROMETER
#endif

float acc_res[ACC_RESULTS] = {0,0,-1.0f};

#ifdef USE_KIONIX_ACCELEROMETER
float acc_res1[ACC_RESULTS];
#endif

int acc_data[3] = {0,0,0};
#ifdef USE_KIONIX_ACCELEROMETER
	KX123 acc(I2C_MASTER_NUM);
#endif

void init_accelerometer()
{
	printf("init_accelerometer()\n");

	static AccelerometerParam_t AccelerometerParam;

	#ifdef USE_KIONIX_ACCELEROMETER
	int error = acc.set_defaults();
	if(error)
	{
		printf("ERROR in acc.set_defaults() code=%d\n",error);
	}

	AccelerometerParam.measure_delay = 5;	//works with dev prototype
	#endif

	#ifdef USE_LIS3DH_ACCELEROMETER
	AccelerometerParam.measure_delay = 100;
	AccelerometerParam.startup_delay = 500;
	#endif

	xTaskCreatePinnedToCore((TaskFunction_t)&process_accelerometer, "process_accelerometer_task", 2048, &AccelerometerParam, PRIORITY_ACCELEROMETER_TASK, NULL, CPU_CORE_ACCELEROMETER_TASK);

	#ifdef ACCELEROMETER_TEST
		accelerometer_test();
		while(1);
	#endif
}

/*
 * time smoothing constant for low-pass filter
 * 0 <= alpha <= 1 ; a smaller value basically means more smoothing
 * See: http://en.wikipedia.org/wiki/Low-pass_filter#Discrete-time_realization
 */
#define lpALPHA 0.5f

void acc_lowPass(float *input, float *output)
{
	for(int i=0;i<3; i++)
	{
		output[i] = output[i] + lpALPHA * (input[i] - output[i]);
	}
}

void acc_lowPass_int(int16_t *input, int *output)
{
	for(int i=0;i<3; i++)
	{
		output[i] = output[i] + (input[i] - output[i]) / 2;
	}
}

#ifdef USE_KIONIX_ACCELEROMETER

void process_accelerometer(void *pvParameters)
{
	int measure_delay = ((AccelerometerParam_t*)pvParameters)->measure_delay;
	//int cycle_delay = ((AccelerometerParam_t*)pvParameters)->cycle_delay;

	//#ifdef DEBUG_ACCELEROMETER
	printf("process_accelerometer(): task running on core ID=%d\n",xPortGetCoreID());
	printf("process_accelerometer(): measure_delay = %d\n",measure_delay);
	//printf("process_accelerometer(): cycle_delay = %d\n",cycle_delay);
	//#endif

	while(1)
	{
		Delay(measure_delay);

		if(!accelerometer_active)
		{
			continue;
		}

		if(i2c_driver_installed)
		{
			acc.getresults_g(acc_res1);
		}
		else
		{
			#ifdef DEBUG_ACCELEROMETER
			//printf("process_accelerometer(): I2C driver not installed, skipping iteration[1]\n");
			printf("I2C offline\n");
			#endif
			continue;
		}

		if(persistent_settings.ACC_INVERT)
		{
			if(persistent_settings.ACC_INVERT & 0x01)
			{
				acc_res1[0] = -acc_res1[0];
			}
			if(persistent_settings.ACC_INVERT & 0x02)
			{
				acc_res1[1] = -acc_res1[1];
			}
			if(persistent_settings.ACC_INVERT & 0x04)
			{
				acc_res1[2] = -acc_res1[2];
			}
		}

		if(persistent_settings.ACC_ORIENTATION)
		{
			float tmp;
			if(persistent_settings.ACC_ORIENTATION==ACC_ORIENTATION_XZY)
			{
				tmp = acc_res1[1];
				acc_res1[1] = acc_res1[2];
				acc_res1[2] = tmp;
			}
			if(persistent_settings.ACC_ORIENTATION==ACC_ORIENTATION_YXZ)
			{
				tmp = acc_res1[1];
				acc_res1[1] = acc_res1[0];
				acc_res1[0] = tmp;
			}
			#ifdef ACC_ORIENTATION_YZX
			if(persistent_settings.ACC_ORIENTATION==ACC_ORIENTATION_YZX)
			{
				tmp = acc_res1[0];
				acc_res1[0] = acc_res1[1];
				acc_res1[1] = acc_res1[2];
				acc_res1[2] = tmp;
			}
			#endif
			#ifdef ACC_ORIENTATION_ZXY
			if(persistent_settings.ACC_ORIENTATION==ACC_ORIENTATION_ZXY)
			{
				tmp = acc_res1[2];
				acc_res1[2] = acc_res1[1];
				acc_res1[1] = acc_res1[0];
				acc_res1[0] = tmp;
			}
			#endif
			if(persistent_settings.ACC_ORIENTATION==ACC_ORIENTATION_ZYX)
			{
				tmp = acc_res1[2];
				acc_res1[2] = acc_res1[0];
				acc_res1[0] = tmp;
			}
		}

		acc_lowPass(acc_res1,acc_res);

		#ifdef DEBUG_ACCELEROMETER
		printf("process_accelerometer(): in=%f,%f,%f out=%f,%f,%f\n",acc_res1[0],acc_res1[1],acc_res1[2],acc_res[0],acc_res[1],acc_res[2]);
		#endif
	}
}

//#ifdef ACCELEROMETER_TEST
void accelerometer_test()
{
	int error;
	float res[3];

	KX123 acc(I2C_MASTER_NUM);
	uint8_t axis;
	uint8_t int_reason;

	error = acc.set_defaults();
	printf("acc.set_defaults() err=%d\n",error);

	while(1)
	{
		acc.get_tap_interrupt_axis((e_axis*)&axis);

		acc.get_interrupt_reason((e_interrupt_reason*)&int_reason);

		if(int_reason > 0)
		{
			printf("get_interrupt_reason=%x ... ",int_reason);
			printf("get_tap_interrupt_axis=%x \n",axis);
		}

		acc.clear_interrupt();

		error = acc.getresults_g(res);
		printf("err=%d,res=%f,%f,%f\n",error,res[0],res[1],res[2]);

		Delay(10);
	}
}
//#endif

#endif

#ifdef USE_LIS3DH_ACCELEROMETER

uint16_t LIS3DH_read_register(uint8_t reg)
{
    uint8_t val;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, LIS3DH_I2C_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, LIS3DH_I2C_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &val, I2C_MASTER_NACK);

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return val;
}

uint16_t LIS3DH_read_word(uint8_t reg)
{
    uint16_t val;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, LIS3DH_I2C_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg | 0x80, ACK_CHECK_EN); //auto-increment

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, LIS3DH_I2C_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, (unsigned char*)&val, 2, I2C_MASTER_LAST_NACK);

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return val;
}

void LIS3DH_write_register(uint8_t reg, uint8_t val)
{
    esp_err_t result;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    result = i2c_master_write_byte(cmd, LIS3DH_I2C_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    printf("LIS3DH_write_register(): result = %d\n", result);
    result = i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    printf("LIS3DH_write_register(): result = %d\n", result);
    result = i2c_master_write_byte(cmd, val, ACK_CHECK_EN);
    printf("LIS3DH_write_register(): result = %d\n", result);
    i2c_master_stop(cmd);
    result = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    printf("LIS3DH_write_register(): result = %d\n", result);
    i2c_cmd_link_delete(cmd);
}

void process_accelerometer(void *pvParameters)
{
	int measure_delay = ((AccelerometerParam_t*)pvParameters)->measure_delay;
	int startup_delay = ((AccelerometerParam_t*)pvParameters)->startup_delay;

	printf("process_accelerometer(): task running on core ID=%d\n",xPortGetCoreID());
	printf("process_accelerometer(): measure_delay = %d\n",measure_delay);

	int8_t byte_reg;
	int16_t acc_data1[3];

	Delay(startup_delay);

	while(i2c_bus_mutex)
	{
		printf("process_accelerometer(): waiting for I2C\n");
		Delay(10);
	}

	i2c_bus_mutex = 1;

	byte_reg = LIS3DH_read_register(0x0F); //0x0F: WHO_AM_I register
	printf("process_accelerometer(): WHO_AM_I reg = %d\n", byte_reg);

	byte_reg = LIS3DH_read_register(0x1E); //0x1E: CTRL_REG0 register
	printf("process_accelerometer(): CTRL_REG0 reg = %d\n", byte_reg);

	byte_reg = LIS3DH_read_register(0x20); //0x20: CTRL_REG1 register
	printf("process_accelerometer(): CTRL_REG1 reg = %d\n", byte_reg);

	LIS3DH_write_register(0x20, 0x77); //0x20: CTRL_REG1 register -> set ODR to 400Hz (0111xxxx)

	byte_reg = LIS3DH_read_register(0x20); //0x20: CTRL_REG1 register
	printf("process_accelerometer(): CTRL_REG1 reg = %d\n", byte_reg);

	i2c_bus_mutex = 0;

	while(1)
	{
		Delay(measure_delay);

		if(!accelerometer_active)
		{
			continue;
		}

		if(i2c_driver_installed)
		{
			if(i2c_bus_mutex)
			{
				continue;
			}

			i2c_bus_mutex = 1;

			//acc_val = LIS3DH_read_register(0x28);
			//acc_val = LIS3DH_read_register(0x29);
			//acc_res1[0] = ((float)acc_val)/64.0f;
			//printf("process_accelerometer(): acc_val[0] = %d\n", acc_val);
			acc_data1[0] = LIS3DH_read_word(0x28);
			//printf("process_accelerometer(): acc_data1[0] = %d\n", acc_data1[0]);

			//acc_val = LIS3DH_read_register(0x2a);
			//acc_val = LIS3DH_read_register(0x2b);
			//acc_res1[1] = ((float)acc_val)/64.0f;
			//printf("process_accelerometer(): acc_val[1] = %d\n", acc_val);
			acc_data1[1] = LIS3DH_read_word(0x2a);
			//printf("process_accelerometer(): acc_data1[1] = %d\n", acc_data1[1]);

			//acc_val = LIS3DH_read_register(0x2c);
			//acc_val = LIS3DH_read_register(0x2d);
			//acc_res1[2] = ((float)acc_val)/64.0f;
			//printf("process_accelerometer(): acc_val[2] = %d\n", acc_val);
			acc_data1[2] = LIS3DH_read_word(0x2c);
			//printf("process_accelerometer(): acc_data1[2] = %d\n", acc_data1[2]);

			i2c_bus_mutex = 0;

			//printf("acc:	%f	%f	%f\n", acc_res1[0], acc_res1[1], acc_res1[2]);
		}
		else
		{
			#ifdef DEBUG_ACCELEROMETER
			printf("process_accelerometer(): I2C offline\n");
			#endif
			continue;
		}

		acc_lowPass_int(acc_data1,acc_data);
		acc_res[0] = ((float)acc_data[0])/16384.0f;
		acc_res[1] = ((float)acc_data[1])/16384.0f;
		acc_res[2] = ((float)acc_data[2])/16384.0f;
		#ifdef DEBUG_ACCELEROMETER
		printf("process_accelerometer(): in =	%d	%d	%d out =	%d	%d	%d,	%f	%f	%f\n",acc_data1[0],acc_data1[1],acc_data1[2],acc_data[0],acc_data[1],acc_data[2],acc_res[0],acc_res[1],acc_res[2]);
		#endif
	}
}

//#ifdef ACCELEROMETER_TEST
void accelerometer_test()
{
	accelerometer_active = 1;
	channel_running = 1;
	while(1)
	{
		printf("process_accelerometer(): %d	%d	%d,	%f	%f	%f\n",acc_data[0],acc_data[1],acc_data[2],acc_res[0],acc_res[1],acc_res[2]);
		Delay(250);
	}
}
//#endif

#endif
