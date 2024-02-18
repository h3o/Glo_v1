/**
 ******************************************************************************
 * File Name          : random.c
 * Author			  : Xavier Halgand
 * Date               :
 * Description        :
 ******************************************************************************
 */

#include "random.h"
#include <hw/signals.h>


//---------------------------------------------------------------------------
//void randomGen_init(void)
//{
//	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
//	RNG_Cmd(ENABLE);
//	while (RNG_GetFlagStatus(RNG_FLAG_DRDY) == RESET)
		//;
//	srand(RNG_GetRandomNumber());
//}
//---------------------------------------------------------------------------
/**************
 * returns a random float between a and b
 *****************/
float_t frand_a_b(float_t a, float_t b)
{
	//return ( rand()/(float_t)RAND_MAX ) * (b-a) + a ;

	//PseudoRNG1a_next_float() returns random float between -0.5f and +0.5f
	//return (PseudoRNG1a_next_float_new()+0.5f) * (b-a) + a ;
	//new_random_value();
	//return ((float_t)random_value/(float_t)UINT_MAX) * (b-a) + a;
	return PseudoRNG1a_next_float_new01() * (b-a) + a ;
}


//---------------------------------------------------------------------------
/**************
 * returns a random float between 0 and 1
 *****************/
float_t randomNum(void)
{
	//float_t random = 1.0f;
//	while (RNG_GetFlagStatus(RNG_FLAG_DRDY) == RESET)
//		;
	//random = (float_t) (RNG_GetRandomNumber() / 4294967294.0f);
	//return random;

	//PseudoRNG1a_next_float() returns random float between -0.5f and +0.5f
	return PseudoRNG1a_next_float_new()+0.5;
}

/*-----------------------------------------------------------------------------*/
/**************
 * returns a random integer between 0 and MIDI_MAX
 *****************/
uint8_t MIDIrandVal(void)
{
	return (uint8_t)lrintf(frand_a_b(0 , MIDI_MAX));
}
