/*
 * v1_filters.cpp
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Apr 27, 2016
 *      Author: mario
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of GNU GPLv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/
 *
 */

#include "v1_filters.h"
#include "v1_freqs.h"
#include "legacy.h"

v1_filters::v1_filters(void)
{
	fp.volume_f = 19.0f / 128.0f;
	fp.reso2 = 0.99; //main test preset (v081)

	fp.enable_mixing_deltas = (ENABLE_MIXING_DELTAS == 1);

	fp.cutoff2_default = 0.159637183;

	fp.cutoff2lim[0] = 0.079818594;
	fp.cutoff2lim[1] = 0.319274376;

	fp.reso2lim[0] = 0.950;
	fp.reso2lim[1] = 0.995;

	fp.arpeggiator_filter_pair = -1; //none by default

	chord = new v1_Chord();
	chord->parse_notes(chord->base_notes, chord->bases_parsed);
	chord->generate(CHORD_MAX_VOICES);
}

v1_filters::~v1_filters(void)
{
	delete[] iir2;
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
		iir2[i].setCutoffAndLimits((float)v1_freqs[i%(FILTERS/2)] * FILTERS_FREQ_CORRECTION / (float)v1_I2S_AUDIOFREQ * 2 * 3);

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

	update_filters_loop--;
	int filter_n = update_filters_f[update_filters_loop];

	if(filter_n >= 0) //something to update
	{
		if(fp.arpeggiator_filter_pair >= 0 && (filter_n == fp.arpeggiator_filter_pair || filter_n == CHORD_MAX_VOICES + fp.arpeggiator_filter_pair)) //arpeggiator
		{
			fil->iir2[filter_n].setResonanceKeepFeedback(0.9998); //higher reso for arpeggiated voice
			fil->iir2[filter_n].setCutoffAndLimits(update_filters_freq[update_filters_loop] * FILTERS_FREQ_CORRECTION / (float)v1_I2S_AUDIOFREQ * 2 * 3);
			fil->iir2[filter_n].disturbFilterBuffers();
		}
		else //background chord
		{
			fil->iir2[filter_n].setCutoffAndLimits(update_filters_freq[update_filters_loop] * FILTERS_FREQ_CORRECTION / (float)v1_I2S_AUDIOFREQ * 2 * 3);
		}
	}
}

IRAM_ATTR void v1_filters::add_to_update_filters_pairs(int filter_n, float freq)
{
	if(update_filters_loop>0)
	{
		//some filters haven't finished updating
		//update_filters_loop = 0;
	}

	update_filters_f[update_filters_loop] = filter_n;
	update_filters_freq[update_filters_loop] = freq;//*tuning_l/440.0f;;
	update_filters_loop++;

	update_filters_f[update_filters_loop] = filter_n + FILTERS/2; //second filter of the pair starts at FILTERS/2 position
	update_filters_freq[update_filters_loop] = freq;//*tuning_r/440.0f;;
	update_filters_loop++;
}
