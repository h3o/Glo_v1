/*
 * gpio.h
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

#ifndef GPIO_H_
#define GPIO_H_

#include <stdbool.h>
#include "E:\dev\ESP32\esp-idf\components\driver\include\driver\gpio.h"
#include "board.h"

//esp-32

#ifdef BOARD_WHALE

#define RGB_LED_PIN_R	GPIO_NUM_15
#define RGB_LED_PIN_G	GPIO_NUM_14
#define RGB_LED_PIN_B	GPIO_NUM_13

#define RGB_LED_R_ON	gpio_set_level(RGB_LED_PIN_R,0)  //LEDs have common anode
#define RGB_LED_R_OFF	gpio_set_level(RGB_LED_PIN_R,1)
#define RGB_LED_G_ON	gpio_set_level(RGB_LED_PIN_G,0)
#define RGB_LED_G_OFF	gpio_set_level(RGB_LED_PIN_G,1)
#define RGB_LED_B_ON	gpio_set_level(RGB_LED_PIN_B,0)
#define RGB_LED_B_OFF	gpio_set_level(RGB_LED_PIN_B,1)

#define RGB_LED_OFF		RGB_LED_R_OFF;RGB_LED_G_OFF;RGB_LED_B_OFF

//-------------------------------------------------------

#define BUTTON_U1_PIN		((gpio_num_t)34) //GPIO_NUM_34 //PWR button
#define BUTTON_U2_PIN		((gpio_num_t)0) //GPIO_NUM_0 //BOOT button
#define BUTTON_U3_PIN		((gpio_num_t)35) //GPIO_NUM_35 //RST button

#define BUTTON_U1_ON		(gpio_get_level(BUTTON_U1_PIN)==1) //BT1 connects to VDD when pressed
#define BUTTON_U2_ON		(gpio_get_level(BUTTON_U2_PIN)==0) //BT2 connects to GND when pressed
#define BUTTON_U3_ON		(gpio_get_level(BUTTON_U3_PIN)==1) //BT3 connects to VDD when pressed

#define ANY_USER_BUTTON_ON (BUTTON_U1_ON || BUTTON_U2_ON || BUTTON_U3_ON)// || BUTTON_U4_ON)

#endif

//gecho

#ifdef BOARD_GECHO

#define ANY_USER_BUTTON_ON (BUTTON_U1_ON || BUTTON_U2_ON || BUTTON_U3_ON || BUTTON_U4_ON)

#define LED_SIG			GPIO_NUM_18
#define LED_SIG_ON		gpio_set_level(LED_SIG,0) //reversed logic
#define LED_SIG_OFF		gpio_set_level(LED_SIG,1)

#endif

#ifdef BOARD_GECHO_V172

#define LED_RDY		GPIO_NUM_23
#define LED_RDY_ON	gpio_set_level(LED_RDY,0) //reversed logic
#define LED_RDY_OFF	gpio_set_level(LED_RDY,1)

#define IR_DRV1		LED_RDY
#define IR_DRV1_ON	LED_RDY_OFF
#define IR_DRV1_OFF	LED_RDY_ON

#define IR_DRV2		GPIO_NUM_19
#define IR_DRV2_ON	gpio_set_level(IR_DRV2,1)
#define IR_DRV2_OFF	gpio_set_level(IR_DRV2,0)

#define LED_B0		GPIO_NUM_3
#define LED_B0_ON	gpio_set_level(LED_B0,0) //reversed logic
#define LED_B0_OFF	gpio_set_level(LED_B0,1)

#define LED_B1		GPIO_NUM_1
#define LED_B1_ON	gpio_set_level(LED_B1,0) //reversed logic
#define LED_B1_OFF	gpio_set_level(LED_B1,1)

#endif

#ifdef BOARD_GECHO_V173

#define IR_DRV1		GPIO_NUM_19
#define IR_DRV1_ON	gpio_set_level(IR_DRV1,1)
#define IR_DRV1_OFF	gpio_set_level(IR_DRV1,0)

#define IR_DRV2		GPIO_NUM_23
#define IR_DRV2_ON	gpio_set_level(IR_DRV2,1)
#define IR_DRV2_OFF	gpio_set_level(IR_DRV2,0)

#define LED_RDY_ON	LED_bits[MAP_ORANGE_LEDS]|=0x08
#define LED_RDY_OFF	LED_bits[MAP_ORANGE_LEDS]&=~0x08

#define BUTTON_SET_ON	(Buttons_bits&0x01)
#define BUTTON_U4_ON	(Buttons_bits&0x02)
#define BUTTON_U3_ON	(Buttons_bits&0x04)
#define BUTTON_U2_ON	(Buttons_bits&0x08)
#define BUTTON_U1_ON	(Buttons_bits&0x10)
#define BUTTON_RST_ON	(Buttons_bits&0x20)

#define LED_R8_0_ON		(LED_bits[MAP_RED_LEDS]|=0x80)
#define LED_R8_0_OFF	(LED_bits[MAP_RED_LEDS]&=~0x80)
#define LED_R8_1_ON		(LED_bits[MAP_RED_LEDS]|=0x40)
#define LED_R8_1_OFF	(LED_bits[MAP_RED_LEDS]&=~0x40)
#define LED_R8_2_ON		(LED_bits[MAP_RED_LEDS]|=0x20)
#define LED_R8_2_OFF	(LED_bits[MAP_RED_LEDS]&=~0x20)
#define LED_R8_3_ON		(LED_bits[MAP_RED_LEDS]|=0x10)
#define LED_R8_3_OFF	(LED_bits[MAP_RED_LEDS]&=~0x10)
#define LED_R8_4_ON		(LED_bits[MAP_RED_LEDS]|=0x08)
#define LED_R8_4_OFF	(LED_bits[MAP_RED_LEDS]&=~0x08)
#define LED_R8_5_ON		(LED_bits[MAP_RED_LEDS]|=0x04)
#define LED_R8_5_OFF	(LED_bits[MAP_RED_LEDS]&=~0x04)
#define LED_R8_6_ON		(LED_bits[MAP_RED_LEDS]|=0x02)
#define LED_R8_6_OFF	(LED_bits[MAP_RED_LEDS]&=~0x02)
#define LED_R8_7_ON		(LED_bits[MAP_RED_LEDS]|=0x01)
#define LED_R8_7_OFF	(LED_bits[MAP_RED_LEDS]&=~0x01)

#define LED_O1_ON		(LED_bits[MAP_ORANGE_LEDS]|=0x80)
#define LED_O1_OFF		(LED_bits[MAP_ORANGE_LEDS]&=~0x80)
#define LED_O2_ON		(LED_bits[MAP_ORANGE_LEDS]|=0x40)
#define LED_O2_OFF		(LED_bits[MAP_ORANGE_LEDS]&=~0x40)
#define LED_O3_ON		(LED_bits[MAP_ORANGE_LEDS]|=0x20)
#define LED_O3_OFF		(LED_bits[MAP_ORANGE_LEDS]&=~0x20)
#define LED_O4_ON		(LED_bits[MAP_ORANGE_LEDS]|=0x10)
#define LED_O4_OFF		(LED_bits[MAP_ORANGE_LEDS]&=~0x10)

#define LED_O4_0_ON		LED_O1_ON
#define LED_O4_0_OFF	LED_O1_OFF
#define LED_O4_1_ON		LED_O2_ON
#define LED_O4_1_OFF	LED_O2_OFF
#define LED_O4_2_ON		LED_O3_ON
#define LED_O4_2_OFF	LED_O3_OFF
#define LED_O4_3_ON		LED_O4_ON
#define LED_O4_3_OFF	LED_O4_OFF

#define LED_B5_0_ON		(LED_bits[MAP_BLUE_LEDS]|=0x80)
#define LED_B5_0_OFF	(LED_bits[MAP_BLUE_LEDS]&=~0x80)
#define LED_B5_1_ON		(LED_bits[MAP_BLUE_LEDS]|=0x40)
#define LED_B5_1_OFF	(LED_bits[MAP_BLUE_LEDS]&=~0x40)
#define LED_B5_2_ON		(LED_bits[MAP_BLUE_LEDS]|=0x20)
#define LED_B5_2_OFF	(LED_bits[MAP_BLUE_LEDS]&=~0x20)
#define LED_B5_3_ON		(LED_bits[MAP_BLUE_LEDS]|=0x10)
#define LED_B5_3_OFF	(LED_bits[MAP_BLUE_LEDS]&=~0x10)
#define LED_B5_4_ON		(LED_bits[MAP_BLUE_LEDS]|=0x08)
#define LED_B5_4_OFF	(LED_bits[MAP_BLUE_LEDS]&=~0x08)

#define LED_W8_0_ON		(LED_bits[MAP_WHITE_LEDS]|=0x80)
#define LED_W8_0_OFF	(LED_bits[MAP_WHITE_LEDS]&=~0x80)
#define LED_W8_1_ON		(LED_bits[MAP_WHITE_LEDS]|=0x40)
#define LED_W8_1_OFF	(LED_bits[MAP_WHITE_LEDS]&=~0x40)
#define LED_W8_2_ON		(LED_bits[MAP_WHITE_LEDS]|=0x20)
#define LED_W8_2_OFF	(LED_bits[MAP_WHITE_LEDS]&=~0x20)
#define LED_W8_3_ON		(LED_bits[MAP_WHITE_LEDS]|=0x10)
#define LED_W8_3_OFF	(LED_bits[MAP_WHITE_LEDS]&=~0x10)
#define LED_W8_4_ON		(LED_bits[MAP_WHITE_LEDS]|=0x08)
#define LED_W8_4_OFF	(LED_bits[MAP_WHITE_LEDS]&=~0x08)
#define LED_W8_5_ON		(LED_bits[MAP_WHITE_LEDS]|=0x04)
#define LED_W8_5_OFF	(LED_bits[MAP_WHITE_LEDS]&=~0x04)
#define LED_W8_6_ON		(LED_bits[MAP_WHITE_LEDS]|=0x02)
#define LED_W8_6_OFF	(LED_bits[MAP_WHITE_LEDS]&=~0x02)
#define LED_W8_7_ON		(LED_bits[MAP_WHITE_LEDS]|=0x01)
#define LED_W8_7_OFF	(LED_bits[MAP_WHITE_LEDS]&=~0x01)

#endif

//#define MIDI_SIGNAL_HW_TEST

//#define LED_TEST_DELAY 250
#define LED_TEST_DELAY 	20

#define EXPANDERS 4
#define EXPANDERS_TIMING_DELAY 10
extern uint8_t exp_bits[EXPANDERS*2];
extern uint8_t LED_bits[EXPANDERS*2];
extern uint8_t Buttons_bits;

#define EXP4_INT		GPIO_NUM_35

#define EXP_WHITE_LEDS 	0
#define EXP_RED_LEDS 	1
#define EXP_ORANGE_LEDS	2 //and also last 3 green
#define EXP_GREEN_LEDS	3 //first 2 green

#define MAP_RED_LEDS 	0
#define MAP_ORANGE_LEDS	1
#define MAP_BLUE_LEDS	2
#define MAP_GREEN_LEDS	MAP_BLUE_LEDS
#define MAP_WHITE_LEDS 	3

#define BUTTONS_BITS LED_bits[MAP_GREEN_LEDS]

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported variables ------------------------------------------------------- */
//extern __IO uint32_t sys_timer;
//extern __IO uint32_t sys_clock;

//extern int run_program;

/* Exported functions ------------------------------------------------------- */


void Delay(int d);

void whale_init_power();
void whale_shutdown();

void whale_init_buttons_GPIO();
void whale_init_RGB_LED();

void RGB_LEDs_blink(int count, int delay);
void error_blink(int rc, int rd, int gc, int gd, int bc, int bd); //RGB:count,delay

void gecho_init_buttons_GPIO();

//void LED_Blink(void *pvParameters); //function for RTOS task

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H_ */
