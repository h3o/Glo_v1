/*
 * init.c
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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <sys/time.h>

#include "init.h"
#include "gpio.h"
#include "leds.h"
#include "codec.h"
#include "signals.h"
#include "sha1.h"
#include "midi.h"
#include "glo_config.h"
#include "ui.h"

i2c_port_t i2c_num;
uint8_t i2c_driver_installed = 0;
uint8_t i2c_bus_mutex = 0;
uint8_t glo_run = 0;

int channel_loop = 0,channel_loop_remapped;
int channel_running = 0;
uint8_t volume_ramp = 0;
uint8_t beeps_enabled = 1;
uint8_t sd_playback_channel = 0;

uint16_t ms10_counter = 0; //sync counter for various processes (buttons, volume ramps, MCLK off)
uint16_t auto_power_off = 0;

uint8_t channel_uses_codec = 1;

size_t i2s_bytes_rw;

int init_free_mem;

float t_start_channel;

const uint8_t acc_orientation_indication[ACC_ORIENTATION_MODES] = {0x15, 0x6d, 0x6b, 0x5b, 0xee, 0x77};
const uint8_t acc_invert_indication[ACC_INVERT_MODES] = {0x15, 0x2b, 0x2d, 0x5b, 0x35, 0x6b, 0x6d, 0xdb};

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

void init_i2s_and_gpio(int buf_count, int buf_len, int sample_rate)
{
	printf("Initialize I2S\n");

	i2s_driver_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,                    // enable TX and RX
        .sample_rate = sample_rate,
        .bits_per_sample = 16,                                                  //16-bit per channel
		.bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT,
		.mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
        .communication_format = I2S_COMM_FORMAT_STAND_I2S, //I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
#ifdef USE_APLL
		.use_apll = true,
#else
		.use_apll = false,
#endif
		.dma_buf_count = buf_count,
		.dma_buf_len = buf_len,
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1                                //Interrupt level 1
    };
    i2s_pin_config_t pin_config = {

		#ifdef BOARD_WHALE
        .bck_io_num = 25,			//bit clock
        .ws_io_num = 12,			//byte clock
        .data_in_num = 27,			//MISO
        .data_out_num = 26,			//MOSI
		#else //gecho has different signal used here

		//old wiring, 1.75-1.77 without mod
		//.bck_io_num = 25,			//bit clock
		//.ws_io_num = 33,			//byte clock
        //.data_in_num = 34,		//MISO
        //.data_out_num = 26,		//MOSI

        //new wiring, 1.75 with mod, 1.78
		.bck_io_num = 17,			//bit clock
		.ws_io_num = 33,			//byte clock
        .data_in_num = 34,			//MISO
        .data_out_num = 27,			//MOSI

		#endif
    };

	#ifdef BOARD_WHALE
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_NUM_12], PIN_FUNC_GPIO); //normally JTAG MTDI, need to change
	#endif

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);

    current_sampling_rate = sample_rate;
}

void init_deinit_TWDT()
{
	printf("Initialize TWDT\n");
	//Initialize or reinitialize TWDT
	//CHECK_ERROR_CODE(esp_task_wdt_init(TWDT_TIMEOUT_S, false), ESP_OK);
	CHECK_ERROR_CODE(esp_task_wdt_init(5, false), ESP_OK);

	//Subscribe Idle Tasks to TWDT if they were not subscribed at startup
	#ifndef CONFIG_TASK_WDT_CHECK_IDLE_TASK_CPU0
	esp_task_wdt_add(xTaskGetIdleTaskHandleForCPU(0));
	#endif
	#ifndef CONFIG_TASK_WDT_CHECK_IDLE_TASK_CPU1
	esp_task_wdt_add(xTaskGetIdleTaskHandleForCPU(1));
	#endif

    //unsubscribe idle tasks
    CHECK_ERROR_CODE(esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0)), ESP_OK);     //Unsubscribe Idle Task from TWDT
    CHECK_ERROR_CODE(esp_task_wdt_status(xTaskGetIdleTaskHandleForCPU(0)), ESP_ERR_NOT_FOUND);      //Confirm Idle task has unsubscribed

    CHECK_ERROR_CODE(esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(1)), ESP_OK);     //Unsubscribe Idle Task from TWDT
    CHECK_ERROR_CODE(esp_task_wdt_status(xTaskGetIdleTaskHandleForCPU(1)), ESP_ERR_NOT_FOUND);      //Confirm Idle task has unsubscribed

    //Deinit TWDT after all tasks have unsubscribed
    CHECK_ERROR_CODE(esp_task_wdt_deinit(), ESP_OK);
    CHECK_ERROR_CODE(esp_task_wdt_status(NULL), ESP_ERR_INVALID_STATE);     //Confirm TWDT has been deinitialized

    printf("TWDT Deinitialized\n");
}

float micros()
{
    static struct timeval time_of_day;
    gettimeofday(&time_of_day, NULL);
    return time_of_day.tv_sec + time_of_day.tv_usec / 1000000.0;
}

uint32_t micros_i()
{
    static struct timeval time_of_day;
    gettimeofday(&time_of_day, NULL);
    return 1000000.0 * time_of_day.tv_sec + time_of_day.tv_usec;
}

uint32_t millis()
{
    static struct timeval time_of_day;
    gettimeofday(&time_of_day, NULL);
    return 1000.0 * time_of_day.tv_sec + time_of_day.tv_usec / 1000;
}

void enable_I2S_MCLK_clock() {
	printf("Codec Init: Enabling MCLK...");
	vTaskDelay(100 / portTICK_RATE_MS);

	/*
	periph_module_enable(PERIPH_LEDC_MODULE);
    ledc_timer_bit_t bit_num = (ledc_timer_bit_t) 2;      //normally 3
    int duty = pow(2, (int) bit_num) / 2;

    ledc_timer_config_t timer_conf;
    timer_conf.bit_num = bit_num;

    //timer_conf.freq_hz = 44100 * 256;
    timer_conf.freq_hz = 10;//SAMPLE_RATE /2;// * 4;
    //timer_conf.freq_hz = 48000 * 256; //= 12288000; //gives 8MHz
    //timer_conf.freq_hz = 48000 * 256; //= 12288000; //gives 8MHz
    //timer_conf.freq_hz = 18874368; // = 48000 * 256 * (48000 * 256 / 8000000) -> should give 12288000
    //timer_conf.freq_hz = 18874360; // = 48000 * 256 * (48000 * 256 / 8000000) -> should give 12288000
    //timer_conf.freq_hz = 44100 * 256 * (48000 * 256 / 8000000);
    //timer_conf.freq_hz = 48000 * 256 * (48000 * 256 / 8000000);
    //timer_conf.freq_hz = 18874368;//18874368;

    //timer_conf.freq_hz = 8192000; // 22579200

    timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    timer_conf.timer_num = LEDC_TIMER_0;
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        //ESP_LOGE(TAG, "ledc_timer_config failed, rc=%x", err);
    }

    ledc_channel_config_t ch_conf;
    ch_conf.channel = LEDC_CHANNEL_0;
    ch_conf.timer_sel = LEDC_TIMER_0;
    ch_conf.intr_type = LEDC_INTR_DISABLE;
    ch_conf.duty = duty;
    ch_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    ch_conf.gpio_num = MCLK_GPIO; //s_config.pin_xclk;
    err = ledc_channel_config(&ch_conf);
    if (err != ESP_OK) {
        //ESP_LOGE(TAG, "ledc_channel_config failed, rc=%x", err);
    }

	// Set the PWM to the duty specified
	ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
	*/

	/*
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[MCLK_GPIO], PIN_FUNC_GPIO); //normally U0TXD, need to change

    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, MCLK_GPIO);
    mcpwm_pin_config_t pin_config2 = {
    	.mcpwm0a_out_num = MCLK_GPIO
    };
    mcpwm_set_pin(MCPWM_UNIT_0, &pin_config2);
    gpio_pulldown_en(MCLK_GPIO);
    mcpwm_config_t pwm_config;
    //pwm_config.frequency = 48000;//100;//100Hz is enough to start PLL //(SAMPLE_RATE)*256; //when frequency is too high (actually only 1MHZ)there is no signal
    pwm_config.frequency = 96000;//100;//100Hz is enough to start PLL //(SAMPLE_RATE)*256; //when frequency is too high (actually only 1MHZ)there is no signal
    pwm_config.cmpr_a = 50.0;
    pwm_config.cmpr_b = 50.0;
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
	*/

	//experiments:

	//PIN_FUNC_SELECT(PIN_CTRL, CLK_OUT1_S);
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

	//REG_WRITE(PIN_CTRL, 0b111111110000);//0xff0
	////PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1); //enable CLK_OUT1 function
	//PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[MCLK_GPIO], FUNC_GPIO0_CLK_OUT1); //normally U0TXD, need to change

	//PIN_FUNC_SELECT(PIN_CTRL, CLK_OUT1);
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

	////PIN_FUNC_SELECT(PIN_CTRL, CLK_OUT3_S);
	//PIN_FUNC_SELECT(PIN_CTRL, CLK_OUT3);
	////REG_WRITE(PIN_CTRL, 0b111111111000);//0xff8
	////PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD_CLK_OUT3);
	//PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[MCLK_GPIO], FUNC_U0TXD_CLK_OUT3);

	//REG_WRITE(PIN_CTRL, 0b111111110000); //???
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1); //enable CLK_OUT1 function

	//-------------------------------------------------------------------------
	//https://esp32.com/viewtopic.php?f=5&t=1585&start=10
	//PIN_FUNC_SELECT(PIN_CTRL, CLK_OUT1_S);
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

	#ifdef BOARD_WHALE
	REG_WRITE(PIN_CTRL, 0b000000000000);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD_CLK_OUT3);
	#else //if BOARD_GECHO
	REG_WRITE(PIN_CTRL, 0b111111110000);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
	#endif

	//-------------------------------------------------------------------------

	vTaskDelay(100 / portTICK_RATE_MS);
    printf(" done\n");
}

void disable_I2S_MCLK_clock()
{
	printf("disable_out_clock(): Disabling MCLK... ");

	//method 0: LEDC driver
	//periph_module_disable(PERIPH_LEDC_MODULE);

	//method 1: pwm generator - works in whale, not here
	//mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);

	//method 2: CLK_OUT - works here
	#ifdef BOARD_WHALE
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD_U0TXD); //back to TXD function
	#else //if BOARD_GECHO
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_GPIO0_0); //back to GPIO function
	#endif

	printf(" done\n");
}

void i2c_master_init(int speed)
{
	printf("I2C Master Init...");

	int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;//GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;//GPIO_PULLUP_ENABLE;
    conf.clk_flags = 0;

    if(speed)
    {
    	printf(" FAST MODE (1MHz)");
    	conf.master.clk_speed = I2C_MASTER_FREQ_HZ_FAST;
    }
    else
    {
    	printf(" NORMAL MODE (400kHz)");
    	conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    }

    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_MASTER_RX_BUF_DISABLE,
                       I2C_MASTER_TX_BUF_DISABLE, 0);

    printf(" driver installed\n");
    i2c_driver_installed = 1;
}

void i2c_master_deinit()
{
	i2c_driver_installed = 0;
	printf("I2C Master Deinit...");

	int i2c_master_port = I2C_MASTER_NUM;
    i2c_driver_delete(i2c_master_port);

    printf(" driver uninstalled\n");
}

void i2c_scan_for_devices(int print, uint8_t *addresses, uint8_t *found)
{
	int ret;
	if(addresses && found)
	{
		found[0] = 0;
	}
    for(uint16_t address = 0;address<128;address++)
	{
    	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK_EN); //set W/R bit to write
        i2c_master_write_byte(cmd, 0x00, ACK_CHECK_EN); //send a zero byte
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        if(ret == ESP_OK)
		{
			if(print)
			{
				printf("device responds at address %u\n",address);
			}
			if(addresses && found)
			{
				if(address)
				{
					addresses[found[0]] = address;
					found[0]++;
				}
			}
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
		else //e.g. ESP_FAIL
		{
		}
	}
	if(print)
	{
		printf("\nScan done.\n");
	}
}

int i2c_codec_two_byte_command(uint8_t b1, uint8_t b2)
{
	//printf("i2c_codec_two_byte_command(reg=%x(%d), val=%x(%d): ", b1, b1, b2, b2);

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, CODEC_I2C_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, b1, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, b2, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    int ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    //printf("result=%x(%d)\n", ret, ret);
    return ret;
}

uint8_t i2c_codec_register_read(uint8_t reg)
{
	uint8_t val;
	//printf("i2c_codec_register_read(reg=%x(%d): ", reg, reg);

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, CODEC_I2C_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, CODEC_I2C_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &val, I2C_MASTER_NACK);
	i2c_master_stop(cmd);
    int ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if(ret!=ESP_OK)
    {
    	printf("ERROR=%x(%d)\n-------------------------------------------------\n", ret, ret);
    	return 0;
    }

    //printf("result=%x(%d)\n", val, val);
    return val;
}

int i2c_bus_write(int addr_rw, unsigned char *data, int len)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, addr_rw, ACK_CHECK_EN);
	i2c_master_write(cmd, data, len, ACK_CHECK_EN);

	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if(ret == ESP_OK)
	{
		//printf("[i2c_bus_write]: I2C command %x accepted at address %x \n", data[0], addr_rw);
	}
	else //e.g. ESP_FAIL
	{
		printf("[i2c_bus_write]: I2C command %x NOT accepted at address %x \n", data[0], addr_rw);
	}
	return ret;
}

int i2c_bus_read(int addr_rw, unsigned char *buf, int buf_len)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, addr_rw, ACK_CHECK_EN);

	//i2c_master_read(cmd, buf, buf_len, I2C_MASTER_ACK);
	//i2c_master_read(cmd, buf, buf_len, I2C_MASTER_NACK);
	i2c_master_read(cmd, buf, buf_len, I2C_MASTER_LAST_NACK);
    //I2C_MASTER_ACK = 0x0,        /*!< I2C ack for each byte read */
    //I2C_MASTER_NACK = 0x1,       /*!< I2C nack for each byte read */
    //I2C_MASTER_LAST_NACK = 0x2,   /*!< I2C nack for the last byte*/

	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if(ret == ESP_OK)
	{
		//printf("[i2c_bus_read]: I2C data %x received from address %x \n", buf[0], addr_rw);
	}
	else //e.g. ESP_FAIL
	{
		printf("[i2c_bus_read]: I2C data NOT received from address %x \n", addr_rw);
	}
	return ret;
}

void sha1_to_hex(char *code_sha1_hex, uint8_t *code_sha1)
{
	char hex_char[3];

	for (int i=0;i<20;i++)
	{
		//sprintf(code_sha1_hex+(2*i), "%02x", code_sha1[i]); //"%02x" modifier won't work here

		sprintf(hex_char, "%x", code_sha1[i]);

		if (strlen(hex_char)==2)
		{
			strncpy(code_sha1_hex+(2*i), hex_char, 2);
		}
		else if (strlen(hex_char)==1)
		{
			(code_sha1_hex+(2*i))[0] = '0';
			(code_sha1_hex+(2*i))[1] = hex_char[0];
		}
		else
		{
			strncpy(code_sha1_hex+(2*i), "00\0", 3);
		}
	}
	code_sha1_hex[40] = 0;
}

#ifdef BOARD_GECHO
const char *BINARY_ID = "[0x000000001234]\n\0\0";
const char* FW_VERSION = "[Gv2/1.0.118]";
const char* GLO_HASH = "f495dc2975eec3287700ad297a9134a037a3a411";
#endif

uint16_t hardware_check()
{
	uint8_t adr[20];
	uint8_t found;
	i2c_scan_for_devices(0, adr, &found);
	printf("hardware_check(): found %d i2c devices: ", found);

	#ifdef BOARD_GECHO_V179
	#ifdef BOARD_GECHO_V181
	#define I2C_DEVICES_EXPECTED 4
	uint8_t expected[I2C_DEVICES_EXPECTED] = {0x18,0x19,0x21,0x27};
	#else
	#define I2C_DEVICES_EXPECTED 4
	uint8_t expected[I2C_DEVICES_EXPECTED] = {0x18,0x19,0x60,0x67};
	#endif
	#else
	#define I2C_DEVICES_EXPECTED 6
	uint8_t expected[I2C_DEVICES_EXPECTED] = {0x18,0x1f,0x38,0x39,0x3b,0x3f};
	#endif
	uint8_t as_expected = 0;
	uint8_t i2c_errors = 0;

	for(int i=0;i<found;i++)
	{
		printf("%02x ", adr[i]);
		if(adr[i]==expected[i])
		{
			as_expected++;
		}
		else
		{
			i2c_errors += 1<<i;
		}
	}
	printf("\n");

	printf("hardware_check(): as_expected = %d\n", as_expected);

	if(found!=I2C_DEVICES_EXPECTED || as_expected!=I2C_DEVICES_EXPECTED)
	{
		printf("hardware_check(): errors found!\n");
		return i2c_errors + ((I2C_DEVICES_EXPECTED-found) << 8);
	}
	else
	{
		return 0;
	}
}

void clear_unallocated_memory()
{
	void *tmp1=NULL, *tmp2=NULL;

	int largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
	printf("clear_unallocated_memory(): heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT) returns %u\n", largest_block);

	if(largest_block>0)
	{
		tmp1 = malloc(largest_block);
		printf("clear_unallocated_memory(): tmp1 alocated: %p\n", tmp1);
		memset(tmp1,0,largest_block);
	}

	largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
	printf("clear_unallocated_memory(): heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT) returns %u\n", largest_block);

	if(largest_block>0)
	{
		tmp2 = malloc(largest_block);
		printf("clear_unallocated_memory(): tmp2 alocated: %p\n", tmp2);
		memset(tmp2,0,largest_block);
	}
	if(tmp1)
	{
		free(tmp1);
		printf("clear_unallocated_memory(): tmp1 freed\n");
	}
	if(tmp2)
	{
		free(tmp2);
		printf("clear_unallocated_memory(): tmp2 freed\n");
	}
}

/*
void spdif_transceiver_init()
{
    unsigned char buffer[5];
    unsigned char reg;
    int error;

    i2c_num = I2C_MASTER_NUM;
	i2c_master_init(0);

    vTaskDelay(10 / portTICK_RATE_MS);
	printf("WM8804G SPDIF Transceiver Init: Configuring...\n");

	int ret;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SPDIF_TX_I2C_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x00, ACK_CHECK_EN); //RST/DEVID1
    i2c_master_write_byte(cmd, 0x00, ACK_CHECK_DIS); //reset the device
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    //if (ret != ESP_OK && ret != ESP_FAIL) { //fail is probably OK as the device doesn't ask, it gets reset
    if (ret != ESP_OK) { //not expecting ACK anyway
    	printf("WM8804G [RESET DEVICE]: command failed, err = %d\n",ret);
    }

    vTaskDelay(100 / portTICK_RATE_MS);

    //------------------- PWR CONFIG ---------------------------------------

    //Register 30 (0x1e): PWRDN

	//D7-D6: n/a

	//D5: TRIOP - Tri-state all outputs
    //0: Outputs not tri-stated
    //1: Outputs tri-stated

	//D4: AIFPD - Digital audio interface power down
    //0: Power up
    //1: Power down

	//D3: OSCPD - Oscillator power down
    //0: Power up
    //1: Power down

	//D2: SPDIFTXPD - S/PDIF transmitter power down
    //0: S/PDIF transmitter enabled
	//1: S/PDIF transmitter disabled

	//D1: SPDIFRXPD - S/PDIF receiver power down
    //0: S/PDIF receiver enabled
	//1: S/PDIF receiver disabled

	//D0: PLLPD - PLL power down
    //0: PLL enabled
	//1: PLL disabled

	buffer[0] = 0x1e;
	buffer[1] = 0x02; //00000010 //enable PLL, AIF, OSC and TX
	//buffer[1] = 0x0b; //00001011 //enable AIF and TX
    ret = i2c_bus_write( (int)((SPDIF_TX_I2C_ADDRESS << 1) | WRITE_BIT), (unsigned char*)&buffer, 2 );

	if (ret != ESP_OK) {
    	printf("WM8804G [PWRDN]: Power up command error = %d\n", ret);
    }

	//------------------- S/PDIF TX CONFIG ---------------------------------

    //Register 21 (0x15): SPDTX4

	//default: 01110001

	//D6: S/PDIF Transmitter Data Source
	//0: S/PDIF Received Data
	//1: Digital AIF Received Data

	// nothing to do, default values fine

	//------------------- PLL CONFIG ---------------------------------------

	// PLL not used

	//------------------- GP0 PIN CONFIG -----------------------------------

    //Register 23 (0x17): GPO0

	//default: 01110000

	//D7-D4: 0111 (default)
	//D3-D0: 0000 - INT_N (interrupt flag)
	//D3-D0: 1111 - always 0 (logic level)

	buffer[0] = 0x17;
	buffer[1] = 0x7f; //01111111 - set GPO0 to logic 0 to not conflict with "SW mode" select at reset (it's wired to GND)
    ret = i2c_bus_write( (int)((SPDIF_TX_I2C_ADDRESS << 1) | WRITE_BIT), (unsigned char*)&buffer, 2 );

	if (ret != ESP_OK) {
    	printf("WM8804G [GPO0]: Pin config command error = %d\n", ret);
    }

	//------------------- AIFRX CONFIG -------------------------------------

    //Register 28 (0x1c): AIFRX

	//default: 00000110

	//D7: SYNC_OFF = 0: LRCLK, BCLK are not output when S/PDIF source has been removed (relevant in master mode only)
	//D6: AIF_MS = 0: Slave Mode
	//D5: AIFRX_LRP = 0: normal LRCLK polarity / DSP Mode A
	//D4: AIFRX_BCP = 0: BCLK not inverted
	//D3-D2: AIFRX_WL = 00: 16 bits
	//D1-D0: AIFRX_FMT = 10: I2S mode

	buffer[0] = 0x1c;
	buffer[1] = 0x02; //00000010 - slave mode, 16bit, i2s
    ret = i2c_bus_write( (int)((SPDIF_TX_I2C_ADDRESS << 1) | WRITE_BIT), (unsigned char*)&buffer, 2 );

	if (ret != ESP_OK) {
    	printf("WM8804G [AIFRX]: Register config command error = %d\n", ret);
    }

	//------------------- AIFTX CONFIG -------------------------------------

    //Register 27 (0x1b): AIFTX

	//default: 00000110

	//D7-D6: 00 (reserved)
	//D5: AIFTX_LRP = 0: normal LRCLK polarity / DSP Mode A
	//D4: AIFTX_BCP = 0: BCLK not inverted
	//D3-D2: AIFTX_WL = 00: 16 bits
	//D1-D0: AIFTX_FMT = 10: I2S mode

	buffer[0] = 0x1b;
	buffer[1] = 0x02; //00000010 - slave mode, 16bit, i2s
    ret = i2c_bus_write( (int)((SPDIF_TX_I2C_ADDRESS << 1) | WRITE_BIT), (unsigned char*)&buffer, 2 );

	if (ret != ESP_OK) {
    	printf("WM8804G [AIFTX]: Register config command error = %d\n", ret);
    }

	//------------------- READ IDs -----------------------------------------

    reg = 0x00;
    i2c_bus_write( (int)((SPDIF_TX_I2C_ADDRESS << 1) | WRITE_BIT), (unsigned char*)&reg, (int)1 );
    error = i2c_bus_read( (int)((SPDIF_TX_I2C_ADDRESS << 1) | READ_BIT), (unsigned char*)buffer, 1);
    if(error)
    {
    	printf("WM8804G [READ IDs]: error = %d\n", error);
    }
	printf("WM8804G [READ IDs]: DEVID1 = %x\n", buffer[0]);

	reg = 0x01;
    i2c_bus_write( (int)((SPDIF_TX_I2C_ADDRESS << 1) | WRITE_BIT), (unsigned char*)&reg, (int)1 );
    error = i2c_bus_read( (int)((SPDIF_TX_I2C_ADDRESS << 1) | READ_BIT), (unsigned char*)buffer, 1);
    if(error)
    {
    	printf("WM8804G [READ IDs]: error = %d\n", error);
    }
	printf("WM8804G [READ IDs]: DEVID2 = %x\n", buffer[0]);

	i2c_master_deinit();
    vTaskDelay(1000 / portTICK_RATE_MS);
}
*/

#ifdef BOARD_WHALE
void whale_test_RGB()
{
	int l=0;//, result;
	while(1)
	{
		RGB_LED_R_ON;
		Delay(50);
		RGB_LED_R_OFF;
		Delay(50);
		RGB_LED_G_ON;
		Delay(50);
		RGB_LED_G_OFF;
		Delay(50);
		RGB_LED_B_ON;
		Delay(50);
		RGB_LED_B_OFF;
		Delay(50);
		/*
		result = gpio_set_level(GPIO_NUM_13,0);//RGB_LED_R_ON;
		printf("r=%d,",result);
		Delay(250);
		result = gpio_set_level(GPIO_NUM_13,1);//RGB_LED_R_OFF;
		printf("r=%d,",result);
		Delay(250);
		result = gpio_set_level(GPIO_NUM_14,0);//RGB_LED_G_ON;
		printf("r=%d,",result);
		Delay(250);
		result = gpio_set_level(GPIO_NUM_14,1);//RGB_LED_G_OFF;
		printf("r=%d,",result);
		Delay(250);
		result = gpio_set_level(GPIO_NUM_15,0);//RGB_LED_B_ON;
		printf("r=%d,",result);
		Delay(250);
		result = gpio_set_level(GPIO_NUM_15,1);//RGB_LED_B_OFF;
		printf("r=%d,",result);
		Delay(250);
		*/

		int val;
		val = gpio_get_level(GPIO_NUM_0);
		printf("GPIO0=%d,",val);
		val = gpio_get_level(GPIO_NUM_34);
		printf("GPIO34=%d,",val);
		val = gpio_get_level(GPIO_NUM_35);
		printf("GPIO35=%d\n",val);

		printf("loop[%d]...\n",l++);
	}
}
#endif

#ifdef BOARD_GECHO

#ifdef BOARD_GECHO_V179
#ifdef BOARD_GECHO_V181
#define EXP1_I2C_ADDRESS 0x21 //33
#define EXP2_I2C_ADDRESS 0x27 //39
#else
#define EXP1_I2C_ADDRESS 0x60 //96
#define EXP2_I2C_ADDRESS 0x67 //103
#endif
#else
//PCA9554A at 0x38, 0x39, 0x3B, 0x3F
#define EXP1_I2C_ADDRESS 0x38 //56
#define EXP2_I2C_ADDRESS 0x3B //59
#define EXP3_I2C_ADDRESS 0x39 //57
#define EXP4_I2C_ADDRESS 0x3F //63
#endif

void LED_exp_send_address(i2c_cmd_handle_t cmd, uint8_t rw_bit, int exp_no)
{
    if(exp_no==1)
    {
    	i2c_master_write_byte(cmd, EXP1_I2C_ADDRESS << 1 | rw_bit, ACK_CHECK_EN);
    }
    else if(exp_no==2)
    {
    	i2c_master_write_byte(cmd, EXP2_I2C_ADDRESS << 1 | rw_bit, ACK_CHECK_EN);
    }
	#ifndef BOARD_GECHO_V179
    else if(exp_no==3)
    {
    	i2c_master_write_byte(cmd, EXP3_I2C_ADDRESS << 1 | rw_bit, ACK_CHECK_EN);
    }
    else if(exp_no==4)
    {
    	i2c_master_write_byte(cmd, EXP4_I2C_ADDRESS << 1 | rw_bit, ACK_CHECK_EN);
    }
	#endif
}

void LED_exp_2_byte_cmd(uint8_t b1, uint8_t b2, int exp_no)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    LED_exp_send_address(cmd, WRITE_BIT, exp_no);

    i2c_master_write_byte(cmd, b1, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, b2, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    /*int ret =*/ i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}

void LED_exp_multi_byte_cmd(uint8_t reg, uint8_t *bytes, uint8_t count, int exp_no)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    LED_exp_send_address(cmd, WRITE_BIT, exp_no);

    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    for(int i=0;i<count;i++)
    {
    	i2c_master_write_byte(cmd, bytes[i], ACK_CHECK_EN);
    }
    i2c_master_stop(cmd);
    /*int ret =*/ i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}

uint8_t LED_exp_read_register(uint8_t reg, int exp_no)
{
    uint8_t val;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    LED_exp_send_address(cmd, WRITE_BIT, exp_no);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);

    i2c_master_start(cmd);
    LED_exp_send_address(cmd, READ_BIT, exp_no);
    i2c_master_read_byte(cmd, &val, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    /*int ret =*/ i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return val;
}

void LED_exp_read_two_registers(uint8_t reg, int exp_no, uint8_t *d1, uint8_t *d2)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    LED_exp_send_address(cmd, WRITE_BIT, exp_no);
    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);

    i2c_master_start(cmd);
    LED_exp_send_address(cmd, READ_BIT, exp_no);
    i2c_master_read_byte(cmd, d1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, d2, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    /*int ret =*/ i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}

unsigned char byte_bit_reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

extern persistent_settings_t persistent_settings;

void process_expanders(void *pvParameters)
{
	printf("process_expanders(): task running on core ID=%d\n",xPortGetCoreID());

	uint8_t tmp;
	uint8_t reg,reg2,reg3,LEDs_changed;
	//int was_int = 0;

	#ifdef BOARD_GECHO_V179
	#ifndef BOARD_GECHO_V181
	for(int i=0;i<EXPANDERS*2;i++)
	{
		exp_bits[i] = 0x55; //LEDs are Off by default, GPIOs at input
	}
	#endif
	#endif

	while(1)
	{
		Delay(EXPANDERS_TIMING_DELAY);

		if(i2c_bus_mutex)
		{
			continue;
		}
		i2c_bus_mutex = 1;

		#ifdef BOARD_GECHO_V179

		/*
		reg = ~LED_exp_read_register(0x00, 2); //expander #2 (top) has two buttons connected
		//printf("exp #2 reg 0/1: %02x - ", reg);
		reg2 = ~LED_exp_read_register(0x01, 2);
		//printf("%02x\t", reg2);
		*/

		//read both registers at once
		LED_exp_read_two_registers(0x00, 2, &reg, &reg2);
		reg = ~reg;
		reg2 = ~reg2;

		//ff,ff vs. fd,7f when pressed

		//reg0 = LED_exp_read_register(0x00, 1); //expander #1 (bottom) has four buttons connected
		//printf("exp #1 reg 0/1: %02x - ", reg0);
		reg3 = ~LED_exp_read_register(0x01, 1);
		//printf("%02x\n", reg3);

		//ef-df-bf-7f when pressed

		Buttons_bits = ((reg2>>7)&0x01) | ((reg<<4)&0x20) | (reg3&0x10) | ((reg3>>2)&0x08)| ((reg3>>4)&0x04)| ((reg3>>6)&0x02);
		//printf("process_expanders(): Buttons_bits = %x\n", Buttons_bits);

		if(!persistent_settings.ALL_LEDS_OFF)
		{
			//==== LEDs ===============================================================================

			LEDs_changed = 0;

			//B1B2W1W2,B3B4W3W4,W5-W8,B(1-4),B5R7R8,R5R6O3O4,R3R4O1O2,RDY-R1R2

			for(int i=0;i<EXPANDERS;i++)
			if(LED_bits[i]!=LED_bits[i+EXPANDERS])
			{
				LEDs_changed++;

				#ifndef BOARD_GECHO_V181

				if(i==MAP_ORANGE_LEDS)
				{
					//higher 5 bits in orange register, order = 1,2,3,4,RDY
					if(LED_bits[MAP_ORANGE_LEDS]&0x80) { exp_bits[6] &= 0xf3; } else { exp_bits[6] |= 0x04; }
					if(LED_bits[MAP_ORANGE_LEDS]&0x40) { exp_bits[6] &= 0xfc; } else { exp_bits[6] |= 0x01; }
					if(LED_bits[MAP_ORANGE_LEDS]&0x20) { exp_bits[5] &= 0x3f; } else { exp_bits[5] |= 0x40; }
					if(LED_bits[MAP_ORANGE_LEDS]&0x10) { exp_bits[5] &= 0xcf; } else { exp_bits[5] |= 0x10; }

					//RDY
					if(LED_bits[MAP_ORANGE_LEDS]&0x08) { exp_bits[7] &= 0xcf; } else { exp_bits[7] |= 0x10; }
				}

				if(i==MAP_RED_LEDS)
				{

					if(LED_bits[MAP_RED_LEDS]&0x80) { exp_bits[7] &= 0xf3; } else { exp_bits[7] |= 0x04; } //R1
					if(LED_bits[MAP_RED_LEDS]&0x40) { exp_bits[7] &= 0xfc; } else { exp_bits[7] |= 0x01; } //R2
					if(LED_bits[MAP_RED_LEDS]&0x20) { exp_bits[6] &= 0x3f; } else { exp_bits[6] |= 0x40; } //R3
					if(LED_bits[MAP_RED_LEDS]&0x10) { exp_bits[6] &= 0xcf; } else { exp_bits[6] |= 0x10; } //R4
					if(LED_bits[MAP_RED_LEDS]&0x08) { exp_bits[5] &= 0xf3; } else { exp_bits[5] |= 0x04; } //R5
					if(LED_bits[MAP_RED_LEDS]&0x04) { exp_bits[5] &= 0xfc; } else { exp_bits[5] |= 0x01; } //R6
					if(LED_bits[MAP_RED_LEDS]&0x02) { exp_bits[4] &= 0x3f; } else { exp_bits[4] |= 0x40; } //R7
					if(LED_bits[MAP_RED_LEDS]&0x01) { exp_bits[4] &= 0xcf; } else { exp_bits[4] |= 0x10; } //R8
				}

				if(i==MAP_WHITE_LEDS)
				{
					if(LED_bits[MAP_WHITE_LEDS]&0x80) { exp_bits[0] &= 0xcf; } else { exp_bits[0] |= 0x10; } //W1
					if(LED_bits[MAP_WHITE_LEDS]&0x40) { exp_bits[0] &= 0x3f; } else { exp_bits[0] |= 0x40; } //W2
					if(LED_bits[MAP_WHITE_LEDS]&0x20) { exp_bits[1] &= 0xfc; } else { exp_bits[1] |= 0x01; } //W3
					if(LED_bits[MAP_WHITE_LEDS]&0x10) { exp_bits[1] &= 0xf3; } else { exp_bits[1] |= 0x04; } //W4
					if(LED_bits[MAP_WHITE_LEDS]&0x08) { exp_bits[2] &= 0x3f; } else { exp_bits[2] |= 0x40; } //W5
					if(LED_bits[MAP_WHITE_LEDS]&0x04) { exp_bits[2] &= 0xcf; } else { exp_bits[2] |= 0x10; } //W6
					if(LED_bits[MAP_WHITE_LEDS]&0x02) { exp_bits[2] &= 0xf3; } else { exp_bits[2] |= 0x04; } //W7
					if(LED_bits[MAP_WHITE_LEDS]&0x01) { exp_bits[2] &= 0xfc; } else { exp_bits[2] |= 0x01; } //W8
				}

				if(i==MAP_BLUE_LEDS)
				{
					if(LED_bits[MAP_BLUE_LEDS]&0x80) { exp_bits[0] &= 0xfc; } else { exp_bits[0] |= 0x01; } //B1
					if(LED_bits[MAP_BLUE_LEDS]&0x40) { exp_bits[0] &= 0xf3; } else { exp_bits[0] |= 0x04; } //B2
					if(LED_bits[MAP_BLUE_LEDS]&0x20) { exp_bits[1] &= 0xcf; } else { exp_bits[1] |= 0x10; } //B3
					if(LED_bits[MAP_BLUE_LEDS]&0x10) { exp_bits[1] &= 0x3f; } else { exp_bits[1] |= 0x40; } //B4
					if(LED_bits[MAP_BLUE_LEDS]&0x08) { exp_bits[4] &= 0xfc; } else { exp_bits[4] |= 0x01; } //B5
				}

				#else

				if(i==MAP_ORANGE_LEDS)
				{
					if(LED_bits[MAP_ORANGE_LEDS]&0x80) { exp_bits[5] &= 0xfd; } else { exp_bits[5] |= 0x02; } //O1 -> IO1.1
					if(LED_bits[MAP_ORANGE_LEDS]&0x40) { exp_bits[5] &= 0xfe; } else { exp_bits[5] |= 0x01; } //O2 -> IO1.0
					if(LED_bits[MAP_ORANGE_LEDS]&0x20) { exp_bits[4] &= 0x7f; } else { exp_bits[4] |= 0x80; } //O3 -> IO0.7
					if(LED_bits[MAP_ORANGE_LEDS]&0x10) { exp_bits[4] &= 0xbf; } else { exp_bits[4] |= 0x40; } //O4 -> IO0.6

					//RDY
					if(LED_bits[MAP_ORANGE_LEDS]&0x08) { exp_bits[5] &= 0xbf; } else { exp_bits[5] |= 0x40; } //RDY!
				}

				if(i==MAP_RED_LEDS)
				{
					if(LED_bits[MAP_RED_LEDS]&0x80) { exp_bits[5] &= 0xdf; } else { exp_bits[5] |= 0x20; } //R1!
					if(LED_bits[MAP_RED_LEDS]&0x40) { exp_bits[5] &= 0xef; } else { exp_bits[5] |= 0x10; } //R2!
					if(LED_bits[MAP_RED_LEDS]&0x20) { exp_bits[5] &= 0xf7; } else { exp_bits[5] |= 0x08; } //R3!
					if(LED_bits[MAP_RED_LEDS]&0x10) { exp_bits[5] &= 0xfb; } else { exp_bits[5] |= 0x04; } //R4!
					if(LED_bits[MAP_RED_LEDS]&0x08) { exp_bits[4] &= 0xdf; } else { exp_bits[4] |= 0x20; } //R5!
					if(LED_bits[MAP_RED_LEDS]&0x04) { exp_bits[4] &= 0xef; } else { exp_bits[4] |= 0x10; } //R6!
					if(LED_bits[MAP_RED_LEDS]&0x02) { exp_bits[4] &= 0xf7; } else { exp_bits[4] |= 0x08; } //R7!
					if(LED_bits[MAP_RED_LEDS]&0x01) { exp_bits[4] &= 0xfb; } else { exp_bits[4] |= 0x04; } //R8!
				}

				if(i==MAP_WHITE_LEDS)
				{
					if(LED_bits[MAP_WHITE_LEDS]&0x80) { exp_bits[0] &= 0xfb; } else { exp_bits[0] |= 0x04; } //W1!
					if(LED_bits[MAP_WHITE_LEDS]&0x40) { exp_bits[0] &= 0xf7; } else { exp_bits[0] |= 0x08; } //W2!
					if(LED_bits[MAP_WHITE_LEDS]&0x20) { exp_bits[0] &= 0xef; } else { exp_bits[0] |= 0x10; } //W3!
					if(LED_bits[MAP_WHITE_LEDS]&0x10) { exp_bits[0] &= 0xdf; } else { exp_bits[0] |= 0x20; } //W4!
					if(LED_bits[MAP_WHITE_LEDS]&0x08) { exp_bits[1] &= 0xf7; } else { exp_bits[1] |= 0x08; } //W5!
					if(LED_bits[MAP_WHITE_LEDS]&0x04) { exp_bits[1] &= 0xfb; } else { exp_bits[1] |= 0x04; } //W6!
					if(LED_bits[MAP_WHITE_LEDS]&0x02) { exp_bits[1] &= 0xfd; } else { exp_bits[1] |= 0x02; } //W7!
					if(LED_bits[MAP_WHITE_LEDS]&0x01) { exp_bits[1] &= 0xfe; } else { exp_bits[1] |= 0x01; } //W8!
				}

				if(i==MAP_BLUE_LEDS)
				{
					if(LED_bits[MAP_BLUE_LEDS]&0x80) { exp_bits[0] &= 0xfe; } else { exp_bits[0] |= 0x01; } //B1!
					if(LED_bits[MAP_BLUE_LEDS]&0x40) { exp_bits[0] &= 0xfd; } else { exp_bits[0] |= 0x02; } //B2!
					if(LED_bits[MAP_BLUE_LEDS]&0x20) { exp_bits[0] &= 0xbf; } else { exp_bits[0] |= 0x40; } //B3!
					if(LED_bits[MAP_BLUE_LEDS]&0x10) { exp_bits[0] &= 0x7f; } else { exp_bits[0] |= 0x80; } //B4!
					if(LED_bits[MAP_BLUE_LEDS]&0x08) { exp_bits[4] &= 0xfe; } else { exp_bits[4] |= 0x01; } //B5!
				}

				#endif

				LED_bits[i+EXPANDERS] = LED_bits[i];
			}

			if(LEDs_changed) //just check if any of the LEDs status changed, otherwise no need to update anything
			{
				//more optimal, setting all 4 registers in each expander at once
				for(int e=0;e<EXPANDERS/2;e++) //expander no. (2 bits per signal)
				{
					#ifdef BOARD_GECHO_V181
					//LED_exp_multi_byte_cmd(PCA9555_LED_REGS, exp_bits+(e*EXPANDERS), 1, e+1);
					//LED_exp_multi_byte_cmd(PCA9555_LED_REGS+1, exp_bits+1+(e*EXPANDERS), 1, e+1);
					//PCA9555 supports multi register writes too
					LED_exp_multi_byte_cmd(PCA9555_LED_REGS, exp_bits+(e*EXPANDERS), 2, e+1);
					#else
					//PCA9552 supports multi register writes
					LED_exp_multi_byte_cmd(PCA9552_LED_REGS+PCA9552_AUTO_INC_FLAG, exp_bits+(e*EXPANDERS), 2, e+1); //write 2 regs at once
					#endif
				}
				//LED_exp_multi_byte_cmd(PCA9552_LED_REGS+PCA9552_AUTO_INC_FLAG, exp_bits, 2, 1);
				//LED_exp_multi_byte_cmd(PCA9552_LED_REGS+PCA9552_AUTO_INC_FLAG, exp_bits+EXPANDERS, 2, 2);

				/*
				//less optimal, setting each register one by one
				for(int i=0;i<EXPANDERS;i++)
				{
					LED_exp_2_byte_cmd(PCA9552_LED_REGS+i,exp_bits[i],1);
					LED_exp_2_byte_cmd(PCA9552_LED_REGS+i,exp_bits[i+EXPANDERS],2);
				}
				*/
			}

			//=========================================================================================
		}

		#else
		/*
		//check for INT from 4th expander (active low)
		if(!gpio_get_level(EXP4_INT))
		{
			printf("process_expanders(): EXP4_INT detected\n");
			reg = LED_exp_read_register(0x00, 4);
			printf("process_expanders(): EXP4 input register = %x\n", reg);
			Buttons_bits = ~reg & 0x3f;
			was_int = 1;
		}
		//update buttons bits map in case one was released since the last interrupt occured and has not generated another interrupt
		else if(was_int)
		{*/
			reg = LED_exp_read_register(0x00, 4);
			Buttons_bits = ~reg & 0x3f;
			/*was_int = 0;
		}*/
		//printf("process_expanders(): Buttons_bits = %x\n", Buttons_bits);

		if(!persistent_settings.ALL_LEDS_OFF)
		{

			for(int i=0;i<EXPANDERS;i++)
			if(LED_bits[i]!=LED_bits[i+EXPANDERS])
			{
				if(i==MAP_RED_LEDS)
				{
					//direct mapping, just reversed
					exp_bits[EXP_RED_LEDS] = byte_bit_reverse(LED_bits[MAP_RED_LEDS]);
				}

				if(i==MAP_WHITE_LEDS)
				{
					//first 4 need reversing, last 4 good
					tmp = 0xf0 & byte_bit_reverse(LED_bits[MAP_WHITE_LEDS]>>4);
					exp_bits[EXP_WHITE_LEDS] = tmp | (LED_bits[MAP_WHITE_LEDS] & 0x0f);
				}

				if(i==MAP_ORANGE_LEDS)
				{
					//last 5 bits in orange register, order = 1,4,3,2,RDY
					if(LED_bits[MAP_ORANGE_LEDS]&0x80) { exp_bits[EXP_ORANGE_LEDS] |= 0x10; } else { exp_bits[EXP_ORANGE_LEDS] &= 0xef; }
					if(LED_bits[MAP_ORANGE_LEDS]&0x40) { exp_bits[EXP_ORANGE_LEDS] |= 0x02; } else { exp_bits[EXP_ORANGE_LEDS] &= 0xfd; }
					if(LED_bits[MAP_ORANGE_LEDS]&0x20) { exp_bits[EXP_ORANGE_LEDS] |= 0x04; } else { exp_bits[EXP_ORANGE_LEDS] &= 0xfb; }
					if(LED_bits[MAP_ORANGE_LEDS]&0x10) { exp_bits[EXP_ORANGE_LEDS] |= 0x08; } else { exp_bits[EXP_ORANGE_LEDS] &= 0xf7; }
					//RDY
					if(LED_bits[MAP_ORANGE_LEDS]&0x08) { exp_bits[EXP_ORANGE_LEDS] |= 0x01; } else { exp_bits[EXP_ORANGE_LEDS] &= 0xfe; }
				}

				if(i==MAP_GREEN_LEDS)
				{
					//1,2 are first in green register
					exp_bits[EXP_GREEN_LEDS] = LED_bits[MAP_GREEN_LEDS] & 0xc0;

					//3,4,5 are first in orange register
					tmp = 0xe0 & LED_bits[MAP_GREEN_LEDS]<<2;
					exp_bits[EXP_ORANGE_LEDS] = (exp_bits[EXP_ORANGE_LEDS] & 0x1f) | tmp;
				}

				LED_bits[i+EXPANDERS] = LED_bits[i];
			}


			for(int i=0;i<EXPANDERS;i++)
			if(exp_bits[i]!=exp_bits[i+EXPANDERS])
			{
				//printf("expander bits #%d changed, updating\n", i);
				LED_exp_2_byte_cmd(0x01,~exp_bits[i],i+1);
				exp_bits[i+EXPANDERS] = exp_bits[i];
			}
		}
		#endif

		i2c_bus_mutex = 0;
	}
}

void gecho_LED_expander_init()
{
	#ifndef BOARD_GECHO_V179 //there is no init required for PCA9552PW
	printf("gecho_LED_expander_init()\n");
	LED_exp_2_byte_cmd(0x03,0x00,1);
	LED_exp_2_byte_cmd(0x02,0x00,1);
	LED_exp_2_byte_cmd(0x01,0xff,1);
	LED_exp_2_byte_cmd(0x03,0x00,2);
	LED_exp_2_byte_cmd(0x02,0x00,2);
	LED_exp_2_byte_cmd(0x01,0xff,2);
	LED_exp_2_byte_cmd(0x03,0x00,3);
	LED_exp_2_byte_cmd(0x02,0x00,3);
	LED_exp_2_byte_cmd(0x01,0xff,3);
	LED_exp_2_byte_cmd(0x03,0x3f,4); //00x11111 -> two blue LEDs, n/a, buttons 1-4, SET button
	LED_exp_2_byte_cmd(0x02,0x00,4);
	LED_exp_2_byte_cmd(0x01,0xff,4);
	#endif

	#ifdef BOARD_GECHO_V181
	memset(exp_bits,0xff,EXPANDERS*2);	//clear expanders I/O map bits (reverse logic, LED off = 1)
	#else
	memset(exp_bits,0,EXPANDERS*2);	//clear expanders I/O map bits
	#endif

	memset(LED_bits,0,EXPANDERS*2);	//clear LEDs map bits
	Buttons_bits = 0;

	#ifdef BOARD_GECHO_V179

	uint8_t reg;

	while(i2c_bus_mutex)
	{
		Delay(10);
	}
	i2c_bus_mutex = 1;

	#ifndef BOARD_GECHO_V181

	//LEDs Off, configure as input
	LED_exp_2_byte_cmd(6,0x55,1);
	LED_exp_2_byte_cmd(7,0x55,1);
	LED_exp_2_byte_cmd(8,0x55,1);
	LED_exp_2_byte_cmd(9,0x55,1);
	LED_exp_2_byte_cmd(6,0x55,2);
	LED_exp_2_byte_cmd(7,0x55,2);
	LED_exp_2_byte_cmd(8,0x55,2);
	LED_exp_2_byte_cmd(9,0x55,2);

	#else

	//PCA9555 uses one-bit config registers

	//configure accordingly
	LED_exp_2_byte_cmd(6,0x00,1);
	LED_exp_2_byte_cmd(7,0xf0,1);
	LED_exp_2_byte_cmd(6,0x02,2);
	LED_exp_2_byte_cmd(7,0x80,2);

	/*
	//all LEDs off
	LED_exp_2_byte_cmd(2,0xff,1);
	LED_exp_2_byte_cmd(3,0xff,1);
	LED_exp_2_byte_cmd(2,0xff,2);
	LED_exp_2_byte_cmd(3,0xff,2);

	//configure all as input
	LED_exp_2_byte_cmd(6,0xff,1);
	LED_exp_2_byte_cmd(7,0xff,1);
	LED_exp_2_byte_cmd(6,0xff,2);
	LED_exp_2_byte_cmd(7,0xff,2);
	//polarity inversion
	LED_exp_2_byte_cmd(4,0xff,1);
	LED_exp_2_byte_cmd(5,0xff,1);
	LED_exp_2_byte_cmd(4,0xff,2);
	LED_exp_2_byte_cmd(5,0xff,2);
	//no polarity inversion
	LED_exp_2_byte_cmd(4,0x00,1);
	LED_exp_2_byte_cmd(5,0x00,1);
	LED_exp_2_byte_cmd(4,0x00,2);
	LED_exp_2_byte_cmd(5,0x00,2);
	*/
	#endif

	reg = LED_exp_read_register(0x06, 1); //expander #1 (bottom)
	printf("exp #1 reg 6-9: %02x - ", reg);
	reg = LED_exp_read_register(0x07, 1); //expander #1 (bottom)
	printf("%02x - ", reg);
	#ifndef BOARD_GECHO_V181
	reg = LED_exp_read_register(0x08, 1); //expander #1 (bottom)
	printf("%02x - ", reg);
	reg = LED_exp_read_register(0x09, 1); //expander #1 (bottom)
	printf("%02x\n", reg);
	#endif

	reg = LED_exp_read_register(0x06, 2); //expander #2 (top)
	printf("exp #2 reg 6-9: %02x - ", reg);
	reg = LED_exp_read_register(0x07, 2); //expander #2 (top)
	printf("%02x - ", reg);
	#ifndef BOARD_GECHO_V181
	reg = LED_exp_read_register(0x08, 2); //expander #2 (top)
	printf("%02x - ", reg);
	reg = LED_exp_read_register(0x09, 2); //expander #2 (top)
	printf("%02x\n", reg);
	#endif

	i2c_bus_mutex = 0;

	#endif

	//#ifndef BOARD_GECHO_V179 //TODO
	printf("gecho_LED_expander_init(): starting process_expanders task\n");
	xTaskCreatePinnedToCore((TaskFunction_t)&process_expanders, "process_expanders_task", 2048, NULL, PRIORITY_EXPANDERS_TASK, NULL, CPU_CORE_EXPANDERS_TASK);
	//#endif
}

void gecho_LED_expander_test()
{
	printf("gecho_LED_expander_test()\n");
	while(1)
	{
		//printf(".\n");

		#ifdef BOARD_GECHO_V179

		printf("gecho_LED_expander_test() loop start\n");

		for(int e=1;e<=2;e++) //expander no.
		{
			#ifndef BOARD_GECHO_V181
			for(int i=0;i<4;i++) //segment no.
			{
				LED_exp_2_byte_cmd(PCA9552_LED_REGS+i,0x00,e); //LEDs On
				Delay(250);
				LED_exp_2_byte_cmd(PCA9552_LED_REGS+i,0x55,e); //LEDs Off
				Delay(50);
			}
			#endif
		}
		printf("gecho_LED_expander_test() loop end\n");
		Delay(1000);

		//B1B2W1W2,B3B4W3W4,W5-W8,B(1-4),B5R7R8,R5R6O3O4,R3R4O1O2,RDY-R1R2

		#else

		//8 red
		LED_bits[MAP_RED_LEDS] = 0x80; //first red LED on
		Delay(LED_TEST_DELAY);
		for(int i=1;i<8;i++)
		{
			LED_bits[MAP_RED_LEDS]>>=1;
			Delay(LED_TEST_DELAY);
		}
		LED_bits[MAP_RED_LEDS] = 0; //all red LEDs off

		//5 orange (including RDY)
		LED_bits[MAP_ORANGE_LEDS] = 0x80; //first orange LED on
		Delay(LED_TEST_DELAY);
		for(int i=1;i<5;i++)
		{
			LED_bits[MAP_ORANGE_LEDS]>>=1;
			Delay(LED_TEST_DELAY);
		}
		LED_bits[MAP_ORANGE_LEDS] = 0; //all red LEDs off

		//5 green
		LED_bits[MAP_GREEN_LEDS] = 0x80; //first green LED on
		Delay(LED_TEST_DELAY);
		for(int i=1;i<5;i++)
		{
			LED_bits[MAP_GREEN_LEDS]>>=1;
			Delay(LED_TEST_DELAY);
		}
		LED_bits[MAP_GREEN_LEDS] = 0; //all red LEDs off

		//8 white
		LED_bits[MAP_WHITE_LEDS] = 0x80; //first whiteLED on
		Delay(LED_TEST_DELAY);
		for(int i=0;i<7;i++)
		{
			LED_bits[MAP_WHITE_LEDS]>>=1;
			Delay(LED_TEST_DELAY);
		}
		LED_bits[MAP_WHITE_LEDS] = 0; //all white LEDs off

		#endif
	}
}

/*
void gecho_LED_expander_test_v0()
{

	//Delay(500);
	//while(1)
	for(int i=0;i<3;i++)
	{
		//LED_exp_2_byte_cmd(0x01,0x00);
		//Delay(500);
		//LED_exp_2_byte_cmd(0x01,0xff);
		//Delay(500);
		//LED_exp_2_byte_cmd(0x01,0x00);
		//Delay(500);
		//LED_exp_2_byte_cmd(0x01,0xff);

		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xef,1);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xdf,1);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xbf,1);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0x7f,1);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xf7,1);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xfb,1);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xfd,1);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xfe,1);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xff,1);

		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xfe,2);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xfd,2);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xfb,2);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xf7,2);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xef,2);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xdf,2);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xbf,2);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0x7f,2);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xff,2);

		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xfe,3);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xfd,3);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xfb,3);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xf7,3);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xef,3);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xdf,3);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xbf,3);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0x7f,3);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xff,3);

		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xbf,4);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0x7f,4);
		Delay(LED_TEST_DELAY);
		LED_exp_2_byte_cmd(0x01,0xff,4);
	}
}
*/

void gecho_test_LEDs()
{
	for(int i=0;i<3;i++)
	{
		#ifdef BOARD_GECHO_V172
		LED_RDY_ON;
		Delay(500);
		#endif
		LED_SIG_OFF;
		Delay(500);
		#ifdef BOARD_GECHO_V172
		LED_RDY_OFF;
		Delay(500);
		#endif
		LED_SIG_ON;
		Delay(500);
		#ifdef BOARD_GECHO_V172
		IR_DRV1_OFF;
		IR_DRV2_ON;
		Delay(500);
		IR_DRV2_OFF;
		IR_DRV1_ON;
		Delay(500);
		LED_B0_ON;
		Delay(500);
		LED_B0_OFF;
		Delay(500);
		LED_B1_ON;
		Delay(500);
		LED_B1_OFF;
		Delay(500);
		#endif
	}
	LED_SIG_OFF;
}

void gecho_test_buttons()
{
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11); //GPI35 is connected to ADC1 channel #7
	int val;
	while(1)
	{
		val = adc1_get_raw(ADC1_CHANNEL_7);
		printf("GPI35 analog value: %d\n", val);
		Delay(100);
	}
}
#endif

//#define MIDI_SIGNAL_HW_TEST
#define MIDI_OUT_ENABLED

QueueHandle_t uart_queue;

void gecho_init_MIDI(int uart_num, int midi_out_enabled)
{
	#ifdef MIDI_SIGNAL_HW_TEST

	//this config is for Glo only!
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_NUM_16], PIN_FUNC_GPIO); //normally U2RXD
	int result = gpio_set_direction(GPIO_NUM_16, GPIO_MODE_INPUT); //MIDI in (test)
	printf("GPIO16 direction set result = %d\n",result);
	result = gpio_set_pull_mode(GPIO_NUM_16, GPIO_FLOATING);
	printf("GPIO16 pull mode set result = %d\n",result);

	int val,val0=0;
	while(1)
	{
		val = gpio_get_level(GPIO_NUM_16); //MIDI in (test)
		printf("GPIO16=%d...\r",val);
		//Delay(1);
		if(val0!=val)
		{
			val0=val;
			printf("\n");
		}
	}

	/*
	//test midi signal reception at alternative pin #18 (instead of default U2RXD #16)
	int result = gpio_set_direction(GPIO_NUM_18, GPIO_MODE_INPUT); //analog buttons
	printf("GPIO18 direction set result = %d\n",result);
	//result = gpio_set_pull_mode(GPIO_NUM_18, GPIO_FLOATING);
	//printf("GPIO18 pull mode set result = %d\n",result);

	int val,val0=0;
	while(1)
	{
		val = gpio_get_level(GPIO_NUM_18); //MIDI in alt signal (test)
		printf("GPIO18=%d...\r",val);
		//Delay(1);
		if(val0!=val)
		{
			val0=val;
			printf("\n");
		}
	}
	*/
	#else

	//https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/uart.html

	uart_config_t uart_config = {
		.baud_rate = 31250,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,//UART_HW_FLOWCTRL_CTS_RTS,
		.rx_flow_ctrl_thresh = 122,
	};
	// Configure UART parameters
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

	// Set UART pins(TX: IO16 (UART2 default), RX: IO17 (UART2 default), RTS: IO18, CTS: IO19)
	//ESP_ERROR_CHECK(uart_set_pin(MIDI_UART, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 18, 19));
	// Set UART pins(TX: IO16 (UART2 default), RX: IO17 (UART2 default), RTS: unused, CTS: unused)
	//incorrect assignment from ESP-IDF example:
	//ESP_ERROR_CHECK(uart_set_pin(MIDI_UART, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	#ifdef BOARD_WHALE
	#ifdef BOARD_WHALE_ON_EXPANDER_V181
	//swapped wiring on prototype v1.81:
	ESP_ERROR_CHECK(uart_set_pin(MIDI_UART, 16, 17, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	#else
	//corrected assignment:
	ESP_ERROR_CHECK(uart_set_pin(MIDI_UART, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	#endif
	#else //if Gecho
	#ifdef MIDI_OUT_ENABLED
	//optional MIDI Out via GPIO27 and the same connector
	ESP_ERROR_CHECK(uart_set_pin(MIDI_UART, MIDI_OUT_SIGNAL, MIDI_IN_SIGNAL, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	ESP_ERROR_CHECK(uart_set_line_inverse(MIDI_UART, UART_SIGNAL_TXD_INV)); //UART_INVERSE_TXD));
	#else
	//test midi signal reception at alternative pin #18 (instead of default U2RXD #16)
	ESP_ERROR_CHECK(uart_set_pin(MIDI_UART, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	#endif
	#endif

	// Setup UART buffered IO with event queue
	const int uart_buffer_size = (1024 * 2);

	// Install UART driver using an event queue here
	//ESP_ERROR_CHECK(uart_driver_install(MIDI_UART, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
	// Install UART driver without using an event queue
	ESP_ERROR_CHECK(uart_driver_install(MIDI_UART, uart_buffer_size, uart_buffer_size, 10, NULL, 0));

	#ifdef BOARD_GECHO
	#ifdef MIDI_OUT_ENABLED
	if(midi_out_enabled)
	{
		//set GPIO16 to High for MIDI Out powering
		esp_err_t result = gpio_set_direction(MIDI_OUT_PWR, GPIO_MODE_OUTPUT);
		printf("MIDI_OUT_PWR (#%d) direction set result = %d\n", MIDI_OUT_PWR, result);
		gpio_set_level(MIDI_OUT_PWR,1);
	}
	#endif
	#endif

	#endif
}

void gecho_deinit_MIDI()
{
	gecho_stop_receive_MIDI();
	ESP_ERROR_CHECK(uart_driver_delete(MIDI_UART));
}

void gecho_test_MIDI_input_HW()
{
	gecho_deinit_MIDI(); //in case driver already running with a different configuration

	int result = gpio_set_direction(MIDI_IN_SIGNAL, GPIO_MODE_INPUT); //MIDI in (hw test)
	printf("MIDI_IN_SIGNAL:GPIO18 direction set result = %d\n",result);
	//result = gpio_set_pull_mode(MIDI_IN_SIGNAL, GPIO_FLOATING);
	result = gpio_set_pull_mode(MIDI_IN_SIGNAL, GPIO_PULLUP_ONLY);
	printf("MIDI_IN_SIGNAL:GPIO16 pull mode set result = %d\n",result);

	int val,val0=0;
	channel_running = 1;
	unsigned int edge_cnt = 0;
	while(1)
	{
		val = gpio_get_level(MIDI_IN_SIGNAL); //MIDI in (test)
		//printf("MIDI_IN_SIGNAL=%d...\r",val);
		//Delay(1);
		if(val0!=val)
		{
			edge_cnt++;
			val0=val;
			//printf("\n");
			printf("MIDI_IN_SIGNAL=%d, edge=%u...\n",val,edge_cnt);
		}
	}
}

void gecho_test_MIDI_input()
{
	gecho_deinit_MIDI(); //in case driver already running with a different configuration
	gecho_init_MIDI(MIDI_UART, 0); //no midi out

	// Read data from UART.
	//const int uart_num = MIDI_UART;
	uint8_t data[128];
	int length = 0;
	int i;
	channel_running = 1;
	while(1)
	{
		ESP_ERROR_CHECK(uart_get_buffered_data_len(MIDI_UART, (size_t*)&length));
		if(length)
		{
			printf("UART[%d] received %d bytes: ", MIDI_UART, length);
			length = uart_read_bytes(MIDI_UART, data, length, 100);
			//printf("UART[%d] read %d bytes: ", MIDI_UART, length);
			for(i=0;i<length;i++)
			{
				printf("%02x ",data[i]);
			}
			printf("\n");
		}
	}
}

void gecho_test_MIDI_output()
{
	const char note[16][3] = {
		{0x90,0x29,0x64},
		{0x80,0x29,0x7f},
		{0x90,0x2b,0x64},
		{0x80,0x2b,0x7f},
		{0x90,0x2d,0x64},
		{0x80,0x2d,0x7f},
		{0x90,0x2f,0x64},
		{0x80,0x2f,0x7f},
		{0x90,0x30,0x64},
		{0x80,0x30,0x7f},
		{0x90,0x32,0x64},
		{0x80,0x32,0x7f},
		{0x90,0x34,0x64},
		{0x80,0x34,0x7f},
		{0x90,0x35,0x64},
		{0x80,0x35,0x7f}
	};

	gecho_deinit_MIDI(); //in case driver already running with a different configuration
	gecho_init_MIDI(MIDI_UART, 1); //midi out enabled

	channel_running = 1;
	while(1)
	{
		for(int i=0;i<16;i++)
		{
			int length = uart_write_bytes(MIDI_UART, note[i], 3);
			printf("UART[%d] sent %d bytes: %02x %02x %02x\n", MIDI_UART, length, note[i][0], note[i][1], note[i][2]);
			Delay(500);
		}
	}
}

#ifdef BOARD_GECHO

void sync_out_init()
{
	esp_err_t result;

	//set MIDI_OUT_SIGNAL to low, so transistor is closed and does not pull sync SYNC1_IO down
	result = gpio_set_direction(MIDI_OUT_SIGNAL, GPIO_MODE_OUTPUT);
	printf("MIDI_OUT_SIGNAL (#%d) direction set result = %d\n", MIDI_OUT_SIGNAL, result);
	gpio_set_level(MIDI_OUT_SIGNAL,0);

	//digital mode
	result = gpio_set_direction(SYNC1_IO, GPIO_MODE_OUTPUT);
	printf("SYNC1_OUT (#%d) direction set result = %d\n", SYNC1_IO, result);
	gpio_set_level(SYNC1_IO,0);

	result = gpio_set_direction(SYNC2_IO, GPIO_MODE_OUTPUT);
	printf("SYNC2_OUT (#%d) direction set result = %d\n", SYNC2_IO, result);
	gpio_set_level(SYNC2_IO,0);
}

void sync_out_test()
{
	int beat = 0;
	channel_running = 1;
	while(1)
	{
		if(beat%200==0) //every 2000ms at 0ms
		{
			gpio_set_level(SYNC1_IO,1);
			LED_RDY_ON;
		}
		if(beat%200==100) //every 2000ms at 1000ms
		{
			gpio_set_level(SYNC1_IO,0);
			LED_RDY_OFF;
		}


		if(beat%50==0 || beat%50==25) //every 500ms at 0ms or 250ms
		{
			gpio_set_level(SYNC2_IO,1);
			LED_SIG_ON;
		}
		if(beat%50==49 || beat%50==24) //every 500ms at 490ms or 240ms
		{
			gpio_set_level(SYNC2_IO,0);
			LED_SIG_OFF;
		}

		beat++;
		Delay(10);
	}
}

void cv_out_init()
{
	esp_err_t result;

	//set MIDI_OUT_SIGNAL to low, so transistor is closed and does not pull sync SYNC1_IO down
	result = gpio_set_direction(MIDI_OUT_SIGNAL, GPIO_MODE_OUTPUT);
	printf("SYNC1_OUT (#%d) direction set result = %d\n", MIDI_OUT_SIGNAL, result);
	gpio_set_level(MIDI_OUT_SIGNAL,0);

	//analog mode - DAC1 & DAC2
	dac_output_enable(DAC_CHANNEL_1);
	dac_output_enable(DAC_CHANNEL_2);
}

void cv_out_test()
{
	int beat = 0;
	int note = 0;
	channel_running = 1;
	while(1)
	{
		//DAC output is 8-bit. Maximum (255) corresponds to VDD.

		if(beat%50==0 || beat%50==25) //every 500ms at 0ms or 250ms
		{
			dac_output_voltage(DAC_CHANNEL_2, 150); //cca 1.76V
			LED_SIG_ON;
		}
		if(beat%50==49 || beat%50==24) //every 500ms at 490ms or 240ms
		{
			dac_output_voltage(DAC_CHANNEL_2, 50); //cca 0.59V
			LED_SIG_OFF;

			dac_output_voltage(DAC_CHANNEL_1, note);
			note+=8;
			if(note >=64)
			{
				note = 0;
			}
		}

		beat++;
		Delay(10);
	}
}

void sync_in_test()
{
	esp_err_t result;

	//set MIDI_OUT_SIGNAL to low, so transistor is closed and does not pull sync SYNC1_IO down
	result = gpio_set_direction(MIDI_OUT_SIGNAL, GPIO_MODE_OUTPUT);
	printf("MIDI_OUT_SIGNAL (#%d) direction set result = %d\n", MIDI_OUT_SIGNAL, result);
	gpio_set_level(MIDI_OUT_SIGNAL,0); //this seems to have no effect on thresholds below

	/*
	//if MIDI_OUT_SIGNAL left as input, detection of digital signal at 4.5V works first time
	result = gpio_set_direction(MIDI_OUT_SIGNAL, GPIO_MODE_INPUT);
	printf("MIDI_OUT_SIGNAL (#%d) direction set result = %d\n", MIDI_OUT_SIGNAL, result);
	//gpio_set_pull_mode(MIDI_OUT_SIGNAL, GPIO_FLOATING);
	//gpio_set_pull_mode(MIDI_OUT_SIGNAL, GPIO_PULLUP_ONLY);
	//also if set to input with pull-down, it works first time
	gpio_set_pull_mode(MIDI_OUT_SIGNAL, GPIO_PULLDOWN_ONLY);
	*/

	//need to assign these pins back to GPIO in case it was used as analog input previously
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[SYNC1_IO], PIN_FUNC_GPIO);
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[SYNC2_IO], PIN_FUNC_GPIO);

	result = gpio_set_direction(SYNC1_IO, GPIO_MODE_INPUT);
	printf("SYNC1_IO (#%d) direction set result = %d\n", SYNC1_IO, result);
	gpio_set_pull_mode(SYNC1_IO, GPIO_PULLUP_ONLY); //with the help of pullup the threshold is 4V

	result = gpio_set_direction(SYNC2_IO, GPIO_MODE_INPUT);
	printf("SYNC2_IO (#%d) direction set result = %d\n", SYNC2_IO, result);
	gpio_set_pull_mode(SYNC2_IO, GPIO_PULLUP_ONLY); //with the help of pullup the threshold is 4V

	int s1,s2;
	channel_running = 1;
	while(1)
	{
		s1 = gpio_get_level(SYNC1_IO);
		s2 = gpio_get_level(SYNC2_IO);

		printf("%d	%d\n", s1,s2);

		Delay(2);
	}
}

void cv_in_test()
{
	esp_err_t result;

	//set MIDI_OUT_SIGNAL to low, so transistor is closed and does not pull sync SYNC1_IO down
	result = gpio_set_direction(MIDI_OUT_SIGNAL, GPIO_MODE_OUTPUT);
	printf("MIDI_OUT_SIGNAL (#%d) direction set result = %d\n", MIDI_OUT_SIGNAL, result);
	gpio_set_level(MIDI_OUT_SIGNAL,0);

	//adc2_config_width(ADC_WIDTH_BIT_12);

	//the range is 3.0V over 4096 values
	//adc2_config_channel_atten(ADC2_CHANNEL_8, ADC_ATTEN_DB_0); //GPIO25 is connected to ADC2 channel #8
	//adc2_config_channel_atten(ADC2_CHANNEL_9, ADC_ATTEN_DB_0); //GPIO26 is connected to ADC2 channel #9

	//the range is 4.1V over 4096 values
	//adc2_config_channel_atten(ADC2_CHANNEL_8, ADC_ATTEN_DB_2_5); //GPIO25 is connected to ADC2 channel #8
	//adc2_config_channel_atten(ADC2_CHANNEL_9, ADC_ATTEN_DB_2_5); //GPIO26 is connected to ADC2 channel #9

	//the range is 5.8V over 4096 values
	adc2_config_channel_atten(ADC2_CHANNEL_8, ADC_ATTEN_DB_6); //GPIO25 is connected to ADC2 channel #8
	adc2_config_channel_atten(ADC2_CHANNEL_9, ADC_ATTEN_DB_6); //GPIO26 is connected to ADC2 channel #9

	//the range is 9V over 4096 values
	//adc2_config_channel_atten(ADC2_CHANNEL_8, ADC_ATTEN_DB_11); //GPIO25 is connected to ADC2 channel #8
	//adc2_config_channel_atten(ADC2_CHANNEL_9, ADC_ATTEN_DB_11); //GPIO26 is connected to ADC2 channel #9

	int s1,s2;
	channel_running = 1;
	while(1)
	{
		adc2_get_raw(ADC2_CHANNEL_8, ADC_WIDTH_12Bit, &s1);
		adc2_get_raw(ADC2_CHANNEL_9, ADC_WIDTH_12Bit, &s2);

		printf("%d	%d\n", s1,s2);

		Delay(2);
	}
}
#endif

void whale_restart()
{
	codec_set_mute(1); //mute the codec
	Delay(20);
	codec_reset();
	Delay(10);
	esp_restart();
	while(1);
}

int channel_name_ends_with(char *suffix, char *channel_name)
{
	//printf("channel_name_ends_with(suffix=[%s], name=[%s])\n", suffix, channel_name);

	if( channel_name == NULL || suffix == NULL )
	    return 0;

	  size_t str_len = strlen(channel_name);
	  size_t suffix_len = strlen(suffix);

	  if(suffix_len > str_len)
	    return 0;

	  return 0 == strncmp( channel_name + str_len - suffix_len, suffix, suffix_len );
}

void free_memory_info()
{
	int current_free_mem = xPortGetFreeHeapSize();
	printf("free_memory_info(): initial free memory = %u, currently free: %u, loss = %d\n", init_free_mem, current_free_mem, init_free_mem - current_free_mem);
}

int load_song_nvs(char *song_buf, int song_id)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("user_song", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("load_song_nvs(): problem with nvs_open(), error = %d\n", res);
		return 0;
	}

	size_t bytes_loaded;
	char str_id[50];
	sprintf(str_id,"s%d", song_id);

	printf("load_song_nvs(): reading key \"%s\" (a string)\n", str_id);
	res = nvs_get_str(handle, str_id, song_buf, &bytes_loaded);

	if(res!=ESP_OK) //problem reading data
	{
		printf("load_song_nvs(): problem with nvs_get_str() while reading key \"%s\" from namespace \"user_song\", error = %d\n", str_id, res);
		nvs_close(handle);
		return 0;
	}
	nvs_close(handle);

	if(bytes_loaded == 0)
	{
		printf("load_song_nvs(): no user song found\n");
		song_buf[0]=0;
		return 0;
	}
	printf("load_song_nvs(): user song found, length = %d\n", bytes_loaded);
	return bytes_loaded;
}

int store_song_nvs(char *song_buf, int song_id)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("user_song", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("store_song_nvs(): problem with nvs_open(), error = %d\n", res);
		return 0;
	}

	char str_id[50];
	sprintf(str_id,"s%d", song_id);

	printf("store_song_nvs(): writing key \"%s\" (a string)\n", str_id);
	res = nvs_set_str(handle, str_id, song_buf);

	if(res!=ESP_OK) //problem writing data
	{
		printf("store_song_nvs(): problem with nvs_set_str() while writing key \"%s\" in namespace \"user_song\", error = %d\n", str_id, res);
		nvs_close(handle);
		return 0;
	}

	res = nvs_commit(handle);
	if(res!=ESP_OK) //problem writing data
	{
		printf("store_song_nvs(): problem with nvs_commit() while writing key \"%s\" in namespace \"user_song\", error = %d\n", str_id, res);
		nvs_close(handle);
		return 0;
	}

	nvs_close(handle);

	printf("store_song_nvs(): user song stored, length = %d\n", strlen(song_buf));
	return strlen(song_buf);
}

int delete_song_nvs(int song_id)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("user_song", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("delete_song_nvs(): problem with nvs_open(), error = %d\n", res);
		return 0;
	}

	char str_id[50];
	sprintf(str_id,"s%d", song_id);

	printf("delete_song_nvs(): erasing key \"%s\" (a string)\n", str_id);
	res = nvs_erase_key(handle, str_id);

	if(res!=ESP_OK) //problem writing data
	{
		printf("delete_song_nvs(): problem with nvs_erase_key() while erasing key \"%s\" in namespace \"user_song\", error = %d\n", str_id, res);
		nvs_close(handle);
		return 0;
	}

	res = nvs_commit(handle);
	if(res!=ESP_OK) //problem writing data
	{
		printf("delete_song_nvs(): problem with nvs_commit() while erasing key \"%s\" in namespace \"user_song\", error = %d\n", str_id, res);
		nvs_close(handle);
		return 0;
	}

	nvs_close(handle);

	printf("delete_song_nvs(): user song deleted\n");
	return 1;
}

void set_mic_bias(int bias)
{
	printf("set_mic_bias(%d)\n", bias);
	codec_set_mic_bias(bias);
	persistent_settings.MIC_BIAS = bias;
	persistent_settings.MIC_BIAS_updated = 1;
	persistent_settings.update = 0; //to avoid duplicate update if timer already running down
	store_persistent_settings(&persistent_settings);

	if(bias==MIC_BIAS_AVDD)
	{
		indicate_context_setting(0x04, 4, 200);
	}
	else if(bias==MIC_BIAS_2V)
	{
		indicate_context_setting(0x02, 4, 200);
	}
	else if(bias==MIC_BIAS_2_5V)
	{
		indicate_context_setting(0x12, 4, 200);
	}
}

void show_board_serial_no()
{
	channel_running = 1;
	ui_ignore_events = 1;
	display_BCD_numbers((char*)BINARY_ID+11,4);
	printf("Board s/n = %s\n", BINARY_ID);
	while(!BUTTON_RST_ON) { Delay(10); }
}

void show_fw_version()
{
	channel_running = 1;
	ui_ignore_events = 1;
	char ver[4];
	//example: "[Gv2/1.0.113]"
	ver[0] = FW_VERSION[7];
	ver[1] = FW_VERSION[9];
	ver[2] = FW_VERSION[10];
	ver[3] = FW_VERSION[11];
	display_BCD_numbers(ver,4);

	printf("FW Version = %s\n", FW_VERSION);
	while(!BUTTON_RST_ON) { Delay(10); }
}

/*
void float_speed_test()
{
	float a = 12.3456789;
	float b = 34.5678901;
	float c = 0;

	float t0 = micros();

	for (int x=0;x<40;x++)
	{
		for (int i=0;i<1000000;i++)
		{
			for (int j=0;i<100000;i++)
			{

				c += a * b;
				a*=0.56789f;
				b*=0.54321f;

				if(a<0.0000001 && a>-0.0000001)
				{
					a = c-b;
				}
				if(b<0.0000001 && b>-0.0000001)
				{
					b = c-a;
				}

				if(c>100000000 || c<-100000000)
				{
					c *= 0.00000000000000001;
				}
			}
		}
		printf("%f, 100T loops in %f\n", c, micros() - t0);
		t0 = micros();
	}
}

void double_speed_test()
{
	double a = 12.3456789;
	double b = 34.5678901;
	double c = 0;

	double t0 = micros();

	for (int x=0;x<40;x++)
	{
		for (int i=0;i<1000000;i++)
		{
			for (int j=0;i<100000;i++)
			{

				c += a * b;
				a*=0.56789f;
				b*=0.54321f;

				if(a<0.0000001 && a>-0.0000001)
				{
					a = c-b;
				}
				if(b<0.0000001 && b>-0.0000001)
				{
					b = c-a;
				}

				if(c>100000000 || c<-100000000)
				{
					c *= 0.00000000000000001;
				}
			}
		}
		printf("%f, 100T loops in %f\n", c, micros() - t0);
		t0 = micros();
	}
}
*/

int flashed_config_new_enough()
{
	int16_t FW_VERSION_NUM = fw_version_to_uint16(FW_VERSION+5); //format is: [Gv2/x.y.zzz]
	printf("flashed_config_new_enough(): global_settings.CONFIG_FW_VERSION = %d, FW_VERSION_NUM = %d\n", global_settings.CONFIG_FW_VERSION, FW_VERSION_NUM);
	if(global_settings.CONFIG_FW_VERSION < FW_VERSION_NUM)
	{
		return 0;
	}
	return 1;
}

uint16_t fw_version_to_uint16(char *str)
{
	char *cfg = malloc(40), *c_ptr;
	int j=0;
	for(int i=0;i<40;i++)
	{
		c_ptr = str+i;
		//printf("fw_version_to_uint16(): i=%d,j=%d,c_ptr=%s\n", i, j, c_ptr);
		if(c_ptr[0] >= '0' && c_ptr[0] <= '9')
		{
			cfg[j] = c_ptr[0];

			if(++j==5)
			{
				//break;
			}
		}
		else if(c_ptr[0]==0 || c_ptr[0]=='\n' || c_ptr[0]==']' || c_ptr[0]=='/')
		{
			break;
		}
	}
	cfg[j] = 0;
	//printf("fw_version_to_uint16(): cfg=%s\n", cfg);
	uint16_t ret = atol(cfg);
	//printf("ret=%d\n", ret);
	free(cfg);
	return ret;
}
