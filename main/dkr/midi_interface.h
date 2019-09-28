/**
 ******************************************************************************
 * File Name          	: midi_interface.h
 * Author				: Xavier Halgand
 * Date               	:
 * Description        	:
 ******************************************************************************
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MIDI_INTERFACE_H
#define __MIDI_INTERFACE_H

/* Includes ------------------------------------------------------------------*/
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "CONSTANTS.h"
#include "random.h"
#include "drifter.h"
#include "sinetable.h"
#include "notesTables.h"
#include "drifter.h"
#include "soundGen.h"

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported functions ------------------------------------------------------- */
//void MIDI_Decode(uint8_t * outBuf);
void MagicFX(uint8_t val);
void MagicPatch(uint8_t val);

#ifdef __cplusplus
}
#endif

/****************************************************************************************************************************/
#endif /* __MIDI_INTERFACE_H */
