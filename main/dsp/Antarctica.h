/*
 * Antarctica.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Sep 24, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of GNU GPLv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/
 *
 */

#ifndef ANTARCTICA_H_
#define ANTARCTICA_H_

typedef struct isochronic_def_str {
	int ALPHA_LOW;
	int ALPHA_HIGH;
	int BETA_LOW;
	int BETA_HIGH;
	int DELTA_LOW;
	int DELTA_HIGH;
	int THETA_LOW;
	int THETA_HIGH;
	int PROGRAM_LENGTH;
	int TONE_LENGTH;
	int TONE_LENGTH_STEP;
	int TONE_LENGTH_MAX;
	float TONE_CUTOFF;
	float TONE_CUTOFF_STEP;
	float TONE_CUTOFF_MAX;
	float TONE_VOLUME;
	float TONE_VOLUME_STEP;
	float TONE_VOLUME_MAX;
	float WIND_VOLUME;
	float SIGNAL_LIMIT_MIN;
	float SIGNAL_LIMIT_MAX;
} isochronic_def;

void song_of_wind_and_ice();

#endif /* ANTARCTICA_H_ */
