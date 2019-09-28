/*
 * codec.c
 *
 *  Created on: Apr 6, 2016
 *      Author: mario
 *
 * Based on TLV320AIC3104 documentation:
 * http://www.ti.com/product/TLV320AIC3104/technicaldocuments
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

#include "codec.h"
#include <hw/gpio.h>
#include <hw/init.h>
#include <hw/ui.h>
#include <string.h>

volatile uint32_t sampleCounter = 0;
int add_beep = 0;
int mclk_enabled;
int mics_off = 0;

int codec_analog_volume;// = CODEC_ANALOG_VOLUME_DEFAULT;
int codec_digital_volume;// = CODEC_DIGITAL_VOLUME_DEFAULT;
int codec_volume_user;// = CODEC_DIGITAL_VOLUME_DEFAULT;

#define CODEC_PLL_44K

#define CODEC_EQ_COEFFS_DEFAULT

#define EQ_COEFS 10 //10 x 16-bit registers
int16_t EQ_coefs[1+EQ_COEFS] = {
	0x0000,	//reserve 2 bytes for destination register address
#ifdef CODEC_EQ_COEFFS_DEFAULT
	//wrong endian for codec, good endian for ESP32 int16_t
	0x6be3,	//N0
	0x9666,	//N1
	0x675d,	//N2
	0x6be3,	//N3
	0x9666,	//N4
	0x675d,	//N5
	0x7d83,	//D1
	0x84ee,	//D2
	0x7d83,	//D4
	0x84ee	//D5
/*
	//good endian for codec, wrong endian for ESP32 int16_t
	0xe36b,	//N0
	0x6696,	//N1
	0x5d67,	//N2
	0xe36b,	//N3
	0x6696,	//N4
	0x5d67,	//N5
	0x837d,	//D1
	0xee84,	//D2
	0x837d,	//D4
	0xee84	//D5
*/
#else

	//Coefficients_9dB_bass_shelf_and_treble_attenuation.txt
	0x7FFF,
	0x8205,
	0x7C08,
	0x318B,
	0xD90E,
	0x2024,
	0x7ECB,
	0x8262,
	0x6F7D,
	0x9D39
#endif
};

#define EQ_COEFS_OPTIONS 		14

#define EQ_COEFS_BASS_9_neg		0
#define EQ_COEFS_BASS_6_neg		1
#define EQ_COEFS_BASS_3_neg		2
#define EQ_COEFS_BASS_0			3
#define EQ_COEFS_BASS_3			4
#define EQ_COEFS_BASS_6			5
#define EQ_COEFS_BASS_9			6
#define EQ_COEFS_TREBLE_9_neg	7
#define EQ_COEFS_TREBLE_6_neg	8
#define EQ_COEFS_TREBLE_3_neg	9
#define EQ_COEFS_TREBLE_0		10
#define EQ_COEFS_TREBLE_3		11
#define EQ_COEFS_TREBLE_6		12
#define EQ_COEFS_TREBLE_9		13

//#define BASS_EQ_AT_100HZ
#define BASS_EQ_AT_400HZ

const int16_t EQ_coef_options[EQ_COEFS_OPTIONS][5] = {

	#ifdef BASS_EQ_AT_100HZ
	{0x7F2F,
	0x8202,
	0x7CD2,
	0x7DFB,
	0x83F8}, //bass -9dB @ 100Hz

	{0x7F8C,
	0x8188,
	0x7D68,
	0x7E77,
	0x8307}, //bass -6dB @ 100Hz

	{0x7FC5,
	0x8169,
	0x7D6E,
	0x7E97,
	0x82C9}, //bass -3dB @ 100Hz
	#endif

	#ifdef BASS_EQ_AT_400HZ
	{0x7CC8,
	0x87E5,
	0x73C5,
	0x77F4,
	0x8F23}, //bass -9dB @ 400Hz

	{0x7E38,
	0x860D,
	0x75F5,
	0x79E1,
	0x8BAD}, //bass -6dB @ 400Hz

	{0x7F1B,
	0x8597,
	0x760C,
	0x7A60,
	0x8AC6}, //bass -3dB @ 400Hz
	#endif

	{0x6BE3,
	0x9666,
	0x675D,
	0x7D83,
	0x84EE}, //0dB

	#ifdef BASS_EQ_AT_100HZ
	{0x7FFF,
	0x8169,
	0x7D37,
	0x7ED0,
	0x825A}, //bass +3dB @ 100Hz

	{0x7FFF,
	0x8189,
	0x7CF9,
	0x7EE9,
	0x8227}, //bass +6dB @ 100Hz

	{0x7FFF,
	0x8205,
	0x7C08,
	0x7ECB,
	0x8262}, //bass +9dB @ 100Hz
	#endif

	#ifdef BASS_EQ_AT_400HZ
	{0x7FFF,
	0x85A0,
	0x753A,
	0x7B44,
	0x8921}, //bass +3dB @ 400Hz

	{0x7FFF,
	0x861F,
	0x7453,
	0x7BAA,
	0x8862}, //bass +6dB @ 400Hz

	{0x7FFF,
	0x880C,
	0x70DD,
	0x7B33,
	0x8940}, //bass +9dB @ 400Hz
	#endif

	{0x318B,
	0xD90E,
	0x2024,
	0x6F7D,
	0x9D39}, //treble -9dB

	{0x44C1,
	0xCB79,
	0x2A9D,
	0x6A61,
	0xA4ED}, //treble -6dB

	{0x5DCB,
	0xB68B,
	0x3C7C,
	0x6885,
	0xA797}, //treble -3dB

	{0x6BE3,
	0x9666,
	0x675D,
	0x7D83,
	0x84EE}, //0dB

	{0x7FFF,
	0x977B,
	0x5869,
	0x643E,
	0xAD75}, //treble +3dB

	{0x7FFF,
	0x959F,
	0x5B13,
	0x61CA,
	0xB0AA}, //treble +6dB

	{0x7FFF,
	0x9083,
	0x62C7,
	0x649C,
	0xACF8} //treble +9dB
};

int EQ_bass_setting = EQ_BASS_DEFAULT, EQ_treble_setting = EQ_TREBLE_DEFAULT;

int16_t EQ_coefs_se[1+EQ_COEFS];

void swap_endian(int16_t *src, int16_t *dest, int n)
{
	//printf("swap_endian(): ");
	for(int i=0;i<n;i++)
	{
		dest[i] = ((src[i]>>8) & 0x00ff) | ((src[i]<<8) & 0xff00);
		//printf("[%04x->%04x]",src[i],dest[i]);
	}
	//printf("\n");
}

static esp_err_t i2c_codec_ctrl_init(i2c_port_t set_i2c_num, int8_t enable_AGC, int8_t AGC_max_gain, int8_t AGC_target_level)
{
	i2c_num = set_i2c_num;
	//i2c_master_init();

    //LED_BLUE_ON;
    vTaskDelay(10 / portTICK_RATE_MS);
    //LED_BLUE_OFF;
	printf("i2c_codec_ctrl_init(): Configuring...\n");
    gpio_set_level(CODEC_RST_PIN, 1);  //release the codec reset
    vTaskDelay(2 / portTICK_RATE_MS);

	int ret;
	uint8_t reg_status;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, CODEC_I2C_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x00, ACK_CHECK_EN); //Page 0/Register 0: Page Select Register
    i2c_master_write_byte(cmd, 0x00, ACK_CHECK_EN); //select page #0
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        return ret;
    }

    //vTaskDelay(30 / portTICK_RATE_MS);

    //------------------- PLL CONFIG ---------------------------------------

    //Page 0/Register 101: Clock Register
    //D0: CODEC_CLKIN Source Selection => 0: CODEC_CLKIN uses PLLDIV_OUT (default)

    //Page 0/Register 102: Clock Generation Control Register
    //D7-D6: CLKDIV_IN Source Selection
    //00: CLKDIV_IN uses MCLK (default)
    //01: CLKDIV_IN uses GPIO2
    //10: CLKDIV_IN uses BCLK
    //D5-D4: PLLCLK_IN Source Selection
    //00: PLLCLK_IN uses MCLK.
    //01: PLLCLK_IN uses GPIO2.
    //10: PLLCLK_IN uses BCLK
    //D3-D0: R/W 0010 Reserved. Write only 0010 to these bits.
    ret = i2c_codec_two_byte_command(0x66, 0x22); //00100010 ->CLKDIV_IN:MCLK,PLLCLK_IN:BCLK
    //ret = i2c_codec_two_byte_command(0x66, 0x02); //00000010 ->CLKDIV_IN:MCLK,PLLCLK_IN:MCLK
    if (ret != ESP_OK) { return ret; }

    //Page 0/Register 3: PLL Programming Register A
    //D7: PLL Control Bit
    //0: PLL is disabled (default)
    //1: PLL is enabled
    //D6..D3: PLL Q Value => 0010: Q = 2 (default)
    //D2-D0: PLL P Value => 001: P = 1
    ret = i2c_codec_two_byte_command(0x03, 0x91); //10010001
    if (ret != ESP_OK) { return ret; }

    //Page 0/Register 4: PLL Programming Register B
    //D7..D2: PLL J Value => 100000: J = 32, 110000: J = 48,
    //D1-D0: R/W 00 Reserved. Write only zeros to these bits.

	#ifdef CODEC_PLL_44K
    ret = i2c_codec_two_byte_command(0x04, 0x80); //10000000
	#else
    ret = i2c_codec_two_byte_command(0x04, 0xa0); //11000000 -> should be correct for 48k but does not sound well
	#endif

    if (ret != ESP_OK) { return ret; }

    //Page 0/Register 5: PLL Programming Register C
    //Page 0/Register 6: PLL Programming Register D
    //all 0 => D = 0 (default)

    //Page 0/Register 11: Audio Codec Overflow Flag Register
    //D3-D0: PLL R Value => 0001: R = 1 (default)

	//------------------- PLL CONFIG (end) ---------------------------------

    //Page 0/Register 7: Codec Data-Path Setup Register
	//D7->1: fS(ref) = 44.1 kHz
	//D4-D3->01: Left-DAC data path plays left-channel input data
	//D2-D1->01: Right-DAC data path plays right-channel input data
    ret = i2c_codec_two_byte_command(0x07, 0x8a); //10001010
    if (ret != ESP_OK) { return ret; }

    //Page 0/Register 37: DAC Power and Output Driver Control Register
	//D7: 1: Left DAC is powered up
	//D6: 1: Right DAC is powered up
	//D5-D4: 10: HPLCOM configured as independent single-ended output
    ret = i2c_codec_two_byte_command(0x25, 0xe8); //11100000
    if (ret != ESP_OK) { return ret; }

    //Page 0/Register 38: High-Power Output Driver Control Register
	//D5-D3: 010: HPRCOM configured as independent single-ended output
	//D2: 1: Short-circuit protection on all high-power output drivers is enabled
	//D1: 1: If short-circuit protection is enabled, it powers down the output driver automatically when a short is detected
    //    0: If short-circuit protection is enabled, it limits the maximum current to the load.
    //ret = i2c_codec_two_byte_command(0x26, 0x16); //00010110 -> power down the output driver on short circuit
    ret = i2c_codec_two_byte_command(0x26, 0x14); //00010100 -> limit the maximum current on short circuit
    if (ret != ESP_OK) { return ret; }

    //Page 0/Register 41: DAC Output Switching Control Register
	//D7-D6: 00: Left-DAC output selects DAC_L1 path
	//D5-D4: 00: Right-DAC output selects DAC_R1 path
	//D1-D0: 10: Right-DAC volume follows the left-DAC digital volume control register
    ret = i2c_codec_two_byte_command(0x29, 0x02); //00000010
    if (ret != ESP_OK) { return ret; }

    //Page 0/Register 42: Output Driver Pop Reduction Register
	//D7-D4: 1001: Driver power-on time = 800 ms
	//D7-D4: 0111: Driver power-on time = 200 ms
	//D3-D2: 10: Driver ramp-up step time = 2 ms
	//D1: 0: Weakly driven output common-mode voltage is generated from resistor divider off the AVDD supply
    //ret = i2c_two_byte_command(0x2a, 0x78); //01111000 -> 200ms power on delay, 2ms step
    //ret = i2c_two_byte_command(0x2a, 0x98); //10011000 -> 800ms power on delay, 2ms step
    //ret = i2c_two_byte_command(0x2a, 0x9c); //10011100 -> 800ms power on delay, 4ms step
    //ret = i2c_two_byte_command(0x2a, 0xac); //10101100 -> 2s power on delay, 4ms step
    ret = i2c_codec_two_byte_command(0x2a, 0x78); //01111100 -> 200ms power on delay, 4ms step
    if (ret != ESP_OK) { return ret; }

    //----------------- setting volumes -------------------------------------------------------------------

    //Page 0/Register 43: Left-DAC Digital Volume Control Register
    //D7: 0: The left-DAC channel is not muted
	//D6-D0: 001 1000: Gain = -12 dB
	//D6-D0: 000 1100: Gain = -6 dB
    //range = 0dB .. -63dB
    //Right-DAC volume follows the left-DAC digital volume control register (by setting reg.41 D1-D0)
    ret = i2c_codec_two_byte_command(0x2b, 0x0C); //-6db
    if (ret != ESP_OK) { return ret; }

    //codec_digital_volume = CODEC_DIGITAL_VOLUME_DEFAULT;
    codec_digital_volume = CODEC_DIGITAL_VOLUME_MUTE;

    //----------------- analog controllable volume --------------------------------------------------------

    //Page 0/Register 47: DAC_L1 to HPLOUT Volume Control Register
	//D7: 1: DAC_L1 is routed to HPLOUT
	//D6-D0: DAC_L1 to HPLOUT Analog Volume Control -> 000 1100: Gain = -6 dB

    //range = 0dB .. -78dB

    ret = i2c_codec_two_byte_command(0x2f, 0x80 | codec_analog_volume); //10001100
    //ret = i2c_two_byte_command(0x2f, 0x80 | CODEC_ANALOG_VOLUME_MUTE);
    if (ret != ESP_OK) { return ret; }

    //Page 0/Register 64: DAC_R1 to HPROUT Volume Control Register
	//D7: 1: DAC_R1 is routed to HPROUT
	//D6-D0: DAC_R1 to HPROUT Analog Volume Control -> 000 1100: Gain = -6 dB

    //range = 0dB .. -78dB

    ret = i2c_codec_two_byte_command(0x40, 0x80 | codec_analog_volume); //10001100
    //ret = i2c_two_byte_command(0x40, 0x80 | CODEC_ANALOG_VOLUME_MUTE);
    if (ret != ESP_OK) { return ret; }

    //----------------- analog controllable volume ---------- [end] ---------------------------------------

    //Page 0/Register 51: HPLOUT Output Level Control Register
	//D7-D4: 0000: Output level control = 0 dB
	//D3: 1: HPLOUT is not muted
	//D2: 0: HPLOUT is weakly driven to a common-mode when powered down
	//D1: 0: All programmed gains to HPLOUT have been applied
	//D0: 1: HPLOUT is fully powered up
    ret = i2c_codec_two_byte_command(0x33, 0x69); //01101001 -> 6dB output level
    if (ret != ESP_OK) { return ret; }

    //Page 0/Register 65: HPROUT Output Level Control Register
	//D7-D4: 0000: Output level control = 0 dB
	//D3: 1: HPROUT is not muted
	//D2: 0: HPROUT is weakly driven to a common mode when powered down
	//D1: 0: All programmed gains to HPROUT have been applied
	//D0: 1: HPROUT is fully powered up
    ret = i2c_codec_two_byte_command(0x41, 0x69); //01101001 -> 6dB output level
    if (ret != ESP_OK) { return ret; }

    //----------------- setting volumes ------ [end] ------------------------------------------------------

	//------------------- EQ CONFIG ----------------------------------------
    /*
    //select page 1 to access EQ coefficients registers
    //D0: Page Select Bit
    ret = i2c_codec_two_byte_command(0x00, 0x01);
    if (ret != ESP_OK) { return ret; }

    printf("Setting EQ Coeffs: %d %d %d %d %d %d %d %d %d %d\n",EQ_coefs[1],EQ_coefs[2],EQ_coefs[3],EQ_coefs[4],EQ_coefs[5],EQ_coefs[6],EQ_coefs[7],EQ_coefs[8],EQ_coefs[9],EQ_coefs[10]);
    swap_endian(EQ_coefs, EQ_coefs_se, EQ_COEFS+1);

    //write default coefficients array
    //Page 1/Register 1: Left-Channel Audio Effects Filter N0 Coefficient MSB Register (and subsequent registers)
    ((unsigned char*)EQ_coefs_se)[1] = 0x01;
    ret = i2c_bus_write(CODEC_I2C_ADDRESS << 1 | WRITE_BIT, (unsigned char*)EQ_coefs_se + 1, 1 + EQ_COEFS * 2);
	if (ret != ESP_OK) { return ret; }

	//Page 1/Register 27: Right-Channel Audio Effects Filter N0 Coefficient MSB Register (and subsequent registers)
    ((unsigned char*)EQ_coefs_se)[1] = 0x1B;
    ret = i2c_bus_write(CODEC_I2C_ADDRESS << 1 | WRITE_BIT, (unsigned char*)EQ_coefs_se + 1, 1 + EQ_COEFS * 2);
	if (ret != ESP_OK) { return ret; }

    //select page 0 again
    //D0: Page Select Bit
    ret = i2c_codec_two_byte_command(0x00, 0x00);
    if (ret != ESP_OK) { return ret; }

    //Page 0/Register 12: Audio Codec Digital Filter Control Register
    //D7-D6: Left-ADC High-Pass Filter Control -> 00: Left-ADC high-pass filter disabled
    //D5-D4: Right-ADC High-Pass Filter Control -> 00: Right-ADC high-pass filter disabled
    //D3: Left-DAC Digital Effects Filter Control -> 1: Left-DAC digital effects filter enabled
    //D2: Left-DAC De-Emphasis Filter Control -> 0: Left-DAC de-emphasis filter disabled (bypassed)
    //D1: Right-DAC Digital Effects Filter Control -> 1: Right-DAC digital effects filter enabled
    //D0: Right-DAC De-Emphasis Filter Control -> 0: Right-DAC de-emphasis filter disabled (bypassed)
    ret = i2c_codec_two_byte_command(0x0c, 0x0a); //00001010
    if (ret != ESP_OK) { return ret; }
    */

    ret = codec_set_eq();
    if (ret != ESP_OK) { return ret; }

    //------------------- EQ CONFIG (end) ----------------------------------

    //verify if DAC and HP driver is powered up -> TODO
    /*
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, CODEC_I2C_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data_h, ACK_VAL);
    i2c_master_read_byte(cmd, data_l, NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    */

	//#define USE_LINE_IN
    #define USE_MICS

    //ADC setup -----------------------------------------------------------------------------------

    //Page 0/Register 15: Left-ADC PGA Gain Control Register
	//D7: 0: The left-ADC PGA is not muted
	//D6-D0: 000 1100: Gain = 6 dB
	#if defined(USE_LINE_IN) && !defined(USE_MICS)
    ret = i2c_codec_two_byte_command(0x0f, 0x0c); //00000000 -> 0dB
    //ret = i2c_codec_two_byte_command(0x0f, 0x0c); //00001100 -> 6dB
    //ret = i2c_codec_two_byte_command(0x0f, 0x18); //00011000 -> 12dB
	#else
    ret = i2c_codec_two_byte_command(0x0f, 0x40); //01000000 -> 32dB
    //ret = i2c_codec_two_byte_command(0x0f, 0x60); //01100000 -> 48dB
	#endif
    if (ret != ESP_OK) { return ret; }

	//Page 0/Register 16: Right-ADC PGA Gain Control Register
	//D7: 0: The right ADC PGA is not muted
	//D6-D0: 000 1100: Gain = 6 dB
	#if defined(USE_LINE_IN) && !defined(USE_MICS)
    ret = i2c_codec_two_byte_command(0x10, 0x0c); //00000000 -> 0dB
    //ret = i2c_codec_two_byte_command(0x10, 0x0c); //00001100 -> 6dB
    //ret = i2c_codec_two_byte_command(0x10, 0x18); //00001100 -> 12dB
	#else
    ret = i2c_codec_two_byte_command(0x10, 0x40); //01000000 -> 32dB
    //ret = i2c_codec_two_byte_command(0x10, 0x60); //01100000 -> 48dB
	#endif
    if (ret != ESP_OK) { return ret; }

	//Line-in setup -------------------------------------------------------------------------------

	#ifdef USE_LINE_IN

	//connect MIC2/LINE2 lines to ADC and set gain

    //Page 0/Register 17: MIC2L/R to Left-ADC Control Register
	//MIC2L Input Level Control for Left-ADC PGA Mix
    //Setting the input level control to one of the following gains automatically connects MIC2L to the left-ADC PGA mix.
	//D7-D4: 0100: Input level control gain = –6 dB
	//D7-D4: 1000: Input level control gain = –12 dB
	//MIC2R/LINE2R Input Level Control for Left-ADC PGA Mix
    //Setting the input level control to one of the following gains automatically connects MIC2R to the left-ADC PGA mix.
	//D3-D0: 1111: MIC2R/LINE2R is not connected to the left-ADC PGA.
    //ret = i2c_codec_two_byte_command(0x11, 0x4f); //01001111 -> -6dB
    ret = i2c_codec_two_byte_command(0x11, 0x8f); //10001111 -> -12dB
    if (ret != ESP_OK) { return ret; }

    //Page 0/Register 18: MIC2/LINE2 to Right-ADC Control Register
    //MIC2L/LINE2L Input Level Control for Right -DC PGA Mix
    //Setting the input level control to one of the following gains automatically connects MIC2L to the right-ADC PGA mix.
    //D7-D4: 1111: MIC2L/LINE2L is not connected to the right-ADC PGA.
    //MIC2R/LINE2R Input Level Control for Right-ADC PGA Mix
    //Setting the input level control to one of the following gains automatically connects MIC2R to the right-ADC PGA mix.
    //D3-D0: 0100: Input level control gain = –6 dB
    //ret = i2c_codec_two_byte_command(0x12, 0xf4); //11110100 -> -6dB
    ret = i2c_codec_two_byte_command(0x12, 0xf8); //11111000 -> -12dB
    if (ret != ESP_OK) { return ret; }

	#ifndef USE_MICS
    //power up ADC but do not connect MIC1/LINE1 lines

    //Page 0/Register 19: MIC1LP/LINE1LP to Left-ADC Control Register
	//D7: 1: MIC1LP/LINE1LP and MIC1LM/LINE1LM are configured in fully differential mode
	//D6-D3: 1111: LINE1L is not connected to the left-ADC PGA.
	//D2: 1: Left-ADC channel is powered up
	//D1-D0: 00: Left-ADC PGA soft-stepping at once per sample period
    ret = i2c_codec_two_byte_command(0x13, 0xfc); //11111100
    if (ret != ESP_OK) { return ret; }

	//Page 0/Register 22: MIC1RP/LINE1RP to Right-ADC Control Register
	//D7: 1: MIC1RP/LINE1RP and MIC1RM/LINE1RM are configured in fully differential mode
	//D6-D3: 1111: LINE1R is not connected to the right-ADC PGA.
	//D2: 1: Right-ADC channel is powered up
	//D1-D0: 00: Right-ADC PGA soft-stepping at once per sample period
    ret = i2c_codec_two_byte_command(0x16, 0xfc); //11111100
    if (ret != ESP_OK) { return ret; }
	#endif

    #endif

    //Mics setup ----------------------------------------------------------------------------------

    #ifdef USE_MICS

	//connect MIC2/LINE2 lines to ADC and set gain

    //Page 0/Register 19: MIC1LP/LINE1LP to Left-ADC Control Register
	//D7: 1: MIC1LP/LINE1LP and MIC1LM/LINE1LM are configured in fully differential mode
	//D6-D3: 0010: Input level control gain = -3 dB
	//D2: 1: Left-ADC channel is powered up
	//D1-D0: 00: Left-ADC PGA soft-stepping at once per sample period
    ret = i2c_codec_two_byte_command(0x13, 0x94); //10010100
    if (ret != ESP_OK) { return ret; }

	//Page 0/Register 22: MIC1RP/LINE1RP to Right-ADC Control Register
	//D7: 1: MIC1RP/LINE1RP and MIC1RM/LINE1RM are configured in fully differential mode
	//D6-D3: 0010: Input level control gain = -3 dB
	//D2: 1: Right-ADC channel is powered up
	//D1-D0: 00: Right-ADC PGA soft-stepping at once per sample period
    ret = i2c_codec_two_byte_command(0x16, 0x94); //10010100
    if (ret != ESP_OK) { return ret; }

	//set MIC bias (used to power the active analog microphones)

    //Page 0/Register 25: MICBIAS Control Register
	//D7-D6: 01: MICBIAS output is powered to 2 V
    ret = i2c_codec_two_byte_command(0x19, 0x40); //01000000
    if (ret != ESP_OK) { return ret; }

	#endif

    //Page 0/Register 107: New Programmable ADC Digital Path and I2C Bus Condition Register
	//D5-D4: 11: Left and right analog microphones are used
    ret = i2c_codec_two_byte_command(0x6b, 0x30); //00110000
    if (ret != ESP_OK) { return ret; }

	//AGC controls -----------------------------------------------------------------------------------

    if(enable_AGC)
    {
    	printf("i2c_codec_ctrl_init(): Enabling AGC\n");

    	ret = codec_configure_AGC(enable_AGC, AGC_max_gain, AGC_target_level);
        if (ret != ESP_OK) { return ret; }
    }
    else
    {
    	printf("i2c_codec_ctrl_init(): AGC not enabled\n");
    }

    //AGC controls ----- [end] -----------------------------------------------------------------------

	//Line Out routing -------------------------------------------------------------------------------

    //Page 0/Register 82: DAC_L1 to LEFT_LOP/M Volume Control Register
    //D7: DAC_L1 Output Routing Control
    //1: DAC_L1 is routed to LEFT_LOP/M.
    //D6-D0: DAC_L1 to LEFT_LOP/M Analog Volume Control
    //8 (0x08) = -4db
    //16 (0x10) = -8db
    //32 (0x20) = -16db
    //48 (0x30) = -24.1db
	ret = i2c_codec_two_byte_command(0x52, 0x80); //10000000 -> 0dB
	//ret = i2c_codec_two_byte_command(0x52, 0x88); //10001000 -> -4dB
	//ret = i2c_codec_two_byte_command(0x52, 0x90); //10010000 -> -8dB
	//ret = i2c_codec_two_byte_command(0x52, 0xa0); //10100000 -> -16dB
	//ret = i2c_codec_two_byte_command(0x52, 0xb0); //10110000 -> -24.1dB
	if (ret != ESP_OK) { return ret; }

	//Page 0/Register 86: LEFT_LOP/M Output Level Control Register
	//D7-D4: LEFT_LOP/M Output Level Control
	//0000: Output level control = 0 dB
	//0010: Output level control = 2 dB
	//1001: Output level control = 9 dB
	//D3: LEFT_LOP/M Mute
	//1: LEFT_LOP/M is not muted.
	//ret = i2c_codec_two_byte_command(0x56, 0x08); //00001000 -> 0dB gain, not muted
	//ret = i2c_codec_two_byte_command(0x56, 0x09); //00001001 -> 0dB gain, not muted
	//ret = i2c_codec_two_byte_command(0x56, 0x49); //01001001 -> 4dB gain, not muted
	ret = i2c_codec_two_byte_command(0x56, 0x89); //10001001 -> 8dB gain, not muted
	if (ret != ESP_OK) { return ret; }

	//reg_status = i2c_codec_register_read(0x56);
	//printf("i2c_codec_ctrl_init(): reg_status(0x56) = %x\n", reg_status);

	//Page 0/Register 92: DAC_R1 to RIGHT_LOP/M Volume Control Register
	//D7: DAC_R1 Output Routing Control
	//1: DAC_R1 is routed to RIGHT_LOP/M.
	//D6-D0: DAC_R1 to RIGHT_LOP/M Analog Volume Control
    //8 (0x08) = -4db
    //16 (0x10) = -8db
    //32 (0x20) = -16db
    //48 (0x30) = -24.1db
	ret = i2c_codec_two_byte_command(0x5c, 0x80); //10000000 -> 0dB
	//ret = i2c_codec_two_byte_command(0x5c, 0x88); //10001000 -> -4dB
	//ret = i2c_codec_two_byte_command(0x5c, 0x90); //10010000 -> -8dB
	//ret = i2c_codec_two_byte_command(0x5c, 0xa0); //10100000 -> -16dB
	//ret = i2c_codec_two_byte_command(0x5c, 0xb0); //10110000 -> -24.1dB
	if (ret != ESP_OK) { return ret; }

	//Page 0/Register 93: RIGHT_LOP/M Output Level Control Register
	//D7-D4: RIGHT_LOP/M Output Level Control
	//0000: Output level control = 0 dB
	//0010: Output level control = 2 dB
	//1001: Output level control = 9 dB
	//D3: RIGHT_LOP/M Mute
	//1: RIGHT_LOP/M is not muted.
	//ret = i2c_codec_two_byte_command(0x5d, 0x08); //00001000 -> 0dB gain, not muted
	//ret = i2c_codec_two_byte_command(0x5d, 0x09); //00001001 -> 0dB gain, not muted
	//ret = i2c_codec_two_byte_command(0x5d, 0x49); //01001001 -> 4dB gain, not muted
	ret = i2c_codec_two_byte_command(0x5d, 0x89); //10001001 -> 8dB gain, not muted
	if (ret != ESP_OK) { return ret; }

	//reg_status = i2c_codec_register_read(0x5d);
	//printf("i2c_codec_ctrl_init(): reg_status(0x5d) = %x\n", reg_status);

	/*
	Delay(10);

	//Page 0/Register 0: Page Select Register
	//reg_status = i2c_codec_register_read(0x00);
	//printf("i2c_codec_ctrl_init(): reg_status(0x00) = %x\n", reg_status);

	//Page 0/Register 94: Module Power Status Register
	reg_status = i2c_codec_register_read(0x5e);
	printf("i2c_codec_ctrl_init(): reg_status(0x5e) = %x\n", reg_status);

	//Page 0/Register 86: LEFT_LOP/M Output Level Control Register
	reg_status = i2c_codec_register_read(0x56);
	printf("i2c_codec_ctrl_init(): reg_status(0x56) = %x\n", reg_status);

	//Page 0/Register 93: RIGHT_LOP/M Output Level Control Register
	reg_status = i2c_codec_register_read(0x5d);
	printf("i2c_codec_ctrl_init(): reg_status(0x5d) = %x\n", reg_status);
	*/

	//Line Out routing --[end]------------------------------------------------------------------------

    return ret;
}

void codec_init()
{
	while(i2c_bus_mutex);
	i2c_bus_mutex = 1;

	printf("codec_init();\n");
	//uint8_t sensor_data_h, sensor_data_l;
    //int AGC_enabled = CODEC_ENABLE_AGC;
	int ret = i2c_codec_ctrl_init(I2C_MASTER_NUM, global_settings.AGC_ENABLED, global_settings.AGC_MAX_GAIN, global_settings.AGC_TARGET_LEVEL);//, &sensor_data_h, &sensor_data_l);

    if(ret == ESP_ERR_TIMEOUT) {
		printf("ESP_ERR_TIMEOUT (i2c)\n");

    	while(1)
    	{
    		error_blink(5,100,0,0,25,30); //RGB:count,delay
    	}
    } else if(ret == ESP_OK) {
		printf("Codec configured OK!\n");
		//RGB_LED_G_ON;
    	//error_blink(0,0,0,0,3,50); //RGB:count,delay
    } else { //no ACK
		printf("No ACK (i2c)\n");
    	while(1)
    	{
    		error_blink(25,30,0,0,5,100); //RGB:count,delay
    	}
    }
	i2c_bus_mutex = 0;
}

void codec_reset()
{
	printf("codec_reset();\n");
	gpio_set_direction(CODEC_RST_PIN, GPIO_MODE_OUTPUT_OD); //needs to be open drain otherwise partially blocks RST rubber button action
    gpio_set_level(CODEC_RST_PIN, 0);  //reset the codec
}

int codec_select_input(int input) //0 = off, 1 = mic, 2 = line-in
{
    int ret;

    if(input==ADC_INPUT_OFF)
    {
        //Page 0/Register 15: Left-ADC PGA Gain Control Register
    	//D7: 1: The left-ADC PGA is muted
    	//D6-D0: 000 0000: Gain = 0 dB
    	ret = i2c_codec_two_byte_command(0x0f, 0x80); //10000000 -> 0dB, muted
        if (ret != ESP_OK) { return ret; }

    	//Page 0/Register 16: Right-ADC PGA Gain Control Register
    	//D7: 1: The right ADC PGA is muted
    	//D6-D0: 000 0000: Gain = 0 dB
        ret = i2c_codec_two_byte_command(0x10, 0x80); //10000000 -> 0dB, muted
        if (ret != ESP_OK) { return ret; }

        //power down ADC

		//Page 0/Register 19: MIC1LP/LINE1LP to Left-ADC Control Register
		//D7: 1: MIC1LP/LINE1LP and MIC1LM/LINE1LM are configured in fully differential mode
		//D6-D3: 1111: LINE1L is not connected to the left-ADC PGA.
		//D2: 0: Left-ADC channel is not powered up
		//D1-D0: 00: Left-ADC PGA soft-stepping at once per sample period
		ret = i2c_codec_two_byte_command(0x13, 0xf8); //11111000
		if (ret != ESP_OK) { return ret; }

		//Page 0/Register 22: MIC1RP/LINE1RP to Right-ADC Control Register
		//D7: 1: MIC1RP/LINE1RP and MIC1RM/LINE1RM are configured in fully differential mode
		//D6-D3: 1111: LINE1R is not connected to the right-ADC PGA.
		//D2: 0: Right-ADC channel is not powered up
		//D1-D0: 00: Right-ADC PGA soft-stepping at once per sample period
		ret = i2c_codec_two_byte_command(0x16, 0xf8); //11111000
		if (ret != ESP_OK) { return ret; }

		return ESP_OK;
    }

	//ADC setup -----------------------------------------------------------------------------------

    //Page 0/Register 15: Left-ADC PGA Gain Control Register
	//D7: 0: The left-ADC PGA is not muted
	//D6-D0: 000 1100: Gain = 6 dB
	if(input==ADC_INPUT_LINE_IN) {
		ret = i2c_codec_two_byte_command(0x0f, 0x0c); //00000000 -> 0dB
		//ret = i2c_codec_two_byte_command(0x0f, 0x0c); //00001100 -> 6dB
		//ret = i2c_codec_two_byte_command(0x0f, 0x18); //00011000 -> 12dB
	} else {
		ret = i2c_codec_two_byte_command(0x0f, 0x40); //01000000 -> 32dB
		//ret = i2c_codec_two_byte_command(0x0f, 0x60); //01100000 -> 48dB
	}
    if (ret != ESP_OK) { return ret; }

	//Page 0/Register 16: Right-ADC PGA Gain Control Register
	//D7: 0: The right ADC PGA is not muted
	//D6-D0: 000 1100: Gain = 6 dB
    if(input==ADC_INPUT_LINE_IN) {
    	ret = i2c_codec_two_byte_command(0x10, 0x0c); //00000000 -> 0dB
    	//ret = i2c_codec_two_byte_command(0x10, 0x0c); //00001100 -> 6dB
    	//ret = i2c_codec_two_byte_command(0x10, 0x18); //00001100 -> 12dB
	} else {
		ret = i2c_codec_two_byte_command(0x10, 0x40); //01000000 -> 32dB
		//ret = i2c_codec_two_byte_command(0x10, 0x60); //01100000 -> 48dB
	}
    if (ret != ESP_OK) { return ret; }

	//Line-in setup -------------------------------------------------------------------------------

    if(input==ADC_INPUT_LINE_IN) {

    	//connect MIC2/LINE2 lines to ADC and set gain

    	//Page 0/Register 17: MIC2L/R to Left-ADC Control Register
		//MIC2L Input Level Control for Left-ADC PGA Mix
		//Setting the input level control to one of the following gains automatically connects MIC2L to the left-ADC PGA mix.
		//D7-D4: 0100: Input level control gain = -6 dB
		//D7-D4: 1000: Input level control gain = -12 dB
		//MIC2R/LINE2R Input Level Control for Left-ADC PGA Mix
		//Setting the input level control to one of the following gains automatically connects MIC2R to the left-ADC PGA mix.
		//D3-D0: 1111: MIC2R/LINE2R is not connected to the left-ADC PGA.
		//ret = i2c_codec_two_byte_command(0x11, 0x4f); //01001111 -> -6dB
		ret = i2c_codec_two_byte_command(0x11, 0x8f); //10001111 -> -12dB
		if (ret != ESP_OK) { return ret; }

		//Page 0/Register 18: MIC2/LINE2 to Right-ADC Control Register
		//MIC2L/LINE2L Input Level Control for Right -DC PGA Mix
		//Setting the input level control to one of the following gains automatically connects MIC2L to the right-ADC PGA mix.
		//D7-D4: 1111: MIC2L/LINE2L is not connected to the right-ADC PGA.
		//MIC2R/LINE2R Input Level Control for Right-ADC PGA Mix
		//Setting the input level control to one of the following gains automatically connects MIC2R to the right-ADC PGA mix.
		//D3-D0: 0100: Input level control gain = -6 dB
		//ret = i2c_codec_two_byte_command(0x12, 0xf4); //11110100 -> -6dB
		ret = i2c_codec_two_byte_command(0x12, 0xf8); //11111000 -> -12dB
		if (ret != ESP_OK) { return ret; }

		//power up ADC but do not connect MIC1/LINE1 lines

		//Page 0/Register 19: MIC1LP/LINE1LP to Left-ADC Control Register
		//D7: 1: MIC1LP/LINE1LP and MIC1LM/LINE1LM are configured in fully differential mode
		//D6-D3: 1111: LINE1L is not connected to the left-ADC PGA.
		//D2: 1: Left-ADC channel is powered up
		//D1-D0: 00: Left-ADC PGA soft-stepping at once per sample period
		ret = i2c_codec_two_byte_command(0x13, 0xfc); //11111100
		if (ret != ESP_OK) { return ret; }

		//Page 0/Register 22: MIC1RP/LINE1RP to Right-ADC Control Register
		//D7: 1: MIC1RP/LINE1RP and MIC1RM/LINE1RM are configured in fully differential mode
		//D6-D3: 1111: LINE1R is not connected to the right-ADC PGA.
		//D2: 1: Right-ADC channel is powered up
		//D1-D0: 00: Right-ADC PGA soft-stepping at once per sample period
		ret = i2c_codec_two_byte_command(0x16, 0xfc); //11111100
		if (ret != ESP_OK) { return ret; }

	} else {

    	//disconnect MIC2/LINE2 lines

		//Page 0/Register 17: MIC2L/R to Left-ADC Control Register
		//D7-D4: 1111: MIC2L is not connected to the left-ADC PGA.
		//D3-D0: 1111: MIC2R/LINE2R is not connected to the left-ADC PGA.
		ret = i2c_codec_two_byte_command(0x11, 0xff); //11111111
		if (ret != ESP_OK) { return ret; }

		//Page 0/Register 18: MIC2/LINE2 to Right-ADC Control Register
		//D7-D4: 1111: MIC2L/LINE2L is not connected to the right-ADC PGA.
		//D3-D0: 1111: MIC2R/LINE2R is not connected to right-ADC PGA.
		ret = i2c_codec_two_byte_command(0x12, 0xff); //11111111
		if (ret != ESP_OK) { return ret; }

    	//connect MIC1/LINE1 lines to ADC and set gain

		//Page 0/Register 19: MIC1LP/LINE1LP to Left-ADC Control Register
		//D7: 1: MIC1LP/LINE1LP and MIC1LM/LINE1LM are configured in fully differential mode
		//D6-D3: 0010: Input level control gain = -3 dB
		//D2: 1: Left-ADC channel is powered up
		//D1-D0: 00: Left-ADC PGA soft-stepping at once per sample period
		ret = i2c_codec_two_byte_command(0x13, 0x94); //10010100
		if (ret != ESP_OK) { return ret; }

		//Page 0/Register 22: MIC1RP/LINE1RP to Right-ADC Control Register
		//D7: 1: MIC1RP/LINE1RP and MIC1RM/LINE1RM are configured in fully differential mode
		//D6-D3: 0010: Input level control gain = -3 dB
		//D2: 1: Right-ADC channel is powered up
		//D1-D0: 00: Right-ADC PGA soft-stepping at once per sample period
		ret = i2c_codec_two_byte_command(0x16, 0x94); //10010100
		if (ret != ESP_OK) { return ret; }
	}

    return ESP_OK;
}

void codec_set_analog_volume()
{
	//printf("codec_set_analog_volume(): codec_analog_volume = %d\n", codec_analog_volume);
	//printf("ca=%d ", codec_analog_volume);

	//Page 0/Register 47: DAC_L1 to HPLOUT Volume Control Register
	//D7: 1: DAC_L1 is routed to HPLOUT
	//D6-D0: DAC_L1 to HPLOUT Analog Volume Control -> 000 1100: Gain = -6 dB
    //range = 0dB .. -78dB
    int ret = i2c_codec_two_byte_command(0x2f, 0x80 | codec_analog_volume);
    if (ret != ESP_OK) { printf("codec_set_analog_volume() problem in i2c_two_byte_command(%x,%x)\n",0x2f, 0x80 | codec_analog_volume); }
    //Page 0/Register 64: DAC_R1 to HPROUT Volume Control Register
    ret = i2c_codec_two_byte_command(0x40, 0x80 | codec_analog_volume);
    if (ret != ESP_OK) { printf("codec_set_analog_volume() problem in i2c_two_byte_command(%x,%x)\n",0x40, 0x80 | codec_analog_volume); }
}

void codec_set_digital_volume()
{
	//printf("codec_set_digital_volume(): codec_digital_volume = %d\n", codec_digital_volume);
	//printf("cd=%d ", codec_digital_volume);


    //Page 0/Register 43: Left-DAC Digital Volume Control Register
    //D7: 0: The left-DAC channel is not muted
	//D6-D0: 001 1000: Gain = -12 dB
	//D6-D0: 000 1100: Gain = -6 dB
    //range = 0dB .. -63dB

    //Right-DAC volume follows the left-DAC digital volume control register (by setting reg.41 D1-D0)
    int ret = i2c_codec_two_byte_command(0x2b, codec_digital_volume);
    if (ret != ESP_OK) { printf("codec_set_digital_volume() problem in i2c_two_byte_command(%x,%x)\n",0x2b, codec_digital_volume); }
}

void codec_set_mute(int status)
{
	if(status) //mute
	{
		//only if volume_ramp not in process
		if(!volume_ramp)
		{
			codec_volume_user = codec_digital_volume; //store the user-set level
			printf("codec_set_mute(%d): storing user-set level = %d\n",status,codec_volume_user);
		}

	    codec_digital_volume = CODEC_DIGITAL_VOLUME_MUTE;
	    codec_set_digital_volume();
		//codec_analog_volume = CODEC_ANALOG_VOLUME_MUTE;
	    //codec_set_analog_volume();
	}
	else //un-mute
	{
		//codec_analog_volume = CODEC_ANALOG_VOLUME_DEFAULT;
	    //codec_set_analog_volume();
		codec_digital_volume = codec_volume_user; //load the user-set level
		printf("codec_set_mute(%d): loading user-set level = %d\n",status,codec_volume_user);
	    codec_set_digital_volume();
	}
}

int codec_set_eq()
{
	EQ_coefs[1] = EQ_coef_options[(EQ_bass_setting/EQ_BASS_STEP)+EQ_COEFS_BASS_0][0];
	EQ_coefs[2] = EQ_coef_options[(EQ_bass_setting/EQ_BASS_STEP)+EQ_COEFS_BASS_0][1];
	EQ_coefs[3] = EQ_coef_options[(EQ_bass_setting/EQ_BASS_STEP)+EQ_COEFS_BASS_0][2];
	EQ_coefs[7] = EQ_coef_options[(EQ_bass_setting/EQ_BASS_STEP)+EQ_COEFS_BASS_0][3];
	EQ_coefs[8] = EQ_coef_options[(EQ_bass_setting/EQ_BASS_STEP)+EQ_COEFS_BASS_0][4];

	EQ_coefs[4] = EQ_coef_options[(EQ_treble_setting/EQ_TREBLE_STEP)+EQ_COEFS_TREBLE_0][0];
	EQ_coefs[5] = EQ_coef_options[(EQ_treble_setting/EQ_TREBLE_STEP)+EQ_COEFS_TREBLE_0][1];
	EQ_coefs[6] = EQ_coef_options[(EQ_treble_setting/EQ_TREBLE_STEP)+EQ_COEFS_TREBLE_0][2];
	EQ_coefs[9] = EQ_coef_options[(EQ_treble_setting/EQ_TREBLE_STEP)+EQ_COEFS_TREBLE_0][3];
	EQ_coefs[10] = EQ_coef_options[(EQ_treble_setting/EQ_TREBLE_STEP)+EQ_COEFS_TREBLE_0][4];

	int ret;

	//make sure we are at page 0
	//D0: Page Select Bit
	ret = i2c_codec_two_byte_command(0x00, 0x00);
	if (ret != ESP_OK) { return ret; }

	//Page 0/Register 12: Audio Codec Digital Filter Control Register
    ret = i2c_codec_two_byte_command(0x0c, 0x00); //disable all filters
    if (ret != ESP_OK) { return ret; }

    //select page 1 to access EQ coefficients registers
	//D0: Page Select Bit
	ret = i2c_codec_two_byte_command(0x00, 0x01);
	if (ret != ESP_OK) { return ret; }

	//printf("Setting EQ Coeffs: %d %d %d %d %d %d %d %d %d %d\n",EQ_coefs[1],EQ_coefs[2],EQ_coefs[3],EQ_coefs[4],EQ_coefs[5],EQ_coefs[6],EQ_coefs[7],EQ_coefs[8],EQ_coefs[9],EQ_coefs[10]);
	swap_endian(EQ_coefs, EQ_coefs_se, EQ_COEFS+1);

	//write default coefficients array
	//Page 1/Register 1: Left-Channel Audio Effects Filter N0 Coefficient MSB Register (and subsequent registers)
	((unsigned char*)EQ_coefs_se)[1] = 0x01;
	ret = i2c_bus_write(CODEC_I2C_ADDRESS << 1 | WRITE_BIT, (unsigned char*)EQ_coefs_se + 1, 1 + EQ_COEFS * 2);
	if (ret != ESP_OK) { return ret; }

	//Page 1/Register 27: Right-Channel Audio Effects Filter N0 Coefficient MSB Register (and subsequent registers)
	((unsigned char*)EQ_coefs_se)[1] = 0x1B;
	ret = i2c_bus_write(CODEC_I2C_ADDRESS << 1 | WRITE_BIT, (unsigned char*)EQ_coefs_se + 1, 1 + EQ_COEFS * 2);
	if (ret != ESP_OK) { return ret; }

	//select page 0 again
	//D0: Page Select Bit
	ret = i2c_codec_two_byte_command(0x00, 0x00);
	if (ret != ESP_OK) { return ret; }

    //Page 0/Register 12: Audio Codec Digital Filter Control Register
    //D7-D6: Left-ADC High-Pass Filter Control -> 00: Left-ADC high-pass filter disabled
    //D5-D4: Right-ADC High-Pass Filter Control -> 00: Right-ADC high-pass filter disabled
    //D3: Left-DAC Digital Effects Filter Control -> 1: Left-DAC digital effects filter enabled
    //D2: Left-DAC De-Emphasis Filter Control -> 0: Left-DAC de-emphasis filter disabled (bypassed)
    //D1: Right-DAC Digital Effects Filter Control -> 1: Right-DAC digital effects filter enabled
    //D0: Right-DAC De-Emphasis Filter Control -> 0: Right-DAC de-emphasis filter disabled (bypassed)
    ret = i2c_codec_two_byte_command(0x0c, 0x0a); //00001010
    if (ret != ESP_OK) { return ret; }

    return ESP_OK;
}

int codec_configure_AGC(int8_t enabled, int8_t max_gain, int8_t target_level)
{
	int ret = 0;

	if(!enabled) //if not enabled, send command to disable it
	{
		//Page 0/Register 26: Left-AGC Control Register A
		//D7: R/W 0 Left-AGC Enable
		//0: Left AGC is disabled.
		//1: Left AGC is enabled.
		ret = i2c_codec_two_byte_command(0x1a, 0x00); //00000000 - disabled
		if (ret != ESP_OK) { return ret; }
		ret = i2c_codec_two_byte_command(0x1d, 0x00); //the same for Register 29: Right-AGC Control Register A
		//if (ret != ESP_OK) { return ret; }

		return ret;
	}

	//Page 0/Register 26: Left-AGC Control Register A
	//D7: R/W 0 Left-AGC Enable
	//0: Left AGC is disabled.
	//1: Left AGC is enabled.
	//D6–D4: R/W 000 Left-AGC Target Level
	//000: Left-AGC target level = –5.5 dB
	//001: Left-AGC target level = –8 dB
	//010: Left-AGC target level = –10 dB
	//011: Left-AGC target level = –12 dB
	//100: Left-AGC target level = –14 dB
	//101: Left-AGC target level = –17 dB
	//110: Left-AGC target level = –20 dB
	//111: Left-AGC target level = –24 dB
	//D3–D2 R/W 00 Left-AGC Attack Time
	//00: Left-AGC attack time = 8 ms
	//01: Left-AGC attack time = 11 ms
	//10: Left-AGC attack time = 16 ms
	//11: Left-AGC attack time = 20 ms
	//D1–D0 R/W 00 Left-AGC Decay Time
	//00: Left-AGC decay time = 100 ms
	//01: Left-AGC decay time = 200 ms
	//10: Left-AGC decay time = 400 ms
	//11: Left-AGC decay time = 500 ms

	//ret = i2c_codec_two_byte_command(0x1a, 0x96); //10010110 - enabled,-8dB,11ms,400ms
	//ret = i2c_codec_two_byte_command(0x1d, 0x96); //the same for Register 29: Right-AGC Control Register A
	#if defined(USE_LINE_IN) && !defined(USE_MICS)
		ret = i2c_codec_two_byte_command(0x1a, 0xf6); //11110110 - enabled,-24dB,11ms,400ms
		if (ret != ESP_OK) { return ret; }
		ret = i2c_codec_two_byte_command(0x1d, 0xf6); //the same for Register 29: Right-AGC Control Register A
		if (ret != ESP_OK) { return ret; }
	#else
		printf("AGC Target Level set to %d\n", target_level);

		//#if CODEC_AGC_LEVEL == 5
		if(target_level==-5) {
			ret = i2c_codec_two_byte_command(0x1a, 0x83); //10000011 - enabled,-5.5dB,8ms,500ms
			ret = i2c_codec_two_byte_command(0x1d, 0x83); //the same for Register 29: Right-AGC Control Register A
		//#elif CODEC_AGC_LEVEL == 8
		} else if(target_level==-8) {
			ret = i2c_codec_two_byte_command(0x1a, 0x93); //10010011 - enabled,-8dB,8ms,500ms
			ret = i2c_codec_two_byte_command(0x1d, 0x93); //the same for Register 29: Right-AGC Control Register A
		} else if(target_level==-10) {
			ret = i2c_codec_two_byte_command(0x1a, 0xa3); //10100011 - enabled,-10dB,8ms,500ms
			ret = i2c_codec_two_byte_command(0x1d, 0xa3); //the same for Register 29: Right-AGC Control Register A
		//#elif CODEC_AGC_LEVEL == 12
		} else if(target_level==-12) {
			ret = i2c_codec_two_byte_command(0x1a, 0xb3); //10110011 - enabled,-12dB,8ms,500ms
			ret = i2c_codec_two_byte_command(0x1d, 0xb3); //the same for Register 29: Right-AGC Control Register A
		} else if(target_level==-14) {
			ret = i2c_codec_two_byte_command(0x1a, 0xc3); //11000011 - enabled,-14dB,8ms,500ms
			ret = i2c_codec_two_byte_command(0x1d, 0xc3); //the same for Register 29: Right-AGC Control Register A
		//#elif CODEC_AGC_LEVEL == 17
		} else if(target_level==-17) {
			ret = i2c_codec_two_byte_command(0x1a, 0xd3); //11010011 - enabled,-17dB,8ms,500ms
			ret = i2c_codec_two_byte_command(0x1d, 0xd3); //the same for Register 29: Right-AGC Control Register A
		} else if(target_level==-20) {
			ret = i2c_codec_two_byte_command(0x1a, 0xe3); //11100011 - enabled,-17dB,8ms,500ms
			ret = i2c_codec_two_byte_command(0x1d, 0xe3); //the same for Register 29: Right-AGC Control Register A
		} else if(target_level==-24) {
			ret = i2c_codec_two_byte_command(0x1a, 0xf3); //11110011 - enabled,-17dB,8ms,500ms
			ret = i2c_codec_two_byte_command(0x1d, 0xf3); //the same for Register 29: Right-AGC Control Register A
		//#endif
		}
		//if (ret != ESP_OK) { return ret; }
		if (ret != ESP_OK) { return ret; }
	#endif

	//Page 0/Register 27: Left-AGC Control Register B
	//D7-D1: R/W 1111 111 Left-AGC Maximum Gain Allowed
	//0000 000: Maximum gain = 0 dB
	//0000 001: Maximum gain = 0.5 dB
	//0000 010: Maximum gain = 1 dB
	//…
	//1110 110: Maximum gain = 59 dB
	//1110 111–111 111: Maximum gain = 59.5 dB
	//D0 R/W: 0 Reserved. Write only zero to this bit.
	//ret = i2c_two_byte_command(0x1b, 0x00); //00000000 - max.gain 0dB

	//ret = i2c_two_byte_command(0x1b, 0x18); //00011000 - max.gain 6dB
	//ret = i2c_two_byte_command(0x1e, 0x18); //the same for Register 30: Right-AGC Control Register B
	//ret = i2c_two_byte_command(0x1b, 0x38); //00111000 - max.gain 14dB
	//ret = i2c_two_byte_command(0x1e, 0x38); //the same for Register 30: Right-AGC Control Register B
	//ret = i2c_codec_two_byte_command(0x1b, 0x60); //01100000 - max.gain 24dB

	//ret = i2c_codec_two_byte_command(0x1e, 0x60); //the same for Register 30: Right-AGC Control Register B

	uint8_t AGC_max_gain_u; //in dB, range is 0..60dB (actually 59.5dB)
	AGC_max_gain_u = max_gain;
	//AGC_max_gain_u = 32;
	//AGC_max_gain_u = 40; //causes feedback loop on high frequencies, which limiter does not detect
	//AGC_max_gain_u = 48;
	//AGC_max_gain_u = 56;

	printf("AGC Max Gain set to %u\n", AGC_max_gain_u);

	ret = i2c_codec_two_byte_command(0x1b, AGC_max_gain_u<<2); //resolution is 0.5dB, the LSB is reserved (0)
	if (ret != ESP_OK) { return ret; }
	ret = i2c_codec_two_byte_command(0x1e, AGC_max_gain_u<<2); //the same for Register 30: Right-AGC Control Register B
	if (ret != ESP_OK) { return ret; }

	//Page 0/Register 28: Left-AGC Control Register C
	//D7-D6: R/W 00 Noise Gate Hysteresis Level Control
	//00: Hysteresis = 1 dB
	//01: Hysteresis = 2 dB
	//10: Hysteresis = 3 dB
	//11: Hysteresis is disabled.
	//D5–D1: R/W 00 000 Left-AGC Noise Threshold Control
	//00000: Left-AGC noise/silence detection disabled
	//00001: Left-AGC noise threshold = –30 dB
	//00010: Left-AGC noise threshold = –32 dB
	//00011: Left-AGC noise threshold = –34 dB
	//…
	//11101: Left-AGC noise threshold = –86 dB
	//11110: Left-AGC noise threshold = –88 dB
	//11111: Left-AGC noise threshold = –90 dB
	//D0: R/W 0 Left-AGC Clip Stepping Control
	//0: Left-AGC clip stepping disabled
	//1: Left-AGC clip stepping enabled

	//ret = i2c_two_byte_command(0x1c, 0x0f); //00001111 - hysteresis 1dB, threshold -42db, clip stepping enabled
	//ret = i2c_two_byte_command(0x1f, 0x0f); //the same for Register 31: Right-AGC Control Register C
	//if (ret != ESP_OK) { return ret; }

	return ret;
}

void start_MCLK()
{
	printf("Starting MCLK...\n");
	enable_I2S_MCLK_clock();
	mclk_enabled = 1;
}

void stop_MCLK()
{
	disable_I2S_MCLK_clock();
	mclk_enabled = 0;
	printf("MCLK stopped\n");
}

void set_sampling_rate(int new_rate)
{
	//printf("set_sampling_rate(%d)\n", new_rate);
	esp_err_t ret = i2s_set_sample_rates(I2S_NUM, new_rate);
    if(ret != ESP_OK)
    {
    	printf("set_sampling_rate(%d): problem setting sampling rate, err = %d\n", new_rate, ret);
    }
    else
    {
    	printf("set_sampling_rate(%d): new sampling rate set\n", new_rate);
    }
}

void beep(int freq_div)
{
	if(!beeps_enabled)
	{
		return;
	}
	if(volume_ramp)
	{
		codec_digital_volume = codec_volume_user;
		codec_set_digital_volume();
	}
	if(freq_div>10)
	{
		while(freq_div>10)
		{
			add_beep = freq_div%10;
			Delay(100);
			freq_div/=10;
		}
	}
	add_beep = freq_div;
	Delay(100);
	add_beep = 0;
}
