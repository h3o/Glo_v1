/*
 * init.c
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

#include "init.h"
#include "gpio.h"
#include "codec.h"
#include "signals.h"
#include "i2c_encoder_v2.h"
#include "sha1.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <sys/time.h>

i2c_port_t i2c_num;
int i2c_driver_installed = 0;
int i2c_bus_mutex = 0;
int glo_run = 0;

int channel_loop = 0,channel_loop_remapped;

int channel_running = 0;
int volume_ramp = 0;
int beeps_enabled = 1;

uint16_t ms10_counter = 0; //sync counter for various processes (buttons, volume ramps, MCLK off)
uint16_t auto_power_off;

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

void init_i2s_and_gpio(int buf_count, int buf_len)
{
	printf("Initialize I2S\n");

	i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,                    // enable TX and RX
        .sample_rate = SAMPLE_RATE_DEFAULT,
        .bits_per_sample = 16,                                                  //16-bit per channel
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
#ifdef USE_APLL
		.use_apll = true,
#else
		.use_apll = false,
#endif
		//.dma_buf_count = 2,
		//.dma_buf_count = 4,	//filters
		//.dma_buf_count = 8,		//clouds
		//.dma_buf_count = 16,
		//.dma_buf_count = 32,
		.dma_buf_count = buf_count,

        //.dma_buf_len = 8,
        //.dma_buf_len = 16,
		//.dma_buf_len = 32,
        //.dma_buf_len = 64,	//filters
        //.dma_buf_len = 128,
        //.dma_buf_len = 256,		//clouds
		.dma_buf_len = buf_len,

		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1                                //Interrupt level 1
    };
    i2s_pin_config_t pin_config = {

        .bck_io_num = 25,			//bit clock
		#ifdef BOARD_WHALE
        .ws_io_num = 12,			//byte clock
        .data_in_num = 27,			//MISO
		#else //gecho has different signal used here
		.ws_io_num = 33,			//byte clock
        .data_in_num = 34,			//MISO
		#endif
        .data_out_num = 26,			//MOSI
    };

	#ifdef BOARD_WHALE
	PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[GPIO_NUM_12], PIN_FUNC_GPIO); //normally JTAG MTDI, need to change
	#endif

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
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

	//printf("NOT REALLY\n");
	//return;

	/*
	periph_module_enable(PERIPH_LEDC_MODULE);
    ledc_timer_bit_t bit_num = (ledc_timer_bit_t) 2;      // 3 normally
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


    //printf(" [1]");
	//PIN_FUNC_SELECT(PIN_CTRL, CLK_OUT1_S);
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

    //printf(" [2]");
	//REG_WRITE(PIN_CTRL, 0b111111110000);//0xff0
	////PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1); //enable CLK_OUT1 function
	//PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[MCLK_GPIO], FUNC_GPIO0_CLK_OUT1); //normally U0TXD, need to change

    //printf(" [3]");
	//PIN_FUNC_SELECT(PIN_CTRL, CLK_OUT1);
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

    //printf(" [4]");
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

void i2c_scan_for_devices()
{
	int ret;
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
			printf("device responds at address %u\n",address);
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
		else //e.g. ESP_FAIL
		{
		}
	  }

    printf("\nScan done.\n");
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
			strncpy(code_sha1_hex+(2*i), "00", 2);
		}
	}
	code_sha1_hex[40] = 0;
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

void generate_random_seed()
{
	//randomize the pseudo RNG seed
	bootloader_random_enable();
	set_pseudo_random_seed((double)esp_random() / (double)UINT32_MAX);
	bootloader_random_disable();
}

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

//PCA9554A at 0x38, 0x39, 0x3B, 0x3F
#define EXP1_I2C_ADDRESS 0x38 //56
#define EXP2_I2C_ADDRESS 0x3B //59
#define EXP3_I2C_ADDRESS 0x39 //57
#define EXP4_I2C_ADDRESS 0x3F //63

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
    else if(exp_no==3)
    {
    	i2c_master_write_byte(cmd, EXP3_I2C_ADDRESS << 1 | rw_bit, ACK_CHECK_EN);
    }
    else if(exp_no==4)
    {
    	i2c_master_write_byte(cmd, EXP4_I2C_ADDRESS << 1 | rw_bit, ACK_CHECK_EN);
    }
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

unsigned char byte_bit_reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

void process_expanders(void *pvParameters)
{
	printf("process_expanders(): task running on core ID=%d\n",xPortGetCoreID());

	uint8_t tmp;
	uint8_t reg;
	int was_int = 0;

	while(1)
	{
		Delay(EXPANDERS_TIMING_DELAY);

		if(i2c_bus_mutex)
		{
			continue;
		}
		i2c_bus_mutex = 1;

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

		i2c_bus_mutex = 0;
	}
}

void gecho_LED_expander_init()
{
	printf("gecho_LED_expander_init()\n");
	LED_exp_2_byte_cmd(0x03,0x00,1);
	LED_exp_2_byte_cmd(0x01,0xff,1);
	LED_exp_2_byte_cmd(0x03,0x00,2);
	LED_exp_2_byte_cmd(0x01,0xff,2);
	LED_exp_2_byte_cmd(0x03,0x00,3);
	LED_exp_2_byte_cmd(0x01,0xff,3);
	LED_exp_2_byte_cmd(0x03,0x3f,4); //00x11111 -> two blue LEDs, n/a, buttons 1-4, SET button
	LED_exp_2_byte_cmd(0x01,0xff,4);

	memset(exp_bits,0,8);	//clear expanders I/O map bits
	memset(LED_bits,0,8);	//clear LEDs map bits
	Buttons_bits = 0;

	printf("gecho_LED_expander_init(): starting process_expanders task\n");
	xTaskCreatePinnedToCore((TaskFunction_t)&process_expanders, "process_expanders_task", 2048, NULL, 10, NULL, 1);
}

void gecho_LED_expander_test()
{
	while(1)
	{
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

void gecho_init_MIDI(int uart_num)
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
	//ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 18, 19));
	// Set UART pins(TX: IO16 (UART2 default), RX: IO17 (UART2 default), RTS: unused, CTS: unused)
	//incorrect assignment from ESP-IDF example:
	//ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	#ifdef BOARD_WHALE
	#ifdef BOARD_WHALE_ON_EXPANDER_V181
	//swapped wiring on prototype v1.81:
	ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, 16, 17, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	#else
	//corrected assignment:
	ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	#endif
	#else //if Gecho
	#ifdef MIDI_OUT_ENABLED
	//optional MIDI Out via GPIO27 and the same connector
	ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, MIDI_OUT_SIGNAL, MIDI_IN_SIGNAL, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	ESP_ERROR_CHECK(uart_set_line_inverse(UART_NUM_2, UART_INVERSE_TXD));
	#else
	//test midi signal reception at alternative pin #18 (instead of default U2RXD #16)
	ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	#endif
	#endif

	// Setup UART buffered IO with event queue
	const int uart_buffer_size = (1024 * 2);
	// Install UART driver using an event queue here
	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));

	#ifdef BOARD_GECHO
	#ifdef MIDI_OUT_ENABLED

	//set GPIO16 to High for MIDI Out powering
	esp_err_t result = gpio_set_direction(MIDI_OUT_PWR, GPIO_MODE_OUTPUT);
	printf("MIDI_OUT_PWR (#%d) direction set result = %d\n", MIDI_OUT_PWR, result);
	gpio_set_level(MIDI_OUT_PWR,1);

	#endif
	#endif
}

void gecho_deinit_MIDI(int uart_num)
{
	ESP_ERROR_CHECK(uart_driver_delete(UART_NUM_2));
}

void gecho_test_MIDI_input()
{
	gecho_init_MIDI(MIDI_UART);

	// Read data from UART.
	//const int uart_num = UART_NUM_2;
	uint8_t data[128];
	int length = 0;
	int i;
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
	#endif
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

	gecho_init_MIDI(MIDI_UART);

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


#define ENCODER1_I2C_ADDRESS 0x20

void rotary_encoder_write_reg(uint8_t reg, uint8_t val, int encoder_id)
{
	printf("rotary_encoder_write_reg(%x,%x,%d)\n",reg,val,encoder_id);

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    if(encoder_id==0)
    {
    	i2c_master_write_byte(cmd, ENCODER1_I2C_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    }

    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, val, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    int ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    printf("rotary_encoder_write_reg() result = %d\n", ret);
}

int rotary_encoder_read(uint8_t reg, uint8_t *buf, int bytes, int encoder_id)
{
	//printf("rotary_encoder_read(%x,&buffer,%d,%d)\n",reg,bytes,encoder_id);

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    if(encoder_id==0)
    {
    	i2c_master_write_byte(cmd, ENCODER1_I2C_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    }

    i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    int ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if(ret!=ESP_OK)
    {
    	return ret;
    }

	cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);

    if(encoder_id==0)
    {
    	i2c_master_write_byte(cmd, ENCODER1_I2C_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    }

    i2c_master_read(cmd, buf, bytes, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

uint8_t rotary_encoder_read_byte(uint8_t reg, int encoder_id)
{
	//printf("rotary_encoder_read_byte(%x,%d)\n",reg,encoder_id);

    uint8_t val;
    /*int ret =*/ rotary_encoder_read(reg, &val, 1, encoder_id);

    //printf("rotary_encoder_read_byte() result = %d, val = %x\n", ret, val);
    return val;
}

uint32_t rotary_encoder_read_dword(uint8_t reg, int encoder_id)
{
	//printf("rotary_encoder_read_dword(%x,%d)\n",reg,encoder_id);

    uint32_t val;
    uint8_t buf[8];

    /*int ret =*/ rotary_encoder_read(reg, buf, 4, encoder_id);

    buf[4]=buf[2];
    buf[5]=buf[1];
    buf[6]=buf[0];
    memcpy(&val,buf+3,4);

    //printf("rotary_encoder_read_dword() result = %d, val = %x\n", ret, val);
    return val;
}

void rotary_encoder_init()
{
	rotary_encoder_write_reg(REG_GCONF, INT_DATA | WRAP_ENABLE | DIRE_RIGHT | IPUP_ENABLE | RMOD_X1 | RGB_ENCODER, 0); //init

	rotary_encoder_write_reg(REG_FADERGB, 0x04, 0);		//set RGB LED fade timer

	rotary_encoder_write_reg(REG_CMAXB4, 0x00, 0);		//set max range to 65535
	rotary_encoder_write_reg(REG_CMAXB3, 0x00, 0);
	rotary_encoder_write_reg(REG_CMAXB2, 0xff, 0);
	rotary_encoder_write_reg(REG_CMAXB1, 0xff, 0);
	rotary_encoder_write_reg(REG_CMINB4, 0x00, 0);		//set min range to 0
	rotary_encoder_write_reg(REG_CMINB3, 0x00, 0);
	rotary_encoder_write_reg(REG_CMINB2, 0x00, 0);
	rotary_encoder_write_reg(REG_CMINB1, 0x00, 0);
	//rotary_encoder_write_reg(REG_DPPERIOD, 0x20, 0);	//set double click period
}

void rotary_encoder_test()
{
	for (int i=0;i<2;i++)
	{
		rotary_encoder_write_reg(REG_RLED, 0x40, 0);		Delay(LED_TEST_DELAY);
		rotary_encoder_write_reg(REG_GLED, 0x80, 0);		Delay(LED_TEST_DELAY);
		rotary_encoder_write_reg(REG_BLED, 0xa0, 0);		Delay(LED_TEST_DELAY);
		rotary_encoder_write_reg(REG_RLED, 0x00, 0);		Delay(LED_TEST_DELAY);
		rotary_encoder_write_reg(REG_GLED, 0x00, 0);		Delay(LED_TEST_DELAY);
		rotary_encoder_write_reg(REG_BLED, 0x00, 0);		Delay(LED_TEST_DELAY);
		rotary_encoder_write_reg(REG_RLED, 0xff, 0);		Delay(LED_TEST_DELAY);
		rotary_encoder_write_reg(REG_GLED, 0xff, 0);		Delay(LED_TEST_DELAY);
		rotary_encoder_write_reg(REG_BLED, 0xff, 0);		Delay(LED_TEST_DELAY);
	}

	uint32_t pos;
	uint8_t status;

	while(1)
	{
		pos = rotary_encoder_read_dword(REG_CVALB4, 0);
		status = rotary_encoder_read_byte(REG_ESTATUS, 0);
		printf("pos = %d, status = %x\n", pos, status);
		Delay(100);
	}
}

#ifdef BOARD_GECHO
void sync_out_init()
{
	esp_err_t result = gpio_set_direction(SYNC1_IO, GPIO_MODE_OUTPUT);
	printf("SYNC1_OUT (#%d) direction set result = %d\n", SYNC1_IO, result);
	gpio_set_level(SYNC1_IO,0);

	result = gpio_set_direction(SYNC2_IO, GPIO_MODE_OUTPUT);
	printf("SYNC2_OUT (#%d) direction set result = %d\n", SYNC2_IO, result);
	gpio_set_level(SYNC2_IO,0);
}

void sync_out_test()
{
	int beat = 0;
	while(1)
	{
		if(beat%50==5) //every 500ms at 50ms
		{
			gpio_set_level(SYNC1_IO,1);
			LED_RDY_ON;
		}
		if(beat%50==10) //every 500ms at 100ms
		{
			gpio_set_level(SYNC1_IO,0);
			LED_RDY_OFF;
		}

		if(beat%50==0 || beat%50==25) //every 500ms at 0ms or 250ms
		{
			gpio_set_level(SYNC2_IO,1);
			LED_SIG_ON;
		}
		if(beat%50==10 || beat%50==35) //every 500ms at 100ms or 350ms
		{
			gpio_set_level(SYNC2_IO,0);
			LED_SIG_OFF;
		}

		beat++;
		Delay(10);
	}
}
#endif

void whale_restart()
{
	codec_set_mute(1); //mute the codec
	Delay(100);
	codec_reset();
	Delay(100);
	esp_restart();
	while(1);
}

void low_voltage_poweroff(void *params)
{
	printf("low_voltage_poweroff()\n");
	whale_shutdown();
}

void brownout_init()
{
	#define BROWNOUT_DET_LVL 0

	REG_WRITE(RTC_CNTL_BROWN_OUT_REG,
            RTC_CNTL_BROWN_OUT_ENA // Enable BOD
            | RTC_CNTL_BROWN_OUT_PD_RF_ENA // Automatically power down RF
            //Reset timeout must be set to >1 even if BOR feature is not used
            | (2 << RTC_CNTL_BROWN_OUT_RST_WAIT_S)
            | (BROWNOUT_DET_LVL << RTC_CNTL_DBROWN_OUT_THRES_S));

    rtc_isr_register(low_voltage_poweroff, NULL, RTC_CNTL_BROWN_OUT_INT_ENA_M);
    printf("Initialized BOD\n");

    REG_SET_BIT(RTC_CNTL_INT_ENA_REG, RTC_CNTL_BROWN_OUT_INT_ENA_M);
}
