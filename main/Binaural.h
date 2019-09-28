/*
 * binaural.h
 *
 *  Created on: Mar 5, 2019
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

#ifndef BINAURAL_H_
#define BINAURAL_H_

//#include "hw/init.h"
//#include <Dekrispator.h>

extern int beats_selected;

void indicate_binaural(const char *wave);
void binaural_program(void *pvParameters);

#endif /* BINAURAL_H_ */
