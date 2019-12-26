/*
 * adc.h
 *
 *  Created on: Apr 27, 2016
 *      Author: mayo
 */

#ifndef V1_SIGNALS_H_
#define V1_SIGNALS_H_

//#include "stm32f4xx_adc.h"
#include <stdint.h>

//#define ADC_PA0_PC0 10
//#define ADC_PA2_PC2 20
//#define ADC_PA6_PA7 30

//#define v1_I2S_AUDIOFREQ 22050
//#define v1_I2S_AUDIOFREQ 24000

//#define v1_I2S_AUDIOFREQ 23275 //half of the real APLL rate 46550
#define v1_I2S_AUDIOFREQ 25390 //half of the real APLL rate 50780

//#define v1_I2S_AUDIOFREQ 44100
//#define v1_I2S_AUDIOFREQ 50780

#ifdef __cplusplus
 extern "C" {
#endif

 //void ADC_configure_MIC(int pins_channels);
 //void ADC_configure_SENSORS(volatile uint16_t *converted_values);
//int adc_readPA0();
//int adc_readPC0();
//int ADC1_read();
//int ADC2_read();
//int ADC3_read();

/*
void DAC_RCC_GPIO_config(void);
void TIM6_Config(void);
void DAC_DMA_config(void);
void DAC_Ch1_EscalatorConfig(void);
void DAC_Ch2_SineWaveConfig(void);
void DAC_test(void);
*/
void RNG_Config (void);

float v1_PseudoRNG1a_next_float();
float v1_PseudoRNG1b_next_float();

//void DMA2_Stream1_IRQHandler(void);

uint32_t PseudoRNG2_next_int32();

//extern uint16_t DAC_Ch1_sample,DAC_Ch2_sample;

/*
typedef struct {
	float tabs[8];
	float params[8];
	uint8_t currIndex;
} fir_8;

float updateFilter(fir_8* theFilter, float newValue);
void initFilter(fir_8* theFilter);
*/

//extern fir_8 filt;

//int KissFFT_test(float *fft_test_buf, int fft_samples);

#ifdef __cplusplus
}
#endif

#endif /* V1_SIGNALS_H_ */
