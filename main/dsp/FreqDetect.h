/*
 * FreqDetect.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Apr 27, 2016
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
