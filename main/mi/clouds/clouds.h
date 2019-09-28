/*
 * clouds.h
 *
 *  Created on: Mar 6, 2019
 *      Author: mario
 *
 *  Interface for MI Clouds sound engine
 *  Source: https://mutable-instruments.net/modules/clouds/open_source/
 *  Repo: https://github.com/pichenettes/eurorack
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

#ifndef MI_CLOUDS_H_
#define MI_CLOUDS_H_

#define CLOUDS_FAUX_LOW_SAMPLE_RATE
#define CLOUDS_PATCHABLE_PARAMS	10

void clouds_main(int change_sampling_rate);

#endif /* MI_CLOUDS_H_ */
 
