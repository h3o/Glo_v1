/*
 * gpio.c
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

#include <init.h>

#include <hw/codec.h>
#include <hw/gpio.h>
#include <string.h>

uint8_t exp_bits[EXPANDERS*2]; //memory map for I/O signals driven by 4 expanders (double size to keep a previous state)
uint8_t LED_bits[EXPANDERS*2]; //memory map for individual LEDs before remapping to 4 expanders (double size to keep a previous state)
uint8_t Buttons_bits;	//memory map for 6 buttons (SET, RST and 1-4)

void Delay(int d)
{
	vTaskDelay(d / portTICK_RATE_MS);
}

void whale_init_power()
{
	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_2,1); //hold the power on
}

#ifdef BOARD_WHALE
void whale_shutdown()
{
	printf("whale_shutdown(): Shutting down...\n");
	codec_set_mute(1); //mute the codec
	Delay(100);
	codec_reset();
	Delay(100);
	gpio_set_level(GPIO_NUM_2,0); //power off
	RGB_LED_OFF;
	while(1);
}

void whale_init_buttons_GPIO()
{
	int result;
	result = gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
	printf("GPIO0 direction set result = %d\n",result);

	result = gpio_set_direction(GPIO_NUM_34, GPIO_MODE_INPUT); //BT1_PWR
	printf("GPI34 direction set result = %d\n",result);
	result = gpio_set_pull_mode(GPIO_NUM_34, GPIO_PULLDOWN_ONLY);
	printf("GPI34 pull mode set result = %d\n",result);

	/*
	gpio_config_t btn_config;
	btn_config.intr_type = GPIO_INTR_DISABLE;
	btn_config.mode = GPIO_MODE_INPUT;           //Set as Input
	btn_config.pin_bit_mask = (1 << GPIO_NUM_34); //Bitmask
	btn_config.pull_up_en = GPIO_PULLUP_DISABLE;    //Disable pullup
	btn_config.pull_down_en = GPIO_PULLDOWN_ENABLE; //Enable pulldown
	gpio_config(&btn_config);
	*/

	result = gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT);
	printf("GPI35 direction set result = %d\n",result);
	result = gpio_set_pull_mode(GPIO_NUM_35, GPIO_PULLDOWN_ONLY);
	printf("GPI35 pull mode set result = %d\n",result);
}

void whale_init_RGB_LED()
{
	//PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_NUM_13], PIN_FUNC_GPIO);
	//PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_NUM_14], PIN_FUNC_GPIO);
	//PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_NUM_15], PIN_FUNC_GPIO);
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[RGB_LED_PIN_R], PIN_FUNC_GPIO);
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[RGB_LED_PIN_G], PIN_FUNC_GPIO);
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[RGB_LED_PIN_B], PIN_FUNC_GPIO);

	gpio_set_direction(RGB_LED_PIN_R, GPIO_MODE_OUTPUT);
	gpio_set_direction(RGB_LED_PIN_G, GPIO_MODE_OUTPUT);
	gpio_set_direction(RGB_LED_PIN_B, GPIO_MODE_OUTPUT);
	//gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT_OD);
	//gpio_set_direction(GPIO_NUM_14, GPIO_MODE_OUTPUT_OD);
	//gpio_set_direction(GPIO_NUM_15, GPIO_MODE_OUTPUT_OD);

	RGB_LED_R_OFF;
	RGB_LED_G_OFF;
	RGB_LED_B_OFF;
	/*
	int result;
	result = gpio_set_level(GPIO_NUM_13,1);//RGB_LED_R_OFF;
	printf("r=%d,",result);
	result = gpio_set_level(GPIO_NUM_14,1);//RGB_LED_G_OFF;
	printf("r=%d,",result);
	result = gpio_set_level(GPIO_NUM_15,1);//RGB_LED_B_OFF;
	printf("r=%d,",result);
	*/
}

void RGB_LEDs_blink(int count, int delay)
{
	while(count)
	{
		RGB_LED_R_ON;
		Delay(delay);
		RGB_LED_R_OFF;
		Delay(delay);
		RGB_LED_G_ON;
		Delay(delay);
		RGB_LED_G_OFF;
		Delay(delay);
		RGB_LED_B_ON;
		Delay(delay);
		RGB_LED_B_OFF;
		Delay(delay);
		count--;
	}
}
#endif

void error_blink(int rc, int rd, int gc, int gd, int bc, int bd) //RGB:count,delay
{
	#ifdef BOARD_WHALE
	for(int i=0;i<rc;i++)
	{
		RGB_LED_R_ON;
		vTaskDelay(rd / portTICK_PERIOD_MS);
		RGB_LED_R_OFF;
		vTaskDelay(rd / portTICK_PERIOD_MS);
	}
	for(int i=0;i<gc;i++)
	{
		RGB_LED_R_ON;
		vTaskDelay(gd / portTICK_PERIOD_MS);
		RGB_LED_R_OFF;
		vTaskDelay(gd / portTICK_PERIOD_MS);
	}
	for(int i=0;i<bc;i++)
	{
		//printf(".");
		RGB_LED_B_ON;
		vTaskDelay(bd / portTICK_PERIOD_MS);
		RGB_LED_B_OFF;
		vTaskDelay(bd / portTICK_PERIOD_MS);
	}
	#endif
}

//------------ LED Blink task test -------------------------

//LED_BlinkParam_t LED_BlinkParam;
//LED_BlinkParam.count = 10;
//LED_BlinkParam.delay = 20;
//LED_BlinkParam.repeat = 10;
////xTaskCreate((TaskFunction_t)&LED_Blink, "led_blink_task", 1024, &LED_BlinkParam, 10, NULL);
//xTaskCreatePinnedToCore((TaskFunction_t)&LED_Blink, "led_blink_task", 2048, &LED_BlinkParam, 10, NULL, 1);

/*
typedef struct {
    int delay;
    int count;
    int repeat;
} LED_BlinkParam_t;

void LED_Blink(void *pvParameters)
{
	int delay = ((LED_BlinkParam_t*)pvParameters)->delay;
	int repeat = ((LED_BlinkParam_t*)pvParameters)->repeat;
	int count = ((LED_BlinkParam_t*)pvParameters)->count;
	printf("LED_Blink(): task running on core ID=%d\n",xPortGetCoreID());
	printf("LED_Blink(): delay = %d\n",delay);
	printf("LED_Blink(): count = %d\n",count);
	printf("LED_Blink(): repeat = %d\n",repeat);

	for(int i=0;i<repeat;i++)
	{
		RGB_LEDs_blink(count,delay);
	}

	printf("LED_Blink(): done blinking\n");

	//while(1);

    printf("LED_Blink(): deleting task...\n");
	vTaskDelete(NULL);
    printf("LED_Blink(): task deleted\n");
}
*/

void gecho_init_buttons_GPIO()
{
	int result;

#ifdef BOARD_GECHO_V172
	result = gpio_set_direction(LED_RDY, GPIO_MODE_OUTPUT);
	printf("LED_RDY (#%d) direction set result = %d\n",LED_RDY,result);
	gpio_set_level(LED_RDY,0); //set the RDY LED on, so the sensor IR LEDs are off (LED is driven by logic low)

	//result = gpio_set_direction(LED_SIG, GPIO_MODE_OUTPUT);
	//printf("LED_SIG (#%d) direction set result = %d\n",LED_SIG,result);
	//gpio_set_level(LED_SIG,1); //light the SIG LED off (driven by logic low)

	result = gpio_set_direction(IR_DRV2, GPIO_MODE_OUTPUT);
	printf("IR_DRV2 (#%d) direction set result = %d\n",IR_DRV2,result);
	gpio_set_level(IR_DRV2,0); //turn off the sensor IR LEDs

	result = gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT); //analog buttons
	printf("GPI34 direction set result = %d\n",result);
	result = gpio_set_pull_mode(GPIO_NUM_35, GPIO_FLOATING);
	printf("GPI34 pull mode set result = %d\n",result);

	#ifdef OVERRIDE_RX_TX_LEDS
	Delay(10);
	printf("Overriding Rx Tx LEDs to drive blue LEDs #0 & #1\n");
	Delay(10);
	//UART0 Rx Tx pins shared with first two blue leds
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_NUM_1], PIN_FUNC_GPIO); //normally U0TXD
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_NUM_3], PIN_FUNC_GPIO); //normally U0RXD

	result = gpio_set_direction(LED_B0, GPIO_MODE_OUTPUT);
	printf("LED_B0 (#%d) direction set result = %d\n",LED_B0,result);
	gpio_set_level(LED_B0,1); //set the LED off (driven by logic low)
	result = gpio_set_direction(LED_B1, GPIO_MODE_OUTPUT);
	printf("LED_B1 (#%d) direction set result = %d\n",LED_B1,result);
	gpio_set_level(LED_B1,1); //set the LED off (driven by logic low)
	#endif

	result = gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT); //analog buttons
	printf("GPI35 direction set result = %d\n",result);
	result = gpio_set_pull_mode(GPIO_NUM_35, GPIO_FLOATING);
	printf("GPI35 pull mode set result = %d\n",result);

#endif

#ifdef BOARD_GECHO_V173
	result = gpio_set_direction(LED_SIG, GPIO_MODE_OUTPUT);
	printf("LED_SIG (#%d) direction set result = %d\n",LED_SIG,result);
	gpio_set_level(LED_SIG,1); //light the SIG LED off (driven by logic low)

	result = gpio_set_direction(IR_DRV1, GPIO_MODE_OUTPUT);
	printf("IR_DRV1 (#%d) direction set result = %d\n",IR_DRV1,result);
	gpio_set_level(IR_DRV1,0); //turn off the sensor IR LEDs

	result = gpio_set_direction(IR_DRV2, GPIO_MODE_OUTPUT);
	printf("IR_DRV2 (#%d) direction set result = %d\n",IR_DRV2,result);
	gpio_set_level(IR_DRV2,0); //turn off the sensor IR LEDs

	result = gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT); //buttons expander INT
	printf("GPI35 direction set result = %d\n",result);
	result = gpio_set_pull_mode(GPIO_NUM_35, GPIO_FLOATING);
	printf("GPI35 pull mode set result = %d\n",result);

#endif
}

