/*
 * v1_freqs.h
 *
 *  Created on: Apr 27, 2016
 *      Author: mayo
 */

#ifndef V1_FREQS_H_
#define V1_FREQS_H_

//Chords ----------------------------------------------------------

#if FILTERS == 32
	int v1_freqs[] = {
		55, 110, 220, 440, 880, 1760,
		275, 550, 1100, 2200, 3300,
		165, 330, 660, 1320, 2640};
#endif

#if FILTERS == 30
	int v1_freqs[] = {55, 110, 220, 440, 880, 1760, 275, 550, 1100, 2200, 165, 330, 660, 1320, 2640};
#endif

#if FILTERS == 28
	int v1_freqs[] = {55, 110, 220, 440, 880, 1760, 550, 1100, 2200, 165, 330, 660, 1320, 2640};
#endif

#if FILTERS == 24
	int v1_freqs[] = {55, 110, 220, 440, 880, 1760, 550, 1100, 2200, 330, 660, 2640};
#endif

#if FILTERS == 22
	int v1_freqs[] = {55, 110, 440, 660, 880, 550, 220, 1760, 1100, 2200, 2640};
#endif

#if FILTERS == 20
	int v1_freqs[] = {110, 440, 660, 880, 550, 220, 1760, 1100, 2200, 2640};
#endif

#if FILTERS == 18
	int v1_freqs[] = {440, 660, 880, 550, 220, 1760, 1100, 2200, 2640};
#endif

#if FILTERS == 16
	//int v1_freqs[] = {440, 660, 880, 550, 220, 1760, 1100, 2200};//, 520}; //2640
	#if ACCORD == 'MAJOR'
		int v1_freqs[] = {440, 660, 880, 550, 220, 1760, 1100, 110};//, 520}; //2640
	#elif ACCORD == 'MINOR'
		int v1_freqs[] = {440, 660, 880, 520, 220, 1760, 1040, 110};//, 520}; //2640
	#endif
#endif

#if FILTERS == 14
	int v1_freqs[] = {110, 220, 440, 520, 660, 880, 1040};//, 1760};//, 520};
#endif

#if FILTERS == 12
	#if ACCORD == 'MAJOR'
		////int v1_freqs[FILTERS/2] = {440, 660, 880, 550, 220};//, 520};
		int v1_freqs[] = {440, 550, 660, 880, 1100, 2200};
	#elif ACCORD == 'MINOR'
		int v1_freqs[] = {440, 660, 880, 520, 220, 1760};//, 1040};//, 520};
	#endif
#endif

#if FILTERS == 10
	#if ACCORD == 'MAJOR'
		int v1_freqs[] = {440, 550, 660, 880, 1100};
	#elif ACCORD == 'MINOR'
		//int v1_freqs[] = {440, 660, 880, 520, 220};
		int v1_freqs[] = {440, 660, 110, 520, 220};
	#endif
#endif

#if FILTERS == 6
	#if ACCORD == 'MAJOR'
		int v1_freqs[] = {440, 550, 660}; //major chord
	#elif ACCORD == 'MINOR'
		int v1_freqs[] = {440, 520, 660}; //minor chord
	#endif
#endif

#if FILTERS == 4
	//int v1_freqs[] = {440, 1760};
	int v1_freqs[] = {440, 550};
#endif

#if FILTERS == 2
	int v1_freqs[] = {440};
	//int v1_freqs[] = {880};
#endif

#endif /* V1_FREQS_H_ */
