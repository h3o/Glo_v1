/*
 * binaural.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Mar 5, 2019
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

#ifndef BINAURAL_H_
#define BINAURAL_H_

extern int beats_selected;

void indicate_binaural(const char *wave);
void binaural_program(void *pvParameters);

#endif /* BINAURAL_H_ */
