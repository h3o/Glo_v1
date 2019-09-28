/*
 * FreqDetect.cpp
 *
 *  Created on: Oct 31, 2016
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

#include "FreqDetect.h"
#include <hw/signals.h>
#include <hw/init.h>

void AutoCorrelation(uint16_t *samples_buffer, int samples_cnt, int *result)
{
	float *correlation = (float*)malloc(samples_cnt * sizeof(float));

	float sum;

	//float sum_correlations = 0;

	for(int i=0;i<samples_cnt/2;i++)
	{
		sum = 0;
		for(int j=0;j<samples_cnt-i;j++)
		{
			sum+=samples_buffer[j]*samples_buffer[j+i];
		}
		correlation[i] = sum;
		//sum_correlations += sum;
	}

	//sum_correlations += 1;

	float max_corr = -10000.0f;
	float corr_at_point;
	int max_lag = -1;
	int max_lag0 = -1;
	int max_lag1 = -1;
	int max_lag2 = -1;
	for(int i=15;i<samples_cnt/2;i++)
	{
		corr_at_point = correlation[i];// + 0.65 * correlation[i/2];
		/*
			if(i<N_SAMPLES/4)
			{
				corr_at_point += 0.35 * correlation[i*2];
			}
		*/
		if(corr_at_point > max_corr)
		{
			max_corr = corr_at_point;
			max_lag2 = max_lag1;
			max_lag1 = max_lag0;
			max_lag0 = max_lag;
			max_lag = i;
		}
	}

	free(correlation);

	result[0] = max_lag;
	result[1] = max_lag0;
	result[2] = max_lag1;
	result[3] = max_lag2;
}

//=============================================================================================

void process_autocorrelation(void *pvParameters)
{
	printf("process_autocorrelation(): started on core ID=%d\n", xPortGetCoreID());

	while(1)
	{
		//printf("process_autocorrelation(): running on core ID=%d\n", xPortGetCoreID());

		//if(autocorrelation_buffer_full)
		if(!autocorrelation_rec_sample)
		{
			//printf("process_autocorrelation(): buffer full, processing...\n");

			AutoCorrelation(autocorrelation_buffer, AUTOCORRELATION_REC_SAMPLES, autocorrelation_result);
			//printf("AutoCorrelation result=%d,%d,%d,%d\n", autocorrelation_result[0], autocorrelation_result[1], autocorrelation_result[2], autocorrelation_result[3]);

			if(autocorrelation_result[0]>-1 &&
			   autocorrelation_result[1]>-1 &&
			   autocorrelation_result[2]>-1 &&
			   autocorrelation_result[3]>-1)
			{
				autocorrelation_result_rdy = 1;
			}

			//autocorrelation_buffer_full = 0;
			autocorrelation_rec_sample = AUTOCORRELATION_REC_SAMPLES;
			//while(1){};
		}
	}
}

TaskHandle_t autocorrelation_task_handle;

void start_autocorrelation(int core)
{
	printf("start_autocorrelation(): starting on core ID=%d\n", core);
	//#ifdef USE_AUTOCORRELATION
	autocorrelation_buffer = (uint16_t*)malloc(AUTOCORRELATION_REC_SAMPLES * sizeof(uint16_t) + 4);
	autocorrelation_rec_sample = AUTOCORRELATION_REC_SAMPLES;
	//autocorrelation_buffer_full = 0;
	xTaskCreatePinnedToCore((TaskFunction_t)&process_autocorrelation, "process_autocorrelation_task", 2048, NULL, 10, &autocorrelation_task_handle, core);
	//#endif
	printf("start_autocorrelation(): task handle = 0x%x\n", (uint32_t)autocorrelation_task_handle);
}

void stop_autocorrelation()
{
	printf("stop_autocorrelation(): deleting task\n");
	vTaskDelete(autocorrelation_task_handle);
}

const float ac_to_freqs[49] =
{
		0,0,0,0,0,0,0,0, //0-7
		0,0,0,0,0,0,0, //8-14
		1760.00, //15 = a6 //last detectable note
		1567.98, //16 = g6
		1479.98, //17 = f#6
		1396.91, //18 = f6
		0,
		1046.50, //20 = e6
		1244.51, //21 = d#6
		1046.50, //22 = d6
		1108.73, //23 = c#6
		1046.50, //24 = c6
		0,
		987.77, //26 = b5
		0,
		880.00, //28 = a5
		0,
		830.61,	//30 = g#5
		0,
		783.99, //32 = g5
		0,
		739.99,	//34 f#5
		0,
		698.46, //36 = f5
		0,
		659.26, //38 = e5
		0,
		622.25,	//40 d#5
		0,
		587.33, //42 = d5
		0,
		0,
		0,
		554.37,	//46 c#5
		0,
		523.25 //48 = c5
};

uint16_t ac_to_freq(int ac_result)
{

	if(ac_result <= 48)
	{
		return ac_to_freqs[ac_result];
	}
	ac_result/=2;
	if(ac_result <= 48)
	{
		return ac_to_freqs[ac_result]/2;
	}
	ac_result/=2;
	if(ac_result <= 48)
	{
		return ac_to_freqs[ac_result]/4;
	}
	return 0;
}
