/*
 * freqs.h
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

#ifndef FREQS_H_
#define FREQS_H_

//Chords ----------------------------------------------------------

#define MAJOR 1
#define MINOR 2

#if FILTERS == 32
	int freqs[] = {
		55, 110, 220, 440, 880, 1760,
		275, 550, 1100, 2200, 3300,
		165, 330, 660, 1320, 2640};
#endif

#if FILTERS == 30
	int freqs[] = {55, 110, 220, 440, 880, 1760, 275, 550, 1100, 2200, 165, 330, 660, 1320, 2640};
#endif

#if FILTERS == 28
	int freqs[] = {55, 110, 220, 440, 880, 1760, 550, 1100, 2200, 165, 330, 660, 1320, 2640};
#endif

#if FILTERS == 26
	int freqs[] = {55, 110, 220, 440, 880, 1760, 550, 1100, 2200, 165, 330, 660, 1320};
#endif

#if FILTERS == 24
	int freqs[] = {55, 110, 220, 440, 880, 1760, 550, 1100, 2200, 330, 660, 2640};
#endif

#if FILTERS == 22
	int freqs[] = {55, 110, 440, 660, 880, 550, 220, 1760, 1100, 2200, 2640};
#endif

#if FILTERS == 20
	int freqs[] = {110, 440, 660, 880, 550, 220, 1760, 1100, 2200, 2640};
#endif

#if FILTERS == 18
	int freqs[] = {440, 660, 880, 550, 220, 1760, 1100, 2200, 2640};
#endif

#if FILTERS == 16
	#if ACCORD == MAJOR
		int freqs[] = {440, 660, 880, 550, 220, 1760, 1100, 110};
	#elif ACCORD == MINOR
		int freqs[] = {440, 660, 880, 520, 220, 1760, 1040, 110};
	#endif
#endif

#if FILTERS == 14
	int freqs[] = {110, 220, 440, 520, 660, 880, 1040};
#endif

#if FILTERS == 12
	#if ACCORD == MAJOR
		int freqs[] = {440, 550, 660, 880, 1100, 2200};
	#elif ACCORD == MINOR
		int freqs[] = {440, 660, 880, 520, 220, 1760};
	#endif
#endif

#if FILTERS == 10
	#if ACCORD == MAJOR
		int freqs[] = {440, 550, 660, 880, 1100};
	#elif ACCORD == MINOR
		int freqs[] = {440, 660, 110, 520, 220};
	#endif
#endif

#if FILTERS == 8
	#if ACCORD == MAJOR
		int freqs[] = {440, 550, 660, 880};
	#elif ACCORD == MINOR
		int freqs[] = {440, 660, 110, 520};
	#endif
#endif

#if FILTERS == 6
	#if ACCORD == MAJOR
		int freqs[] = {440, 550, 660}; //major chord
	#elif ACCORD == MINOR
		int freqs[] = {440, 520, 660}; //minor chord
	#endif
#endif

#if FILTERS == 4
	int freqs[] = {440, 550};
#endif

#if FILTERS == 2
	int freqs[] = {440};
#endif

#endif /* FREQS_H_ */
