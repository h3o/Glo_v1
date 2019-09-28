/*
 * FreqDetect.h
 *
 *  Created on: Apr 27, 2016
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

#ifndef FREQ_DETECT_H_
#define FREQ_DETECT_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

void AutoCorrelation(uint16_t *samples_buffer, int samples_cnt, int *result);
void process_autocorrelation(void *pvParameters);
void start_autocorrelation(int core);
void stop_autocorrelation();
uint16_t ac_to_freq(int ac_result);

#endif /* FREQ_DETECT_H_ */
