/*
 * v1_filters.cpp
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

#ifndef V1_FILTERS_H_
#define V1_FILTERS_H_

#include "v1_FilterIIR00.h"
#include "v1_FilterSimpleIIR02.h"
#include "v1_Chord.h"

#define USE_FILTERS

//#define FILTERS 32 //still works with 2nd order F02
//#define FILTERS 30 //does not work with 4th order F02
//#define FILTERS 28 //max with 4th order F02
//#define FILTERS 24
//#define FILTERS 22 //still works now with ART accelerator on
//#define FILTERS 20
//#define FILTERS 18 //does not work with f00, only with f02
#define FILTERS 16 //looks like max wit 4th order F00
//#define FILTERS 14 //looks like max with Serial
//#define FILTERS 12
//#define FILTERS 10	//v71 had this 10/minor
#define ACCORD 'MINOR'
//#define ACCORD 'MAJOR'
//#define FILTERS 6 //3 base freqs
//#define FILTERS 4
//#define FILTERS 2 //Classic A

//#define CHORD 'NONE'
//#define FILTERS 6

#define CHORD_MAX_VOICES 8	//gives 16 channels total
#define FILTERS_TO_USE 2 //0 or 1 or 2 - which filter algorithm (0 for none)
#define FILTERS_FREQ_CORRECTION 1.035
#define ENABLE_MIXING_DELTAS 0
#define DEFAULT_ARPEGGIATOR_FILTER_PAIR 7 //the last one, numbered from 0

typedef struct {

	float volume_coef,volume_f,vol_ramp,reso2,feedback2;
	bool enable_mixing_deltas;
	float mixing_volumes[FILTERS];
	int mixing_deltas[FILTERS];

	float cutoff2_default;
	float cutoff2[FILTERS];
	float cutoff2lim[2];

	float reso2lim[2];
	float feedback2lim[2];

	int arpeggiator_filter_pair;

} v1_FILTERS_PARAMS;

class v1_filters
{
	public:

		v1_FILTER *iir;
		v1_FilterSimpleIIR02 *iir2;
		v1_FILTERS_PARAMS fp;
		v1_Chord *chord;

		v1_filters(void);
		~v1_filters(void);

		//void filter_setup();
		void filter_setup02();

		void start_update_filters(int f1, int f2, int f3, int f4, float freq1, float freq2, float freq3, float freq4);
		void start_next_chord();
		void progress_update_filters(v1_filters *fil, bool reset_buffers);
		void add_to_update_filters_pairs(int filter_n, float freq);

	private:
		int update_filters_f[CHORD_MAX_VOICES*2];
		float update_filters_freq[CHORD_MAX_VOICES*2];
		int update_filters_loop = 0;

};

#endif /* V1_FILTERS_H_ */
