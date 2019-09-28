/*
 * freqs.h
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
	//int freqs[] = {440, 660, 880, 550, 220, 1760, 1100, 2200};//, 520}; //2640
	#if ACCORD == MAJOR
		int freqs[] = {440, 660, 880, 550, 220, 1760, 1100, 110};//, 520}; //2640
	#elif ACCORD == MINOR
		int freqs[] = {440, 660, 880, 520, 220, 1760, 1040, 110};//, 520}; //2640
	#endif
#endif

#if FILTERS == 14
	int freqs[] = {110, 220, 440, 520, 660, 880, 1040};//, 1760};//, 520};
#endif

#if FILTERS == 12
	#if ACCORD == MAJOR
		////int freqs[FILTERS/2] = {440, 660, 880, 550, 220};//, 520};
		int freqs[] = {440, 550, 660, 880, 1100, 2200};
	#elif ACCORD == MINOR
		int freqs[] = {440, 660, 880, 520, 220, 1760};//, 1040};//, 520};
	#endif
#endif

#if FILTERS == 10
	#if ACCORD == MAJOR
		int freqs[] = {440, 550, 660, 880, 1100};
	#elif ACCORD == MINOR
		//int freqs[] = {440, 660, 880, 520, 220};
		int freqs[] = {440, 660, 110, 520, 220};
	#endif
#endif

#if FILTERS == 8
	#if ACCORD == MAJOR
		int freqs[] = {440, 550, 660, 880};
	#elif ACCORD == MINOR
		//int freqs[] = {440, 660, 880, 520, 220};
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
	//int freqs[] = {440, 1760};
	int freqs[] = {440, 550};
#endif

#if FILTERS == 2
	int freqs[] = {440};
	//int freqs[] = {880};
#endif

//Others----------------------------------------------------------

//Formantic Synthesis - VOWEL SOUND AS IN... F1 F2 F3
//source: http://www.soundonsound.com/sos/mar01/articles/synthsec.asp
/*
#if FORMANTS == 'F_EE' //"ee" leap
	int freqs[] = {270, 2300, 3000};
#endif


#if FORMANTS == 'F_OO' //"oo" loop
	int freqs[] = {300, 870, 2250};
#endif

#if FORMANTS == 'F_I' //"i" lip
	int freqs[] = {400, 2000, 2550};
#endif

#if FORMANTS == 'F_E' //"e" let
	int freqs[] = {530, 1850, 2500};
#endif

#if FORMANTS == 'F_U' //"u" lug
	int freqs[] = {640, 1200, 2400};
#endif

#if FORMANTS == 'F_A' //"a" lap
	int freqs[] = {660, 1700, 2400};
#endif
*/

#endif /* FREQS_H_ */
