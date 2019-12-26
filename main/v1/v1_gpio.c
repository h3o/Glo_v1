/*
 * gpio.c
 *
 *  Created on: Jun 21, 2016
 *      Author: mayo
 */

#include "gpio.h"

bool RedLED_state = false;
bool OrangeLED_state = false;

int led_RED_state = 0, led_GREEN_state = 0;

//cycle-through sequence counter
int leds_RED_seq = -1, leds_BLUE_seq = -1;

//__IO uint32_t sys_timer = 0;
//__IO uint32_t sys_clock = 0;


void LEDs_BLUE_next(int limit)
{
	leds_BLUE_seq++;
	if(leds_BLUE_seq==limit)
	{
		leds_BLUE_seq = 0;
	}

	if(leds_BLUE_seq==0)
	{
		//LED_SB1_ON;
		//LED_B5_1_ON;
		LED_R8_0_ON;
	}
	else if(leds_BLUE_seq==1)
	{
		//LED_SB2_ON;
		//LED_B5_2_ON;
		LED_R8_1_ON;
	}
	else if(leds_BLUE_seq==2)
	{
		//LED_SB3_ON;
		//LED_B5_3_ON;
		LED_R8_2_ON;
	}
	else if(leds_BLUE_seq==3)
	{
		//LED_SB4_ON;
		//LED_B5_4_ON;
		LED_R8_3_ON;
	}
	else if(leds_BLUE_seq==4)
	{
		LED_R8_4_ON;
	}
	else if(leds_BLUE_seq==5)
	{
		LED_R8_5_ON;
	}
	else if(leds_BLUE_seq==6)
	{
		LED_R8_6_ON;
	}
	else if(leds_BLUE_seq==7)
	{
		LED_R8_7_ON;
	}
}

void LEDs_BLUE_off()
{
	if(leds_BLUE_seq==0)
	{
		//LED_SB1_OFF;
		//LED_B5_1_OFF;
		LED_R8_0_OFF;
	}
	else if(leds_BLUE_seq==1)
	{
		//LED_SB2_OFF;
		//LED_B5_2_OFF;
		LED_R8_1_OFF;
	}
	else if(leds_BLUE_seq==2)
	{
		//LED_SB3_OFF;
		//LED_B5_3_OFF;
		LED_R8_2_OFF;
	}
	else if(leds_BLUE_seq==3)
	{
		//LED_SB4_OFF;
		//LED_B5_4_OFF;
		LED_R8_3_OFF;
	}
	else if(leds_BLUE_seq==4)
	{
		LED_R8_4_OFF;
	}
	else if(leds_BLUE_seq==5)
	{
		LED_R8_5_OFF;
	}
	else if(leds_BLUE_seq==6)
	{
		LED_R8_6_OFF;
	}
	else if(leds_BLUE_seq==7)
	{
		LED_R8_7_OFF;
	}
}

void LEDs_RED_next(int limit)
{
	leds_RED_seq++;
	if(leds_RED_seq==limit)
	{
		leds_RED_seq = 0;
	}

	if(leds_RED_seq==0)
	{
		//LED_SR1_ON;
		LED_O4_0_ON;
		//Delay(100);
	}
	else if(leds_RED_seq==1)
	{
		//LED_SR2_ON;
		LED_O4_1_ON;
		//Delay(100);
	}
	else if(leds_RED_seq==2)
	{
		//LED_SR3_ON;
		LED_O4_2_ON;
		//Delay(100);
	}
	else if(leds_RED_seq==3)
	{
		//LED_SR4_ON;
		LED_O4_3_ON;
		//Delay(100);
	}
}

void LEDs_RED_off()
{
	if(leds_RED_seq==0)
	{
		//LED_SR1_OFF;
		LED_O4_0_OFF;
	}
	else if(leds_RED_seq==1)
	{
		//LED_SR2_OFF;
		LED_O4_1_OFF;
	}
	else if(leds_RED_seq==2)
	{
		//LED_SR3_OFF;
		LED_O4_2_OFF;
	}
	else if(leds_RED_seq==3)
	{
		//LED_SR4_OFF;
		LED_O4_3_OFF;
	}
}

void LEDs_RED_reset()
{
	LEDs_RED_off();
	leds_RED_seq=-1;
}

