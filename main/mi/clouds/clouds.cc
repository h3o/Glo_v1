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

// Modified by mario for the purpose of integrating into Glo & Gecho Loopsynth

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

#include "../hw/init.h"
#include "../hw/codec.h"
#include "../hw/signals.h"
#include "../hw/gpio.h"
#include "../hw/ui.h"

#include <stdlib.h>
#include <string.h>

// #define PROFILE_INTERRUPT 1
#define DYNAMIC_OBJECTS
//#define DYNAMIC_BUFFERS //depends on ECHO_BUFFER_STATIC in signals.h

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
	/*(else if(param==8)
	{
		if(val!=0)p->dry_wet += val;
		printf("%f", p->dry_wet);
	}*/
	else if(param==6)
	{
		if(val!=0)p->reverb += val;
		printf("%f", p->reverb);
	}
	else if(param==7)
	{
		if(val!=0)p->stereo_spread += 10*val;
		printf("%f", p->stereo_spread);
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
			p->feedback,
			p->dry_wet,
			p->reverb,
			p->stereo_spread,
			p->post_gain);
	}
}

void clouds_main(int change_sampling_rate) {
	printf("clouds_main()\n");
	cloudsInit();
	//printf("ui.DoEvents()\n");
  //while (1) {
    //ui.DoEvents();
	//printf("processor.Prepare()\n");
    //processor.Prepare();
  //}

	BUTTONS_SEQUENCE_TIMEOUT = BUTTONS_SEQUENCE_TIMEOUT_SHORT;

	#define n 32 //good with 44.1k rate
	//#define n 24 //not good with 32k rate
    uint16_t input[n*2];
    uint16_t output[n*2];
    int ptr = 0;

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
	processor->set_playback_mode(PLAYBACK_MODE_GRANULAR);
	//processor->set_playback_mode(PLAYBACK_MODE_SPECTRAL);
	//processor->set_playback_mode(PLAYBACK_MODE_STRETCH);
	//processor->set_playback_mode(PLAYBACK_MODE_LOOPING_DELAY);

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
	#define PARAMS_N 8

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
			"stereo_spread"};

	//p->gate = false;
	p->trigger = false;
	p->freeze = false;
	p->position = 0.1;
	p->size = 0.5;
	p->pitch = 4;

	p->density = 0.89f; //for granular mode
	p->texture = 1.3f; //0.9f;

	//p->density = 1.5f; //for stretch mode
	//p->texture = 2.5f;

	p->feedback = 0.4f;
	p->dry_wet = 1;
	p->reverb = 1;
	p->stereo_spread = 0.9f;

	p->post_gain = 1.2f;

	CLOUDS_HARD_LIMITER_POSITIVE = global_settings.CLOUDS_HARD_LIMITER_POSITIVE;
	CLOUDS_HARD_LIMITER_NEGATIVE = global_settings.CLOUDS_HARD_LIMITER_NEGATIVE;

	//i2s_zero_dma_buffer(I2S_NUM);
	int scroll_params = 0;
	int direct_update_params = 0;
	int selected_patch = 0;

	RGB_LED_R_OFF;
	RGB_LED_G_ON;
	RGB_LED_B_ON;

	//float *float_p = (float*)((void *)p);

	int clouds_patches = load_clouds_patch(selected_patch+1, (float*)p, CLOUDS_PATCHABLE_PARAMS);
	printf("Clouds patches found: %d\n", clouds_patches);
	printf("Clouds patch #%d parameters: ", selected_patch+1);
	print_params(p, 0);

	while(!event_next_channel || direct_update_params || p->freeze)
    {
	    //printf("while(!event_next_channel)\n");

	    if(event_next_channel && p->freeze)
	    {
	    	event_next_channel = 0;

	    	p->freeze = false;
    		RGB_LED_R_OFF;
    		RGB_LED_G_ON;
    		RGB_LED_B_ON;
	    }
	    else if(event_channel_options)
	    {
	    	if(p->freeze)
	    	{
	    		p->freeze = false;
	    		RGB_LED_R_OFF;
	    		RGB_LED_G_ON;
	    		RGB_LED_B_ON;
	    	}
	    	else
	    	{
	    		p->freeze = true;
	    		RGB_LED_R_OFF;
	    		RGB_LED_G_OFF;
	    		RGB_LED_B_ON;
	    	}
	    	event_channel_options = 0;
	    }

		if(short_press_sequence==2)
		{
			short_press_sequence = 0;
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
		if(short_press_sequence==-2)
		{
			short_press_sequence = 0;
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

		if(short_press_sequence==3)
		{
			short_press_sequence = 0;
			printf("Direct updating of parameters enabled\n");
			direct_update_params = 1;
		}
		if(short_press_sequence==-3)
		{
			short_press_sequence = 0;
			printf("Direct updating of parameters disabled\n");
			direct_update_params = 0;
		}

		if(direct_update_params)
		{
			if(event_next_channel)
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

			if(short_press_volume_plus)
			{
				change_parameter(p, params_ptr, 0.1f, param_names[params_ptr]);
				short_press_volume_plus = 0;
			}
			if(short_press_volume_minus)
			{
				change_parameter(p, params_ptr, -0.1f, param_names[params_ptr]);
				short_press_volume_minus = 0;
			}
		}
		else
		{
			if(short_press_volume_plus)
			{
				short_press_volume_plus = 0;
				selected_patch++;
				if(selected_patch>=clouds_patches)
				{
					selected_patch = 0;
				}
				printf("Loading patch #%d\n", selected_patch+1);
				load_clouds_patch(selected_patch+1, (float*)p, CLOUDS_PATCHABLE_PARAMS);
				print_params(p, 1);
			}
			if(short_press_volume_minus)
			{
				short_press_volume_minus = 0;
				selected_patch--;
				if(selected_patch<0)
				{
					selected_patch = clouds_patches-1;
				}
				printf("Loading patch #%d\n", selected_patch+1);
				load_clouds_patch(selected_patch+1, (float*)p, CLOUDS_PATCHABLE_PARAMS);
				print_params(p, 1);
			}
		}

		//printf("processor.Process()\n");
#ifdef DYNAMIC_OBJECTS
		processor->Process((ShortFrame*)input, (ShortFrame*)output, n);
#else
		processor.Process((ShortFrame*)input, (ShortFrame*)output, n);
#endif
		//meter.Process(processor.parameters().freeze ? output : input, n);

		for(ptr=0;ptr<4*n;ptr+=4)
    	{
			#ifdef CLOUDS_FAUX_LOW_SAMPLE_RATE
    		i2s_push_sample(I2S_NUM, (char*)output+ptr, portMAX_DELAY);
			#endif
    		i2s_push_sample(I2S_NUM, (char*)output+ptr, portMAX_DELAY);
    	//}
	    //for(ptr=0;ptr<4*n;ptr+=4)
    	//{
    		//i2s_pop_sample(I2S_NUM, (char*)input+ptr, portMAX_DELAY);
			#ifdef CLOUDS_FAUX_LOW_SAMPLE_RATE
    		i2s_pop_sample(I2S_NUM, (char*)input+ptr, 2);
			#endif
        	if(!i2s_pop_sample(I2S_NUM, (char*)input+ptr, 2))
    		{
    			((uint32_t*)((char*)input+ptr))[0] = 0;
    		}
    	}
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
