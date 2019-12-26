/*
 * leds.c
 *
 *  Created on: Jun 21, 2016
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

#include "leds.h"
#include "gpio.h"
#include "ui.h"
#include "midi.h"
#include <Binaural.h>

#ifdef BOARD_GECHO

int sensor_changed[4] = {0,0,0,0};
int sensor_active[4] = {0,0,0,0};

int sequencer_LEDs_timing_seq = 0;
uint8_t binaural_LEDs_timing_seq = 0;

void LED_sequencer_indicators() //int current_chord, int total_chords)
{
	if(settings_indication_in_progress)
	{

	}
	//else if(menu_indicator_active)
	//{

	//}
	else if(context_menu_active)
	{
		//if(context_menu_active==(CONTEXT_MENU_TEMPO/2+CONTEXT_MENU_TEMPO%2))
		if(last_settings_level==CONTEXT_MENU_TEMPO)
		{
			LED_R8_set(3, sequencer_LEDs_timing_seq%2==0);
			LED_R8_set(4, sequencer_LEDs_timing_seq%2==0);
		}
	}
	else
	{
		if(!sensor_active[0])
		{
			LED_R8_set((sequencer_LEDs_timing_seq/8)%8, sequencer_LEDs_timing_seq%2==0);
		}
		if(!sensor_active[1])
		{
			LED_O4_set((sequencer_LEDs_timing_seq/2)%4, sequencer_LEDs_timing_seq%2==0);
		}
	}

	sequencer_LEDs_timing_seq++;

	if(sequencer_LEDs_timing_seq>=64)
	{
		sequencer_LEDs_timing_seq=0;
	}
}

void display_chord(int8_t *chord_LEDs, int total_leds)
{
	/*
	if(mode_set_flag)
	{
		return;
	}
	*/

	int disp_led;

	if(!sensor_active[2])
	{
		LED_B5_all_OFF();
	}
	if(!sensor_active[3])
	{
		LED_W8_all_OFF();
	}

	for(int led=0; led<total_leds; led++)
	{
		disp_led = chord_LEDs[led];
		//printf("display_chord(): led #%d = %d\n", led, disp_led);

		if(disp_led < 10 && !sensor_active[3])
		{
			if(disp_led == 0)
			{
				LED_W8_0_ON;
			}
			else if(disp_led == 1)
			{
				LED_W8_1_ON;
			}
			else if(disp_led == 2)
			{
				LED_W8_2_ON;
			}
			else if(disp_led == 3)
			{
				LED_W8_3_ON;
			}
			else if(disp_led == 4)
			{
				LED_W8_4_ON;
			}
			else if(disp_led == 5)
			{
				LED_W8_5_ON;
			}
			else if(disp_led == 6)
			{
				LED_W8_6_ON;
			}
			else if(disp_led == 7)
			{
				LED_W8_7_ON;
			}
		}
		else if(!sensor_active[2])
		{
			if(disp_led == 10)
			{
				LED_B5_0_ON;
			}
			else if(disp_led == 11)
			{
				LED_B5_1_ON;
			}
			else if(disp_led == 12)
			{
				LED_B5_2_ON;
			}
			else if(disp_led == 13)
			{
				LED_B5_3_ON;
			}
			else if(disp_led == 14)
			{
				LED_B5_4_ON;
			}
		}
	}
}

void LED_W8_all_ON()
{
	LED_W8_0_ON;
	LED_W8_1_ON;
	LED_W8_2_ON;
	LED_W8_3_ON;
	LED_W8_4_ON;
	LED_W8_5_ON;
	LED_W8_6_ON;
	LED_W8_7_ON;
}

void LED_W8_all_OFF()
{
	LED_W8_0_OFF;
	LED_W8_1_OFF;
	LED_W8_2_OFF;
	LED_W8_3_OFF;
	LED_W8_4_OFF;
	LED_W8_5_OFF;
	LED_W8_6_OFF;
	LED_W8_7_OFF;
}

void LED_B5_all_OFF()
{
	LED_B5_0_OFF;
	LED_B5_1_OFF;
	LED_B5_2_OFF;
	LED_B5_3_OFF;
	LED_B5_4_OFF;
}

void LED_O4_all_OFF()
{
	LED_O4_0_OFF;
	LED_O4_1_OFF;
	LED_O4_2_OFF;
	LED_O4_3_OFF;
}

void LED_R8_all_OFF()
{
	LED_R8_0_OFF;
	LED_R8_1_OFF;
	LED_R8_2_OFF;
	LED_R8_3_OFF;
	LED_R8_4_OFF;
	LED_R8_5_OFF;
	LED_R8_6_OFF;
	LED_R8_7_OFF;
}

void LEDs_all_OFF()
{
	LED_W8_all_OFF();
	LED_B5_all_OFF();
	LED_O4_all_OFF();
	LED_R8_all_OFF();
}



void LED_R8_set(int led, int status)
{
	if(led==0)
	{
		status ? (LED_R8_0_ON) : (LED_R8_0_OFF);
	}
	if(led==1)
	{
		status ? (LED_R8_1_ON) : (LED_R8_1_OFF);
	}
	if(led==2)
	{
		status ? (LED_R8_2_ON) : (LED_R8_2_OFF);
	}
	if(led==3)
	{
		status ? (LED_R8_3_ON) : (LED_R8_3_OFF);
	}
	if(led==4)
	{
		status ? (LED_R8_4_ON) : (LED_R8_4_OFF);
	}
	if(led==5)
	{
		status ? (LED_R8_5_ON) : (LED_R8_5_OFF);
	}
	if(led==6)
	{
		status ? (LED_R8_6_ON) : (LED_R8_6_OFF);
	}
	if(led==7)
	{
		status ? (LED_R8_7_ON) : (LED_R8_7_OFF);
	}
}

void LED_O4_set(int led, int status)
{
	if(led==0)
	{
		status ? (LED_O4_0_ON) : (LED_O4_0_OFF);
	}
	if(led==1)
	{
		status ? (LED_O4_1_ON) : (LED_O4_1_OFF);
	}
	if(led==2)
	{
		status ? (LED_O4_2_ON) : (LED_O4_2_OFF);
	}
	if(led==3)
	{
		status ? (LED_O4_3_ON) : (LED_O4_3_OFF);
	}
}

void LED_R8_set_byte(int value)
{
	(value&0x01) ? (LED_R8_0_ON) : (LED_R8_0_OFF);
	(value&0x02) ? (LED_R8_1_ON) : (LED_R8_1_OFF);
	(value&0x04) ? (LED_R8_2_ON) : (LED_R8_2_OFF);
	(value&0x08) ? (LED_R8_3_ON) : (LED_R8_3_OFF);
	(value&0x10) ? (LED_R8_4_ON) : (LED_R8_4_OFF);
	(value&0x20) ? (LED_R8_5_ON) : (LED_R8_5_OFF);
	(value&0x40) ? (LED_R8_6_ON) : (LED_R8_6_OFF);
	(value&0x80) ? (LED_R8_7_ON) : (LED_R8_7_OFF);
}

void LED_O4_set_byte(int value)
{
	(value&0x01) ? (LED_O4_0_ON) : (LED_O4_0_OFF);
	(value&0x02) ? (LED_O4_1_ON) : (LED_O4_1_OFF);
	(value&0x04) ? (LED_O4_2_ON) : (LED_O4_2_OFF);
	(value&0x08) ? (LED_O4_3_ON) : (LED_O4_3_OFF);
}

void LED_B5_set_byte(int value)
{
	(value&0x01) ? (LED_B5_0_ON) : (LED_B5_0_OFF);
	(value&0x02) ? (LED_B5_1_ON) : (LED_B5_1_OFF);
	(value&0x04) ? (LED_B5_2_ON) : (LED_B5_2_OFF);
	(value&0x08) ? (LED_B5_3_ON) : (LED_B5_3_OFF);
	(value&0x10) ? (LED_B5_4_ON) : (LED_B5_4_OFF);
}

void LED_W8_set_byte(int value)
{
	(value&0x01) ? (LED_W8_0_ON) : (LED_W8_0_OFF);
	(value&0x02) ? (LED_W8_1_ON) : (LED_W8_1_OFF);
	(value&0x04) ? (LED_W8_2_ON) : (LED_W8_2_OFF);
	(value&0x08) ? (LED_W8_3_ON) : (LED_W8_3_OFF);
	(value&0x10) ? (LED_W8_4_ON) : (LED_W8_4_OFF);
	(value&0x20) ? (LED_W8_5_ON) : (LED_W8_5_OFF);
	(value&0x40) ? (LED_W8_6_ON) : (LED_W8_6_OFF);
	(value&0x80) ? (LED_W8_7_ON) : (LED_W8_7_OFF);
}

void LED_R8_set_byte_RL(int value)
{
	(value&0x01) ? (LED_R8_7_ON) : (LED_R8_7_OFF);
	(value&0x02) ? (LED_R8_6_ON) : (LED_R8_6_OFF);
	(value&0x04) ? (LED_R8_5_ON) : (LED_R8_5_OFF);
	(value&0x08) ? (LED_R8_4_ON) : (LED_R8_4_OFF);
	(value&0x10) ? (LED_R8_3_ON) : (LED_R8_3_OFF);
	(value&0x20) ? (LED_R8_2_ON) : (LED_R8_2_OFF);
	(value&0x40) ? (LED_R8_1_ON) : (LED_R8_1_OFF);
	(value&0x80) ? (LED_R8_0_ON) : (LED_R8_0_OFF);
}

void LED_O4_set_byte_RL(int value)
{
	(value&0x01) ? (LED_O4_3_ON) : (LED_O4_3_OFF);
	(value&0x02) ? (LED_O4_2_ON) : (LED_O4_2_OFF);
	(value&0x04) ? (LED_O4_1_ON) : (LED_O4_1_OFF);
	(value&0x08) ? (LED_O4_0_ON) : (LED_O4_0_OFF);
}

void LED_B5_set_byte_RL(int value)
{
	(value&0x01) ? (LED_B5_4_ON) : (LED_B5_4_OFF);
	(value&0x02) ? (LED_B5_3_ON) : (LED_B5_3_OFF);
	(value&0x04) ? (LED_B5_2_ON) : (LED_B5_2_OFF);
	(value&0x08) ? (LED_B5_1_ON) : (LED_B5_1_OFF);
	(value&0x10) ? (LED_B5_0_ON) : (LED_B5_0_OFF);
}

void LED_W8_set_byte_RL(int value)
{
	(value&0x01) ? (LED_W8_7_ON) : (LED_W8_7_OFF);
	(value&0x02) ? (LED_W8_6_ON) : (LED_W8_6_OFF);
	(value&0x04) ? (LED_W8_5_ON) : (LED_W8_5_OFF);
	(value&0x08) ? (LED_W8_4_ON) : (LED_W8_4_OFF);
	(value&0x10) ? (LED_W8_3_ON) : (LED_W8_3_OFF);
	(value&0x20) ? (LED_W8_2_ON) : (LED_W8_2_OFF);
	(value&0x40) ? (LED_W8_1_ON) : (LED_W8_1_OFF);
	(value&0x80) ? (LED_W8_0_ON) : (LED_W8_0_OFF);
}

//test all LEDs
void all_LEDs_test() // todo: combine w noise
{
	int led_test_counter = 0;
	channel_running = 1;

	//while(led_test_counter < 700000) {
	while(!event_next_channel)
	{
		led_test_counter++;

		if(led_test_counter%1000000==0){
			LED_R8_0_OFF;
			LED_W8_0_ON;
		}
		if(led_test_counter%1000000==500000){
			LED_R8_0_ON;
			LED_W8_0_OFF;
		}
		if(led_test_counter%2200000==0){
			LED_R8_1_OFF;
			LED_W8_1_ON;
		}
		if(led_test_counter%2200000==900000){
			LED_R8_1_ON;
			LED_W8_1_OFF;
		}
		if(led_test_counter%300000==0){
			LED_R8_2_OFF;
			LED_W8_2_ON;
		}
		if(led_test_counter%300000==70000){
			LED_R8_2_ON;
			LED_W8_2_OFF;
		}
		if(led_test_counter%400000==0){
			LED_R8_3_OFF;
			LED_W8_3_ON;
		}
		if(led_test_counter%400000==90000){
			LED_R8_3_ON;
			LED_W8_3_OFF;
		}
		if(led_test_counter%500000==0){
			LED_R8_4_OFF;
			LED_W8_4_ON;
		}
		if(led_test_counter%500000==150000){
			LED_R8_4_ON;
			LED_W8_4_OFF;
		}
		if(led_test_counter%600000==0){
			LED_R8_5_OFF;
			LED_W8_5_ON;
		}
		if(led_test_counter%600000==200000){
			LED_R8_5_ON;
			LED_W8_5_OFF;
		}
		if(led_test_counter%700000==0){
			LED_R8_6_OFF;
			LED_W8_6_ON;
		}
		if(led_test_counter%700000==300000){
			LED_R8_6_ON;
			LED_W8_6_OFF;
		}
		if(led_test_counter%800000==0){
			LED_R8_7_OFF;
			LED_W8_7_ON;
		}
		if(led_test_counter%800000==400000){
			LED_R8_7_ON;
			LED_W8_7_OFF;
		}

		if(led_test_counter%1000000==0){
			LED_O4_3_OFF;
			LED_O4_0_ON;
		}
		if(led_test_counter%1000000==250000){
			LED_O4_0_OFF;
			LED_O4_1_ON;
		}
		if(led_test_counter%1000000==500000){
			LED_O4_1_OFF;
			LED_O4_2_ON;
		}
		if(led_test_counter%1000000==750000){
			LED_O4_2_OFF;
			LED_O4_3_ON;
		}

		if(led_test_counter%100000==0){
			LED_SIG_ON;
		}
		if(led_test_counter%100000==50000){
			LED_SIG_OFF;
		}

		if(led_test_counter%40000==0){
			LED_RDY_ON;
		}
		if(led_test_counter%40000==20000){
			LED_RDY_OFF;
		}

		if(led_test_counter%1000000==0){
			LED_B5_4_OFF;
			LED_B5_0_ON;
		}
		if(led_test_counter%1000000==200000){
			LED_B5_0_OFF;
			LED_B5_1_ON;
		}
		if(led_test_counter%1000000==400000){
			LED_B5_1_OFF;
			LED_B5_2_ON;
		}
		if(led_test_counter%1000000==600000){
			LED_B5_2_OFF;
			LED_B5_3_ON;
		}
		if(led_test_counter%1000000==800000){
			LED_B5_3_OFF;
			LED_B5_4_ON;
		}

		//check_for_reset();
	}
	channel_running = 0;
}

//#define LED_TEST_DELAY 250

void all_LEDs_test_seq1()
{
	//reset all GPIOs in case some shared ones may be allocated e.g. for USART
//	#ifdef CAN_BLOCK_SWD_DEBUG
//		GPIO_Init_all(true);
//	#else
//		GPIO_Init_all(false);
//	#endif

	LED_R8_all_OFF();
	LED_O4_all_OFF();
	LED_B5_all_OFF();
	LED_W8_all_OFF();

	channel_running = 1;

	while(!event_next_channel)
	{
		LED_RDY_ON;

		//row 1
		LED_R8_0_ON;
		Delay(LED_TEST_DELAY);
		LED_R8_0_OFF;

		LED_R8_1_ON;
		Delay(LED_TEST_DELAY);
		LED_R8_1_OFF;

		LED_R8_2_ON;
		Delay(LED_TEST_DELAY);
		LED_R8_2_OFF;

		LED_R8_3_ON;
		Delay(LED_TEST_DELAY);
		LED_R8_3_OFF;

		LED_R8_4_ON;
		Delay(LED_TEST_DELAY);
		LED_R8_4_OFF;

		LED_R8_5_ON;
		Delay(LED_TEST_DELAY);
		LED_R8_5_OFF;

		LED_R8_6_ON;
		Delay(LED_TEST_DELAY);
		LED_R8_6_OFF;

		LED_R8_7_ON;
		Delay(LED_TEST_DELAY);
		LED_R8_7_OFF;

		LED_RDY_OFF;
		LED_SIG_ON;

		//row 2
		LED_O4_0_ON;
		Delay(LED_TEST_DELAY);
		LED_O4_0_OFF;

		LED_O4_1_ON;
		Delay(LED_TEST_DELAY);
		LED_O4_1_OFF;

		LED_O4_2_ON;
		Delay(LED_TEST_DELAY);
		LED_O4_2_OFF;

		LED_O4_3_ON;
		Delay(LED_TEST_DELAY);
		LED_O4_3_OFF;

		LED_SIG_OFF;
		LED_RDY_ON;

		//row 3
		LED_B5_0_ON;
		Delay(LED_TEST_DELAY);
		LED_B5_0_OFF;

		LED_B5_1_ON;
		Delay(LED_TEST_DELAY);
		LED_B5_1_OFF;

		LED_B5_2_ON;
		Delay(LED_TEST_DELAY);
		LED_B5_2_OFF;

		LED_B5_3_ON;
		Delay(LED_TEST_DELAY);
		LED_B5_3_OFF;

		LED_B5_4_ON;
		Delay(LED_TEST_DELAY);
		LED_B5_4_OFF;

		LED_RDY_OFF;
		LED_SIG_ON;

		//row 4
		LED_W8_0_ON;
		Delay(LED_TEST_DELAY);
		LED_W8_0_OFF;

		LED_W8_1_ON;
		Delay(LED_TEST_DELAY);
		LED_W8_1_OFF;

		LED_W8_2_ON;
		Delay(LED_TEST_DELAY);
		LED_W8_2_OFF;

		LED_W8_3_ON;
		Delay(LED_TEST_DELAY);
		LED_W8_3_OFF;

		LED_W8_4_ON;
		Delay(LED_TEST_DELAY);
		LED_W8_4_OFF;

		LED_W8_5_ON;
		Delay(LED_TEST_DELAY);
		LED_W8_5_OFF;

		LED_W8_6_ON;
		Delay(LED_TEST_DELAY);
		LED_W8_6_OFF;

		LED_W8_7_ON;
		Delay(LED_TEST_DELAY);
		LED_W8_7_OFF;

		LED_SIG_OFF;
	}
	channel_running = 0;
}

void all_LEDs_test_seq2()
{
	//reset all GPIOs in case some shared ones may be allocated e.g. for USART
//	#ifdef CAN_BLOCK_SWD_DEBUG
//		GPIO_Init_all(true);
//	#else
//		GPIO_Init_all(false);
//	#endif

	LED_R8_all_OFF();
	LED_O4_all_OFF();
	LED_B5_all_OFF();
	LED_W8_all_OFF();

	channel_running = 1;

	while(!event_next_channel)
	{
		//row 1
		LED_R8_0_OFF;
		Delay(LED_TEST_DELAY);
		LED_R8_0_ON;

		LED_R8_1_OFF;
		Delay(LED_TEST_DELAY);
		LED_R8_1_ON;

		LED_R8_2_OFF;
		Delay(LED_TEST_DELAY);
		LED_R8_2_ON;

		LED_R8_3_OFF;
		Delay(LED_TEST_DELAY);
		LED_R8_3_ON;

		LED_R8_4_OFF;
		Delay(LED_TEST_DELAY);
		LED_R8_4_ON;

		LED_R8_5_OFF;
		Delay(LED_TEST_DELAY);
		LED_R8_5_ON;

		LED_R8_6_OFF;
		Delay(LED_TEST_DELAY);
		LED_R8_6_ON;

		LED_R8_7_OFF;
		Delay(LED_TEST_DELAY);
		LED_R8_7_ON;

		//row 2
		LED_O4_0_OFF;
		Delay(LED_TEST_DELAY);
		LED_O4_0_ON;

		LED_O4_1_OFF;
		Delay(LED_TEST_DELAY);
		LED_O4_1_ON;

		LED_O4_2_OFF;
		Delay(LED_TEST_DELAY);
		LED_O4_2_ON;

		LED_O4_3_OFF;
		Delay(LED_TEST_DELAY);
		LED_O4_3_ON;

		//row 3
		LED_B5_0_OFF;
		Delay(LED_TEST_DELAY);
		LED_B5_0_ON;

		LED_B5_1_OFF;
		Delay(LED_TEST_DELAY);
		LED_B5_1_ON;

		LED_B5_2_OFF;
		Delay(LED_TEST_DELAY);
		LED_B5_2_ON;

		LED_B5_3_OFF;
		Delay(LED_TEST_DELAY);
		LED_B5_3_ON;

		LED_B5_4_OFF;
		Delay(LED_TEST_DELAY);
		LED_B5_4_ON;

		//row 4
		LED_W8_0_OFF;
		Delay(LED_TEST_DELAY);
		LED_W8_0_ON;

		LED_W8_1_OFF;
		Delay(LED_TEST_DELAY);
		LED_W8_1_ON;

		LED_W8_2_OFF;
		Delay(LED_TEST_DELAY);
		LED_W8_2_ON;

		LED_W8_3_OFF;
		Delay(LED_TEST_DELAY);
		LED_W8_3_ON;

		LED_W8_4_OFF;
		Delay(LED_TEST_DELAY);
		LED_W8_4_ON;

		LED_W8_5_OFF;
		Delay(LED_TEST_DELAY);
		LED_W8_5_ON;

		LED_W8_6_OFF;
		Delay(LED_TEST_DELAY);
		LED_W8_6_ON;

		LED_W8_7_OFF;
		Delay(LED_TEST_DELAY);
		LED_W8_7_ON;
	}
	channel_running = 0;
}

void KEY_LED_on_off(int note, int on_off)
{
	if(note==1)
	{
		on_off?KEY_C_ON:KEY_C_OFF;
	}
	else if(note==2)
	{
		on_off?KEY_Cis_ON:KEY_Cis_OFF;
	}
	else if(note==3)
	{
		on_off?KEY_D_ON:KEY_D_OFF;
	}
	else if(note==4)
	{
		on_off?KEY_Dis_ON:KEY_Dis_OFF;
	}
	else if(note==5)
	{
		on_off?KEY_E_ON:KEY_E_OFF;
	}
	else if(note==6)
	{
		on_off?KEY_F_ON:KEY_F_OFF;
	}
	else if(note==7)
	{
		on_off?KEY_Fis_ON:KEY_Fis_OFF;
	}
	else if(note==8)
	{
		on_off?KEY_G_ON:KEY_G_OFF;
	}
	else if(note==9)
	{
		on_off?KEY_Gis_ON:KEY_Gis_OFF;
	}
	else if(note==10)
	{
		on_off?KEY_A_ON:KEY_A_OFF;
	}
	else if(note==11)
	{
		on_off?KEY_Ais_ON:KEY_Ais_OFF;
	}
	else if(note==12)
	{
		on_off?KEY_H_ON:KEY_H_OFF;
	}
	else if(note==13)
	{
		on_off?KEY_C2_ON:KEY_C2_OFF;
	}
}

void KEY_LED_all_off()
{
	LED_W8_all_OFF();
	LED_B5_all_OFF();
}

void display_number_of_chords(int row1, int row2)
{
	LED_R8_0_ON;
	if(row1>1)
	{
		LED_R8_1_ON;
	}
	else
	{
		LED_R8_1_OFF;
	}
	if(row1>2)
	{
		LED_R8_2_ON;
	}
	else
	{
		LED_R8_2_OFF;
	}
	if(row1>3)
	{
		LED_R8_3_ON;
	}
	else
	{
		LED_R8_3_OFF;
	}
	if(row1>4)
	{
		LED_R8_4_ON;
	}
	else
	{
		LED_R8_4_OFF;
	}
	if(row1>5)
	{
		LED_R8_5_ON;
	}
	else
	{
		LED_R8_5_OFF;
	}
	if(row1>6)
	{
		LED_R8_6_ON;
	}
	else
	{
		LED_R8_6_OFF;
	}
	if(row1>7)
	{
		LED_R8_7_ON;
	}
	else
	{
		LED_R8_7_OFF;
	}

	LED_O4_0_ON;
	if(row2>1)
	{
		LED_O4_1_ON;
	}
	else
	{
		LED_O4_1_OFF;
	}
	if(row2>2)
	{
		LED_O4_2_ON;
	}
	else
	{
		LED_O4_2_OFF;
	}
	if(row2>3)
	{
		LED_O4_3_ON;
	}
	else
	{
		LED_O4_3_OFF;
	}
}

void ack_by_signal_LED()
{
	for(int i=0;i<20;i++)
	{
		LED_SIG_ON;
		Delay(20);
		LED_SIG_OFF;
		Delay(20);
	}
}

void display_code_challenge(uint32_t *code)
{
	/*
	#ifdef GECHO_V2
	GPIOB->BSRRH = 0x00ef;				//all RED LEDs off (GPIOB 0..7) except 4 (5th LED)
	GPIOB->BSRRL = code[0];				//some RED LEDs on
	GPIOA->BSRRH = 0x0080;				//all RED LEDs off (GPIOB 0..7) 5th LED is on PA7
	GPIOA->BSRRL = (code[0]<<3) & 0x0080;	//some RED LEDs on
	#else
	GPIOB->BSRRH = 0x00ff;				//all RED LEDs off (GPIOB 0..7)
	GPIOB->BSRRL = code[0];				//some RED LEDs on
	#endif

	GPIOB->BSRRH = 0x3c00;				//all ORANGE LEDs off (GPIOB 10..13)
	GPIOB->BSRRL = code[1] & 0x3c00; 	//some ORANGE LEDs on

	GPIOC->BSRRL = 0x0370;				//all BLUE LEDs off (GPIOC 4,5,6,8,9)
	GPIOC->BSRRH = code[2] & 0x0370; 	//some BLUE LEDs on

	GPIOA->BSRRL = 0xff00;				//all WHITE LEDs off (GPIOA 8..15)
	GPIOA->BSRRH = code[3] & 0xff00; 	//some WHITE LEDs on
	*/
}

void display_volume_level_indicator_f(float value, float min, float max)
{
	int level = 8 - (value-min) / (max-min) * 8;
	LED_R8_set_byte(0x00ff>>level);
}

void display_volume_level_indicator_i(int value, int min, int max)
{
	if(min<0)
	{
		value += -min;
		max += -min;
		min = 0;
	}

	int level = 8 - 8*(value-min) / (max-min);
	LED_R8_set_byte(0x00ff>>level);
}

void display_tempo_indicator(int bpm)
{
	/*
	int x = (bpm - TEMPO_BPM_MIN) / TEMPO_BPM_STEP;

	//range & indicator v1:
	//LED_R8_set_byte(0x01<<(x/7));
	//LED_O4_set_byte(((0x1e<<(x%7))&0xf0)>>4);

	//range & indicator v2:
	LED_R8_set_byte(0x01<<(x/8) | 0x01<<((x+4)/8));
	LED_O4_set_byte(0x01<<(x%4));
	*/

	//range & indicator v3: display normal numbers
	LED_R8_set_byte((0xff00 & (0x01ff<<((bpm / 100) % 10 - 1))) >> 8);
	LED_O4_set_byte((0xff00 & (0x01ff<<((bpm / 10) % 10 - 1))) >> 8);
	LED_B5_set_byte((0xff00 & (0x01ff<<((bpm / 10) % 10 - 5))) >> 8);
	LED_W8_set_byte((0xff00 & (0x01ff<<(bpm % 10 - 1))) >> 8);
}

void display_BCD_numbers(char *digits, int length)
{
	LED_W8_all_OFF();
	LED_B5_all_OFF();
	LED_O4_all_OFF();
	LED_R8_all_OFF();

	if(length>0)LED_R8_set_byte_RL(digits[0] - '0');
	if(length>1)LED_O4_set_byte_RL(digits[1] - '0');
	if(length>2)LED_B5_set_byte_RL(digits[2] - '0');
	if(length>3)LED_W8_set_byte_RL(digits[3] - '0');
}

void display_IR_sensors_levels()
{
	if(context_menu_active && context_menu_active!=CONTEXT_MENU_ALLOW_LEDS)
	{
		//reset active flags to not interfere with chord indication
		//sensor_active[0] = 0;
		//sensor_active[1] = 0;
		sensor_active[2] = 0;
		sensor_active[3] = 0;
		return;
	}

	if((ir_res[0] > IR_sensors_THRESHOLD_1) || sensor_changed[0]) {
		LED_R8_all_OFF();
		sensor_changed[0] = 0;
	}
	if((ir_res[1] > IR_sensors_THRESHOLD_1) || sensor_changed[1]) {
		LED_O4_all_OFF();
		sensor_changed[1] = 0;
	}
	if((ir_res[2] > IR_sensors_THRESHOLD_1) || sensor_changed[2]) {
		LED_B5_all_OFF();
		sensor_changed[2] = 0;
	}

	if((ir_res[3] > IR_sensors_THRESHOLD_1) || sensor_changed[3]) {
		LED_W8_all_OFF();
		sensor_changed[3] = 0;
	}

	if(SENSOR_THRESHOLD_RED_8) {
		LED_R8_7_ON;
	}
	if(SENSOR_THRESHOLD_RED_7) {
		LED_R8_6_ON;
	}
	if(SENSOR_THRESHOLD_RED_6) {
		LED_R8_5_ON;
	}
	if(SENSOR_THRESHOLD_RED_5) {
		LED_R8_4_ON;
	}
	if(SENSOR_THRESHOLD_RED_4) {
		LED_R8_3_ON;
	}
	if(SENSOR_THRESHOLD_RED_3) {
		LED_R8_2_ON;
	}
	if(SENSOR_THRESHOLD_RED_2) {
		LED_R8_1_ON;
	}
	if(SENSOR_THRESHOLD_RED_1) {
		LED_R8_0_ON;
		sensor_changed[0] = 1;
		sensor_active[0] = 1;
	}
	else
	{
		if(sensor_active[0])
		{
			sensor_changed[0] = 1;
		}
		sensor_active[0] = 0;
	}

	if(SENSOR_THRESHOLD_ORANGE_4) {
		LED_O4_3_ON;
	}
	if(SENSOR_THRESHOLD_ORANGE_3) {
		LED_O4_2_ON;
	}
	if(SENSOR_THRESHOLD_ORANGE_2) {
		LED_O4_1_ON;
	}
	if(SENSOR_THRESHOLD_ORANGE_1) {
		LED_O4_0_ON;
		sensor_changed[1] = 1;
		sensor_active[1] = 1;
	}
	else
	{
		if(sensor_active[1])
		{
			sensor_changed[1] = 1;
		}
		sensor_active[1] = 0;
	}

	if(SENSOR_THRESHOLD_BLUE_1) {
		LED_B5_0_ON;
	}
	if(SENSOR_THRESHOLD_BLUE_2) {
		LED_B5_1_ON;
	}
	if(SENSOR_THRESHOLD_BLUE_3) {
		LED_B5_2_ON;
	}
	if(SENSOR_THRESHOLD_BLUE_4) {
		LED_B5_3_ON;
	}
	if(SENSOR_THRESHOLD_BLUE_5) {
		LED_B5_4_ON;
		sensor_changed[2] = 1;
		sensor_active[2] = 1;
	}
	else
	{
		if(sensor_active[2])
		{
			sensor_changed[2] = 1;
		}
		sensor_active[2] = 0;
	}

	if(SENSOR_THRESHOLD_WHITE_1) {
		LED_W8_0_ON;
	}
	if(SENSOR_THRESHOLD_WHITE_2) {
		LED_W8_1_ON;
	}
	if(SENSOR_THRESHOLD_WHITE_3) {
		LED_W8_2_ON;
	}
	if(SENSOR_THRESHOLD_WHITE_4) {
		LED_W8_3_ON;
	}
	if(SENSOR_THRESHOLD_WHITE_5) {
		LED_W8_4_ON;
	}
	if(SENSOR_THRESHOLD_WHITE_6) {
		LED_W8_5_ON;
	}
	if(SENSOR_THRESHOLD_WHITE_7) {
		LED_W8_6_ON;
	}
	if(SENSOR_THRESHOLD_WHITE_8) {
		LED_W8_7_ON;
		sensor_changed[3] = 1;
		sensor_active[3] = 1;
	}
	else
	{
		if(sensor_active[3])
		{
			sensor_changed[3] = 1;
		}
		sensor_active[3] = 0;
	}
}

void indicate_context_setting(uint16_t leds, int blink, int delay)
{
	for(int i=0;i<blink;i++)
	{
		if(leds<0x100)
		{
			LED_R8_set_byte(leds);
			Delay(delay);
			LED_R8_all_OFF();
			Delay(delay);
		}
		else if(leds==SETTINGS_INDICATOR_ANIMATE_LEFT_4 || leds==SETTINGS_INDICATOR_ANIMATE_LEFT_8)
		{
			if(leds==SETTINGS_INDICATOR_ANIMATE_LEFT_8)
			{
				LED_R8_set_byte(0x80);
				Delay(delay);
				LED_R8_set_byte(0x40);
				Delay(delay);
				LED_R8_set_byte(0x20);
				Delay(delay);
				LED_R8_set_byte(0x10);
				Delay(delay);
			}

			LED_R8_set_byte(0x08);
			Delay(delay);
			LED_R8_set_byte(0x04);
			Delay(delay);
			LED_R8_set_byte(0x02);
			Delay(delay);
			LED_R8_set_byte(0x01);
			Delay(delay);
			LED_R8_all_OFF();
		}
		else if(leds==SETTINGS_INDICATOR_ANIMATE_RIGHT_4 || leds==SETTINGS_INDICATOR_ANIMATE_RIGHT_8)
		{
			if(leds==SETTINGS_INDICATOR_ANIMATE_RIGHT_8)
			{
				LED_R8_set_byte(0x01);
				Delay(delay);
				LED_R8_set_byte(0x02);
				Delay(delay);
				LED_R8_set_byte(0x04);
				Delay(delay);
				LED_R8_set_byte(0x08);
				Delay(delay);
			}

			LED_R8_set_byte(0x10);
			Delay(delay);
			LED_R8_set_byte(0x20);
			Delay(delay);
			LED_R8_set_byte(0x40);
			Delay(delay);
			LED_R8_set_byte(0x80);
			Delay(delay);
			LED_R8_all_OFF();
		}
		else if(leds==SETTINGS_INDICATOR_ANIMATE_IRS)
		{
			//LED_R8_set_byte(0x81);
			//LED_O4_set_byte(0x09);
			//Delay(delay);
			//LED_R8_all_OFF();
			//LED_O4_all_OFF();
			//Delay(delay);
			/*
			LED_R8_set_byte(0x01);
			LED_O4_set_byte(0x08);
			Delay(delay);
			LED_R8_set_byte(0x03);
			Delay(delay);
			LED_R8_set_byte(0x07);
			LED_O4_set_byte(0x0c);
			Delay(delay);
			LED_R8_set_byte(0x0f);
			Delay(delay);
			LED_R8_set_byte(0x1f);
			LED_O4_set_byte(0x0e);
			Delay(delay);
			LED_R8_set_byte(0x3f);
			Delay(delay);
			LED_R8_set_byte(0x7f);
			LED_O4_set_byte(0x0f);
			Delay(delay);
			LED_R8_set_byte(0xff);
			Delay(delay);
			*/
			LED_R8_0_ON;
			LED_O4_3_ON;
			Delay(delay);
			LED_R8_1_ON;
			Delay(delay);
			LED_R8_2_ON;
			LED_O4_2_ON;
			Delay(delay);
			LED_R8_3_ON;
			Delay(delay);
			LED_R8_4_ON;
			LED_O4_1_ON;
			Delay(delay);
			LED_R8_5_ON;
			Delay(delay);
			LED_R8_6_ON;
			LED_O4_0_ON;
			Delay(delay);
			LED_R8_7_ON;
			Delay(delay);

			LED_R8_7_OFF;
			Delay(delay);
			LED_R8_6_OFF;
			LED_O4_0_OFF;
			Delay(delay);
			LED_R8_5_OFF;
			Delay(delay);
			LED_R8_4_OFF;
			LED_O4_1_OFF;
			Delay(delay);
			LED_R8_3_OFF;
			Delay(delay);
			LED_R8_2_OFF;
			LED_O4_2_OFF;
			Delay(delay);
			LED_R8_1_OFF;
			Delay(delay);
			LED_R8_0_OFF;
			LED_O4_3_OFF;
			Delay(delay);
		}
		else if(leds==SETTINGS_INDICATOR_ANIMATE_3DS)
		{
			//LED_B5_set_byte(0x15);
			LED_R8_set_byte(0xc3);
			LED_O4_set_byte(0x06);
			Delay(delay);
			//LED_B5_all_OFF();
			LED_R8_all_OFF();
			LED_O4_all_OFF();
			Delay(delay);
		}
		else if(leds==SETTINGS_INDICATOR_ANIMATE_ALL_LEDS_OFF)
		{
			LED_R8_set_byte(0xff);
			LED_O4_set_byte(0x0f);
			LED_B5_set_byte(0x1f);
			LED_W8_set_byte(0xff);
			Delay(delay);
			LED_R8_7_OFF;
			LED_W8_0_OFF;
			LED_B5_0_OFF;
			Delay(delay);
			LED_R8_6_OFF;
			LED_W8_1_OFF;
			LED_O4_0_OFF;
			Delay(delay);
			LED_R8_5_OFF;
			LED_W8_2_OFF;
			LED_B5_1_OFF;
			Delay(delay);
			LED_R8_4_OFF;
			LED_W8_3_OFF;
			LED_O4_1_OFF;
			Delay(delay);
			LED_R8_3_OFF;
			LED_W8_4_OFF;
			LED_B5_2_OFF;
			Delay(delay);
			LED_R8_2_OFF;
			LED_W8_5_OFF;
			LED_O4_2_OFF;
			Delay(delay);
			LED_R8_1_OFF;
			LED_W8_6_OFF;
			LED_B5_3_OFF;
			Delay(delay);
			LED_R8_0_OFF;
			LED_W8_7_OFF;
			LED_O4_3_OFF;
			Delay(delay);
			LED_B5_4_OFF;
			Delay(delay);
		}
		else if(leds==SETTINGS_INDICATOR_ANIMATE_ALL_LEDS_ON)
		{
			LEDs_all_OFF();
			LED_R8_0_ON;
			LED_W8_7_ON;
			LED_O4_3_ON;
			LED_B5_4_ON;
			Delay(delay);
			LED_R8_1_ON;
			LED_W8_6_ON;
			Delay(delay);
			LED_R8_2_ON;
			LED_W8_5_ON;
			LED_O4_2_ON;
			LED_B5_3_ON;
			Delay(delay);
			LED_R8_3_ON;
			LED_W8_4_ON;
			Delay(delay);
			LED_R8_4_ON;
			LED_W8_3_ON;
			LED_O4_1_ON;
			LED_B5_2_ON;
			Delay(delay);
			LED_R8_5_ON;
			LED_W8_2_ON;
			Delay(delay);
			LED_R8_6_ON;
			LED_W8_1_ON;
			LED_O4_0_ON;
			LED_B5_1_ON;
			Delay(delay);
			LED_R8_7_ON;
			LED_W8_0_ON;
			Delay(delay);
			LED_B5_0_ON;
			Delay(delay);
		}
		else if(leds==SETTINGS_INDICATOR_MIDI_POLYPHONY+MIDI_POLYPHONY_HOLD_COMBINED)
		{
			LED_R8_all_OFF();
			Delay(delay);
			LED_R8_set_byte(0x95);
			Delay(delay);
		}
		else if(leds==SETTINGS_INDICATOR_MIDI_POLYPHONY+MIDI_POLYPHONY_SUST_CHORD
			 || leds==SETTINGS_INDICATOR_MIDI_POLYPHONY+MIDI_POLYPHONY_HOLD_CHORD)
		{
			LED_R8_all_OFF();
			Delay(delay);
			LED_R8_set_byte(0x15);
			Delay(delay);
		}
		else if(leds==SETTINGS_INDICATOR_MIDI_POLYPHONY+MIDI_POLYPHONY_SUST_SINGLE_NOTE
			 || leds==SETTINGS_INDICATOR_MIDI_POLYPHONY+MIDI_POLYPHONY_HOLD_SINGLE_NOTE)
		{
			LED_R8_all_OFF();
			Delay(delay);
			LED_R8_set_byte(0x01);
			Delay(delay);
		}
		else if(leds==SETTINGS_INDICATOR_MIDI_POLYPHONY+MIDI_POLYPHONY_SUST_OCTAVE_UP_DOWN
			 || leds==SETTINGS_INDICATOR_MIDI_POLYPHONY+MIDI_POLYPHONY_HOLD_OCTAVE_UP_DOWN)
		{
			LED_R8_all_OFF();
			Delay(delay);
			LED_R8_set_byte(0x81);
			Delay(delay);
		}
		else if(leds==SETTINGS_INDICATOR_ANIMATE_FILL_8_LEFT)
		{
			LED_R8_all_OFF();
			Delay(delay);
			LED_R8_set_byte(0x80);
			Delay(delay);
			LED_R8_set_byte(0xc0);
			Delay(delay);
			LED_R8_set_byte(0xe0);
			Delay(delay);
			LED_R8_set_byte(0xf0);
			Delay(delay);
			LED_R8_set_byte(0xf8);
			Delay(delay);
			LED_R8_set_byte(0xfc);
			Delay(delay);
			LED_R8_set_byte(0xfe);
			Delay(delay);
			LED_R8_set_byte(0xff);
			Delay(delay);
		}
		else if(leds==SETTINGS_INDICATOR_ANIMATE_FILL_8_RIGHT)
		{
			LED_R8_all_OFF();
			Delay(delay);
			LED_R8_set_byte(0x01);
			Delay(delay);
			LED_R8_set_byte(0x03);
			Delay(delay);
			LED_R8_set_byte(0x07);
			Delay(delay);
			LED_R8_set_byte(0x0f);
			Delay(delay);
			LED_R8_set_byte(0x1f);
			Delay(delay);
			LED_R8_set_byte(0x3f);
			Delay(delay);
			LED_R8_set_byte(0x7f);
			Delay(delay);
			LED_R8_set_byte(0xff);
			Delay(delay);
		}
		else if(leds==SETTINGS_INDICATOR_ANIMATE_CLEAR_8_RIGHT)
		{
			LED_R8_set_byte(0xff);
			Delay(delay);
			LED_R8_set_byte(0xfe);
			Delay(delay);
			LED_R8_set_byte(0xfc);
			Delay(delay);
			LED_R8_set_byte(0xf8);
			Delay(delay);
			LED_R8_set_byte(0xf0);
			Delay(delay);
			LED_R8_set_byte(0xe0);
			Delay(delay);
			LED_R8_set_byte(0xc0);
			Delay(delay);
			LED_R8_set_byte(0x80);
			Delay(delay);
			LED_R8_all_OFF();
			Delay(delay);
		}
		else if(leds==SETTINGS_INDICATOR_ANIMATE_CLEAR_8_LEFT)
		{
			LED_R8_set_byte(0xff);
			Delay(delay);
			LED_R8_set_byte(0x7f);
			Delay(delay);
			LED_R8_set_byte(0x3f);
			Delay(delay);
			LED_R8_set_byte(0x1f);
			Delay(delay);
			LED_R8_set_byte(0x0f);
			Delay(delay);
			LED_R8_set_byte(0x07);
			Delay(delay);
			LED_R8_set_byte(0x03);
			Delay(delay);
			LED_R8_set_byte(0x01);
			Delay(delay);
			LED_R8_all_OFF();
			Delay(delay);
		}
	}
}

void indicate_error(uint16_t leds, int blink, int delay)
{
	for(int i=0;i<blink;i++)
	{
		LED_R8_set_byte(leds);
		Delay(delay);
		LED_R8_set_byte(leds>>8);
		Delay(delay);
	}
	LED_R8_all_OFF();
}

void indicate_eq_bass_setting(int value, uint16_t setting)
{
	LED_R8_all_OFF();
	if(setting==SETTINGS_INDICATOR_EQ_BASS || setting==SETTINGS_INDICATOR_EQ_TREBLE)
	{
		if(setting==SETTINGS_INDICATOR_EQ_BASS)
		{
			value = -value;
		}

		if(value==-9)
		{
			LED_R8_0_ON;
			LED_R8_1_ON;
		}
		if(value==-6)
		{
			LED_R8_1_ON;
			LED_R8_2_ON;
		}
		if(value==-3)
		{
			LED_R8_2_ON;
			LED_R8_3_ON;
		}
		if(value==0)
		{
			LED_R8_3_ON;
			LED_R8_4_ON;
		}
		if(value==3)
		{
			LED_R8_4_ON;
			LED_R8_5_ON;
		}
		if(value==6)
		{
			LED_R8_5_ON;
			LED_R8_6_ON;
		}
		if(value==9)
		{
			LED_R8_6_ON;
			LED_R8_7_ON;
		}
	}
}

void LED_sequencer_indicate_position(int chord)
{
	LED_R8_set_byte(1<<(chord%8));
	LED_O4_set_byte(1<<((chord/8)%4));
}

#endif
