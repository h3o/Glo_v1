/*
 * Accelerometer.cpp
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

#include <Accelerometer.h>
//#include <Interface.h>
//#include <InitChannels.h>
//#include <hw/codec.h>
//#include <hw/signals.h>
#include <hw/init.h>
#include <hw/gpio.h>
//#include <string.h>


float acc_res[ACC_RESULTS] = {0,0,-1.0f}, acc_res1[ACC_RESULTS], acc_res2[ACC_RESULTS], acc_res3[ACC_RESULTS];
#ifdef USE_ACCELEROMETER
	KX123 acc(I2C_MASTER_NUM);
	//int acc_i, acc_stable_cnt;
#endif

void init_accelerometer()
{
	printf("init_accelerometer()\n");
	#ifdef USE_ACCELEROMETER
	//#ifdef USE_ACCELEROMETER
	int error = acc.set_defaults();
	if(error)
	{
		printf("ERROR in acc.set_defaults() code=%d\n",error);
	}
	//#endif

	#ifdef ACCELEROMETER_TEST
		accelerometer_test();
		while(1);
	#endif
	#endif

	//acc.getresults_g(acc_res); //read initial value immediately, otherwise KX122 hangs
	static AccelerometerParam_t AccelerometerParam;
	AccelerometerParam.measure_delay = 5;	//works with dev prototype
	//AccelerometerParam.cycle_delay = 10;
	xTaskCreatePinnedToCore((TaskFunction_t)&process_accelerometer, "process_accelerometer_task", 2048, &AccelerometerParam, 10, NULL, 1);
}

/*
void process_accelerometer(void *pvParameters)
{
	#ifdef USE_ACCELEROMETER
	int measure_delay = ((AccelerometerParam_t*)pvParameters)->measure_delay;
	int cycle_delay = ((AccelerometerParam_t*)pvParameters)->cycle_delay;

	printf("process_accelerometer(): task running on core ID=%d\n",xPortGetCoreID());
	printf("process_accelerometer(): measure_delay = %d\n",measure_delay);
	printf("process_accelerometer(): cycle_delay = %d\n",cycle_delay);

	while(1)
	{
		//Delay(1); //can restart faster than measure_delay (e.g. if skipped due to driver off)
		if(i2c_driver_installed)
		{
			acc.getresults_g(acc_res1);
		}
		else
		{
			printf("process_accelerometer(): I2C driver not installed, skipping iteration[1]\n");
			continue;
		}
		//printf("acc_res=%f,%f,%f\n",acc_res1[0],acc_res1[1],acc_res1[2]);

		Delay(measure_delay);
		if(i2c_driver_installed)
		{
			acc.getresults_g(acc_res2);
		}
		else
		{
			printf("process_accelerometer(): I2C driver not installed, skipping iteration[2]\n");
			continue;
		}
		//printf("acc_res=%f,%f,%f\n",acc_res2[0],acc_res2[1],acc_res2[2]);

		Delay(measure_delay);
		if(i2c_driver_installed)
		{
			acc.getresults_g(acc_res3);
		}
		else
		{
			printf("process_accelerometer(): I2C driver not installed, skipping iteration[3]\n");
			continue;
		}
		//printf("acc_res=%f,%f,%f\n",acc_res3[0],acc_res3[1],acc_res3[2]);

		acc_stable_cnt = 0;
		for(acc_i=0;acc_i<3;acc_i++)
		{
			if(fabs(acc_res1[acc_i]-acc_res2[acc_i])<0.1
			&& fabs(acc_res2[acc_i]-acc_res3[acc_i])<0.1
			&& fabs(acc_res1[acc_i]-acc_res3[acc_i])<0.1)
			{
				//acc_res[acc_i] = acc_res3[acc_i];
				acc_stable_cnt++;
				//printf("acc.param[%d] stable... ", acc_i);
			}
			else
			{
				//printf("acc.param[%d] NOT STABLE... ", acc_i);
			}
		}

		if(acc_stable_cnt==3)
		{
			//acc_res[0] = acc_res2[0];
			//acc_res[1] = acc_res2[1];
			//acc_res[2] = acc_res2[2];
			memcpy(acc_res,acc_res2,sizeof(acc_res2));
		}
		//printf("\n");
		Delay(cycle_delay);
	}
	#endif
}
*/


/*
 * time smoothing constant for low-pass filter
 * 0 <= alpha <= 1 ; a smaller value basically means more smoothing
 * See: http://en.wikipedia.org/wiki/Low-pass_filter#Discrete-time_realization
 */
//#define lpALPHA 0.15f
#define lpALPHA 0.025f

void acc_lowPass(float *input, float *output)
{
	for(int i=0;i<3; i++)
	{
		output[i] = output[i] + lpALPHA * (input[i] - output[i]);
	}
}

void process_accelerometer(void *pvParameters)
{
	#ifdef USE_ACCELEROMETER
	int measure_delay = ((AccelerometerParam_t*)pvParameters)->measure_delay;
	//int cycle_delay = ((AccelerometerParam_t*)pvParameters)->cycle_delay;

	//#ifdef DEBUG_ACCELEROMETER
	printf("process_accelerometer(): task running on core ID=%d\n",xPortGetCoreID());
	printf("process_accelerometer(): measure_delay = %d\n",measure_delay);
	//printf("process_accelerometer(): cycle_delay = %d\n",cycle_delay);
	//#endif

	while(1)
	{
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
		Delay(measure_delay);

		acc_lowPass(acc_res1,acc_res);

		#ifdef DEBUG_ACCELEROMETER
		printf("process_accelerometer(): in=%f,%f,%f out=%f,%f,%f\n",acc_res1[0],acc_res1[1],acc_res1[2],acc_res[0],acc_res[1],acc_res[2]);
		#endif
	}
	#endif
}


#ifdef ACCELEROMETER_TEST

void accelerometer_test()
{
	float xmax = 0, xmin = 0;
	int error;
	float res[3];

	KX123 acc(I2C_MASTER_NUM);
	//e_axis axis;
	uint8_t axis;

	//e_interrupt_reason int_reason;
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
		//error = acc.getresults_highpass_g(res);
		printf("err=%d,res=%f,%f,%f\n",error,res[0],res[1],res[2]);

		Delay(10);
	}
}

#endif
