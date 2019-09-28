/*
 * init.h
 *
 *  Created on: Jan 23, 2018
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

#ifndef INIT_H_
#define INIT_H_

//#include <stdint.h>

#include "freertos\include\freertos\FreeRTOS.h"
#include "freertos\include\freertos\task.h"
#include "esp32\include\esp_system.h"
//#include "esp32\include\esp_event.h"
#include "esp32\include\esp_attr.h"
#include "esp32\include\esp_event_loop.h"
#include "esp32\include\esp_task_wdt.h"
#include "bootloader_support\include_priv\bootloader_random.h"
#include "nvs_flash\include\nvs_flash.h"
#include "driver\include\driver\gpio.h"
#include "driver\include\driver\i2s.h"
#include "driver\include\driver\mcpwm.h"
#include "driver\include\driver\ledc.h"
#include "driver\include\driver\i2c.h"
#include "driver\include\driver\adc.h"
#include "spi_flash\include\esp_spi_flash.h"
#include "driver\include\driver\uart.h"
#include "heap\include\esp_heap_caps_init.h"
#include "esp32\include\rom\rtc.h"
#include "soc\esp32\include\soc\rtc_cntl_reg.h"
#include "driver\include\driver\rtc_cntl.h"
//#include "esp32\include\esp_brownout.h"
#include "esp32\include\esp_intr_alloc.h"


#include "board.h"

#define I2S_NUM         ((i2s_port_t)I2S_NUM_0)

#define MCLK_GPIO 1
#define USE_APLL
#define CODEC_RST_PIN GPIO_NUM_32 //works: 0,2,4,18,19,21,22, doesnt: 3 (RX), 15

extern i2c_port_t i2c_num;

#define I2C_MASTER_SCL_IO          GPIO_NUM_22	//gpio number for I2C master clock
#define I2C_MASTER_SDA_IO          GPIO_NUM_21	//gpio number for I2C master data
#define I2C_MASTER_NUM             I2C_NUM_1	//I2C port number for master dev
#define I2C_MASTER_TX_BUF_DISABLE  0			//I2C master do not need buffer
#define I2C_MASTER_RX_BUF_DISABLE  0			//I2C master do not need buffer
//#define I2C_MASTER_FREQ_HZ         100000		//I2C master clock frequency
#define I2C_MASTER_FREQ_HZ         400000		//I2C master clock frequency

//overdrive I2C: 700k works for codec too, 750k not...
//#define I2C_MASTER_FREQ_HZ_FAST         650000	//I2C master clock frequency
#define I2C_MASTER_FREQ_HZ_FAST         1000000		//I2C master clock frequency

#define CODEC_I2C_ADDRESS                  0x18		//slave address for TLV320AIC3104
//#define SPDIF_TX_I2C_ADDRESS               0x3a	//slave address for WM8804G

#define ESP_SLAVE_ADDR                     0x28             //ESP32 slave address, you can set any 7bit value
#define WRITE_BIT                          I2C_MASTER_WRITE //I2C master write
#define READ_BIT                           I2C_MASTER_READ  //I2C master read
#define ACK_CHECK_EN                       0x1              //I2C master will check ack from slave
#define ACK_CHECK_DIS                      0x0              //I2C master will not check ack from slave
#define ACK_VAL                            0x0              //I2C ack value
#define NACK_VAL                           0x1              //I2C nack value

#define MIDI_UART UART_NUM_2


#define TWDT_TIMEOUT_S						5
#define TASK_RESET_PERIOD_S					4

extern int channel_loop,channel_loop_remapped;

extern uint16_t ms10_counter, auto_power_off;

#define AUTO_POWER_OFF_VOLUME_RAMP 60 //takes one minute to lower down the volume

extern int channel_running;
extern int volume_ramp;
extern int beeps_enabled;

extern int i2c_driver_installed;
extern int i2c_bus_mutex;
extern uint32_t sample32;
extern int glo_run;

/*
 * Macro to check the outputs of TWDT functions and trigger an abort if an
 * incorrect code is returned.
 */
#define CHECK_ERROR_CODE(returned, expected) ({                        \
            if(returned != expected){                                  \
                printf("TWDT ERROR\n");                                \
                abort();                                               \
            }                                                          \
})

//-------------------------------------------------------

#define SYNC1_IO	GPIO_NUM_5
#define SYNC2_IO	GPIO_NUM_27

#define CV1_IN		GPIO_NUM_5
#define CV2_IN		GPIO_NUM_27

#define MIDI_OUT_PWR		GPIO_NUM_27
#define MIDI_OUT_SIGNAL		GPIO_NUM_16

#define MIDI_IN_SIGNAL		GPIO_NUM_18

#ifdef __cplusplus
extern "C" {
#endif

void init_i2s_and_gpio(int buf_count, int buf_len);
void init_deinit_TWDT();

float micros();
uint32_t micros_i();
uint32_t millis();

void enable_I2S_MCLK_clock();
void disable_I2S_MCLK_clock();

void i2c_master_init(int speed);
void i2c_master_deinit();
void i2c_scan_for_devices();
int i2c_codec_two_byte_command(uint8_t b1, uint8_t b2);
uint8_t i2c_codec_register_read(uint8_t reg);
int i2c_bus_write(int addr_rw, unsigned char *data, int len);
int i2c_bus_read(int addr_rw, unsigned char *buf, int buf_len);

void clear_unallocated_memory();

//void spdif_transceiver_init();
void generate_random_seed();

void whale_test_RGB();

void gecho_test_LEDs();
void gecho_test_buttons();

void gecho_init_MIDI(int uart_num);
void gecho_deinit_MIDI(int uart_num);
void gecho_test_MIDI_input();
void gecho_test_MIDI_output();

void gecho_LED_expander_init();
void gecho_LED_expander_test();

void rotary_encoder_init();
void rotary_encoder_test();

void sync_out_init();
void sync_out_test();

void whale_restart();
void brownout_init();

#ifdef __cplusplus
}
#endif

#endif
