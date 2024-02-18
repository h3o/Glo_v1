/*
 * v1_gpio.c
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Jun 21, 2016
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

#include "gpio.h"

bool RedLED_state = false;
bool OrangeLED_state = false;

int led_RED_state = 0, led_GREEN_state = 0;

//sequence counters
int leds_RED_seq = -1, leds_BLUE_seq = -1;

void LEDs_BLUE_next(int limit)
{
	leds_BLUE_seq++;
	if(leds_BLUE_seq==limit)
	{
		leds_BLUE_seq = 0;
	}

	if(leds_BLUE_seq==0)
	{
		LED_R8_0_ON;
	}
	else if(leds_BLUE_seq==1)
	{
		LED_R8_1_ON;
	}
	else if(leds_BLUE_seq==2)
	{
		LED_R8_2_ON;
	}
	else if(leds_BLUE_seq==3)
	{
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
		LED_R8_0_OFF;
	}
	else if(leds_BLUE_seq==1)
	{
		LED_R8_1_OFF;
	}
	else if(leds_BLUE_seq==2)
	{
		LED_R8_2_OFF;
	}
	else if(leds_BLUE_seq==3)
	{
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
		LED_O4_0_ON;
	}
	else if(leds_RED_seq==1)
	{
		LED_O4_1_ON;
	}
	else if(leds_RED_seq==2)
	{
		LED_O4_2_ON;
	}
	else if(leds_RED_seq==3)
	{
		LED_O4_3_ON;
	}
}

void LEDs_RED_off()
{
	if(leds_RED_seq==0)
	{
		LED_O4_0_OFF;
	}
	else if(leds_RED_seq==1)
	{
		LED_O4_1_OFF;
	}
	else if(leds_RED_seq==2)
	{
		LED_O4_2_OFF;
	}
	else if(leds_RED_seq==3)
	{
		LED_O4_3_OFF;
	}
}

void LEDs_RED_reset()
{
	LEDs_RED_off();
	leds_RED_seq = -1;
}
