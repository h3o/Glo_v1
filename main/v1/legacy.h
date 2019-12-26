//Includes

#define IRAM_ATTR __attribute__((section(".iram1")))

#include <stdlib.h>
#include <string.h>

#include "v1_filters.h"
//#include "Synth.h"
//#include "gyro.h"
//#include "sensors.h"
//#include "fft.h"
//#include "detectors.h"

#include "v1_gpio.h"
//#include "eeprom.h"
//#include "codec.h"
#include "v1_signals.h"

#define ENABLE_ECHO
#define ENABLE_JAMMING
#define ENABLE_CHORD_LOOP
#define ADD_MICROPHONE_SIGNAL
#define ADD_PLAIN_NOISE
#define ENABLE_LIMITER
#define USE_FAUX_22k_MODE

//#define LEGACY_USE_44k_RATE
#define LEGACY_USE_48k_RATE
//#define LEGACY_USE_22k_RATE

#define PLAIN_NOISE_VOLUME 48.0f //tested on v2 board
//#define PLAIN_NOISE_VOLUME 20.0f //stm32f4-disc1

#define PREAMP_BOOST 24.0f //tested on v2 board
//#define PREAMP_BOOST 12.0f //stm32f4-disc1
//#define PREAMP_BOOST 8 //gecho v1 board

//#define DEBUG_SIGNAL_LEVELS

//-----------------------------------------------------------------------------------

//bool sample_processed = false;
extern int sample_counter;// = 0;
//uint32_t sampleCounter = 0;
//volatile int16_t sample_i16 = 0, sample_i16_left = 0;

void LED_indicators();

#ifdef USE_FILTERS
	extern v1_filters *v1_fil;
#endif

//uint32_t random_value;
//int random_direction = 0;

//char print_buf[100];
extern float sample_f[2],sample_synth[2],sample_mix;
extern unsigned long seconds;

#define SAMPLE_VOLUME_BOOST_MAX 5.0f
extern float SAMPLE_VOLUME_BOOST;// = 1.0f;

//float volume2 = 6000.0f;
//float volume2 = 3200.0f;
//float volume2 = 1600.0f;
extern float volume2;// = 800.0f;
//float volume2 = 400.0f;
//float volume2 = 200.0f;

extern float noise_volume, noise_volume_max;// = 1.0f;

#ifdef ENABLE_JAMMING
	extern int progressive_jamming_factor;// = 1;

	#define PROGRESSIVE_JAMMING_BOOST_RATIO 2
	#define PROGRESSIVE_JAMMING_RAMP_COEF 0.999f
#endif

/*
uint16_t mic_sample;
uint16_t mic_sample_raw=0;
uint32_t mic_sample_sum=0;
uint16_t mic_sample_count=0;
float mic_sample_avg;
uint16_t mic_sample_raw_buf[MIC_OUT_BUF_SIZE];
int mic_sample_raw_buf_ptr=0;
int new_mic_sample_buf_available_cnt=0;
int new_mic_sample_buf_available=0;
*/
//int16_t v1=0,v2=0,v1a,v1b,v2a,v2b;
//int PlaybackPosition=0;

#define AUDIOFREQ_DIV_CORRECTION 4

#define ECHO_BUFFER_LENGTH v1_I2S_AUDIOFREQ
//#define ECHO_BUFFER_LENGTH I2S_AUDIOFREQ /4

#ifdef ENABLE_ECHO
	//uint16_t echo_buffer[ECHO_BUFFER_LENGTH];
	extern int echo_buffer_ptr0, echo_buffer_ptr;
#endif

void legacy_main();

// --------------------------------------------------------------
extern uint32_t ADC_sample, ADC_sample0;
extern int16_t echo_buffer[];
extern uint32_t random_value;
extern volatile int16_t sample_i16;
