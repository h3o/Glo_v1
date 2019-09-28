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
#ifndef KX123_H
#define KX123_H

//#include "RegisterWriter/RegisterWriter/rohm_hal2.h"
//#include "RegisterWriter/RegisterWriter/RegisterWriter.h"

#include "kx123_registers.h"
#include <hw/init.h>

/**
* Kionix KX123 accelerometer i2c driver. For some extend can be used also with
* kx012, kx022, kx023, kx23h, kx112, kx122, kx124, kx222 and kx224. Driver uses
* RegisterWriter -class as (i2c) hardware abstraction layer.
* Note that when doing setup, sensor has to be in setup mode, not in operating mode.
*/
class KX123
{
public:

    KX123(i2c_port_t i2c_port, uint8_t sad = KX123_DEFAULT_SLAVE_ADDRESS, uint8_t wai = KX123_WHO_AM_I_WAI_ID);
    ~KX123();

    bool start_setup_mode(void);
    bool start_measurement_mode(void);
    bool set_defaults(void);
    bool getresults_highpass(int16_t* buf);
    bool getresults_raw(int16_t* buf);
    bool getresults_highpass_g(float* buf);
    bool getresults_g(float* buf);

    
    bool get_tilt(enum e_axis* current_previous);
    bool get_tap_interrupt_axis(enum e_axis* axis);
    bool get_detected_motion_axis(enum e_axis* axis);
    bool set_tilt_axis_mask(uint8_t cnltl2_tilt_mask);
    
    bool get_interrupt_reason(enum e_interrupt_reason* int_reason);

    bool has_interrupt_occured();
    void clear_interrupt();
    void soft_reset();
    bool self_test();

    bool set_cntl3_odrs(uint8_t tilt_position_odr, uint8_t directional_tap_odr, uint8_t motion_wuf_odr); //0xff for DONT_SET
    bool set_odcntl(bool iir_filter_off, uint8_t lowpass_filter_freq_half, uint8_t odr);
    bool int1_setup(uint8_t pwsel,
        bool physical_int_pin_enabled,
        bool physical_int_pin_active_high,
        bool physical_int_pin_latch_disabled,
        bool self_test_polarity_positive,
        bool spi3wire_enabled);
    bool int2_setup(uint8_t pwsel,
        bool physical_int_pin_enabled,
        bool physical_int_pin_active_high,
        bool physical_int_pin_latch_disabled,
        bool aclr2_enabled,
        bool aclr1_enabled);
    bool set_int1_interrupt_reason(uint8_t interrupt_reason);
    bool set_int2_interrupt_reason(uint8_t interrupt_reason);

    bool set_motion_detect_axis(uint8_t xxyyzz, bool axis_and_combination_enabled = false);
    bool set_tap_axis(uint8_t xxyyzz);
/**
* Note that not all sensor setup registers are exposed with this driver yet.
* There are also setup registers for motion detect, tap/double tap detect, free
* fall detect, tilt hysteresis, low power and FIFO.
*/

private:
    bool setup_mode_on;
    void set_tilt_position_defaults();
    
    //RegisterWriter i2c_rw;
    uint16_t resolution_divider;
    uint8_t _sad;
    uint8_t _wai;

    bool change_bits(uint8_t sad, uint8_t reg, uint8_t mask, uint8_t bits);
    uint8_t read_register(uint8_t sad, uint8_t reg, uint8_t* buf, uint8_t buf_len);
    //bool write_register(uint8_t sad, uint8_t reg, uint8_t* data, uint8_t data_len);
    bool write_register(uint8_t sad, uint8_t reg, uint8_t data);
    int i2c_bus_write(int addr_rw, unsigned char *data, int len);
    int i2c_bus_read(int addr_rw, unsigned char *buf, int buf_len);
};

#endif
