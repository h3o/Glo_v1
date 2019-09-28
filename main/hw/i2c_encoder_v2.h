/*
 * init.c
 *
 *  Created on: Mar 3, 2019
 *      Author: Simone Caron
 *    Based on: https://github.com/Fattoresaimon/ArduinoDuPPaLib/blob/master/src/i2cEncoderLibV2.h
 *   Get it at: https://www.tindie.com/products/Saimon/i2cencoder-v2-connect-multiple-encoder-on-i2c-bus/
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

#ifndef i2cEncoderLibV2_H
#define i2cEncoderLibV2_H

/*Encoder register definition*/
enum I2C_Register {
  REG_GCONF   =  0x00,
  REG_GP1CONF  = 0x01,
  REG_GP2CONF   = 0x02,
  REG_GP3CONF  = 0x03,
  REG_INTCONF  = 0x04,
  REG_ESTATUS  = 0x05,
  REG_I2STATUS = 0x06,
  REG_FSTATUS  = 0x07,
  REG_CVALB4  = 0x08,
  REG_CVALB3  = 0x09,
  REG_CVALB2  = 0x0A,
  REG_CVALB1  = 0x0B,
  REG_CMAXB4  = 0x0C,
  REG_CMAXB3  = 0x0D,
  REG_CMAXB2  = 0x0E,
  REG_CMAXB1  = 0x0F,
  REG_CMINB4  = 0x10,
  REG_CMINB3  = 0x11,
  REG_CMINB2  = 0x12,
  REG_CMINB1  = 0x13,
  REG_ISTEPB4 = 0x14,
  REG_ISTEPB3 = 0x15,
  REG_ISTEPB2 = 0x16,
  REG_ISTEPB1 = 0x17,
  REG_RLED    = 0x18,
  REG_GLED = 0x19,
  REG_BLED = 0x1A,
  REG_GP1REG = 0x1B,
  REG_GP2REG = 0x1C,
  REG_GP3REG = 0x1D,
  REG_ANTBOUNC = 0x1E,
  REG_DPPERIOD = 0x1F,
  REG_FADERGB  = 0x20,
  REG_FADEGP = 0x21,
  REG_EEPROMS  = 0x80,
} i2creg;

/* Encoder configuration bit. Use with GCONF */
enum GCONF_PARAMETER {
  FLOAT_DATA  = 0x01,
  INT_DATA  = 0x00,

  WRAP_ENABLE = 0x02,
  WRAP_DISABLE = 0x00,

  DIRE_LEFT = 0x04,
  DIRE_RIGHT  = 0x00,

  IPUP_DISABLE =  0x08,
  IPUP_ENABLE = 0x00,

  RMOD_X2 = 0x10,
  RMOD_X1 = 0x00,

  RGB_ENCODER = 0x20,
  STD_ENCODER = 0x00,

  EEPROM_BANK1 =  0x40,
  EEPROM_BANK2 =  0x00,

  RESET =  0x80,
};

/* Encoder status bits and setting. Use with: INTCONF for set and with ESTATUS for read the bits  */
enum Int_Status {
  PUSHR = 0x01,
  PUSHP = 0x02,
  PUSHD = 0x04,
  RINC  = 0x08,
  RDEC  = 0x10,
  RMAX  = 0x20,
  RMIN  = 0x40,
  INT2  = 0x80,
};

/* Encoder Int2 bits. Use to read the bits of I2STATUS  */
enum Int2_Status {
  GP1_POS  = 0x01,
  GP1_NEG  = 0x02,
  GP2_POS  = 0x04,
  GP2_NEG  = 0x08,
  GP3_POS  = 0x10,
  GP3_NEG  = 0x20,
  FADE_INT = 0x40,
};

/* Encoder Fade status bits. Use to read the bits of FSTATUS  */
enum Fade_Status {
  FADE_R = 0x01,
  FADE_G = 0x02,
  FADE_B = 0x04,
  FADES_GP1 = 0x08,
  FADES_GP2 = 0x10,
  FADES_GP3 = 0x20,
};

/* GPIO Configuration. Use with GP1CONF,GP2CONF,GP3CONF */
enum GP_PARAMETER {
  GP_PWM = 0x00,
  GP_OUT = 0x01,
  GP_AN = 0x02,
  GP_IN = 0x03,

  GP_PULL_EN = 0x04,
  GP_PULL_DI = 0x00,

  GP_INT_DI = 0x00,
  GP_INT_PE = 0x08,
  GP_INT_NE = 0x10,
  GP_INT_BE = 0x18,
};


/* Interrupt source for the callback. Use with attachInterrupt */
enum SourceInt {
  BUTTON_RELEASE,
  BUTTON_PUSH,
  BUTTON_DOUBLE_PUSH,
  ENCODER_INCREMENT,
  ENCODER_DECREMENT,
  ENCODER_MAX,
  ENCODER_MIN,
  GP1_POSITIVE,
  GP1_NEGATIVE,
  GP2_POSITIVE,
  GP2_NEGATIVE,
  GP3_POSITIVE,
  GP3_NEGATIVE,
  FADE,
  SOURCE_INT_NUMB,
};

#endif
