/*
 * filters.cpp
 *
 *  Created on: Apr 27, 2016
 *      Author: mayo
 */
#include "v1_filters.h"
#include "v1_freqs.h"
#include "legacy.h"

v1_filters::v1_filters(void)
{
	/*
	//OLD filters!
	fp.reso = 7; //9; //12; //6;
	fp.gain = 128; //256;
	//fp.gain = 1;
	fp.volume_coef = 19.0f; //32.0f;
	//fp.volume_f = fp.volume_coef/fp.gain;
	fp.vol_ramp = 0.0f;
	*/

	//NEW filters!
	//fp.reso2 = 0.96;
	//fp.reso2 = 0.987;
	fp.volume_f = 19.0f / 128.0f;
	fp.reso2 = 0.99; //main test preset (v081)
	//fp.reso2 = 0.999; //hammond organ

	fp.enable_mixing_deltas = (ENABLE_MIXING_DELTAS == 1);

	fp.cutoff2_default = 0.159637183;

	fp.cutoff2lim[0] = 0.079818594;
	fp.cutoff2lim[1] = 0.319274376;

	fp.reso2lim[0] = 0.950;
	fp.reso2lim[1] = 0.995;

	//unsigned long vol_fadein = I2S_AUDIOFREQ * 2 * 5; //2 channels, 5 seconds ramp
	//unsigned long vol_ramp_delay = 1; //3 sec delay before volume fades up

	chord = new v1_Chord();
	chord->parse_notes(chord->base_notes, chord->bases_parsed);
	chord->generate(CHORD_MAX_VOICES);
}

v1_filters::~v1_filters(void)
{
	//for(int i=0;i<FILTERS;i++)
	//{
	//delete iir2;
	delete[] iir2;
	//}
	delete(chord);
}

/*
void filters::filter_setup()
{
  iir = new FILTER[FILTERS];
	//for(i=0;i<FILTERS;i++)
  //{
	//filter[i] = new FilterIIR00();
	//filter[i]->prepare_IIR(&iir[i], v1_freqs[i%(FILTERS/2)], reso, gain);
  //}
	for(int i=0;i<FILTERS;i++)
	{
		FilterIIR00::prepare_IIR(&iir[i], v1_freqs[i%(FILTERS/2)], fp.reso, fp.gain);
		//mixing_volumes[i/2]=0.0f;
		//mixing_deltas[i/2]=0;

		if(fp.enable_mixing_deltas)
		{
			fp.mixing_volumes[i]=0.0f;
		}
		else
		{
			fp.mixing_volumes[i]=1.0f;
		}

		fp.mixing_deltas[i]=0;
	}
}
*/

IRAM_ATTR void v1_filters::filter_setup02()
{
	iir2 = new v1_FilterSimpleIIR02[FILTERS];

	for(int i=0;i<FILTERS;i++)
	{
		iir2[i].setResonance(fp.reso2);
		//iir2[i].setCutoff((float)v1_freqs[i%(FILTERS/2)] / (float)I2S_AUDIOFREQ * 2 * 2);
		iir2[i].setCutoffAndLimits((float)v1_freqs[i%(FILTERS/2)] * FILTERS_FREQ_CORRECTION / (float)v1_I2S_AUDIOFREQ * 2 * 3);
		//iir2[i].setCutoff(v1_freqs[i%(FILTERS/2)]);
		//iir2[i].setResonance(12.8);
		//iir2[i].setCutoff(0.5);
		//iir2[i].setFilterMode(FilterSimpleIIR02::FILTER_MODE_BANDPASS);

		if(fp.enable_mixing_deltas)
		{
			fp.mixing_volumes[i] = 0.0f;
		}
		else
		{
			fp.mixing_volumes[i] = 1.0f;
		}

		fp.mixing_deltas[i] = 0;
		fp.cutoff2[i] = fp.cutoff2_default;
	}
	//iir2[0].setResonance(0.98);
	//iir2[0].setCutoff(0.5);
	//iir2[1].setResonance(0.99);
	//iir2[1].setCutoff(0.4);

	//calculate Feedback boundaries: ???
	fp.feedback2 = 0.96;
	fp.feedback2lim[0] = 0.5;
	fp.feedback2lim[1] = 1.0;
}

IRAM_ATTR void v1_filters::start_update_filters(int f1, int f2, int f3, int f4, float freq1, float freq2, float freq3, float freq4)
{
	update_filters_f[0] = f1;
	update_filters_f[1] = f2;
	update_filters_f[2] = f3;
	update_filters_f[3] = f4;
	update_filters_freq[0] = freq1;
	update_filters_freq[1] = freq2;
	update_filters_freq[2] = freq3;
	update_filters_freq[3] = freq4;

	update_filters_loop = 4;
}

IRAM_ATTR void v1_filters::start_next_chord()
{
	for(int i=0;i<CHORD_MAX_VOICES;i++)
	{
		update_filters_f[i] = i;
		update_filters_freq[i] = chord->chords[chord->current_chord].v1_freqs[i];

		update_filters_f[CHORD_MAX_VOICES + i] = CHORD_MAX_VOICES + i;
		update_filters_freq[CHORD_MAX_VOICES + i] = chord->chords[chord->current_chord].v1_freqs[i];
	}

	update_filters_loop = CHORD_MAX_VOICES * 2;

	chord->current_chord++;
	if(chord->current_chord >= chord->total_chords)
	{
		chord->current_chord = 0;
	}
}

IRAM_ATTR void v1_filters::progress_update_filters(v1_filters *fil, bool reset_buffers)
{
	if(!update_filters_loop)
	{
		return;
	}

	//at once
	//for(update_filters_loop;update_filters_loop>0;)
	//{

	update_filters_loop--;
	fil->iir2[update_filters_f[update_filters_loop]].setCutoffAndLimits(update_filters_freq[update_filters_loop] * FILTERS_FREQ_CORRECTION / (float)v1_I2S_AUDIOFREQ * 2 * 3);

	/*
	if(reset_buffers && update_filters_loop == 0)
	{
		for(int f=0;f<CHORD_MAX_VOICES*2;f++)
		{
			fil->iir2[update_filters_f[f]].resetFilterBuffers();
		}
	}
	*/
}
