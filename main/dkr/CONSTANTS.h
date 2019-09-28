/**
 ******************************************************************************
 * File Name          : CONSTANTS.h
 * Author			  : Xavier Halgand
 * Date               :
 * Description        :
 ******************************************************************************
 */
#ifndef __DKR_CONSTANTS_H__
#define __DKR_CONSTANTS_H__

#include <hw/init.h>
#include <hw/codec.h>

/*--------------------------------------------------------------------------------------*/

//#define SAMPLERATE              48000
//#define SAMPLERATE              44100
//#define SAMPLERATE              32000
//#define SAMPLERATE              22050
#define SAMPLERATE				  SAMPLE_RATE_LOW //use the sample rate as defined in codec.h

#define Fs             			((float)(SAMPLERATE))	// samplerate
#define Ts						(1.f/Fs)  // sample period
#define _2PI                    6.283185307f
#define _PI                    	3.14159265f

/*
#define BUFF_LEN_DIV4           400 // number of samples in the audiobuffer for each channel <==> XX ms latency at 48kHz
#define BUFF_LEN_DIV2           (2*BUFF_LEN_DIV4)
#define BUFF_LEN                (4*BUFF_LEN_DIV4)  // Audio buffer length : count in 16bits half-words
#define VOL                     70 // initial output DAC volume
#define MAXVOL                  100 // maximal output DAC volume

#define SYSTICK_FREQ			500 // system tick interruption frequency in Hz
#define DEBOUNCE_TIME			500
#define PARAM_MAX				21 // maximal parameter index, starting at 0
*/

#define MIDI_MAX				127.f 	// floating max value
#define MIDI_MAXi				127		// integer max value

#define _CCM_					//__attribute__((section(".ccm"))) //

//#if defined   (__GNUC__)        /* GNU Compiler */
//    #define __ALIGN    __attribute__ ((aligned (4)))
//#endif /* __GNUC__ */

/************************************************************************************/
#endif  /*__DKR_CONSTANTS_H__ */
