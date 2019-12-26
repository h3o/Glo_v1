/*
 * Filters.cpp
 *
 *  Created on: Apr 27, 2016
 *      Author: mario
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/gechologists/
 *
 */

#include <FilteredChannels.h>
#include "Filters.h"
#include "freqs.h"
#include "Interface.h"
#include "glo_config.h"

#include <hw/init.h>
#include <hw/midi.h>
#include <hw/ui.h>

Filters::Filters(int song_id, float default_resonance)
{
	fp.resonance = default_resonance; //0.99;
	update_filters_loop = 0;

	/*
	fp.tuning_arp_l = 440.0f;
	fp.tuning_arp_r = 440.0f;
	fp.tuning_chords_l = 440.0f;
	fp.tuning_chords_r = 440.0f;
	fp.tuning_melody_l = 440.0f;
	fp.tuning_melody_r = 440.0f;
	*/

	chord = new MusicBox(song_id);

	if(song_id>0)
	{
		parse_notes(chord->base_notes, chord->bases_parsed, chord->led_indicators, chord->midi_notes);
		chord->generate(CHORD_MAX_VOICES, 1); //allocate memory for freqs
	}

	fp.melody_filter_pair = -1; //none by default
	fp.arpeggiator_filter_pair = -1; //none by default
}

Filters::~Filters(void)
{
	//printf("~Filters()\n");

	//delete(iir2);
	for(int i=0;i<FILTERS;i++)
	{
		//printf("delete(iir2[i=%d]\n",i);
		delete(iir2[i]);
	}
	//printf("delete(chord=0x%lx)\n",(unsigned long)chord);
	delete(chord);
}

void Filters::setup() //with no parameters will set up to default mode (Low-pass, 4th order)
{
	this->setup(DEFAULT_FILTERS_TYPE_AND_ORDER);
}

void Filters::setup(int filters_type_and_order)
{
	fp.filters_type = filters_type_and_order;

	//if(iir2!=NULL) //in case we are resetting all filters
	//{
	//	printf("freeing iir2...\n");
	//	free(iir2);
	//}
/*
	printf("test-create iir2\n");
	printf("iir2 = new IIR_Filter[FILTERS];...");
	for(int i=0;i<FILTERS;i++)
	{
		iir3[i] = new IIR_Filter;
	}
	printf("OK\n");

	printf("test-delete iir2\n");
	for(int i=0;i<FILTERS;i++)
	{
		delete(iir3[i]);
	}
	//delete(iir2);

	for(int i=0;i<10;i++)
	{
		vTaskDelay(200);
		printf("OK...");
	}
	printf("\n");
*/
	if(filters_type_and_order == FILTERS_TYPE_LOW_PASS + FILTERS_ORDER_4)
	{
		printf("iir2 = new IIR_Filter_LOW_PASS_4TH_ORDER[FILTERS]; ...");
		//iir2 = new IIR_Filter[FILTERS];
		//iir2 = new IIR_Filter_LOW_PASS_4TH_ORDER[FILTERS];
		for(int i=0;i<FILTERS;i++)
		{
			iir2[i] = new IIR_Filter_LOW_PASS_4TH_ORDER;
		}
		//printf("OK\n");
	}
	/*
	else if(filters_type_and_order == FILTERS_TYPE_LOW_PASS + FILTERS_ORDER_8)
	{
		printf("iir2 = new IIR_Filter_LOW_PASS_8TH_ORDER[FILTERS]; ...");
		iir2 = new IIR_Filter_LOW_PASS_8TH_ORDER[FILTERS];
		printf("OK\n");
	}
	*/
	else if(filters_type_and_order == FILTERS_TYPE_HIGH_PASS + FILTERS_ORDER_4)
	{
		printf("iir2 = new IIR_Filter_HIGH_PASS_4TH_ORDER[FILTERS]; ...");
		//iir2 = new IIR_Filter[FILTERS];
		//iir2 = new IIR_Filter_HIGH_PASS_4TH_ORDER[FILTERS];
		for(int i=0;i<FILTERS;i++)
		{
			iir2[i] = new IIR_Filter_HIGH_PASS_4TH_ORDER;
		}
		printf("OK\n");
	}

	//int test = 0;
	//test += iir2->BoundaryFn001();
	//test += iir2->BoundaryFn002();

	for(int i=0;i<FILTERS;i++)
	{
		iir2[i]->setResonance(fp.resonance);
		iir2[i]->setCutoffAndLimits((float)freqs[i%(FILTERS/2)] * FILTERS_FREQ_CORRECTION / (float)I2S_AUDIOFREQ * 2 * 3);

		if(i == fp.melody_filter_pair || (fp.melody_filter_pair >= 0 && i == fp.melody_filter_pair + CHORD_MAX_VOICES))
		{
			fp.mixing_volumes[i] = MELODY_MIXING_VOLUME;
		}
		else
		{
			fp.mixing_volumes[i] = 1.0f;
		}
	}

	update_filters_loop = 0;
	printf("...Done!\n");
}

void Filters::release()
{
	for(int i=0;i<FILTERS;i++)
	{
		delete(iir2[i]);
	}
}

void Filters::update_resonance()
{
	for(int i=0;i<FILTERS;i++)
	{
		iir2[i]->setResonance(fp.resonance);
	}
}
/*
void filters::start_update_filters(int f1, int f2, int f3, int f4, float freq1, float freq2, float freq3, float freq4)
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
*/

void Filters::start_update_filters_pairs(int *filter_n, float *freq, int filters_to_update)
{
	if(update_filters_loop!=0)
	{
		//problem - has not finished with previous update (possibly a melody)
		//update_filters_loop = 0;
		printf("puf![%d] ",update_filters_loop);
	}

	for(int i=0;i<filters_to_update;i++)
	{
		update_filters_f[i] = filter_n[i];
		update_filters_freq[i] = freq[i];
		update_filters_f[i+filters_to_update] = filter_n[i] + FILTERS/2; //second filter of the pair starts at FILTERS/2 position
		update_filters_freq[i+filters_to_update] = freq[i];
	}
	update_filters_loop = filters_to_update * 2;
}

void Filters::start_next_chord_MIDI(int skip_voices, uint8_t *MIDI_chord)
{
	if(update_filters_loop!=0)
	{
		//problem - has not finished with previous update (possibly a melody)
		//update_filters_loop = update_filters_loop;
		update_filters_loop = 0;
		//printf("puf! ");
	}

	float freq;

	for(int i=skip_voices;i<CHORD_MAX_VOICES;i++)
	{
		if(MIDI_notes_updated) //if another update came in meanwhile, abort this one
		{
			return;
		}

		if(i == fp.melody_filter_pair)
		{
			//nothing to update, skip this voice
		}
		else
		{
			#ifdef MIDI_4_KEYS_POLYPHONY
			if(MIDI_last_chord[3])
			{
				freq = MIDI_note_to_freq(MIDI_last_chord[chord->expand_multipliers4[i][0]]);

				if(freq==0)
				{
					chord->midi_chords_expanded[i] = 10.0f; //low enough to count as note off
				}
				else
				{
					if(chord->expand_multipliers4[i][1] > 0)
					{
						chord->midi_chords_expanded[i] = freq * (float)chord->expand_multipliers4[i][1];
					}
					else
					{
						chord->midi_chords_expanded[i] = freq / (float)(-chord->expand_multipliers4[i][1]);
					}
				}
			}
			else
			{
			#endif
				freq = MIDI_note_to_freq(MIDI_last_chord[chord->expand_multipliers[i][0]]);

				if(freq==0)
				{
					chord->midi_chords_expanded[i] = 10.0f; //low enough to count as note off
				}
				else
				{
					if(chord->expand_multipliers[i][1] > 0)
					{
						//freq = bases_parsed[3*c + expand_multipliers[t][0]] * (float)expand_multipliers[t][1];
						chord->midi_chords_expanded[i] = freq * (float)chord->expand_multipliers[i][1];
					}
					else
					{
						//chords[c].freqs[t] = bases_parsed[3*c + expand_multipliers[t][0]] / (float)(-expand_multipliers[t][1]);
						chord->midi_chords_expanded[i] = freq / (float)(-chord->expand_multipliers[i][1]);
					}
				}
			#ifdef MIDI_4_KEYS_POLYPHONY
			}
			#endif

			//printf("midi exp note %d = %f\n", i, chord->midi_chords_expanded[i]);

			update_filters_f[update_filters_loop] = i;
			update_filters_freq[update_filters_loop] = chord->midi_chords_expanded[i]*fp.tuning_chords_l/440.0f;
			update_filters_loop++;

			update_filters_f[update_filters_loop] = CHORD_MAX_VOICES + i;
			update_filters_freq[update_filters_loop] = chord->midi_chords_expanded[i]*fp.tuning_chords_r/440.0f;
			update_filters_loop++;
		}
	}
}

void Filters::start_next_chord(int skip_voices)
{
	/*
	if(update_filters_loop!=0)
	{
		//problem - has not finished with previous update (possibly a melody)
		//update_filters_loop = update_filters_loop;
		printf("puf! ");
	}
	*/
	for(int i=skip_voices;i<CHORD_MAX_VOICES;i++)
	{
		if(i == fp.melody_filter_pair)
		{
			//nothing to update, skip this voice
		}
		else
		{
			update_filters_f[update_filters_loop] = i;
			update_filters_freq[update_filters_loop] = chord->chords[chord->current_chord].freqs[i]*fp.tuning_chords_l/440.0f;
			update_filters_loop++;

			update_filters_f[update_filters_loop] = CHORD_MAX_VOICES + i;
			update_filters_freq[update_filters_loop] = chord->chords[chord->current_chord].freqs[i]*fp.tuning_chords_r/440.0f;
			update_filters_loop++;
		}
	}

	//update_filters_loop = CHORD_MAX_VOICES * 2;

    if(midi_sync_mode==MIDI_SYNC_MODE_MIDI_IN_OUT || MIDI_SYNC_MODE_MIDI_OUT || MIDI_SYNC_MODE_MIDI_CLOCK_OUT)
	{
		send_MIDI_notes(chord->midi_notes, chord->current_chord, MIDI_CHORD_NOTES);
	}

	chord->current_chord++;
	if(chord->current_chord >= chord->total_chords)
	{
		chord->current_chord = 0;
	}
}

void Filters::start_next_melody_note()
{
	int i = fp.melody_filter_pair;

	int test_freq = 0;

	if(chord->melody_freqs_parsed[chord->current_melody_note] != 0)
	{
		update_filters_f[update_filters_loop] = i;
		update_filters_freq[update_filters_loop] = chord->melody_freqs_parsed[chord->current_melody_note]*fp.tuning_melody_l/440.0f;;
		update_filters_loop++;

		update_filters_f[update_filters_loop] = CHORD_MAX_VOICES + i;
		update_filters_freq[update_filters_loop] = chord->melody_freqs_parsed[chord->current_melody_note]*fp.tuning_melody_r/440.0f;;

		test_freq = update_filters_freq[update_filters_loop];
		update_filters_loop++;
	}

	chord->current_melody_note++;
	if(chord->current_melody_note >= chord->total_melody_notes)
	{
		chord->current_melody_note = 0;
	}

	test_freq += 0;
}

void Filters::progress_update_filters(Filters *fil, bool reset_buffers)
{
	if(!update_filters_loop)
	{
		return;
	}

	update_filters_loop--;
	int filter_n = update_filters_f[update_filters_loop];

	if(filter_n >= 0) //something to update
	{
		if(fp.melody_filter_pair >= 0 && (filter_n == fp.melody_filter_pair || filter_n == CHORD_MAX_VOICES + fp.melody_filter_pair)) //melody note
		{
			//fil->iir2[filter_n].setCutoffAndLimits_reso2(update_filters_freq[update_filters_loop] * FILTERS_FREQ_CORRECTION / (float)I2S_AUDIOFREQ * 2 * 3);
			fil->iir2[filter_n]->setResonanceKeepFeedback(0.9998); //higher reso for melody voice
			fil->iir2[filter_n]->setCutoffAndLimits(update_filters_freq[update_filters_loop] * FILTERS_FREQ_CORRECTION / (float)I2S_AUDIOFREQ * 2 * 3);
			fil->iir2[filter_n]->disturbFilterBuffers();
		}
		else if(fp.arpeggiator_filter_pair >= 0 && (filter_n == fp.arpeggiator_filter_pair || filter_n == CHORD_MAX_VOICES + fp.arpeggiator_filter_pair)) //arpeggiator
		{
			fil->iir2[filter_n]->setResonanceKeepFeedback(0.9998); //higher reso for arpeggiated voice
			fil->iir2[filter_n]->setCutoffAndLimits(update_filters_freq[update_filters_loop] * FILTERS_FREQ_CORRECTION / (float)I2S_AUDIOFREQ * 2 * 3);
			fil->iir2[filter_n]->disturbFilterBuffers();
			//fil->iir2[filter_n].resetFilterBuffers(); //causes more ticking
			//fil->iir2[filter_n].rampFilter(); //mask the initial ticking
		}
		else //background chord
		{
			fil->iir2[filter_n]->setCutoffAndLimits(update_filters_freq[update_filters_loop] * FILTERS_FREQ_CORRECTION / (float)I2S_AUDIOFREQ * 2 * 3);
		}
	}

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

void Filters::start_nth_chord(int chord_n)
{
	chord_n = chord_n % chord->total_chords; //wrap around, to be extra safe

	for(int i=0;i<CHORD_MAX_VOICES;i++)
	{
		update_filters_f[i] = i;
		update_filters_freq[i] = chord->chords[chord_n].freqs[i];

		update_filters_f[CHORD_MAX_VOICES + i] = CHORD_MAX_VOICES + i;
		update_filters_freq[CHORD_MAX_VOICES + i] = chord->chords[chord_n].freqs[i];
	}

	update_filters_loop = CHORD_MAX_VOICES * 2;
}

void Filters::set_melody_voice(int filter_pair, int melody_id)
{
	//printf("set_melody_voice(): melody_id = %d\n",melody_id);

	fp.melody_filter_pair = filter_pair;

	/*
	if(melody_id == 112 && melody_str != NULL)
	{
		chord->use_melody = melody_str;
	}
	else
	{
		chord->use_melody = (char*)chord->SONGS[melody_id * 2 - 1];
	}
	*/

	#define PARSE_MELODY_BUFFER 4000

	char *melody_buf = (char*)malloc(PARSE_MELODY_BUFFER);
	int total_length = load_song_or_melody(melody_id, "melody", melody_buf);
	printf("MusicBox(%d): load melody total lenght = %d\n", melody_id, total_length);
	if(total_length > PARSE_MELODY_BUFFER)
	{
		printf("MusicBox(%d): PARSE_MELODY_BUFFER overflow: %d > %d\n", melody_id, total_length, PARSE_MELODY_BUFFER);
		while(1); //halt
	}
	if(total_length==0)
	{
		printf("MusicBox(%d): could not find the melody in config block\n", melody_id);
		while(1); //halt
	}

	chord->use_melody = melody_buf;

	//printf("set_melody_voice(): chord->use_melody = %s\n",chord->use_melody);

	if(chord->use_melody != NULL)
	{
		chord->total_melody_notes = chord->get_song_total_melody_notes(chord->use_melody);
		//printf("set_melody_voice(): total melody notes = %d\n",chord->total_melody_notes);

		//printf("set_melody_voice(): allocating memory\n");

		chord->melody_freqs_parsed = (float*)malloc(chord->total_melody_notes * sizeof(float));
		//printf("set_melody_voice(): memory allocated for [melody_freqs_parsed] at %x\n", (unsigned int)chord->melody_freqs_parsed);
		chord->melody_indicator_leds = (int8_t*)malloc(chord->total_melody_notes * sizeof(int8_t));
		//printf("set_melody_voice(): memory allocated for [melody_indicator_leds] at %x\n", (unsigned int)chord->melody_indicator_leds);

		//printf("set_melody_voice(): parsing notes\n");
		parse_notes(chord->use_melody, chord->melody_freqs_parsed, chord->melody_indicator_leds, NULL);
		//printf("set_melody_voice(): notes parsed\n");
	}

	chord->use_melody = NULL;
	free(melody_buf);
}

void Filters::add_to_update_filters_pairs(int filter_n, float freq, float tuning_l, float tuning_r)
{
	if(update_filters_loop>0)
	{
		//some filters haven't finished updating
		//update_filters_loop = 0;
	}

	update_filters_f[update_filters_loop] = filter_n;
	update_filters_freq[update_filters_loop] = freq*tuning_l/440.0f;;
	update_filters_loop++;

	update_filters_f[update_filters_loop] = filter_n + FILTERS/2; //second filter of the pair starts at FILTERS/2 position
	update_filters_freq[update_filters_loop] = freq*tuning_r/440.0f;;
	update_filters_loop++;
}
