/*
 * KarplusStrong.h
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

#ifndef DSP_KARPLUS_STRONG_H_
#define DSP_KARPLUS_STRONG_H_

#include <string.h>
#include <stdlib.h>
#include <math.h>

//#define USE_KS_REVERB
#define KS_USES_SMALL_BUFFERS
#define KS_LEN_MAX 1250 //lowest playable note = E1 (MIDI #28)

#define ks_float_t float
#define ks_high_t double //sounds good in combination with KS_FREQ_CORRECTION

#define KS_MIXING_MELODY			1.5f
#define KS_MIXING_BASSLINE_ROOT		1.4f
#define KS_MIXING_BASSLINE_THIRD	1.8f
#define KS_MIXING_BASSLINE_FIFTH	1.8f

#define KS_VELOCITY_SCALING			64.0f

#ifdef __cplusplus

class KarplusStrong {

public:

	// constructor and destructor
	KarplusStrong(int voices);
	~KarplusStrong(void);

	void new_note(float freq, unsigned char velocity, float *volume_ramp);
	float process(int voice);
	void process_all(float *lr_samples);
	float ks_remove_DC_offset_IIR(float sample, int left_right);

private:

	int ks_voices;
	int ks_voice = 0;

	ks_high_t *ks_freq, *inverse_fs, *ks_delay_samples;
	int *ks_len, *ks_pos;
	ks_high_t *ks_fd;

	ks_high_t KS_FREQ_CORRECTION;

	ks_float_t *a1, *s0;
	int *ks_offs;

	const ks_float_t ks_cutoff = 2500.0f;
	const ks_float_t ks_lowpass_amt = 0.1f;

	float *ks_velocity;

	int *ks_voice_active; //also used for delayed envelope onset
	float *ks_voice_envelope;

	void ks_note(int voice, ks_high_t freq, unsigned char velocity, int pluck);

	static inline float ks_random()
	{
		return (float)rand()/((float)RAND_MAX);
	}

	static inline ks_float_t clamp01(ks_float_t x)
	{
		if(x>1.0f)
		{
			return 1.0f;
		}
		else if(x<0.0f)
		{
			return 0.0f;
		}
		return x;
	}

	static inline ks_float_t LERP(ks_float_t a, ks_float_t b, ks_float_t t)
	{
		return a+t*(b-a);
	}

	#ifdef KS_USES_SMALL_BUFFERS
	//ks_float_t *ks_buf[];
	ks_float_t **ks_buf;
	#else
	ks_float_t *ks_buf;
	#endif

	//---------- Reverb by kumaashi -------------------------------------------------------------------------------------

	struct DelayBuffer
	{
	  enum
	  {
		//Max = 100000,
		//Max = 40000,
		Max = 20000,
		//Max = 10000,
		//Max = 1000,
	  };
	  float Buf[Max];
	  unsigned long   Rate;
	  unsigned int   Index;
	  void Init(unsigned long m)        { Rate = m;  }
	  void Update(float a)              { Buf[Index++ % Rate ] = a;  }
	  float Sample(unsigned long n = 0) { return Buf[ (Index + n) % Rate]; }
	};

	//http://www.ari-web.com/service/soft/reverb-2.htm
	struct Reverb
	{
	  enum
	  {
	    CombMax = 4,
	    AllMax  = 2,
	  };
	  DelayBuffer comb[CombMax];
	  DelayBuffer all[AllMax];

	    const int tau[2][4][4] =
	    {
	      //prime
	      {
	        {2063, 1847, 1523, 1277}, {3089, 2927, 2801, 2111}, {5479, 5077, 4987, 4057}, {9929, 7411, 4951, 1063}
	      },
	      {
	        {2053, 1867, 1531, 1259}, {3109, 2939, 2803, 2113}, {5477, 5059, 4993, 4051}, {9949, 7393, 4957, 1097}
	      }
	    };

	    const float gain[4] =
	    {
	       -(0.8733), -(0.8223), -(0.8513), -(0.8503)
	    };

	  float Sample(float a, int index = 0, int character = 0, int lpfnum = 5)
	  {
	    float D = a * 0.5;
	    float E = 0;

	    float lpfnum_f = (float)lpfnum;

	    //Comb
	    for(int i = 0 ; i < CombMax; i++)
	    {
	      DelayBuffer *reb = &comb[i];
	      reb->Init(tau[character % 2][index % 4][i]);
	      float k = 0;

	      for(int h = 0; h < lpfnum; h++)
	      {
	        k += reb->Sample(h * 2);
	      }
	      k /= lpfnum_f;

	      k += a;

	      reb->Update(k * gain[i]);

	      E += k;
	    }
	    D = (D + E) * 0.3;
	    return D;
	  }
	};

public:

	#ifdef USE_KS_REVERB
	Reverb *rebL;
	Reverb *rebR;
	#endif

	//---------- Reverb by kumaashi ----[END]---------------------------------------------------------------------------
};

#else

//this needs to be accessible from peripherals.c with c-linkage
//void channel_karplus_strong();

#endif

#endif /* DSP_KARPLUS_STRONG_H_ */
