/*
 * init.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Jan 23, 2018
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

#ifndef INIT_H_
#define INIT_H_

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
//#include "esp32/include/esp_event.h"
#include "esp_event_legacy.h"
#include "esp_attr.h"
#include "esp_sleep.h"
//#include "esp_event/include/esp_event_loop.h"
#include "esp_task_wdt.h"
#include "bootloader_random.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "driver/mcpwm.h"
#include "driver/ledc.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "esp_spi_flash.h"
#include "driver/uart.h"
#include "esp_heap_caps_init.h"
#include "driver/dac.h"
#include "esp32/rom/rtc.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_periph.h"
#include "freertos/FreeRTOSConfig.h"

#include "board.h"

#define CPU_CORE_BUTTONS_CONTROLS_TASK	1
#define CPU_CORE_ACCELEROMETER_TASK		1
#define CPU_CORE_EXPANDERS_TASK			1
#define CPU_CORE_SENSORS_TASK			1
#define CPU_CORE_RECEIVE_MIDI_TASK		1
#define CPU_CORE_RECEIVE_SYNC_TASK		1
#define CPU_CORE_TEMPO_DETECT_TASK		1
#define CPU_CORE_SD_RECORDING_TASK		1
#define CPU_CORE_SD_SECTOR_LOAD_TASK	1
#define CPU_CORE_CHANNEL_DCO_TASK		0 //uses float
#define CPU_CORE_BINAURAL_PROGRAM_TASK	0

#define PRIORITY_BUTTONS_CONTROLS_TASK	10
#define PRIORITY_ACCELEROMETER_TASK		10
#define PRIORITY_EXPANDERS_TASK			10
#define PRIORITY_SENSORS_TASK			10
#define PRIORITY_RECEIVE_MIDI_TASK		9
#define PRIORITY_RECEIVE_SYNC_TASK		9
#define PRIORITY_TEMPO_DETECT_TASK		9
#define PRIORITY_SD_RECORDING_TASK		8//12
#define PRIORITY_SD_SECTOR_LOAD_TASK	8//12
#define PRIORITY_CHANNEL_DCO_TASK		12//24
#define PRIORITY_BINAURAL_PROGRAM_TASK	12

#define I2S_NUM         ((i2s_port_t)I2S_NUM_0)

extern size_t i2s_bytes_rw;

#define MCLK_GPIO 1
#define USE_APLL
#define CODEC_RST_PIN GPIO_NUM_32 //works: 0,2,4,18,19,21,22, doesnt: 3 (RX), 15

extern i2c_port_t i2c_num;

#define I2C_MASTER_SCL_IO				GPIO_NUM_22	//gpio number for I2C master clock
#define I2C_MASTER_SDA_IO				GPIO_NUM_21	//gpio number for I2C master data
#define I2C_MASTER_NUM					I2C_NUM_1	//I2C port number for master dev
#define I2C_MASTER_TX_BUF_DISABLE		0			//I2C master do not need buffer
#define I2C_MASTER_RX_BUF_DISABLE		0			//I2C master do not need buffer
//#define I2C_MASTER_FREQ_HZ			100000		//I2C master clock frequency
#define I2C_MASTER_FREQ_HZ				400000		//I2C master clock frequency

//overdrive I2C: 700k works for codec too, 750k not...
//#define I2C_MASTER_FREQ_HZ_FAST		650000		//I2C master clock frequency
#define I2C_MASTER_FREQ_HZ_FAST			1000000		//I2C master clock frequency

#define CODEC_I2C_ADDRESS				0x18		//slave address for TLV320AIC3104
//#define SPDIF_TX_I2C_ADDRESS			0x3a		//slave address for WM8804G

#define ESP_SLAVE_ADDR					0x28             //ESP32 slave address, you can set any 7bit value
#define WRITE_BIT						I2C_MASTER_WRITE //I2C master write
#define READ_BIT						I2C_MASTER_READ  //I2C master read
#define ACK_CHECK_EN					0x1              //I2C master will check ack from slave
#define ACK_CHECK_DIS					0x0              //I2C master will not check ack from slave
#define ACK_VAL							0x0              //I2C ack value
#define NACK_VAL						0x1              //I2C nack value

#define MIDI_UART UART_NUM_2

#define TWDT_TIMEOUT_S					5
#define TASK_RESET_PERIOD_S				4

extern const char* GLO_HASH;

extern int channel_loop,channel_loop_remapped;

extern uint16_t ms10_counter, auto_power_off;

#define AUTO_POWER_OFF_VOLUME_RAMP		60 //takes one minute to lower down the volume

extern int channel_running;
extern uint8_t volume_ramp;
extern uint8_t beeps_enabled;
extern uint8_t sd_playback_channel;

extern uint8_t i2c_driver_installed;
extern uint8_t i2c_bus_mutex;
extern uint32_t sample32;
extern uint8_t glo_run;
extern uint8_t channel_uses_codec;
extern int init_free_mem;
extern float t_start_channel;

extern int sensor_active[4];
extern float ir_res[];
//extern float acc_res[];

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

//old wiring, 1.75-1.77 without mod
/*
#define SYNC1_IO			GPIO_NUM_17 //GPIO_NUM_5
#define SYNC2_IO			GPIO_NUM_27

#define CV1_IN				GPIO_NUM_17 //GPIO_NUM_5
#define CV2_IN				GPIO_NUM_27

#define MIDI_OUT_PWR		GPIO_NUM_27 //pwr = opposite pin to transistor
#define MIDI_OUT_SIGNAL		GPIO_NUM_16

#define MIDI_IN_SIGNAL		GPIO_NUM_18
*/

//new wiring, 1.75 with mod, 1.78
#define SYNC1_IO			GPIO_NUM_25
#define SYNC2_IO			GPIO_NUM_26

#define CV1_IN				GPIO_NUM_25
#define CV2_IN				GPIO_NUM_26

#define MIDI_OUT_PWR		GPIO_NUM_26
#define MIDI_OUT_SIGNAL		GPIO_NUM_16

#define MIDI_IN_SIGNAL		GPIO_NUM_18

//------------------------------------------

#define ACC_ORIENTATION_XYZ		0
#define ACC_ORIENTATION_XZY		1
#define ACC_ORIENTATION_ZYX		2
#define ACC_ORIENTATION_YXZ		3
#define ACC_ORIENTATION_ZXY		4
#define ACC_ORIENTATION_YZX		5
#define ACC_ORIENTATION_MODES	6

#define ACC_ORIENTATION_DEFAULT	ACC_ORIENTATION_XYZ
extern const uint8_t acc_orientation_indication[ACC_ORIENTATION_MODES];

#define ACC_INVERT_PPP			0	//+x+y+z
#define ACC_INVERT_PPM			1	//-x+y+z
#define ACC_INVERT_PMP			2	//+x-y+z
#define ACC_INVERT_PMM			3	//-x-y+z
#define ACC_INVERT_MPP			4	//+x+y-z
#define ACC_INVERT_MPM			5	//-x+y-z
#define ACC_INVERT_MMP			6	//+x-y-z
#define ACC_INVERT_MMM			7	//-x-y-z
#define ACC_INVERT_MODES		8

#define ACC_INVERT_DEFAULT		ACC_INVERT_PPP
extern const uint8_t acc_invert_indication[ACC_INVERT_MODES];

//------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

void init_i2s_and_gpio(int buf_count, int buf_len, int sample_rate);
void init_deinit_TWDT();

float micros();
uint32_t micros_i();
uint32_t millis();

void enable_I2S_MCLK_clock();
void disable_I2S_MCLK_clock();

void i2c_master_init(int speed);
void i2c_master_deinit();
void i2c_scan_for_devices(int print, uint8_t *addresses, uint8_t *found);
int i2c_codec_two_byte_command(uint8_t b1, uint8_t b2);
uint8_t i2c_codec_register_read(uint8_t reg);
int i2c_bus_write(int addr_rw, unsigned char *data, int len);
int i2c_bus_read(int addr_rw, unsigned char *buf, int buf_len);

void sha1_to_hex(char *code_sha1_hex, uint8_t *code_sha1);
uint16_t hardware_check();
void clear_unallocated_memory();

//void spdif_transceiver_init();

unsigned char byte_bit_reverse(unsigned char b);

void whale_test_RGB();

void gecho_test_LEDs();
void gecho_test_buttons();

void gecho_init_MIDI(int uart_num, int midi_out_enabled);
void gecho_deinit_MIDI();
void gecho_test_MIDI_input_HW();
void gecho_test_MIDI_input();
void gecho_test_MIDI_output();

void gecho_LED_expander_init();
void gecho_LED_expander_test();

void sync_out_init();
void sync_out_test();
void sync_in_test();

void cv_out_init();
void cv_out_test();
void cv_in_test();

void whale_restart();

int channel_name_ends_with(char *suffix, char *channel_name);

void free_memory_info();
int load_song_nvs(char *song_buf, int song_id);
int store_song_nvs(char *song_buf, int song_id);
int delete_song_nvs(int song_id);

void set_mic_bias(int bias);

void show_board_serial_no();
void show_fw_version();

//void float_speed_test();
//void double_speed_test();

int flashed_config_new_enough();
uint16_t fw_version_to_uint16(char *str);

#ifdef __cplusplus
}
#endif

#endif
