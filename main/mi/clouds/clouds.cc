// Copyright 2014 Olivier Gillet.
// 
// Author: Olivier Gillet (pichenettes@mutable-instruments.net)
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.

// code ported from STM32 to ESP32 by mario for the purpose of integrating into Glo & Gecho Loopsynth

#include "clouds.h"

#include "clouds/cv_scaler.h"
//#include "clouds/drivers/codec.h"
//#include "clouds/drivers/debug_pin.h"
//#include "clouds/drivers/debug_port.h"
//#include "clouds/drivers/system.h"
//#include "clouds/drivers/version.h"
#include "clouds/dsp/granular_processor.h"
//#include "clouds/meter.h"
#include "clouds/resources.h"
#include "clouds/settings.h"
#include "clouds/ui.h"

#include "hw/init.h"
#include "hw/codec.h"
#include "hw/signals.h"
#include "hw/gpio.h"
#include "hw/ui.h"
#include "hw/sdcard.h"
#include "hw/leds.h"
#include "hw/midi.h"

#include <stdlib.h>
#include <string.h>

// #define PROFILE_INTERRUPT 1
#define DYNAMIC_OBJECTS
//#define DYNAMIC_BUFFERS //depends on ECHO_BUFFER_STATIC in signals.h
#define LOAD_CLOUDS_PATCH

#define CLOUDS_MIDI_PITCH_BASE			60		//note c4 will be pitch 0
#define CLOUDS_MIDI_PITCH_MAX			48.0f	//two octaves up or down
//#define CLOUDS_MIDI_PITCH_MAX			72.0f

float CLOUDS_MIDI_REVERB_MAX;
#define CLOUDS_MIDI_REVERB_MAX1			2.0f	//1.1f //max value for reverb by continuous controller wheel, good for GRANULAR mode
#define CLOUDS_MIDI_REVERB_MAX2			8.0f	//good for STRETCH and LOOPING_DELAY modes

float CLOUDS_MIDI_TEXTURE_MAX;
#define CLOUDS_MIDI_TEXTURE_MAX1		12.0f	//max value for texture by pitch-bend wheel, good for GRANULAR mode
#define CLOUDS_MIDI_TEXTURE_MAX2		1.0f	//good for STRETCH and LOOPING_DELAY modes

float CLOUDS_MIDI_DENSITY_MAX;
#define CLOUDS_MIDI_DENSITY_MAX1		1.0f//0.99f	//good for GRANULAR mode
#define CLOUDS_MIDI_DENSITY_MAX2		12.0f	//good for STRETCH and LOOPING_DELAY modes

#define CLOUDS_MIDI_POSITION_MAX		1.0f//1.5f
#define CLOUDS_MIDI_SIZE_MAX			1.0f
#define CLOUDS_MIDI_FEEDBACK_MAX		0.99f
#define CLOUDS_MIDI_STEREO_SPREAD_MAX	12.0f
#define CLOUDS_MIDI_DRY_WET_MAX			1.0f
#define CLOUDS_MIDI_POST_GAIN_MAX		2.0f

//#define GATE_TRIGGER_CONTROLS

/*
#define CLOUDS_MIDI_SPECTRAL_PHASE_RAND_MAX		6.0f
#define CLOUDS_MIDI_SPECTRAL_QUANTIZATION_MAX	6.0f
#define CLOUDS_MIDI_SPECTRAL_REFRESH_MAX		6.0f
#define CLOUDS_MIDI_SPECTRAL_WARP_MAX			6.0f
*/

using namespace clouds;
//using namespace stmlib;

#ifdef DYNAMIC_OBJECTS
GranularProcessor *processor;
CvScaler *cv_scaler;
Settings *settings;
Ui *ui;
#else
GranularProcessor processor;
//Codec codec;
//DebugPort debug_port;
CvScaler cv_scaler;
//Meter meter;
Settings settings;
Ui ui;
#endif

// Pre-allocate big blocks in main memory and CCM. No malloc here.
//uint8_t block_mem[0x1D000];//[0x17000];//118784]; 118784 = 1D000
//uint8_t block_ccm[65536 - 128];// __attribute__ ((section (".ccmdata")));
//uint8_t block_ccm[32768 - 128];// __attribute__ ((section (".ccmdata")));
//uint8_t block_mem[8192];
//uint8_t block_ccm[2048 - 128];

uint8_t *block_mem;
uint8_t *block_ccm;

//int __errno;
/*
// Default interrupt handlers.
extern "C" {

void NMI_Handler() { }
void HardFault_Handler() { while (1); }
void MemManage_Handler() { while (1); }
void BusFault_Handler() { while (1); }
void UsageFault_Handler() { while (1); }
void SVC_Handler() { }
void DebugMon_Handler() { }
void PendSV_Handler() { }

}
*/
//extern "C" {

/*
void SysTick_Handler() {
  ui.Poll();
  if (settings.freshly_baked()) {
    if (debug_port.readable()) {
      uint8_t command = debug_port.Read();
      uint8_t response = ui.HandleFactoryTestingRequest(command);
      debug_port.Write(response);
    }
  }
}
*/
//}

/*
void FillBuffer(Codec::Frame* input, Codec::Frame* output, size_t n) {
#ifdef PROFILE_INTERRUPT
  TIC
#endif  // PROFILE_INTERRUPT
  cv_scaler.Read(processor.mutable_parameters());
  processor.Process((ShortFrame*)input, (ShortFrame*)output, n);
  //meter.Process(processor.parameters().freeze ? output : input, n);
#ifdef PROFILE_INTERRUPT
  TOC
#endif  // PROFILE_INTERRUPT
}
*/

void cloudsInit() {
  //System sys;
  //Version version;
#ifdef DYNAMIC_OBJECTS
	processor = new GranularProcessor();
	cv_scaler = new CvScaler();
	settings = new Settings();
	ui = new Ui();
#endif


  //sys.Init(true);
  //version.Init();
	printf("cloudsInit()[1] Free heap: %u\n", xPortGetFreeHeapSize());

	#define BUFFER_1_SIZE 0x1D000
	#define BUFFER_2_SIZE (65536 - 128)
	//total = 184192

#ifdef DYNAMIC_BUFFERS
	block_mem = (uint8_t*)malloc(BUFFER_1_SIZE);//[0x17000];//118784]; 118784 = 1D000
#else
	block_mem = (uint8_t*)echo_buffer; //re-use statically allocated echo buffer
#endif
	block_ccm = (uint8_t*)malloc(BUFFER_2_SIZE);// __attribute__ ((section (".ccmdata")));
	printf("block_mem = %x, block_ccm = %x\n", (unsigned int)block_mem, (unsigned int)block_ccm);
	printf("cloudsInit()[2] Free heap: %u\n", xPortGetFreeHeapSize());

	memset(block_mem,0,BUFFER_1_SIZE);
	memset(block_ccm,0,BUFFER_2_SIZE);

#ifdef DYNAMIC_OBJECTS
	// Init granular processor.
	processor->Init(
			//block_mem, sizeof(block_mem),
			//block_ccm, sizeof(block_ccm));
			block_mem, BUFFER_1_SIZE,
			block_ccm, BUFFER_2_SIZE);

	//TODO processor init
  settings->Init();
  //cv_scaler.Init(settings.mutable_calibration_data());
  //meter.Init(32000);
  ui->Init(settings, cv_scaler, processor, NULL);//&meter);
#else
	processor.Init(block_mem, BUFFER_1_SIZE, block_ccm, BUFFER_2_SIZE);
	settings.Init();
	ui.Init(&settings, &cv_scaler, &processor, NULL);//&meter);
#endif

/*
  bool master = !version.revised();
  if (!codec.Init(master, 32000)) {
    ui.Panic();
  }
  if (!codec.Start(32, &FillBuffer)) {
    ui.Panic();
  }
  if (settings.freshly_baked()) {
#ifdef PROFILE_INTERRUPT
    DebugPin::Init();
#else
    debug_port.Init();
#endif  // PROFILE_INTERRUPT
  }
  sys.StartTimers();
*/
}

extern int codec_digital_volume;

void change_parameter(Parameters* p, int param, float val, const char *param_name)
{
	if(val==0)
	{
		printf("Current value of [%s] = ", param_name);
	}
	else
	{
		printf("Updating parameter [%s], new value = ", param_name);
	}
	/*
	if(param==0)
	{
		if(val!=0)p->trigger = (p->trigger?false:true);
		printf(p->trigger?"true":"false");
	}
	else if(param==1)
	{
		if(val!=0)p->freeze = (p->freeze?false:true);
		printf(p->freeze?"true":"false");
	}
		//pot_noise += 0.05f * ((Random::GetSample() / 32768.0f) * 0.00f - pot_noise);
	else*/
	if(param==0)
	{
		if(val!=0)p->position += val;
		printf("%f", p->position);
	}
	else if(param==1)
	{
		if(val!=0)p->size += val;
		printf("%f", p->size);
	}
	else if(param==2)
	{
		if(val!=0)p->pitch += 10*val;
		printf("%f", p->pitch);
	}
	else if(param==3)
	{
		if(val!=0)p->density += val/10;
		printf("%f", p->density);
	}
	else if(param==4)
	{
		if(val!=0)p->texture += val;
		printf("%f", p->texture);
	}
	else if(param==5)
	{
		if(val!=0)p->feedback += val;
		printf("%f", p->feedback);
	}
	else if(param==6)
	{
		if(val!=0)p->reverb += val;
		printf("%f", p->reverb);
	}
	else if(param==7)
	{
		if(val!=0)p->stereo_spread += 10*val;
		printf("%f", p->stereo_spread);
		p->granular.stereo_spread = p->stereo_spread;
	}
	else if(param==8)
	{
		if(val!=0)p->dry_wet += val;
		printf("%f", p->dry_wet);
	}
	else if(param==9)
	{
		if(val!=0)p->post_gain += val;
		printf("%f", p->post_gain);
	}
	printf("\n");
}

void print_params(Parameters* p, int print_names)
{
	if(print_names)
	{
		printf("position=%f,size=%f,pitch=%f,density=%f,texture=%f,dry_wet=%f,stereo_spread=%f,feedback=%f,reverb=%f,post_gain=%f\n",
			p->position,
			p->size,
			p->pitch,
			p->density,
			p->texture,
			p->dry_wet,
			p->stereo_spread,
			p->feedback,
			p->reverb,
			p->post_gain);
	}
	else
	{
		printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
			p->position,
			p->size,
			p->pitch,
			p->density,
			p->texture,
			p->dry_wet,
			p->stereo_spread,
			p->feedback,
			p->reverb,
			p->post_gain);
	}
}

int last_midi_control_indication = -1;
int midi_control_indication_timeout = -1;
#define MIDI_CONTROL_INDICATION_TIMEOUT		300

void indicate_MIDI_controls(int midi_control)
{
	LED_B5_set_byte(midi_ctrl_cc_values[midi_control]>>2);
	if(last_midi_control_indication != midi_control)
	{
		LED_R8_all_OFF();
		LED_O4_all_OFF();
		if(midi_control<8) LED_R8_set(midi_control, 1); else LED_O4_set(midi_control-8, 1);
		midi_control_indication_timeout = MIDI_CONTROL_INDICATION_TIMEOUT;
	}
}

IRAM_ATTR void clouds_main(int change_sampling_rate, int playback_mode) {
	printf("clouds_main(): cloudsInit()\n");
	cloudsInit();
	//printf("ui.DoEvents()\n");
  //while (1) {
    //ui.DoEvents();
	//printf("processor.Prepare()\n");
    //processor.Prepare();
  //}

	#ifdef BOARD_WHALE
	BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_SHORT;
	#endif

	#define IO_BUFFER_SIZE 32 //size in stereo samples (32bit), good with 44.1k or 50.78k rate
	//#define IO_BUFFER_SIZE 24 //size in stereo samples (32bit), not good with 32k rate
    uint16_t input[IO_BUFFER_SIZE*2];
    uint16_t output[IO_BUFFER_SIZE*2];
    int ptr = 0;
    char *out_ptr;

	if(change_sampling_rate)
	{
		printf("codec: set_sampling_rate(SAMPLE_RATE_CLOUDS=%d)\n",SAMPLE_RATE_CLOUDS);
		set_sampling_rate(SAMPLE_RATE_CLOUDS);
	}

	printf("codec_set_digital_volume()\n");
    codec_digital_volume=global_settings.CODEC_DIGITAL_VOLUME_DEFAULT;
    //codec_digital_volume=0x20;//-16dB
    //codec_digital_volume=0x30;//-24dB
    //codec_digital_volume=0x50;//-40dB
    codec_set_digital_volume();

#ifdef DYNAMIC_OBJECTS
	processor->set_num_channels(2);
	processor->set_low_fidelity(false);
	//processor->set_playback_mode(PLAYBACK_MODE_GRANULAR);
	//processor->set_playback_mode(PLAYBACK_MODE_SPECTRAL);
	//processor->set_playback_mode(PLAYBACK_MODE_STRETCH);
	//processor->set_playback_mode(PLAYBACK_MODE_LOOPING_DELAY);
	printf("clouds_main(): processor->set_playback_mode(%d)\n", playback_mode);
	processor->set_playback_mode((clouds::PlaybackMode)playback_mode);

	if(playback_mode)
	{
		CLOUDS_MIDI_TEXTURE_MAX = CLOUDS_MIDI_TEXTURE_MAX2;
		CLOUDS_MIDI_DENSITY_MAX = CLOUDS_MIDI_DENSITY_MAX2;
		CLOUDS_MIDI_REVERB_MAX = CLOUDS_MIDI_REVERB_MAX2;
	}
	else
	{
		CLOUDS_MIDI_TEXTURE_MAX = CLOUDS_MIDI_TEXTURE_MAX1;
		CLOUDS_MIDI_DENSITY_MAX = CLOUDS_MIDI_DENSITY_MAX1;
		CLOUDS_MIDI_REVERB_MAX = CLOUDS_MIDI_REVERB_MAX1;
	}

	Parameters* p = processor->mutable_parameters();
#else
	processor.set_num_channels(2);
	processor.set_low_fidelity(false);
	processor.set_playback_mode(PLAYBACK_MODE_GRANULAR);

	Parameters* p = processor.mutable_parameters();
#endif

	printf("processor.Prepare()\n");
#ifdef DYNAMIC_OBJECTS
	processor->Prepare();
#else
	processor.Prepare();
#endif

	channel_running = 1;
	//volume_ramp = 1;

	//processor->set_bypass(true);

	//#define PARAMS_N 11
	#define PARAMS_N 10//8

	int params_ptr = PARAMS_N-1;
	const char *param_names[PARAMS_N] = {
			//"trigger",
			//"freeze",
			"position",
			"size",
			"pitch",
			"density",
			"texture",
			"feedback",
			//"dry_wet",
			"reverb",
			"stereo_spread",
			"dry_wet",
			"post_gain"
	};

	p->gate = false;
	p->trigger = false;
	p->freeze = false;

	/*
	p->position = 0.1;
	p->size = 0.5;
	p->pitch = 4;

	p->density = 0.89f; //for granular mode
	p->texture = 1.3f; //0.9f;

	//p->density = 1.5f; //for stretch mode
	//p->texture = 2.5f;

	p->feedback = 0.4f;
	p->reverb = 1;
	p->stereo_spread = 0.9f;

	p->dry_wet = 1;
	p->post_gain = 1.2f;
	*/

	p->position = 0.0;
	p->size = 0.0;
	p->pitch = 0;

	p->density = 1.66;
	p->texture = 3.7;
	p->feedback = 0.3f;
	p->reverb = 2.4;
	p->stereo_spread = 5.9f;
	p->granular.stereo_spread = p->stereo_spread;

	p->dry_wet = 1;
	p->post_gain = 1.2f;

	CLOUDS_HARD_LIMITER_POSITIVE = global_settings.CLOUDS_HARD_LIMITER_POSITIVE;
	CLOUDS_HARD_LIMITER_NEGATIVE = global_settings.CLOUDS_HARD_LIMITER_NEGATIVE;

	//i2s_zero_dma_buffer(I2S_NUM);
	int scroll_params = 0;
	#ifdef BOARD_WHALE
	int direct_update_params = 0;
	#endif
	int selected_patch = 0;

	#ifdef BOARD_WHALE
	RGB_LED_R_OFF;
	RGB_LED_G_ON;
	RGB_LED_B_ON;
	#endif

	//float *float_p = (float*)((void *)p);

	#ifdef LOAD_CLOUDS_PATCH

	const char* patch_block_names[] = {"[clouds_patches_granular]", "[clouds_patches_stretch]", "[clouds_patches_looping_delay]", "[clouds_patches_spectral]"};

	int clouds_patches = load_clouds_patch(selected_patch+1, (float*)p, CLOUDS_PATCHABLE_PARAMS, patch_block_names[playback_mode]);
	p->granular.stereo_spread = p->stereo_spread;

	printf("Clouds patches found: %d\n", clouds_patches);
	#else
	int clouds_patches = 0;
	#endif
	printf("Clouds patch #%d parameters: ", selected_patch+1);
	print_params(p, 0);

	int options_menu = 0;
	int options_indicator = 0;

	//sampleCounter = 0;

	ui_button3_enabled = 0; //Clouds does not use standard echo settings, will override button #3 functionality

	//clouds always uses sensors only
	sensors_active = 1;
	SENSORS_LEDS_indication_enabled = 0;
	LEDs_all_OFF();

	accelerometer_active = 0;

	int s1_trigger = 0, s2_trigger = 0, s3_trigger = 0;

	//float rnd;
	#ifdef BOARD_WHALE
	while(!event_next_channel || direct_update_params || p->freeze)
	#else
	while(!event_next_channel) //in Gecho, freeze does not prevent exiting the channel
	#endif
    {
	    //printf("while(!event_next_channel)\n");

		if(MIDI_notes_updated)
		{
			MIDI_notes_updated = 0;
			//printf("clouds_main(): MIDI_last_chord[0] = %d, MIDI_keys_pressed = %d\n", MIDI_last_chord[0], MIDI_keys_pressed);

			if(!MIDI_keys_pressed)
			{
				if(p->freeze)
				{
					LED_B5_set_byte(0xff);
				}
			}
			else
			{
				LED_B5_all_OFF();
			}

			LED_W8_all_OFF();
			MIDI_to_LED(MIDI_last_chord[0], 1);
			p->pitch = MIDI_last_chord[0] - CLOUDS_MIDI_PITCH_BASE;
		}

		if(midi_control_indication_timeout>=0)
		{
			midi_control_indication_timeout--;
			if(midi_control_indication_timeout==0)
			{
				LED_R8_all_OFF();
				LED_O4_all_OFF();
			}
		}

		//printf("MIDI_controllers_updated = %d, MIDI_controllers_active_PB = %d, MIDI_controllers_active_CC = %d\n", MIDI_controllers_updated, MIDI_controllers_active_PB, MIDI_controllers_active_CC);

		if(midi_ctrl_cc>1 && midi_ctrl_cc_active)
		{
			#if 0 //test for SPECTRAL mode params
			if(midi_ctrl_cc_updated[0])
			{
				p->spectral.phase_randomization = ((float)midi_ctrl_cc_values[0]/127.0f) * CLOUDS_MIDI_SPECTRAL_PHASE_RAND_MAX;
				printf("p->spectral.phase_randomization => %f\r", p->spectral.phase_randomization);
				midi_ctrl_cc_updated[0] = 0;
				indicate_MIDI_controls(0);
			}
			if(midi_ctrl_cc_updated[1])
			{
				p->spectral.quantization = ((float)midi_ctrl_cc_values[1]/127.0f) * CLOUDS_MIDI_SPECTRAL_QUANTIZATION_MAX;
				printf("p->spectral.quantization => %f\r", p->spectral.quantization);
				midi_ctrl_cc_updated[1] = 0;
				indicate_MIDI_controls(1);
			}
			if(midi_ctrl_cc_updated[2])
			{
				p->spectral.refresh_rate = ((float)midi_ctrl_cc_values[2]/127.0f) * CLOUDS_MIDI_SPECTRAL_REFRESH_MAX;
				printf("p->spectral.refresh_rate => %f\r", p->spectral.refresh_rate);
				midi_ctrl_cc_updated[2] = 0;
				indicate_MIDI_controls(2);
			}
			if(midi_ctrl_cc_updated[3])
			{
				p->spectral.warp = (((float)midi_ctrl_cc_values[3]-64.0f)/127.0f) * CLOUDS_MIDI_SPECTRAL_WARP_MAX;
				printf("p->spectral.warp => %f\r", p->spectral.warp);
				midi_ctrl_cc_updated[3] = 0;
				indicate_MIDI_controls(3);
			}
			#endif

			//parameters order in the structure: position, size, pitch, density, texture, dry_wet, stereo_spread, feedback, reverb, post_gain

			//parameters orter by controllers: texture, reverb,	density, pitch, position, size, feedback, stereo_spread, dry_wet, post_gain

			#if 1
			if(midi_ctrl_cc_updated[0])
			{
				p->texture = ((float)midi_ctrl_cc_values[0]/127.0f) * CLOUDS_MIDI_TEXTURE_MAX;
				printf("p->texture => %f\r", p->texture);
				midi_ctrl_cc_updated[0] = 0;
				indicate_MIDI_controls(0);
			}
			if(midi_ctrl_cc_updated[1])
			{
				p->reverb = ((float)midi_ctrl_cc_values[1]/127.0f) * CLOUDS_MIDI_REVERB_MAX;
				printf("p->reverb => %f\r", p->reverb);
				midi_ctrl_cc_updated[1] = 0;
				indicate_MIDI_controls(1);
			}
			if(midi_ctrl_cc_updated[2])
			{
				p->density = ((float)midi_ctrl_cc_values[2]/127.0f) * CLOUDS_MIDI_DENSITY_MAX;
				printf("p->density => %f\r", p->density);
				midi_ctrl_cc_updated[2] = 0;
				indicate_MIDI_controls(2);
			}
			if(midi_ctrl_cc_updated[3])
			{
				p->pitch = (((float)midi_ctrl_cc_values[3]-64.0f)/127.0f) * CLOUDS_MIDI_PITCH_MAX;
				printf("p->pitch => %f\r", p->pitch);
				midi_ctrl_cc_updated[3] = 0;
				indicate_MIDI_controls(3);
			}
			#endif

			if(midi_ctrl_cc_updated[4])
			{
				p->position = ((float)midi_ctrl_cc_values[4]/127.0f) * CLOUDS_MIDI_POSITION_MAX;
				printf("p->position => %f\r", p->position);
				midi_ctrl_cc_updated[4] = 0;
				indicate_MIDI_controls(4);
			}
			if(midi_ctrl_cc_updated[5])
			{
				p->size = ((float)midi_ctrl_cc_values[5]/127.0f) * CLOUDS_MIDI_SIZE_MAX;
				printf("p->size => %f\r", p->size);
				midi_ctrl_cc_updated[5] = 0;
				indicate_MIDI_controls(5);
			}
			if(midi_ctrl_cc_updated[6])
			{
				p->feedback = ((float)midi_ctrl_cc_values[6]/127.0f) * CLOUDS_MIDI_FEEDBACK_MAX;
				printf("p->feedback => %f\r", p->feedback);
				midi_ctrl_cc_updated[6] = 0;
				indicate_MIDI_controls(6);
			}
			if(midi_ctrl_cc_updated[7])
			{
				p->stereo_spread = ((float)midi_ctrl_cc_values[7]/127.0f) * CLOUDS_MIDI_STEREO_SPREAD_MAX;
				p->granular.stereo_spread = p->stereo_spread;
				printf("p->stereo_spread => %f\r", p->stereo_spread);
				midi_ctrl_cc_updated[7] = 0;
				indicate_MIDI_controls(7);
			}
			if(midi_ctrl_cc_updated[8])
			{
				p->dry_wet = ((float)midi_ctrl_cc_values[8]/127.0f) * CLOUDS_MIDI_DRY_WET_MAX;
				printf("p->dry_wet => %f\r", p->dry_wet);
				midi_ctrl_cc_updated[8] = 0;
				indicate_MIDI_controls(8);
			}
			if(midi_ctrl_cc_updated[9])
			{
				p->post_gain = ((float)midi_ctrl_cc_values[9]/127.0f) * CLOUDS_MIDI_POST_GAIN_MAX;
				printf("p->post_gain => %f\r", p->post_gain);
				midi_ctrl_cc_updated[9] = 0;
				indicate_MIDI_controls(9);
			}
		}
		else
		{
			if(MIDI_controllers_active_CC)
			{
				if(MIDI_controllers_updated==MIDI_WHEEL_CONTROLLER_CC_UPDATED)
				{
					if(playback_mode)
					{
						p->density = ((float)MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]/127.0f) * CLOUDS_MIDI_DENSITY_MAX;
						printf("MIDI_WHEEL_CONTROLLER_CC => %d, density = %f\n", MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC], p->density);
					}
					else
					{
						p->reverb = ((float)MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC]/127.0f) * CLOUDS_MIDI_REVERB_MAX;
						printf("MIDI_WHEEL_CONTROLLER_CC => %d, reverb = %f\n", MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC], p->reverb);
					}
					MIDI_controllers_updated = 0;
				}
			}
			if(MIDI_controllers_active_PB)
			{
				if(MIDI_controllers_updated==MIDI_WHEEL_CONTROLLER_PB_UPDATED)
				{
					p->texture = ((float)MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB]/127.0f) * CLOUDS_MIDI_TEXTURE_MAX;
					printf("MIDI_WHEEL_CONTROLLER_PB => %d, texture = %f\n", MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB], p->texture);
					MIDI_controllers_updated = 0;
				}
			}
		}

		#ifdef BOARD_WHALE
		if(event_next_channel && p->freeze)
	    {
	    	event_next_channel = 0;

	    	p->freeze = false;
			#ifdef BOARD_WHALE
    		RGB_LED_R_OFF;
    		RGB_LED_G_ON;
    		RGB_LED_B_ON;
			#endif
	    }
	    else if(event_channel_options)
	    {
	    	if(p->freeze)
	    	{
	    		p->freeze = false;
				#ifdef BOARD_WHALE
	    		RGB_LED_R_OFF;
	    		RGB_LED_G_ON;
	    		RGB_LED_B_ON;
				#endif
	    	}
	    	else
	    	{
	    		p->freeze = true;
				#ifdef BOARD_WHALE
	    		RGB_LED_R_OFF;
	    		RGB_LED_G_OFF;
	    		RGB_LED_B_ON;
				#endif
	    	}
	    	event_channel_options = 0;
	    }
		#endif

		ui_command = 0;

		#define CLOUDS_UI_CMD_NEXT_PATCH					1
		#define CLOUDS_UI_CMD_PREVIOUS_PATCH				2
		#define CLOUDS_UI_CMD_HARD_LIMITER_INCREASE			3
		#define CLOUDS_UI_CMD_HARD_LIMITER_DECREASE			4
		#define CLOUDS_UI_CMD_DIRECT_UPDATE_ENABLE			5
		#define CLOUDS_UI_CMD_DIRECT_UPDATE_DISABLE			6
		#define CLOUDS_UI_CMD_DIRECT_PARAMETER_INCREASE		7
		#define CLOUDS_UI_CMD_DIRECT_PARAMETER_DECREASE		8
		#define CLOUDS_UI_CMD_DIRECT_PREVIOUS_PARAMETER		9
		#define CLOUDS_UI_CMD_DIRECT_NEXT_PARAMETER			10
		#define CLOUDS_UI_CMD_FREEZE						11
		#ifdef GATE_TRIGGER_CONTROLS
		#define CLOUDS_UI_CMD_TRIGGER						12
		#define CLOUDS_UI_CMD_GATE							13
		#endif

		//if (sampleCounter%2==0) //sampleCounter increases slower than usual (not by every sample)
		//{
			//map UI commands
			#ifdef BOARD_WHALE
			if(!direct_update_params && short_press_volume_plus) { ui_command = CLOUDS_UI_CMD_NEXT_PATCH; short_press_volume_plus = 0; }
			if(!direct_update_params && short_press_volume_minus) { ui_command = CLOUDS_UI_CMD_PREVIOUS_PATCH; short_press_volume_minus = 0; }
			if(short_press_sequence==2) { ui_command = CLOUDS_UI_CMD_HARD_LIMITER_INCREASE; short_press_sequence = 0; }
			if(short_press_sequence==-2) { ui_command = CLOUDS_UI_CMD_HARD_LIMITER_DECREASE; short_press_sequence = 0; }
			//these have alternative controls
			if(short_press_sequence==3 || long_press_set_button) { ui_command = CLOUDS_UI_CMD_DIRECT_UPDATE_ENABLE; short_press_sequence = 0; long_press_set_button = 0; }
			if(short_press_sequence==-3 || event_channel_options) { ui_command = CLOUDS_UI_CMD_DIRECT_UPDATE_DISABLE; short_press_sequence = 0; event_channel_options = 0; }
			//short press in direct update mode
			if(direct_update_params && short_press_volume_plus) { ui_command = CLOUDS_UI_CMD_DIRECT_PARAMETER_INCREASE; short_press_volume_plus = 0; }
			if(direct_update_params && short_press_volume_minus) { ui_command = CLOUDS_UI_CMD_DIRECT_PARAMETER_DECREASE; short_press_volume_minus = 0; }
			#endif

			#ifdef BOARD_GECHO

			if(event_channel_options)
			{
				//flip the flag
				options_menu = 1 - options_menu;
				if(!options_menu)
				{
					LED_O4_all_OFF();
					//ui_ignore_events = 0;
					settings_menu_active = 0;
				}
				else
				{
					//ui_ignore_events = 1;
					settings_menu_active = 1;
				}
				event_channel_options = 0;
			}

			/*
			if(btn_event_ext)
			{
				printf("btn_event_ext=%d, options_menu=%d\n",btn_event_ext, options_menu);
			}
			*/

			if(options_menu)
			{
				options_indicator++;
				LED_O4_set_byte(0x0f*(options_indicator%2));

				if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_1) { ui_command = CLOUDS_UI_CMD_HARD_LIMITER_DECREASE; }
				if(btn_event_ext==BUTTON_EVENT_RST_PLUS+BUTTON_2) { ui_command = CLOUDS_UI_CMD_HARD_LIMITER_INCREASE; }

				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = CLOUDS_UI_CMD_DIRECT_PREVIOUS_PARAMETER; }
				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = CLOUDS_UI_CMD_DIRECT_NEXT_PARAMETER; }
				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_4) { ui_command = CLOUDS_UI_CMD_DIRECT_PARAMETER_INCREASE; }
				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_3) { ui_command = CLOUDS_UI_CMD_DIRECT_PARAMETER_DECREASE; }
			}
			else
			{
				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = CLOUDS_UI_CMD_PREVIOUS_PATCH; }
				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command =  CLOUDS_UI_CMD_NEXT_PATCH; }
				//clouds does not use the same echo controls, can override button #3 behaviour
				//if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_3) { ui_command =  CLOUDS_UI_CMD_FREEZE; }
			}

			#ifdef GATE_TRIGGER_CONTROLS
			if(!s1_trigger && SENSOR_THRESHOLD_RED_2)
			{
				s1_trigger = 1;
				ui_command =  CLOUDS_UI_CMD_TRIGGER;
			}
			else if(s1_trigger && !SENSOR_THRESHOLD_RED_1)
			{
				s1_trigger = 0;
			}

			if(!s2_trigger && SENSOR_THRESHOLD_ORANGE_2)
			{
				s2_trigger = 1;
				ui_command =  CLOUDS_UI_CMD_GATE;
			}
			else if(s2_trigger && !SENSOR_THRESHOLD_ORANGE_1)
			{
				s2_trigger = 0;
			}
			#endif

			if(!s3_trigger && SENSOR_THRESHOLD_BLUE_4)
			{
				s3_trigger = 1;
				ui_command =  CLOUDS_UI_CMD_FREEZE;
			}
			else if(s3_trigger && !SENSOR_THRESHOLD_BLUE_5)
			{
				s3_trigger = 0;
			}

			btn_event_ext = 0;
			#endif
		//}

		#ifdef GATE_TRIGGER_CONTROLS
		if(ui_command==CLOUDS_UI_CMD_TRIGGER)
		{
			if(p->trigger)
	    	{
	    		p->trigger = false;
	    		printf("trigger=>0\n");
	    		LED_R8_all_OFF();
	    	}
	    	else
	    	{
	    		p->trigger = true;
	    		printf("trigger=>1\n");
	    		LED_R8_set_byte(0xff);
	    	}
		}

		if(ui_command==CLOUDS_UI_CMD_GATE)
		{
			if(p->gate)
	    	{
	    		p->gate = false;
	    		printf("gate=>0\n");
	    		LED_O4_all_OFF();
	    	}
	    	else
	    	{
	    		p->gate = true;
	    		printf("gate=>1\n");
	    		LED_O4_set_byte(0xff);
	    	}
		}
		#endif

		if(ui_command==CLOUDS_UI_CMD_FREEZE)
		{
			if(p->freeze)
	    	{
	    		p->freeze = false;
	    		//printf("freeze=>0\n");
	    		LED_B5_all_OFF();
	    	}
	    	else
	    	{
	    		p->freeze = true;
	    		//printf("freeze=>1\n");
	    		LED_B5_set_byte(0xff);
	    	}
		}

		if(ui_command==CLOUDS_UI_CMD_HARD_LIMITER_INCREASE)
		{
			if(CLOUDS_HARD_LIMITER_POSITIVE <= global_settings.CLOUDS_HARD_LIMITER_MAX-global_settings.CLOUDS_HARD_LIMITER_STEP)
			{
				CLOUDS_HARD_LIMITER_POSITIVE += global_settings.CLOUDS_HARD_LIMITER_STEP;
			}
			if(CLOUDS_HARD_LIMITER_NEGATIVE >= global_settings.CLOUDS_HARD_LIMITER_STEP-global_settings.CLOUDS_HARD_LIMITER_MAX)
			{
				CLOUDS_HARD_LIMITER_NEGATIVE -= global_settings.CLOUDS_HARD_LIMITER_STEP;
			}
			printf("Hard limiter increased to %d/%d\n", CLOUDS_HARD_LIMITER_NEGATIVE, CLOUDS_HARD_LIMITER_POSITIVE);
		}
		if(ui_command==CLOUDS_UI_CMD_HARD_LIMITER_DECREASE)
		{
			if(CLOUDS_HARD_LIMITER_POSITIVE > global_settings.CLOUDS_HARD_LIMITER_STEP)
			{
				CLOUDS_HARD_LIMITER_POSITIVE -= global_settings.CLOUDS_HARD_LIMITER_STEP;
			}
			if(CLOUDS_HARD_LIMITER_NEGATIVE < -global_settings.CLOUDS_HARD_LIMITER_STEP)
			{
				CLOUDS_HARD_LIMITER_NEGATIVE += global_settings.CLOUDS_HARD_LIMITER_STEP;
			}
			printf("Hard limiter decreased to %d/%d\n", CLOUDS_HARD_LIMITER_NEGATIVE, CLOUDS_HARD_LIMITER_POSITIVE);
		}

		#ifdef BOARD_WHALE
		if(ui_command==CLOUDS_UI_CMD_DIRECT_UPDATE_ENABLE)
		{
			printf("Direct updating of parameters enabled\n");
			direct_update_params = 1;
		}
		if(ui_command==CLOUDS_UI_CMD_DIRECT_UPDATE_DISABLE)
		{
			printf("Direct updating of parameters disabled\n");
			direct_update_params = 0;
		}

		/*
		if(short_press_user_button3)
		{
			short_press_user_button3 = 0;
			if(p->gate)
			{
				p->gate = false;
			}
			else
			{
				p->gate = true;
			}
			printf("Gate = %d\n", p->gate);
		}
		if(short_press_user_button4)
		{
			short_press_user_button4 = 0;
			if(p->trigger)
			{
				p->trigger = false;
			}
			else
			{
				p->trigger = true;
			}
			printf("Trigger = %d\n", p->trigger);
		}
		*/

		if(direct_update_params && event_next_channel)
		{
			if(++scroll_params%4==0)
			{
				print_params(p,0);
			}
			if(++scroll_params%4==1)
			{
				print_params(p,1);
			}

			params_ptr++;
			if(params_ptr==PARAMS_N)
			{
				params_ptr = 0;
			}
			printf("Selected parameter: %s\n", param_names[params_ptr]);
			change_parameter(p, params_ptr, 0, param_names[params_ptr]);

			event_next_channel = 0;
		}
		#else

		if(ui_command==CLOUDS_UI_CMD_DIRECT_PREVIOUS_PARAMETER || ui_command==CLOUDS_UI_CMD_DIRECT_NEXT_PARAMETER)
		{
			scroll_params++;
			if(scroll_params%4==0)
			{
				print_params(p,0);
			}
			else if(scroll_params%4==1)
			{
				print_params(p,1);
			}

			if(ui_command==CLOUDS_UI_CMD_DIRECT_NEXT_PARAMETER)
			{
				params_ptr++;
				if(params_ptr==PARAMS_N)
				{
					params_ptr = 0;
				}
			}
			else
			{
				if(params_ptr)
				{
					params_ptr--;
				}
				else
				{
					params_ptr = PARAMS_N - 1;
				}
			}
			printf("Selected parameter: %s\n", param_names[params_ptr]);
			change_parameter(p, params_ptr, 0, param_names[params_ptr]);
		}
		#endif

		if(ui_command==CLOUDS_UI_CMD_DIRECT_PARAMETER_INCREASE)
		{
			change_parameter(p, params_ptr, 0.1f, param_names[params_ptr]);
		}
		if(ui_command==CLOUDS_UI_CMD_DIRECT_PARAMETER_DECREASE)
		{
			change_parameter(p, params_ptr, -0.1f, param_names[params_ptr]);
		}

		if(ui_command==CLOUDS_UI_CMD_NEXT_PATCH)
		{
			selected_patch++;
			if(selected_patch>=clouds_patches)
			{
				selected_patch = 0;
			}
			printf("Loading patch #%d\n", selected_patch+1);
			load_clouds_patch(selected_patch+1, (float*)p, CLOUDS_PATCHABLE_PARAMS, patch_block_names[playback_mode]);
			p->granular.stereo_spread = p->stereo_spread;
			//print_params(p, 1);
		}
		if(ui_command==CLOUDS_UI_CMD_PREVIOUS_PATCH)
		{
			selected_patch--;
			if(selected_patch<0)
			{
				selected_patch = clouds_patches-1;
			}
			printf("Loading patch #%d\n", selected_patch+1);
			load_clouds_patch(selected_patch+1, (float*)p, CLOUDS_PATCHABLE_PARAMS, patch_block_names[playback_mode]);
			p->granular.stereo_spread = p->stereo_spread;
			//print_params(p, 1);
		}

		//printf("processor.Process()\n");
#ifdef DYNAMIC_OBJECTS
		processor->Process((ShortFrame*)input, (ShortFrame*)output, IO_BUFFER_SIZE);
#else
		processor.Process((ShortFrame*)input, (ShortFrame*)output, IO_BUFFER_SIZE);
#endif
		//meter.Process(processor.parameters().freeze ? output : input, IO_BUFFER_SIZE);

		//printf("sound output\n");

		/*
		for(ptr=0;ptr<4*n;ptr+=4)
    	{
			out_ptr = (char*)output+ptr;
			#ifdef CLOUDS_FAUX_LOW_SAMPLE_RATE
    		//i2s_push_sample(I2S_NUM, out_ptr, portMAX_DELAY);
			i2s_write(I2S_NUM, (void*)out_ptr, 4, &i2s_bytes_rw, portMAX_DELAY);
    		//sd_write_sample((uint32_t*)out_ptr);
			#endif
    		//i2s_push_sample(I2S_NUM, out_ptr, portMAX_DELAY);
    		i2s_write(I2S_NUM, (void*)out_ptr, 4, &i2s_bytes_rw, portMAX_DELAY);
    		//sd_write_sample((uint32_t*)out_ptr);
    	//}

		//for(ptr=0;ptr<4*n;ptr+=4)
    	//{
    		//i2s_pop_sample(I2S_NUM, (char*)input+ptr, portMAX_DELAY);

    		#ifdef CLOUDS_FAUX_LOW_SAMPLE_RATE
    		//i2s_pop_sample(I2S_NUM, (char*)input+ptr, 2);
    		//i2s_read(I2S_NUM, (void*)(input+ptr), 4, &i2s_bytes_rw, 2);
			#endif
    		//i2s_read(I2S_NUM, (void*)(input+ptr), 4, &i2s_bytes_rw, 2);
    		//if(!i2s_pop_sample(I2S_NUM, (char*)input+ptr, 2))
    		//if(i2s_bytes_rw!=4)
    		//{
    		//	((uint32_t*)((char*)input+ptr))[0] = 0;
    		//}
    		//float r = PseudoRNG1a_next_float();
    		//fill_with_random_value((char*)input+ptr);
    	}
    	*/

		//write IO_BUFFER_SIZE stereo samples = IO_BUFFER_SIZE*2*sizeof(uint16_t) = IO_BUFFER_SIZE*4 bytes
		i2s_write(I2S_NUM, (void*)output, IO_BUFFER_SIZE*4, &i2s_bytes_rw, portMAX_DELAY);
		i2s_read(I2S_NUM, (void*)input, IO_BUFFER_SIZE*4, &i2s_bytes_rw, 1);
		sd_write_samples(output, IO_BUFFER_SIZE*4); //size in bytes (one stereo sample = 32bit)

		//uint16_t input[IO_BUFFER_SIZE*2];

		/*
		//ADC_sample = ADC_sampleA[ADC_sample_ptr];
		ADC_sample_ptr++;
		#ifdef USE_FAUX_LOW_SAMPLE_RATE
		ADC_sample_ptr++;
		#endif

		if(ADC_sample_ptr==ADC_SAMPLE_BUFFER)
		{
			//i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
			//i2s_read(I2S_NUM, (void*)&ADC_sample, 4, &i2s_bytes_rw, portMAX_DELAY);
			i2s_read(I2S_NUM, (void*)ADC_sampleA, 4*ADC_SAMPLE_BUFFER, &i2s_bytes_rw, 1);
			ADC_sample_ptr = 0;
		}
		*/

		//sampleCounter ++;
    }

	free(block_ccm);
#ifdef DYNAMIC_BUFFERS
	free(block_mem);
#else
	memset(echo_buffer,0,ECHO_BUFFER_LENGTH*sizeof(int16_t)); //clean echo buffer
#endif

#ifdef DYNAMIC_OBJECTS
	delete(processor);
	delete(cv_scaler);
	delete(settings);
	delete(ui);
#endif

	if(change_sampling_rate)
	{
		printf("codec: set_sampling_rate(SAMPLE_RATE_DEFAULT=%d)\n",SAMPLE_RATE_DEFAULT);
		set_sampling_rate(SAMPLE_RATE_DEFAULT);
	}
}
