/*
 * KarplusStrong.cpp
 *
 *  Created on: 12 Jan 2023
 *      Author: mario
 *
 *  code ported from Karplus-Strong string synthesis by athibaul:
 *  https://dittytoy.net/ditty/0c920dd635
 *
 *  reverb code reused from KarplusStrong by kumaashi:
 *  https://github.com/kumaashi/AudioSandbox/blob/master/KarplusStrongSoundTest/src/karplus.cpp
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of MIT license.
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/gechologists/
 *
 */

#include "KarplusStrong.h"
#include "hw/ui.h"
#include "hw/signals.h"

//#define KS_DEBUG_MEMORY_LEAKS
//#define KS_DEBUG_ENVELOPE
//#define KS_USE_REVERB //adds ~9% CPU load

#define KS_ENVELOPE_ONSET_DELAY				100000
#define KS_ENVELOPE_DECAY_COEFF				0.99999f
#define KS_ENVELOPE_VOICE_OFF_THRESHOLD		0.01f

//---------- Karplus-Strong string synthesis by athibaul ------------------------------------------------------------------

// Karplus-Strong plucked string synthesis

// The string is represented by a delay line, with linear interpolation for fractional delay,
// and a first-order low-pass filter, which feeds back into the delay line.
// Each note is initialized with a short burst of noise.
KarplusStrong::KarplusStrong(int voices)
{
	printf("KarplusStrong::KarplusStrong(voices=%d)\n", voices);

	ks_voices = voices;

	printf("KarplusStrong::KarplusStrong(): allocating dynamic variables\n");

	ks_freq = (ks_high_t*)malloc(ks_voices * sizeof(ks_high_t));
	inverse_fs = (ks_high_t*)malloc(ks_voices * sizeof(ks_high_t));
	ks_delay_samples = (ks_high_t*)malloc(ks_voices * sizeof(ks_high_t));
	memset(ks_freq, 0, ks_voices * sizeof(ks_high_t));
	memset(inverse_fs, 0, ks_voices * sizeof(ks_high_t));
	memset(ks_delay_samples, 0, ks_voices * sizeof(ks_high_t));

	ks_len = (int*)malloc(ks_voices * sizeof(int));
	ks_pos = (int*)malloc(ks_voices * sizeof(int));
	memset(ks_len, 0, ks_voices * sizeof(int));
	memset(ks_pos, 0, ks_voices * sizeof(int));

	ks_fd = (ks_high_t*)malloc(ks_voices * sizeof(ks_high_t));
	memset(ks_fd, 0, ks_voices * sizeof(ks_high_t));

	ks_velocity = (float*)malloc(ks_voices * sizeof(float));
	memset(ks_velocity, 0, ks_voices * sizeof(float));

	ks_voice_active = (int*)malloc(ks_voices * sizeof(int));
	memset(ks_voice_active, 0, ks_voices * sizeof(int));

	ks_voice_envelope = (float*)malloc(ks_voices * sizeof(float));
	memset(ks_voice_envelope, 0, ks_voices * sizeof(float));

	#ifdef KS_USES_SMALL_BUFFERS
	//ks_float_t *ks_buf[KARPLUS_STRONG_VOICES];
	ks_buf = (ks_float_t**)malloc(ks_voices * sizeof(ks_float_t*));
	#endif

	a1 = (ks_float_t*)malloc(ks_voices * sizeof(ks_float_t));
	s0 = (ks_float_t*)malloc(ks_voices * sizeof(ks_float_t));

	ks_offs = (int*)malloc(ks_voices * sizeof(int));

	printf("KarplusStrong::KarplusStrong(): dynamic variables allocated\n");

	#ifdef KS_DEBUG_MEMORY_LEAKS
	int dbg_fm;
	printf("ks_init(): Free heap[before allocating buffers]: %u\n", dbg_fm=xPortGetFreeHeapSize());
	int largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_DEFAULT);
	printf("ks_init(): heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_DEFAULT) returns %u\n", largest_block);
	#endif

	printf("KarplusStrong::KarplusStrong(): allocating buffers\n");

	#ifdef KS_USES_SMALL_BUFFERS
		for(int i=0; i<ks_voices; i++)
		{
			ks_buf[i] = (ks_float_t*)malloc(KS_LEN_MAX * sizeof(ks_float_t)); // buffer used to create a delay
			#ifdef KS_DEBUG_MEMORY_LEAKS
			printf("KS_LEN_MAX=%d, ks_buf[%d]=%x\n", KS_LEN_MAX, i, (unsigned int)(ks_buf[i]));

			if(ks_buf[i]==NULL)
			{
				printf("ks_init(): could not allocate buffer #%d of size %d\n", i, KS_LEN_MAX * sizeof(ks_float_t));
				while(1) { error_blink(5,60,0,0,5,30); /*RGB:count,delay*/ };
			}
			#endif

			//printf("ks_init() buffer[%d] check[0]: %f, %f, %f, %f, %f\n", i, ks_buf[i][0], ks_buf[i][1], ks_buf[i][2], ks_buf[i][3], ks_buf[i][4]);
			memset(ks_buf[i], 0, KS_LEN_MAX * sizeof(ks_float_t));
			//printf("ks_init() buffer[%d] check[1]: %f, %f, %f, %f, %f\n", i, ks_buf[i][0], ks_buf[i][1], ks_buf[i][2], ks_buf[i][3], ks_buf[i][4]);
		}
	#else
		ks_buf = (ks_float_t*)malloc(KS_LEN_MAX * ks_voices * sizeof(ks_float_t)); // buffer used to create a delay
		#ifdef KS_DEBUG_MEMORY_LEAKS
		printf("KS_LEN_MAX=%d, ks_buf=%x\n", KS_LEN_MAX, (unsigned int)(ks_buf));

		if(ks_buf==NULL)
		{
			printf("ks_init(): could not allocate buffer of size %d\n", KS_LEN_MAX * sizeof(ks_float_t));
			store_channel_and_restart();
			while(1) { error_blink(5,60,0,0,5,30); /*RGB:count,delay*/ };
		}
		#endif

		printf("ks_init() buffer check[0]: %f, %f, %f, %f, %f\n", ks_buf[0], ks_buf[1], ks_buf[2], ks_buf[3], ks_buf[4]);
		memset(ks_buf, 0, KS_LEN_MAX * ks_voices * sizeof(ks_float_t));
		printf("ks_init() buffer check[1]: %f, %f, %f, %f, %f\n", ks_buf[0], ks_buf[1], ks_buf[2], ks_buf[3], ks_buf[4]);
	#endif

	#ifdef KS_DEBUG_MEMORY_LEAKS
	printf("ks_init(): Free heap[after allocating buffers]: %u, diff=%d\n", xPortGetFreeHeapSize(), dbg_fm-xPortGetFreeHeapSize());
	if(!heap_caps_check_integrity_all(true)) { printf("---> HEAP CORRUPT!\n"); }
	largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_DEFAULT);
	printf("ks_init(): heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_DEFAULT) returns %u\n", largest_block);
	#endif

	printf("KarplusStrong::KarplusStrong(): buffers allocated\n");

	printf("ks_random() test: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n", ks_random(), ks_random(), ks_random(), ks_random(), ks_random(), ks_random(), ks_random(), ks_random(), ks_random(), ks_random());

	#ifdef USE_ESP_HEAP_TRACE
	heap_trace_start(HEAP_TRACE_ALL);
	#endif

	KS_FREQ_CORRECTION = 1.0f;//global_settings.KS_FREQ_CORRECTION;
	for(int i=0; i<ks_voices; i++)
	{
		ks_note(i, 440.0f, 0, -1);
		//ks_mute[i] = 1;
	}

	#ifdef USE_ESP_HEAP_TRACE
	heap_trace_stop();
	heap_trace_dump();
	#endif

	#ifdef USE_KS_REVERB
	rebL = new Reverb;
	rebR = new Reverb;
	#endif

	printf("KarplusStrong::KarplusStrong(): constructor done\n");
}

KarplusStrong::~KarplusStrong(void)
{
	//printf("ks_deinit()\n");

	#ifdef KS_DEBUG_MEMORY_LEAKS
	int dbg_fm;
	printf("ks_deinit(): Free heap[before deallocating buffers]: %u\n", dbg_fm=xPortGetFreeHeapSize());
	int largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_DEFAULT);
	printf("ks_deinit(): heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_DEFAULT) returns %u\n", largest_block);
	#endif

	#ifdef KS_USES_SMALL_BUFFERS
	for(int i=0; i<ks_voices; i++)
	{
		if(ks_buf[i]!=NULL)
		{
			#ifdef KS_DEBUG_MEMORY_LEAKS
			printf("free(ks_buf[%d]) at 0x%x\n", i, (unsigned int)ks_buf[i]);
			#endif
			free(ks_buf[i]);
			ks_buf[i]=NULL;
		}
	}
	#else
	if(ks_buf!=NULL)
	{
		#ifdef KS_DEBUG_MEMORY_LEAKS
		printf("free(ks_buf) at 0x%x\n", (unsigned int)ks_buf);
		#endif
		free(ks_buf);
		ks_buf=NULL;
	}
	#endif

	#ifdef KS_DEBUG_MEMORY_LEAKS
	printf("ks_deinit(): Free heap[after deallocating buffers]: %u, diff=%d\n", xPortGetFreeHeapSize(), dbg_fm-xPortGetFreeHeapSize());
	if(!heap_caps_check_integrity_all(true)) { printf("---> HEAP CORRUPT!\n"); }
	largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_DEFAULT);
	printf("ks_deinit(): heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_DEFAULT) returns %u\n", largest_block);
	#endif


	free(ks_freq);
	free(inverse_fs);
	free(ks_delay_samples);
	free(ks_len);
	free(ks_pos);
	free(ks_fd);

	free(ks_velocity);
	free(ks_voice_active);
	free(ks_voice_envelope);

	#ifdef KS_USES_SMALL_BUFFERS
	free(ks_buf);
	#endif

	free(a1);
	free(s0);

	free(ks_offs);

	#ifdef USE_KS_REVERB
	delete rebL;
	delete rebR;
	#endif
}

void KarplusStrong::ks_note(int voice, ks_high_t freq, unsigned char velocity, int pluck)
{
	//printf("ks_note(voice=%d, freq=%f, velocity=%d, pluck=%d)\n", voice, freq, velocity, pluck);

	ks_freq[voice] = freq * KS_FREQ_CORRECTION;//440.0f;//midi_to_hz(options.note);
	inverse_fs[voice] = 1.0f / (ks_high_t)I2S_AUDIOFREQ;
	ks_delay_samples[voice] = 1.0f / (ks_freq[voice] * inverse_fs[voice]); // Duration of one period in samples
	//printf("ks_note(): ks_freq[%d]=%f, inverse_fs[%d]=%f, ks_delay_samples[%d]=%f\n", voice, ks_freq[voice], voice, inverse_fs[voice], voice, ks_delay_samples[voice]);

	ks_velocity[voice] = (float)velocity / KS_VELOCITY_SCALING;
	#ifdef KS_DEBUG_ENVELOPE
	printf("ks_note(): ks_velocity[%d]=%f\n", voice, ks_velocity[voice]);
	#endif

	if(ks_delay_samples[voice] >= KS_LEN_MAX)
	{
		printf("ks_delay_samples[%d] >= KS_LEN_MAX (%f > %d), freq = %f\n", voice, ks_delay_samples[voice], KS_LEN_MAX, freq);
		return;
	}

	ks_voice_envelope[voice] = 1.0f;

	ks_len[voice] = floor(ks_delay_samples[voice]) + 1; // buffer size
    //ks_fd = delay_samples % 1; // fractional delay to interpolate between samples
	ks_fd[voice] = ks_delay_samples[voice] - floor(ks_delay_samples[voice]);
	//printf("ks_len[%d]=%d, ks_fd[%d]=%f\n", voice, ks_len[voice], voice, ks_fd[voice]);

    //ks_buf = (float*)malloc(ks_len * sizeof(float)); // buffer used to create a delay
	//printf("ks_len=%d, ks_fd=%f, ks_buf=%x\n", ks_len, ks_fd, (unsigned int)ks_buf);

	ks_pos[voice] = 0; // current position of the reading/writing head
	a1[voice] = clamp01(2 * M_PI * ks_cutoff * inverse_fs[voice]); // Lowpass filter the reinjection
	s0[voice] = 0; // Signal value history for the lowpass filter

	#ifdef USE_ESP_HEAP_TRACE
	//heap_trace_start(HEAP_TRACE_ALL);
	#endif

	//ks_offs[voice] = (int)floor(ks_len[voice] * 0.3f);
	ks_offs[voice] = (int)floor(ks_len[voice] * (0.2 + 0.2 * ks_random())); // the offset determines the plucking position
	//ks_offs[voice] = (int)floor(ks_len[voice] * (0.1 + 0.8 * ks_random())); // can't really tell the difference

	#ifdef USE_ESP_HEAP_TRACE
	//heap_trace_stop();
	//heap_trace_dump();
	#endif

	if(pluck==-1) //silent note
	{
		#ifdef KS_USES_SMALL_BUFFERS
		memset(ks_buf[voice], 0, KS_LEN_MAX * sizeof(ks_float_t));
		#else
		memset(&ks_buf[voice*KS_LEN_MAX], 0, KS_LEN_MAX * sizeof(ks_float_t));
		#endif
	}
	else
	{
		// Initialize part of the buffer with noise
		for(int i=0; i < 70 && i < ks_len[voice]; i++)
		{
			#ifdef KS_USES_SMALL_BUFFERS
			ks_buf[voice][i] = ks_random();
			#else
			ks_buf[voice*KS_LEN_MAX+i] = ks_random();
			#endif
			if(pluck>0)
			{
				// The following line introduces "comb filtering" in the filter input, for more interesting results
				#ifdef KS_USES_SMALL_BUFFERS
				ks_buf[voice][(i+ks_offs[voice])%ks_len[voice]] += -ks_buf[voice][i];
				#else
				ks_buf[voice*KS_LEN_MAX+(i+ks_offs[voice])%ks_len[voice]] += -ks_buf[voice*KS_LEN_MAX+i];
				#endif
			}
		}
	}
	//ks_mute[voice] = 0;
}

void KarplusStrong::new_note(float freq, unsigned char velocity, float *volume_ramp)
{
	//printf("KarplusStrong::ks_new_note(freq=%f, velocity=%d, ...)\n", freq, velocity);

	int pluck1 = (rand()-velocity)%2;
	int pluck2 = (rand()+velocity)%2;

	//fade out whenever a voice stopped by another voice
	if(ks_voice_active[ks_voice])
	{
		volume_ramp[0] += process(ks_voice) * ks_velocity[ks_voice]; //add stopped voice last sample to volume ramp
	}

	ks_note(ks_voice, freq, velocity, pluck1);
	ks_voice_active[ks_voice] = 1;

	ks_voice++;
	if(ks_voice==ks_voices)
	{
		ks_voice = 0;
	}

	//fade out whenever a voice stopped by another voice
	if(ks_voice_active[ks_voice])
	{
		volume_ramp[1] += process(ks_voice) * ks_velocity[ks_voice]; //add stopped voice last sample to volume ramp
	}

	ks_note(ks_voice, freq * (0.985f+ks_random()*0.03f), velocity, pluck2);
	ks_voice_active[ks_voice] = 1;

	ks_voice++;
	if(ks_voice==ks_voices)
	{
		ks_voice = 0;
	}
}

float KarplusStrong::process(int voice)
{
	//printf("KarplusStrong::ks_process(voice=%d)\n", voice);

	//if(ks_mute[voice]) return 0.0f; //this pushes the CPU load just above the level of smooth sound

	int pos = ks_pos[voice];

	//printf("KarplusStrong::ks_process(): pos=%d\n", pos);
	//printf("KarplusStrong::ks_process(): ks_len[voice]]=%d, ks_fd[voice]=%d\n", ks_len[voice], ks_fd[voice]);

	#ifdef KS_USES_SMALL_BUFFERS
	ks_float_t value = LERP(ks_buf[voice][pos], ks_buf[voice][(pos+1)%ks_len[voice]], ks_fd[voice]); // linear interpolation for the fractional delay
	#else
	ks_float_t value = LERP(ks_buf[voice*KS_LEN_MAX+pos], ks_buf[voice*KS_LEN_MAX+(pos+1)%ks_len[voice]], ks_fd[voice]); // linear interpolation for the fractional delay
	#endif

	// Nonlinearity (optional)
    // value /= 1 + Math.max(value,0);
    s0[voice] += a1[voice] * (value - s0[voice]); // lowpass filter

    #ifdef KS_USES_SMALL_BUFFERS
    ks_buf[voice][pos] = LERP(value, s0[voice], ks_lowpass_amt);
	#else
    ks_buf[voice*KS_LEN_MAX+pos] = LERP(value, s0[voice], ks_lowpass_amt);
	#endif

    ks_pos[voice] = (pos+1)%ks_len[voice];

	#ifdef KS_DEBUG_ENVELOPE
    if(ks_voice_active[voice]==KS_ENVELOPE_ONSET_DELAY)
    {
		printf("KarplusStrong::process(): voice %d envelope start\n", voice);
    }
	#endif

    if(ks_voice_active[voice]>KS_ENVELOPE_ONSET_DELAY)
    {
    	ks_voice_envelope[voice] *= KS_ENVELOPE_DECAY_COEFF;
    	if(ks_voice_envelope[voice]<KS_ENVELOPE_VOICE_OFF_THRESHOLD)
    	{
			#ifdef KS_DEBUG_ENVELOPE
    		printf("KarplusStrong::process(): voice %d stopped by envelope decay past the threshold\n", voice);
			#endif
    		ks_voice_active[voice] = 0;
    	}
    	return s0[voice] * ks_voice_envelope[voice]; // The natural decay is a bit slow, so we still apply an envelope
    }
    else
    {
        ks_voice_active[voice]++;
    }

    return s0[voice];
}

void KarplusStrong::process_all(float *lr_samples)
{
	//this seems to be a bit faster
	float sample_mix[2] = {0.0f,0.0f};

	//lr_samples[0] = 0.0f;
	//lr_samples[1] = 0.0f;

	for(int v=0;v<ks_voices/2;v++)
	{
		if(ks_voice_active[v*2])
		{
			sample_mix[0] += process(v*2) * ks_velocity[v*2];
		}

		//add reverb
		//ks_sample = process(v*2) * 4.0f;
		//sample_mix[0] += 0.3 * ks_sample + (rebL->Sample(ks_sample, 3, 0, 2));

		if(ks_voice_active[v*2+1])
		{
			sample_mix[1] += process(v*2+1) * ks_velocity[v*2+1];
		}

		//add reverb
		//ks_sample = process(v*2 + 1) * 4.0f;
		//sample_mix[1] += 0.3 * ks_sample + (rebR->Sample(ks_sample, 2, 1, 2));
	}

	//TODO: optimize - this reverb consumes ~10% CPU (while nothing plays)
	#ifdef KS_USE_REVERB
	//add reverb and gain to all voices
	//lr_samples[0] = rebL->Sample(sample_mix[0], 3, 0, 1) * ENGINE_KS_RELATIVE_GAIN;
	//lr_samples[1] = rebR->Sample(sample_mix[1], 2, 1, 1) * ENGINE_KS_RELATIVE_GAIN;
	lr_samples[0] = rebL->Sample(sample_mix[0], 3, 0, 1);
	lr_samples[1] = rebR->Sample(sample_mix[1], 2, 1, 1);
	#else
	//lr_samples[0] = sample_mix[0] * ENGINE_KS_RELATIVE_GAIN;
	//lr_samples[1] = sample_mix[1] * ENGINE_KS_RELATIVE_GAIN;

	//for comparison of the same signal, processed vs. unprocessed
	//lr_samples[1] = ks_remove_DC_offset_IIR(sample_mix[0], 1) * ENGINE_KS_RELATIVE_GAIN;

	//lr_samples[0] = ks_remove_DC_offset_IIR(sample_mix[0], 0) * ENGINE_KS_RELATIVE_GAIN;
	//lr_samples[1] = ks_remove_DC_offset_IIR(sample_mix[1], 1) * ENGINE_KS_RELATIVE_GAIN;

	//the gain and offset removal was taken outside to accommodate voice-stop volume ramp
	lr_samples[0] = sample_mix[0];
	lr_samples[1] = sample_mix[1];
	#endif

	//printf("sample_mix[]==(%f,%f)\n", sample_mix[0], sample_mix[1]);

	//lr_samples[0] = sample_mix[0];
	//lr_samples[1] = sample_mix[1];
}

#define DC_IIR_GRADE	4//2//8//

float dc_iir_buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define DC_REMOVE_HPF_ALPHA_KS		0.001f

float KarplusStrong::ks_remove_DC_offset_IIR(float sample, int left_right)
{
	//printf("KarplusStrong::ks_remove_DC_offset_IIR(): sample=%f, left_right=%d\n", sample, left_right);

	if(left_right)
	{
		dc_iir_buf[0] += DC_REMOVE_HPF_ALPHA_KS * (sample - dc_iir_buf[0]);
		dc_iir_buf[1] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[0] - dc_iir_buf[1]);
		#if DC_IIR_GRADE==2
		return (sample - dc_iir_buf[1]);	//high-pass
		#else
		dc_iir_buf[2] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[1] - dc_iir_buf[2]);
		dc_iir_buf[3] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[2] - dc_iir_buf[3]);
		#if DC_IIR_GRADE==4
		return (sample - dc_iir_buf[3]);	//high-pass
		#else
		dc_iir_buf[4] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[3] - dc_iir_buf[4]);
		dc_iir_buf[5] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[4] - dc_iir_buf[5]);
		dc_iir_buf[6] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[5] - dc_iir_buf[6]);
		dc_iir_buf[7] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[6] - dc_iir_buf[7]);
		return (sample - dc_iir_buf[7]);	//high-pass
		#endif
		#endif
	}
	else
	{
		dc_iir_buf[8] += DC_REMOVE_HPF_ALPHA_KS * (sample - dc_iir_buf[8]);
		dc_iir_buf[9] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[8] - dc_iir_buf[9]);
		#if DC_IIR_GRADE==2
		return (sample - dc_iir_buf[9]);	//high-pass
		#else
		dc_iir_buf[10] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[9] - dc_iir_buf[10]);
		dc_iir_buf[11] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[10] - dc_iir_buf[11]);
		#if DC_IIR_GRADE==4
		return (sample - dc_iir_buf[11]);	//high-pass
		#else
		dc_iir_buf[12] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[11] - dc_iir_buf[12]);
		dc_iir_buf[13] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[12] - dc_iir_buf[13]);
		dc_iir_buf[14] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[13] - dc_iir_buf[14]);
		dc_iir_buf[15] += DC_REMOVE_HPF_ALPHA_KS * (dc_iir_buf[14] - dc_iir_buf[15]);
		return (sample - dc_iir_buf[15]);	//high-pass
		#endif
		#endif
	}
}
