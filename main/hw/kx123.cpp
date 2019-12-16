/*   Copyright 2016 Rohm Semiconductor

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
//#include "RegisterWriter/RegisterWriter/rohm_hal2.h"
//#include "RegisterWriter/RegisterWriter/RegisterWriter.h"
#include "driver/i2c.h"
#include "init.h"
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "sdkconfig.h"

#include "kx123.h"

#define DEBUG_print printf
#define I2C_WRITE I2C_MASTER_WRITE
#define I2C_READ I2C_MASTER_READ

/**
* Create a KX123 instance for communication through pre-instantiated
* RegisterWriter (I2C) -object.
*
* @param i2c_obj pre-instantiated RegisterWriter (I2C) -object reference
* @param sad slave address of sensor.
* @param wai who_am_i value (i.e. sensor type/model)
*/
KX123::KX123(i2c_port_t i2c_port, uint8_t sad, uint8_t wai) { //: i2c_rw(i2c_obj) {
    _sad = sad;
    _wai = wai;        
    setup_mode_on = false;
    resolution_divider = 0;
}

KX123::~KX123(){
}

/**
* Start setup/stand-by mode and shut off measurements.
* @return true on error, false on ok
*/
bool KX123::start_setup_mode(void){
    setup_mode_on = true;
    return change_bits(_sad, KX122_CNTL1, KX122_CNTL1_PC1, KX122_CNTL1_PC1);
}

/**
* Start operating/measurement mode. Setup is not allowed while in this mode.
* @return true on error, false on ok
*/
bool KX123::start_measurement_mode(void){
    setup_mode_on = false;
    return change_bits(_sad, KX122_CNTL1, KX122_CNTL1_PC1, 0);
}

/**
* Check if sensor is connected, setup defaults to sensor and start measuring.
* @return true on error, false on ok
*/
bool KX123::set_defaults()
{
    unsigned char buf;

    //DEBUG_print("\n\r");
    DEBUG_print("KX123 init started\n\r");
    read_register(_sad, KX122_WHO_AM_I, &buf, 1);
    if (buf == KX123_WHO_AM_I_WAI_ID) {
        DEBUG_print("KX123 found. (WAI %d) ", buf);
    } else {
        DEBUG_print("KX123 not found (WAI %d, not %d). ", buf, KX123_WHO_AM_I_WAI_ID);
        switch(buf){        
            case KX012_WHO_AM_I_WAI_ID:
                DEBUG_print("Found KX012");
                break;
            case KX022_WHO_AM_I_WAI_ID:
                DEBUG_print("Found KX022");
                break;
            case KX023_WHO_AM_I_WAI_ID:
                DEBUG_print("Found KX023");
                break;
            case KX23H_WHO_AM_I_WAI_ID:
                DEBUG_print("Found KX23H");
                break;
            case KX112_WHO_AM_I_WAI_ID:
                DEBUG_print("Found KX112");
                break;
            case KX122_WHO_AM_I_WAI_ID:
                DEBUG_print("Found KX122");
                break;
            case KX124_WHO_AM_I_WAI_ID:
                DEBUG_print("Found KX124");
                break;
            case KX222_WHO_AM_I_WAI_ID:
                DEBUG_print("Found KX222");
                break;
            case KX224_WHO_AM_I_WAI_ID:
                DEBUG_print("Found KX224");
                break;
            default:
                DEBUG_print("Not even other sensor found from same family.\n\r");
                return true;
            }
            DEBUG_print(" though, trying to use that.\n\r");
    }

    //First set CNTL1 PC1-bit to stand-by mode, after that setup can be made
    write_register(_sad, KX122_CNTL1, 0 );
    setup_mode_on = true;

    set_tilt_position_defaults();

    //ODCNTL: Output Data Rate control (ODR) 
    write_register(_sad, KX122_ODCNTL, KX122_ODCNTL_OSA_25600);
    //Setup G-range and 8/16-bit resolution + set CNTL1 PC1-bit to operating mode (also WUF_EN, TP_EN and DT_EN)

    //write_register(_sad, KX122_CNTL1, ( KX122_CNTL1_PC1 | KX122_CNTL1_GSEL_8G | KX122_CNTL1_RES ) );
    write_register(_sad, KX122_CNTL1, ( KX122_CNTL1_PC1 | KX122_CNTL1_GSEL_8G | KX122_CNTL1_TDTE | KX122_CNTL1_RES ) );
    setup_mode_on = false;

    //resolution_divider = 32768/2;  //KX122_CNTL1_GSEL_2G
    //resolution_divider = 32768/4;  //KX122_CNTL1_GSEL_4G
    resolution_divider = 32768/8;  //KX122_CNTL1_GSEL_8G
    
    return false;
}


/**
* Setup default settings for a tilt position
**/
void KX123::set_tilt_position_defaults(){
    if (setup_mode_on == false) return;
    //CNTL3: Tilt position control, directional tap control and motion wakeup control
    write_register(_sad, KX122_CNTL3, ( KX122_CNTL3_OTP_50 | KX122_CNTL3_OTDT_400 ) );
    //TILT_TIMER: Setup tilt position timer (~=filter)
    write_register(_sad, KX122_TILT_TIMER, 0x01);
    return;
}


/**
* Get filtered uint16_t XYZ-values from sensor
* @param *buf to uint16_t[3] for results
* @return true on error, false on read ok.
**/
bool KX123::getresults_highpass(int16_t* buf) {
    #define RESULTS_LEN 6
    uint8_t tmp[RESULTS_LEN];     //XYZ (lhlhlh)
    uint8_t read_bytes;

    read_bytes = read_register(_sad, KX122_XHP_L, &tmp[0], sizeof(tmp));
    if (read_bytes != RESULTS_LEN){
        return true;
        }
    buf[0] = ( tmp[1] << 8 ) | tmp[0];  //X
    buf[1] = ( tmp[3] << 8 ) | tmp[2];  //Y
    buf[2] = ( tmp[5] << 8 ) | tmp[4];  //Z
    return false;
}

/**
* Get raw uint16_t XYZ-values from sensor
* @param *buf to uint16_t[3] for results
* @return true on error, false on read ok.
**/
bool KX123::getresults_raw(int16_t* buf){
    #define RESULTS_LEN 6
    uint8_t tmp[RESULTS_LEN];     //XYZ (lhlhlh)
    uint8_t read_bytes;

    read_bytes = read_register(_sad, KX122_XOUT_L, &tmp[0], sizeof(tmp));
    if (read_bytes != RESULTS_LEN){
        return true;
        }
    buf[0] = ( tmp[1] << 8 ) | tmp[0];  //X
    buf[1] = ( tmp[3] << 8 ) | tmp[2];  //Y
    buf[2] = ( tmp[5] << 8 ) | tmp[4];  //Z
    return false;
}


/**
* Get gravity scaled float XYZ-values from sensor
* @param *buf to float[3] for results
* @return true on error, false on read ok.
**/
bool KX123::getresults_g(float* buf){
    int16_t raw[3];
    int read_error;

    read_error = getresults_raw(&raw[0]);
    if (read_error){
        printf("getresults_g(): read error in getresults_raw()!\n");
    	return read_error;
        }

    //Scale raw values to G-scale
    buf[0] = ((float)raw[0]) / resolution_divider;
    buf[1] = ((float)raw[1]) / resolution_divider;
    buf[2] = ((float)raw[2]) / resolution_divider;
    return false;
}

/**
* Get gravity scaled float XYZ-values from highpass filtered sensor values
* @param *buf to float[3] for results
* @return true on error, false on read ok.
**/
bool KX123::getresults_highpass_g(float* buf){
    int16_t raw[3];
    int read_error;

    read_error = getresults_highpass(&raw[0]);
    if (read_error){
        printf("getresults_highpass_g(): read error in getresults_highpass()!\n");
        return read_error;
        }

    //Scale raw values to G-scale
    buf[0] = ((float)raw[0]) / resolution_divider;
    buf[1] = ((float)raw[1]) / resolution_divider;
    buf[2] = ((float)raw[2]) / resolution_divider;
    return false;
}

/**
* Get axes of current tilt and previous tilt
* @param *current_previous space for storing 2 (uint8_t) values
* @return true on error
*/
bool KX123::get_tilt(enum e_axis* current_previous){
    #define GET_TILT_READ_LEN 2
    uint8_t read_bytes;

    read_bytes = read_register(_sad, KX122_TSCP, (uint8_t*)current_previous, GET_TILT_READ_LEN);

    return (read_bytes != GET_TILT_READ_LEN);
}

/**
* Get axis of triggered tap/double tap interrupt from INS1
* @param *axis space for storing 1 (uint8_t) value in e_axis format
* @return true on error
*/
bool KX123::get_tap_interrupt_axis(enum e_axis* axis){
    #define GET_TAP_READ_LEN 1
    uint8_t read_bytes;
    
    read_bytes = read_register(_sad, KX122_INS1, (uint8_t*)axis, GET_TAP_READ_LEN);

    return (read_bytes != GET_TAP_READ_LEN);
}

/**
* Get axis of triggered motion detect interrupt from INS3
* @param *axis space for storing 1 (uint8_t) value in e_axis format
* @return true on error
*/
bool KX123::get_detected_motion_axis(enum e_axis* axis){
    #define GET_MOTION_READ_LEN 1
    uint8_t read_bytes;
    
    read_bytes = read_register(_sad, KX122_INS3, (uint8_t*)axis, GET_MOTION_READ_LEN);

    return (read_bytes != GET_MOTION_READ_LEN);
}

/**
* Set axis of triggered motion detect interrupt from CNTL2
* @param cnltl2_tilt_mask Orred e_axis values for axes directions to cause interrupt
* @return true on error or setup mode off
*/
bool KX123::set_tilt_axis_mask(uint8_t cnltl2_tilt_mask){
    if (setup_mode_on == false) return true;
    //MSb 00 == no_action
    return write_register(_sad, KX122_CNTL2, (cnltl2_tilt_mask & KX123_AXIS_MASK) );
}

/**
* get cause of interrupt trigger from INS2
* @param *int_reason space for storing 1 (uint8_t) value in e_interrupt_reason format
* @return true on error
*/
bool KX123::get_interrupt_reason(enum e_interrupt_reason* int_reason){
    #define INT_REASON_LEN 1
    uint8_t read_bytes;
    
    read_bytes = read_register(_sad, KX122_INS2, (uint8_t*)int_reason, INT_REASON_LEN);

    return (read_bytes != INT_REASON_LEN);
}

/**
* Check from sensor register if interrupt has occured. Usable feature when
* multiple sensor interrupts are connected to same interrupt pin.
* @return true when interrupt has occured. False on read fail and no interrupt.
*/
bool KX123::has_interrupt_occured(){
    uint8_t status_reg;
    uint8_t read_bytes;
    
    read_bytes = read_register(_sad, KX122_INS2, &status_reg, 1);
    return ((read_bytes == 1) && (status_reg & KX122_STATUS_REG_INT));
}

/**
* Clear interrupt flag when latched. (==Interrupt release) Doesn't work for FIFO
* Buffer Full or FIFO Watermark -interrupts.
*/
void KX123::clear_interrupt(){
    uint8_t value_discarded;

    read_register(_sad, KX122_INT_REL, &value_discarded, 1);

    return;
}

/**
* Initiate software reset and RAM reboot routine and wait untill finished.
*/
void KX123::soft_reset(){
    uint8_t reset_ongoing;

    change_bits(_sad, KX122_CNTL2, KX122_CNTL2_SRST, KX122_CNTL2_SRST );
    do{
        uint8_t cntl2, read_bytes;
        
        read_bytes = read_register(_sad, KX122_CNTL2, &cntl2, 1);
        reset_ongoing = ((read_bytes == 0) || (cntl2 & KX122_CNTL2_SRST));
    } while (reset_ongoing);
    
    return;
}

/**
* Verify proper integrated circuit functionality
* @return true on test fail or setup mode off, false on test ok.
**/
bool KX123::self_test(){
    uint8_t cotr_value;
    bool read_ok;

    if (setup_mode_on == false) return true;
    //first read to make sure COTR is in default value
    read_ok = read_register(_sad, KX122_COTR, &cotr_value, 1);
    read_ok = read_register(_sad, KX122_COTR, &cotr_value, 1);
    if ((cotr_value != 0x55) || (!read_ok) ){
        return true;
        }
    change_bits(_sad, KX122_CNTL2, KX122_CNTL2_COTC, KX122_CNTL2_COTC );
    read_ok = read_register(_sad, KX122_COTR, &cotr_value, 1);
    if ((cotr_value != 0xAA) || (!read_ok) ){
        return true;
        }
    return false;
}

/**
* Setup ODR values for Tilt Position, Directional Tap and Motion Detect.
* @param tilt_position_odr KX122_CNTL3_OTP_* -value or 0xff to skip.
* @param directional_tap_odr KX122_CNTL3_OTDT_* -value or 0xff to skip.
* @param motion_wuf_odr motion detect/high-pass odr (KX122_CNTL3_OWUF_* -value) or 0xff to skip.
* @return true on error or setup mode off, false on setup ok.
**/
bool KX123::set_cntl3_odrs(uint8_t tilt_position_odr, uint8_t directional_tap_odr, uint8_t motion_wuf_odr){
    uint8_t cntl3;
    bool read_ok;

    if (setup_mode_on == false) return true;

    read_ok = read_register(_sad, KX122_CNTL3, &cntl3, 1);
    if(!read_ok) return true;
    
    if (tilt_position_odr != 0xff){
        cntl3 = (cntl3 & ~KX122_CNTL3_OTP_MASK) | (tilt_position_odr & KX122_CNTL3_OTP_MASK);
        }
    if (directional_tap_odr != 0xff){
        cntl3 = (cntl3 & ~KX122_CNTL3_OTDT_MASK) | (directional_tap_odr & KX122_CNTL3_OTDT_MASK);
        }
    if (motion_wuf_odr != 0xff){
        cntl3 = (cntl3 & ~KX122_CNTL3_OWUF_MASK) | (motion_wuf_odr & KX122_CNTL3_OWUF_MASK);
        }
    return write_register(_sad, KX122_CNTL3, cntl3);
}

/**
* Setup ODR value, IIR-filter on/off and lowpass filter corner frequency.
* @param iir_filter_off False to filter ON or true to filter OFF.
* @param lowpass_filter_freq_half Filter corner frequency setup. False to ODR/9. True to ODR/2.
* @param odr Output Data Rate using KX122_ODCNTL_OSA_* -definitions.
* @return true on error or setup mode off, false on setup ok.
**/
bool KX123::set_odcntl(bool iir_filter_off, uint8_t lowpass_filter_freq_half, uint8_t odr){
    uint8_t odcntl;

    if (setup_mode_on == false) return true;

    odcntl = (odr & KX122_ODCNTL_OSA_MASK);
    if (iir_filter_off){
        odcntl = (odcntl | KX122_ODCNTL_IIR_BYPASS);
        }
    if (lowpass_filter_freq_half){
        odcntl = (odcntl | KX122_ODCNTL_LPRO);
        }

    return write_register(_sad, KX122_ODCNTL, odcntl);
}

/**
* Setup physical int1 pin, selftest polarity and SPI 3-wire on/off.
* @param pwsel Pulse width configuration in KX122_INC1_PWSEL1_* values
* @param physical_int_pin_enabled
* @param physical_int_pin_active_high (default true)
* @param physical_int_pin_latch_disabled
* @param self_test_polarity_positive
* @param spi3wire_enabled Use 3 wires instead of 4.
* @return true on error or setup mode off, false on setup ok.
**/
bool KX123::int1_setup(uint8_t pwsel,
    bool physical_int_pin_enabled,
    bool physical_int_pin_active_high,
    bool physical_int_pin_latch_disabled,
    bool self_test_polarity_positive,
    bool spi3wire_enabled){

    uint8_t inc1;

    if (setup_mode_on == false) return true;

    inc1 = (pwsel & KX122_INC1_PWSEL1_MASK);
    if (physical_int_pin_enabled){
        inc1 = (inc1 | KX122_INC1_IEN1);
        }
    if (physical_int_pin_active_high){
        inc1 = (inc1 | KX122_INC1_IEA1);
        }
    if (physical_int_pin_latch_disabled){
        inc1 = (inc1 | KX122_INC1_IEL1);
        }
    if (self_test_polarity_positive){
        inc1 = (inc1 | KX122_INC1_STPOL);
        }
    if (spi3wire_enabled){
        inc1 = (inc1 | KX122_INC1_SPI3E);
        }
    return write_register(_sad, KX122_INC1, inc1);
}


/**
* Setup physical int2 pin and int1/2 autoclear. 
* @param pwsel Pulse width configuration in KX122_INC1_PWSEL1_* values
* @param physical_int_pin_enabled
* @param physical_int_pin_active_high (default true)
* @param physical_int_pin_latch_disabled
* @param aclr2_enabled
* @param aclr1_enabled 
* @return true on error or setup mode off, false on setup ok.
**/
bool KX123::int2_setup(uint8_t pwsel,
    bool physical_int_pin_enabled,
    bool physical_int_pin_active_high,
    bool physical_int_pin_latch_disabled,
    bool aclr2_enabled,
    bool aclr1_enabled){

    uint8_t inc5;

    if (setup_mode_on == false) return true;

    inc5 = (pwsel & KX122_INC5_PWSEL2_MASK);
    if (physical_int_pin_enabled){
        inc5 = (inc5 | KX122_INC5_IEN2);
        }
    if (physical_int_pin_active_high){
        inc5 = (inc5 | KX122_INC5_IEA2);
        }
    if (physical_int_pin_latch_disabled){
        inc5 = (inc5 | KX122_INC5_IEL2);
        }
    if (aclr2_enabled){
        inc5 = (inc5 | KX122_INC5_ACLR2);
        }
    if (aclr1_enabled){
        inc5 = (inc5 | KX122_INC5_ACLR1);
        }
    return write_register(_sad, KX122_INC5, inc5);
}

/**
* Select interrupt reasons that can trigger interrupt for int1.
* @param interrupt_reason One or more e_interrupt_reason -values except doubletap.
* @return true on error or setup mode off, false on setup ok.
**/
bool KX123::set_int1_interrupt_reason(uint8_t interrupt_reason){
    if (setup_mode_on == false) return true;
    return write_register(_sad, KX122_INC4, interrupt_reason);
}

/**
* Select interrupt reasons that can trigger interrupt for int2.
* @param interrupt_reason One or more e_interrupt_reason -values except doubletap.
* @return true on error or setup mode off, false on setup ok.
**/
bool KX123::set_int2_interrupt_reason(uint8_t interrupt_reason){
    if (setup_mode_on == false) return true;
    return write_register(_sad, KX122_INC6, interrupt_reason);
}

/**
* Select axis that are monitored for motion detect interrupt.
* @param xxyyzz combination of e_axis -values for enabling axis
* @param axis_and_combination_enabled true for AND or false for OR
*
* true for AND configuration = (XN || XP) && (YN || YP) && (ZN || ZP)
*
* false for OR configuration = (XN || XP || YN || TP || ZN || ZP)
* @return true on error or setup mode off, false on setup ok.
**/
bool KX123::set_motion_detect_axis(uint8_t xxyyzz, bool axis_and_combination_enabled){
    uint8_t inc2;

    if (setup_mode_on == false) return true;

    inc2 = (xxyyzz & KX122_INC2_WUE_MASK);
    if (axis_and_combination_enabled){
        inc2 = (inc2 | KX122_INC2_AOI_AND);
        }
    return write_register(_sad, KX122_INC2, inc2);
}

/**
* Select axis that are monitored for tap/doubletap interrupt.
* @param xxyyzz combination of e_axis -values for enabling axis
* @return true on error or setup mode off, false on setup ok.
**/
bool KX123::set_tap_axis(uint8_t xxyyzz){
    if (setup_mode_on == false) return true;
    return write_register(_sad, KX122_INC3, xxyyzz);
}

bool KX123::change_bits(uint8_t sad, uint8_t reg, uint8_t mask, uint8_t bits){
    uint8_t value, read_bytes;
    read_bytes = read_register(sad, reg, &value, 1);
    if( read_bytes != 0 ){
        value = value & ~mask;
        value = value | (bits & mask);
        write_register(sad, reg, value);
        return true;
        }
    else{
        //DEBUG_printf("Read before change_bits() failed.");
        return false;
        }
}

uint8_t KX123::read_register(uint8_t sad, uint8_t reg, uint8_t* buf, uint8_t buf_len) {
    int error;

    i2c_bus_write( (int)((sad << 1) | I2C_WRITE), (unsigned char*)&reg, (int)1 );
    error = i2c_bus_read( (int)((sad << 1) | I2C_READ), (unsigned char*)buf, (int)buf_len);
    if(error)
    {
    	return 0;
    }
    //return( error );
    return buf_len;
}
/*
bool write_register(uint8_t sad, uint8_t reg, uint8_t* data, uint8_t data_len) {
    if (write_single)
        return write_register_single(sad, reg, data, data_len);
    return write_register_separate(sad, reg, data, data_len);
}
*/
bool KX123::write_register(uint8_t sad, uint8_t reg, uint8_t data) {
    unsigned char data_to_send[2];
    bool error;

    data_to_send[0] = reg;
    data_to_send[1] = data;
    error = i2c_bus_write( (int)((sad << 1) | I2C_WRITE ), &data_to_send[0], 2);

    return error;
}

int KX123::i2c_bus_write(int addr_rw, unsigned char *data, int len)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, addr_rw, ACK_CHECK_EN);
	//if(len==1)
	//{
	//	i2c_master_write_byte(cmd, data[0], ACK_CHECK_EN);
	//}
	//else
	//{
		i2c_master_write(cmd, data, len, ACK_CHECK_EN);
	//}

	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if(ret == ESP_OK)
	{
		//printf("KX122[i2c_bus_write]: I2C command %x accepted at address %x \n", data[0], addr_rw);
	}
	else //e.g. ESP_FAIL
	{
		printf("KX122[i2c_bus_write]: I2C command %x NOT accepted at address %x \n", data[0], addr_rw);
	}
	return ret;
}

int KX123::i2c_bus_read(int addr_rw, unsigned char *buf, int buf_len)
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
		//printf("KX122[i2c_bus_read]: I2C data %x received from address %x \n", buf[0], addr_rw);
	}
	else //e.g. ESP_FAIL
	{
		printf("KX122[i2c_bus_read]: I2C data NOT received from address %x \n", addr_rw);
	}
	return ret;
}
