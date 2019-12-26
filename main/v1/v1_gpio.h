/*
 * gpio.h
 *
 *  Created on: Jun 21, 2016
 *      Author: mayo
 */

#ifndef V1_GPIO_H_
#define V1_GPIO_H_

//#include "stm32f4xx.h"
//#include "stm32f4xx_it.h"
//#include "stm32f4xx_rcc.h"
//#include "stm32f4xx_gpio.h"

#include <stdbool.h>

//Defines
#define LED_GREEN_ON   GPIOD->BSRRL = GPIO_Pin_12;
#define LED_GREEN_OFF  GPIOD->BSRRH = GPIO_Pin_12;
#define LED_ORANGE_ON   GPIOD->BSRRL = GPIO_Pin_13;
#define LED_ORANGE_OFF  GPIOD->BSRRH = GPIO_Pin_13;
#define LED_RED_ON   GPIOD->BSRRL = GPIO_Pin_14;
#define LED_RED_OFF  GPIOD->BSRRH = GPIO_Pin_14;
#define LED_BLUE_ON   GPIOD->BSRRL = GPIO_Pin_15;
#define LED_BLUE_OFF  GPIOD->BSRRH = GPIO_Pin_15;

//on-shield Blue LEDs (l..r), negative logic L=ON, H=OFF
//PA15,PC11,PA9,PA8
#define LED_SB1_ON   GPIOA->BSRRH = GPIO_Pin_15;
#define LED_SB1_OFF  GPIOA->BSRRL = GPIO_Pin_15;
#define LED_SB2_ON   GPIOC->BSRRH = GPIO_Pin_11;
#define LED_SB2_OFF  GPIOC->BSRRL = GPIO_Pin_11;
#define LED_SB3_ON   GPIOA->BSRRH = GPIO_Pin_9;
#define LED_SB3_OFF  GPIOA->BSRRL = GPIO_Pin_9;
#define LED_SB4_ON   GPIOA->BSRRH = GPIO_Pin_8;
#define LED_SB4_OFF  GPIOA->BSRRL = GPIO_Pin_8;

//on-shield Red LEDs (l..r)
//PC6,PA10,PC8,PC9...
#define LED_SR1_ON   GPIOC->BSRRH = GPIO_Pin_6;
#define LED_SR1_OFF  GPIOC->BSRRL = GPIO_Pin_6;
#define LED_SR2_ON   GPIOA->BSRRH = GPIO_Pin_10;
#define LED_SR2_OFF  GPIOA->BSRRL = GPIO_Pin_10;
#define LED_SR3_ON   GPIOC->BSRRH = GPIO_Pin_8;
#define LED_SR3_OFF  GPIOC->BSRRL = GPIO_Pin_8;
#define LED_SR4_ON   GPIOC->BSRRH = GPIO_Pin_9;
#define LED_SR4_OFF  GPIOC->BSRRL = GPIO_Pin_9;


// struct to initialize GPIO pins
//GPIO_InitTypeDef GPIO_InitStructure;

#ifdef __cplusplus
 extern "C" {
#endif

extern bool RedLED_state, OrangeLED_state;
extern int led_RED_state, led_GREEN_state;
extern int leds_RED_seq, leds_BLUE_seq;

// --------------------------------------

//exported vars
//extern uint32_t sys_timer;
//extern __IO uint32_t sys_timer;
//extern __IO uint32_t sys_clock;

/* Exported functions ------------------------------------------------------- */
//void GPIO_Init_all();
//void GPIO_Test_all();

void LEDs_RED_next(int limit);
void LEDs_RED_off();
void LEDs_RED_reset();

void LEDs_BLUE_next(int limit);
void LEDs_BLUE_off();

//void TimingDelay_Decrement(void);
//void Delay(__IO uint32_t nTime);

#ifdef __cplusplus
}
#endif

#endif /* V1_GPIO_H_ */
