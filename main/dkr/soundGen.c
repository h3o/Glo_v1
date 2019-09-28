/**
 ******************************************************************************
 * File Name          : soundGen.c
 * Author			  : Xavier Halgand
 * Date               :
 * Description        :
 ******************************************************************************
 */

/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
*/

/*-------------------------------------------------------*/

#include "soundGen.h"
#include "Dekrispator.h"
#include "glo_config.h"
#include <hw/init.h>
#include <hw/gpio.h>
#include <hw/codec.h>
#include <hw/signals.h>
#include <hw/ui.h>
#include <hw/midi.h>

/*-------------------------------------------------------*/

#define EPSI				.00002f

//#define RANDOM_PATCHES

/*-------------------------------------------------------*/

//extern bool					demoMode;
//extern bool					freeze;

extern Sequencer_t 			seq;
extern NoteGenerator_t 		noteG;

extern Oscillator_t 		op1 ;
extern Oscillator_t 		op2 ;
extern Oscillator_t 		op3 ;
extern Oscillator_t 		op4 ;

extern VCO_blepsaw_t		mbSawOsc;
extern VCO_bleprect_t		mbRectOsc;
extern VCO_bleptri_t		mbTriOsc;

extern Oscillator_t 		vibr_lfo ;
extern Oscillator_t 		filt_lfo ;
extern Oscillator_t 		filt2_lfo ;
extern Oscillator_t 		amp_lfo ;

extern ResonantFilter 	SVFilter;
extern ResonantFilter 	SVFilter2;
extern float			dk_filterFreq  ;
extern float			dk_filterFreq2  ;

extern ADSR_t 			adsr;

static bool 		autoFilterON _CCM_;
static bool			delayON _CCM_;
static bool			phaserON _CCM_;
static bool 		chorusON _CCM_;
static int8_t		autoSound _CCM_;

static float			f0 _CCM_ ;
static float 			vol  _CCM_;
static float			env  _CCM_;
static enum timbre 		sound _CCM_;

/*===============================================================================================================*/

void autoSound_set(int8_t val)
{
	autoSound = val;
}
/*---------------------------------------------------------*/
void RandSound1(uint8_t val) /* random series of tones */
{
	if (val == MIDI_MAX)
	{
		if (autoSound == 0)
			autoSound = 1;
		else
			autoSound = 0;
	}
}
/*---------------------------------------------------------*/
void RandSound2(uint8_t val) /* random series of tones */
{
	if (val == MIDI_MAX)
	{
		if (autoSound == 0)
			autoSound = 2;
		else
			autoSound = 0;
	}
}
/*---------------------------------------------------------*/
uint8_t soundNumber_get(void)
{
	return sound;
}
/*---------------------------------------------------------*/
//void Parameter_fine_tune(uint8_t val)
//{

//}
/*---------------------------------------------------------*/
/*
void DemoMode_toggle(uint8_t val)
{
	if (val == MIDI_MAX)
	{
		demoMode = !demoMode;
	}
}

void DemoMode_freeze(uint8_t val)
{
	if (val == MIDI_MAX)
	{
		freeze = !freeze;
	}
}
*/
/*-------------------------------------------------------*/
void AmpLFO_amp_set(uint8_t val)
{
	amp_lfo.amp = val / MIDI_MAX;
}
/*-------------------------------------------------------*/
void AmpLFO_freq_set(uint8_t val)
{
	amp_lfo.freq = MAX_VIBRATO_FREQ / MIDI_MAX * val;
}
/*-------------------------------------------------------*/
void Filt1LFO_amp_set(uint8_t val)
{
	filt_lfo.amp = MAX_FILTER_LFO_AMP / MIDI_MAX * val;
}
/*-------------------------------------------------------*/
void Filt1LFO_freq_set(uint8_t val)
{
	filt_lfo.freq = MAX_VIBRATO_FREQ / MIDI_MAX * val;
}
/*-------------------------------------------------------*/
void Filt2LFO_amp_set(uint8_t val)
{
	filt2_lfo.amp = MAX_FILTER_LFO_AMP / MIDI_MAX * val;
}
/*-------------------------------------------------------*/
void Filt2LFO_freq_set(uint8_t val)
{
	filt2_lfo.freq = MAX_VIBRATO_FREQ / MIDI_MAX * val;
}

/*-------------------------------------------------------*/
void toggleVibrato(void)
{
	if (vibr_lfo.amp !=0)
	{
		vibr_lfo.last_amp = vibr_lfo.amp;
		vibr_lfo.amp = 0;
	}
	else
		vibr_lfo.amp = vibr_lfo.last_amp;
}
/*-------------------------------------------------------*/
void VibratoAmp_set(uint8_t val)
{
	vibr_lfo.amp = MAX_VIBRATO_AMP / MIDI_MAX * val;
}
/*-------------------------------------------------------*/
void VibratoFreq_set(uint8_t val)
{
	vibr_lfo.freq = MAX_VIBRATO_FREQ / MIDI_MAX * val;
}
/*-------------------------------------------------------*/
void toggleSynthOut(void)
{
	if (op1.amp !=0)
	{
		op1.last_amp = op1.amp;
		op1.amp = 0;
		op2.last_amp = op2.amp;
		op2.amp = 0;
		op3.last_amp = op3.amp;
		op3.amp = 0;
	}
	else
	{
		op1.amp = op1.last_amp;
		op2.amp = op2.last_amp;
		op3.amp = op3.last_amp;
	}
}
/*-------------------------------------------------------*/
void SynthOut_switch(uint8_t val)
{
	//if (val == 127) toggleSynthOut();
	switch (val)
	{
	case MIDI_MAXi :
		op1.amp = op1.last_amp;
		op2.amp = op2.last_amp;
		op3.amp = op3.last_amp;
		mbSawOsc.amp = mbSawOsc.last_amp;
		mbRectOsc.amp = mbRectOsc.last_amp;
		mbTriOsc.amp = mbTriOsc.last_amp;
		break;

	case 0 :
		op1.last_amp = op1.amp;
		op1.amp = 0;
		op2.last_amp = op2.amp;
		op2.amp = 0;
		op3.last_amp = op3.amp;
		op3.amp = 0;
		mbSawOsc.last_amp = mbSawOsc.amp;
		mbSawOsc.amp = 0;
		mbRectOsc.last_amp = mbRectOsc.amp;
		mbRectOsc.amp = 0;
		mbTriOsc.last_amp = mbTriOsc.amp;
		mbTriOsc.amp = 0;
		break;
	}
}

/*-------------------------------------------------------*/
void incSynthOut(void)
{
	op1.amp *= 1.2f;
	op2.amp *= 1.2f;
	op3.amp *= 1.2f;
}
/*-------------------------------------------------------*/
void decSynthOut(void)
{
	op1.amp *= .8f;
	op2.amp *= .8f;
	op3.amp *= .8f;
}
/*-------------------------------------------------------*/
void SynthOut_amp_set(uint8_t val)
{
	float_t amp;
	amp = (val * 5.f / MIDI_MAX)*(val * 5.f / MIDI_MAX);
	op1.amp = amp;
	op2.amp = amp;
	op3.amp = amp;
	mbSawOsc.amp = amp;
	mbRectOsc.amp = amp;
	mbTriOsc.amp = amp;
}
/*-------------------------------------------------------*/
/*void Delay_toggle(void)
{
	if (delayON)
	{
		delayON = false;
		Delay_clean();
	}
	else delayON = true;
}*/
/*-------------------------------------------------------*/
/*void Delay_switch(uint8_t val)
{

	if (val > 63)
		delayON = true;
	else
		delayON = false;
}*/
/*-------------------------------------------------------*/
void toggleFilter(void)
{
	if (autoFilterON) autoFilterON = false;
	else autoFilterON = true;
}
/*-------------------------------------------------------*/
void Filter_Random_switch(uint8_t val)
{
	if (val > 63)
		autoFilterON = true;
	else
		autoFilterON = false;
}
/*-------------------------------------------------------*/
void Chorus_toggle(void)
{
	if (chorusON) chorusON = false;
	else chorusON = true;
}
/*-------------------------------------------------------*/
void Chorus_switch(uint8_t val)
{

	if (val > 63)
		chorusON = true;
	else
		chorusON = false;
}
/*-------------------------------------------------------*/
void Phaser_switch(uint8_t val)
{

	if (val > 63)
		phaserON = true;
	else
		phaserON = false;
}
/*-------------------------------------------------------*/
void nextSound(void)
{
	if (sound < LAST_SOUND) (sound)++ ; else sound = LAST_SOUND;
}
/*-------------------------------------------------------*/
void prevSound(void)
{
	if (sound > 0) (sound)-- ; else sound = 0;
}
/*-------------------------------------------------------*/
void Sound_set(uint8_t val)
{
	sound = (uint8_t) rintf((LAST_SOUND - 1) / MIDI_MAX * val);
	if (sound != ADDITIVE) AdditiveGen_newWaveform();
}
/*******************************************************************************************************************************/

void FM_OP1_freq_set(uint8_t val)
{
	FM_op_freq_set(&op1, val);
}
/*-------------------------------------------------------*/
void FM_OP1_modInd_set(uint8_t val)
{
	FM_op_modInd_set(&op1, val);
}

/*----------------------------------------------------------------------------------------------------------------------------*/
void FM_OP2_freq_set(uint8_t val)
{
	op2.mul = Lin2Exp(val, 0.2f, 32.f); // the freq of op2 is a multiple of the main pitch freq (op1)

}
/*-------------------------------------------------------*/
void FM_OP2_freqMul_inc(uint8_t val)
{
	if (val == MIDI_MAX)
	{
		op2.mul *= 1.01f;
	}
}
/*-------------------------------------------------------*/
void FM_OP2_freqMul_dec(uint8_t val)
{
	if (val == MIDI_MAX)
	{
		op2.mul *= 0.99f;
	}
}
/*-------------------------------------------------------*/
void FM_OP2_modInd_set(uint8_t val)
{
	FM_op_modInd_set(&op2, val);
}

/*------------------------------------------------------------------------------------------------------------------------------*/
void FM_OP3_freq_set(uint8_t val)
{
	op3.mul = Lin2Exp(val, 0.2f, 32.f); // the freq of op3 is a multiple of the main pitch freq (op1)
}
/*-------------------------------------------------------*/
void FM_OP3_modInd_set(uint8_t val)
{
	FM_op_modInd_set(&op3, val);
}
/*-------------------------------------------------------*/
void FM_OP3_freqMul_inc(uint8_t val)
{
	if (val == MIDI_MAX)
	{
		op3.mul *= 1.01f;
	}
}
/*-------------------------------------------------------*/
void FM_OP3_freqMul_dec(uint8_t val)
{
	if (val == MIDI_MAX)
	{
		op3.mul *= 0.99f;
	}
}

/*--------------------------------------------------------------------------------------------------------------------------*/
void FM_OP4_freq_set(uint8_t val)
{
	op4.mul = Lin2Exp(val, 0.2f, 32.f); // the freq of op4 is a multiple of the main pitch freq (op1)
}
/*-------------------------------------------------------*/
void FM_OP4_modInd_set(uint8_t val)
{
	FM_op_modInd_set(&op4, val);
}
/*-------------------------------------------------------*/
void FM_OP4_freqMul_inc(uint8_t val)
{
	if (val == MIDI_MAX)
	{
		op4.mul *= 1.01f;
	}
}
/*-------------------------------------------------------*/
void FM_OP4_freqMul_dec(uint8_t val)
{
	if (val == MIDI_MAX)
	{
		op4.mul *= 0.99f;
	}
}

/*===============================================================================================================*/

void dekrispator_synth_init(void)
{
	printf("dekrispator_synth_init()\n");

	vol = env = 1;
	sound = MORPH_SAW;
	autoFilterON = false;
	autoSound = 0;
	chorusON = false;
	delayON = false;
	phaserON = false;

	//Delay_init();
	drifter_init();

	seq.track1.note = NULL;
	sequencer_init();

	ADSR_init(&adsr);
	Chorus_init();
	PhaserInit();
	SVF_init();
	dk_filterFreq = 0.25f;
	dk_filterFreq2 = 0.25f;
	osc_init(&op1, 0.8f, 587.f);
	osc_init(&op2, 0.8f, 587.f);
	osc_init(&op3, 0.8f, 587.f);
	osc_init(&op4, 0.8f, 587.f);
	osc_init(&vibr_lfo, 0, VIBRATO_FREQ);
	osc_init(&filt_lfo, 0, 0);
	osc_init(&filt2_lfo, 0, 0);
	osc_init(&amp_lfo, 0, 0);
	AdditiveGen_newWaveform();
	VCO_blepsaw_Init(&mbSawOsc);
	VCO_bleprect_Init(&mbRectOsc);
	VCO_bleptri_Init(&mbTriOsc);
}
/*---------------------------------------------------------------------------------------*/

void dekrispator_synth_deinit(void)
{
	free(seq.track1.note);
	Chorus_deinit();
}
/*---------------------------------------------------------------------------------------*/

void sequencer_newStep_action(void) // User callback function called by sequencer_process()
{
	if ((noteG.automaticON || noteG.chRequested))
	{
		seq_sequence_new();
		noteG.chRequested = false;
		AdditiveGen_newWaveform();
	}

	if ( (noteG.someNotesMuted) && (rintf(frand_a_b(0.4f , 1)) == 0) )
		ADSR_keyOff(&adsr);
	else
		ADSR_keyOn(&adsr);

	if (autoFilterON)
		SVF_directSetFilterValue(&SVFilter, Ts * 600.f * powf(5000.f / 600.f, frand_a_b(0 , 1)));

	if (noteG.transpose != 0)
	{
		noteG.rootNote += noteG.transpose ;
		seq_transpose();
	}

	if (autoSound == 1)
	{
		switch(rand() % 4) // 4 random timbers
		{
		case 0 : sound = CHORD15; break;
		case 1 : AdditiveGen_newWaveform(); sound = ADDITIVE; break;
		case 2 : sound = CHORD13min5; break;
		case 3 : sound = VOICES3; break;
		}
	}
	if (autoSound == 2)
	{
		sound = rand() % LAST_SOUND;
		if ((sound == CHORD13min5) || (sound == CHORD135))
			sound = VOICES3;
		if ( sound == ADDITIVE)
			AdditiveGen_newWaveform();
	}

	f0 = notesFreq[seq.track1.note[seq.step_idx]]; // Main "melody" frequency
	vol = frand_a_b(0.4f , .8f); // slightly random volume for each note

	#ifdef SEND_MIDI_NOTES
	//send MIDI out
	uint8_t note = (seq.track1.note+seq.step_idx)[0];

	#ifdef USE_LOW_SAMPLE_RATE
	note += 20;
	#else
	note += 22;
	#endif

	send_MIDI_notes(&note, 0, 1); //this is melody, only one note per time
	#endif
}
/*---------------------------------------------------------------------------------------*/

void sequencer_newSequence_action(void) // User callback function called by sequencer_process()
{
	/* A new sequence begins ... */
//	if ((demoMode == true)&&(freeze == false))
//	{
//		MagicPatch(MIDI_MAXi);
//		MagicFX(MIDI_MAXi);
//	}
}
/*===============================================================================================================*/

void dekrispator_make_sound(void)///*uint16_t *buf , uint16_t length*/) // To be used with the Sequencer
{

	//uint16_t 	pos;
	//uint16_t 	*outp;
	float	 	y = 0;
	float	 	yL, yR ;
	float 		f1;
	uint16_t 	valueL, valueR;

	//outp = buf;

	//find out how many patches are in config file
	dekrispator_patch_t patch;
	int total_patches = load_dekrispator_patch(999, &patch);
	printf("dekrispator_make_sound(): total of %d patches found in config\n", total_patches);

	int current_patch = 0;
	//int options = 6;			//default: envelope + filters
	int options = 3;			//default: echo + envelope
	#define MAX_OPTIONS 8

	int enable_echo = 1;		//bit weight: 1
	int enable_envelope = 1;	//bit weight: 2
	int enable_filters = 0;		//bit weight: 4

	int dekrispator_sequence = 0;
	extern int seq_filled;

	//initial indication
	#ifdef BOARD_WHALE
	RGB_LED_R_ON;
	RGB_LED_G_ON;
	//RGB_LED_B_OFF;
	#endif

	//for (pos = 0; pos < length; pos++)
	//for (sampleCounter = 0; sampleCounter < SAMPLERATE; sampleCounter++)
	sampleCounter = 0;
	int dekrispator_patch_store_timeout = 0;

	printf("dekrispator_make_sound(): entering main loop\n");
	while(!event_next_channel)
	{
		//--- Sequencer actions and update ---
		//printf("sequencer\n");
		sequencer_process(); //computes f0 and calls sequencer_newStep_action() and sequencer_newSequence_action()

		//--- compute vibrato modulation ---
		//printf("vibrato\n");
		f1 = f0 * (1 +  Osc_WT_SINE_SampleCompute(&vibr_lfo));

		//--- Generate waveform ---
		//printf("waveform\n");
		y = /*0.2f * */ waveCompute(sound, f1);

		//printf("ADC_sample\n");
		y += (float)((int16_t)ADC_sample) * 0.005f;	//OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT*2

		//printf("envelope\n");
		if(enable_envelope)
		{
			//--- Apply envelop and tremolo ---
			env = ADSR_computeSample(&adsr) * (1 + Osc_WT_SINE_SampleCompute(&amp_lfo));

			y *= vol * env; // apply volume and envelop

			if (adsr.cnt_ >= seq.gateTime) ADSR_keyOff(&adsr);
		}
		else
		{
			y *= vol; // apply volume only
		}

		//printf("filters\n");
		if(enable_filters)
		{
			//--- Apply filter effect ---
			// Update the filters cutoff frequencies
			if ((! autoFilterON)&&(filt_lfo.amp != 0))
				SVF_directSetFilterValue(&SVFilter, dk_filterFreq * (1 + OpSampleCompute7bis(&filt_lfo)));
			if (filt2_lfo.amp != 0)
				SVF_directSetFilterValue(&SVFilter2, dk_filterFreq2 * (1 + OpSampleCompute7bis(&filt2_lfo)));
			y = 0.5f * (SVF_calcSample(&SVFilter, y) + SVF_calcSample(&SVFilter2, y)); // Two filters in parallel
		}


		//---  Apply delay effect ----
		//if (delayON) 	y = Delay_compute(y);

		//---  Apply phaser effect ----
		//printf("phaser\n");
		if (phaserON) 	y = Phaser_compute(y);

		//--- Apply chorus/flanger effect ---
		//printf("chorus\n");
		if (chorusON) stereoChorus_compute (&yL, &yR, y) ;
		else
			yL = yR = y;

		//--- clipping ---
		//yL = (yL > 1.0f) ? 1.0f : yL; //clip too loud left samples
		//yL = (yL < -1.0f) ? -1.0f : yL;

		//yR = (yR > 1.0f) ? 1.0f : yR; //clip too loud right samples
		//yR = (yR < -1.0f) ? -1.0f : yR;

		/****** let's hear the new sample *******/

		//valueL = (uint16_t)((int16_t)((32767.0f) * yL)); // conversion float -> int
		//valueR = (uint16_t)((int16_t)((32767.0f) * yR));
		//valueL = (uint16_t)((int16_t)((2000.0f) * yL)); // conversion float -> int
		//valueR = (uint16_t)((int16_t)((2000.0f) * yR));
		valueL = (uint16_t)(2000.0f * yL); // conversion float -> int
		valueR = (uint16_t)(2000.0f * yR);

		//*outp++ = valueL; // left channel sample
		//*outp++ = valueR; // right channel sample

		//printf("echo\n");
		#ifdef DEKRISPATOR_ECHO
		if(enable_echo)
		{
			sample32 = ((add_echo(valueL))<<16) + add_echo(valueR);
		}
		else
		{
			sample32 = (valueL<<16) + valueR;
		}
		#else
		//sample32 = (((int32_t)(valueL) + (int32_t)(((valueR)<<16)&0xffff0000)) >> 8) & 0x00ff00ff;
		sample32 = (valueL<<16) + valueR;
		#endif
/*
		if(sampleCounter & (1<<add_beep))
		{
			if(add_beep)
			{
				sample32 += (100 + (100<<16));
			}
		}
*/
		#ifdef USE_FAUX_LOW_SAMPLE_RATE
		i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
		i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);
		#endif
		i2s_push_sample(I2S_NUM, (char *)&sample32, portMAX_DELAY);
		i2s_pop_sample(I2S_NUM, (char*)&ADC_sample, portMAX_DELAY);

		sampleCounter++;

		if(short_press_volume_plus)
		{
			#ifdef RANDOM_PATCHES
			MagicPatch(MIDI_MAXi);
			#else

			//MagicPatch(1); //patches are numbered from 1

			current_patch++;
			if(current_patch>total_patches)
			{
				current_patch = 1;
			}
			MagicPatch(current_patch); //patches are numbered from 1
			MagicFX(current_patch);
			#endif

			short_press_volume_plus = 0;
		}
		if(short_press_volume_minus)
		{
			#ifdef RANDOM_PATCHES
			MagicFX(MIDI_MAXi);
			#else

			//MagicPatch(2); //patches are numbered from 1

			current_patch--;
			if(current_patch<=0)
			{
				current_patch = total_patches;
			}
			MagicPatch(current_patch); //patches are numbered from 1
			MagicFX(current_patch);
			#endif

			short_press_volume_minus = 0;
		}
		if(event_channel_options)
		{
			//cycle through some options
			options++;
			if(options==MAX_OPTIONS)
			{
				options = 0;
			}

			//enable_echo = 1 - enable_echo;
			enable_echo = options % 2;
			enable_envelope = (options / 2) % 2;
			enable_filters = (options / 4) % 2;
			printf("dekrispator_make_sound(): options set, echo = %d, envelope = %d, filters = %d\n", enable_echo, enable_envelope, enable_filters);

			#ifdef BOARD_WHALE
			if(enable_echo)		{ RGB_LED_R_ON; } else { RGB_LED_R_OFF; }
			if(enable_envelope)	{ RGB_LED_G_ON; } else { RGB_LED_G_OFF; }
			if(enable_filters)	{ RGB_LED_B_ON; } else { RGB_LED_B_OFF; }
			#endif

			event_channel_options = 0;
		}
		if(short_press_sequence==2)
		{
			short_press_sequence = 0;
			dekrispator_sequence++;
			NUMBER_STEPS = load_dekrispator_sequence(dekrispator_sequence, seq.track1.note, &INIT_TEMPO);
			if(NUMBER_STEPS)
			{
				printf("dekrispator_make_sound(): sequence %d loaded, found %d notes, tempo = %d\n", 1, NUMBER_STEPS, INIT_TEMPO);
			}
			else
			{
				NUMBER_STEPS = DKR_NUMBER_STEPS_DEFAULT;
				INIT_TEMPO = DKR_INIT_TEMPO_DEFAULT;
				seq_filled = 0;
				seq_sequence_new();
				dekrispator_sequence = 0;
			}

			sequencer_init();
		}
		if(short_press_sequence==-2)
		{
			short_press_sequence = 0;
			load_dekrispator_patch_nvs(dkr_patch);
			MagicPatch(MIDI_MAXi+2);
			MagicFX(MIDI_MAXi+2);
		}
		if(short_press_sequence==3)
		{
			short_press_sequence = 0;
			MagicPatch(MIDI_MAXi);
			printf("dekrispator_make_sound(): new random MagicPatch loaded\n");
			dekrispator_patch_store_timeout = DEKRISPATOR_PATCH_STORE_TIMEOUT;
		}
		if(short_press_sequence==-3)
		{
			short_press_sequence = 0;
			MagicFX(MIDI_MAXi);
			printf("dekrispator_make_sound(): new random MagicFX loaded\n");
			dekrispator_patch_store_timeout = DEKRISPATOR_PATCH_STORE_TIMEOUT;
		}

		if(dekrispator_patch_store_timeout)
		{
			dekrispator_patch_store_timeout--;
			if(!dekrispator_patch_store_timeout)
			{
				store_dekrispator_patch_nvs(dkr_patch);
			}
		}
		if (TIMING_BY_SAMPLE_EVERY_10_MS == 33) //100Hz
		{
			if(limiter_coeff < DYNAMIC_LIMITER_COEFF_DEFAULT)
			{
				//limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //timing @20Hz, limiter will fully recover within 1 second
				limiter_coeff += DYNAMIC_LIMITER_COEFF_DEFAULT / 20; //timing @ 100Hz, limiter will fully recover within 0.2 second
			}
		}
	}
}

/*===============================================================================================================*/

