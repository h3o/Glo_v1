/*
 * Interface.cpp
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

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "hw/init.h"
#include "hw/codec.h"
#include "hw/gpio.h"
#include "hw/signals.h"
#include "hw/ui.h"
#include "InitChannels.h"
#include "Interface.h"
#include "MusicBox.h"
#include "Chaos.h"

unsigned long seconds;

int progressive_rhythm_factor;
float noise_volume, noise_volume_max, noise_boost_by_sensor, mixed_sample_volume;
int special_effect, selected_song, selected_melody;

int FILTERS_TYPE_AND_ORDER;
int ACTIVE_FILTERS_PAIRS;
int PROGRESS_UPDATE_FILTERS_RATE;

int SHIFT_CHORD_INTERVAL;

int direct_update_filters_id[FILTERS];
float direct_update_filters_freq[FILTERS];

#if WIND_FILTERS == 4
int wind_freqs[] = {880,880};
float wind_cutoff[WIND_FILTERS] = {0.159637183,0.159637183,0.159637183,0.159637183};
float wind_cutoff_limit[2] = {0.079818594,0.319274376};
#endif

#if WIND_FILTERS == 8
int wind_freqs[] = {880,880,880,880};
float wind_cutoff[WIND_FILTERS] = {0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183};
float wind_cutoff_limit[2] = {0.079818594,0.319274376};
#endif

#if WIND_FILTERS == 10
int wind_freqs[] = {880,880,880,880,880};
float wind_cutoff[WIND_FILTERS] = {0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183};
float wind_cutoff_limit[2] = {0.079818594,0.319274376};
#endif

#if WIND_FILTERS == 12
int wind_freqs[] = {880,880,880,880,880,880};
float wind_cutoff[WIND_FILTERS] = {0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183};
float wind_cutoff_limit[2] = {0.079818594,0.319274376};
#endif

int arpeggiator_loop;

int use_alt_settings = 0;
int use_binaural = 0;

void filters_init(float resonance)
{
	//printf("filters_init()\n");
	//printf("selected_song = %d\n", selected_song);

	//initialize filters
	printf("filters_init(): fil = new Filters(selected_song=%d,resonance=%f)... ", selected_song, resonance);
	fil = new Filters(selected_song, resonance);
	set_tuning_all(global_tuning);

	printf("[new Filters]Free heap: %u\n", xPortGetFreeHeapSize());

	if(selected_melody > 0)
	{
		printf("fil->set_melody_voice(DEFAULT_MELODY_VOICE=%d, selected_melody)... ", DEFAULT_MELODY_VOICE);
		fil->set_melody_voice(DEFAULT_MELODY_VOICE, selected_melody);
	}

	printf("fil->setup(FILTERS_TYPE_AND_ORDER=%d)... ",FILTERS_TYPE_AND_ORDER);
	fil->setup(FILTERS_TYPE_AND_ORDER);
	printf("[fil->setup]Free heap: %u\n", xPortGetFreeHeapSize());

	#if WIND_FILTERS > 0

    if(wind_voices>0)
    {
    	printf("filters_init(): wind_voices used, n=%d\n",wind_voices);

    	for(int i=0;i<WIND_FILTERS;i++)
    	{
    		fil->iir2[i]->setResonance(0.970); //value from Antarctica.cpp
    		fil->iir2[i]->setCutoff((float)wind_freqs[i%(WIND_FILTERS/2)] / (float)I2S_AUDIOFREQ * 2 * 2);
    	}
    }
	#endif
}

void program_settings_reset()
{
	noise_volume_max = 1.0f;
	noise_boost_by_sensor = 1.0f;
	special_effect = 0;
	mixed_sample_volume = 1.0f;

	//selected_song = 1;
	selected_melody = 0;
	wind_voices = 0;

    FILTERS_TYPE_AND_ORDER = DEFAULT_FILTERS_TYPE_AND_ORDER;

    ACTIVE_FILTERS_PAIRS = ACTIVE_FILTER_PAIRS_ALL; //normally, all filter pairs are active

	PROGRESS_UPDATE_FILTERS_RATE = 100; //every 10 ms, at 0.129 ms (57/441)
    //DEFAULT_ARPEGGIATOR_FILTER_PAIR = 0;

    SHIFT_CHORD_INTERVAL = 2; //default every 2 seconds

    ECHO_MIXING_GAIN_MUL = 2; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 3; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

    echo_dynamic_loop_length0 = echo_dynamic_loop_length;
    echo_skip_samples = 0;
    echo_skip_samples_from = 0;

	seconds = 0;
    sampleCounter = 0;
    arpeggiator_loop = 0;
	progressive_rhythm_factor = 1;
    noise_volume = noise_volume_max;

	#ifdef BOARD_WHALE
    //reset counters and events to default values
    short_press_volume_minus = 0;
    short_press_volume_plus = 0;
    short_press_volume_both = 0;
    long_press_volume_plus = 0;
    long_press_volume_minus = 0;
	long_press_volume_both = 0;

	short_press_button_play = 0;
    play_button_cnt = 0;
    short_press_sequence = 0;
	#endif

    sample_lpf[0] = 0;
    sample_lpf[1] = 0;

	if(set_chords_map != NULL)
    {
    	free(set_chords_map);
    	set_chords_map = NULL;
    }

	if(temp_song != NULL)
    {
    	free(temp_song);
    	temp_song = NULL;
    }
}

extern "C" void set_tuning(float c_l,float c_r,float m_l,float m_r,float a_l,float a_r)
{
	fil->fp.tuning_chords_l = c_l;
	fil->fp.tuning_chords_r = c_r;
	fil->fp.tuning_melody_l = m_l;
	fil->fp.tuning_melody_r = m_r;
	fil->fp.tuning_arp_l = a_l;
	fil->fp.tuning_arp_r = a_r;
}

extern "C" void set_tuning_all(float freq)
{
	fil->fp.tuning_chords_l = freq;
	fil->fp.tuning_chords_r = freq;
	fil->fp.tuning_melody_l = freq;
	fil->fp.tuning_melody_r = freq;
	fil->fp.tuning_arp_l = freq;
	fil->fp.tuning_arp_r = freq;
}

extern "C" void transpose_song(int direction)
{
	fil->musicbox->transpose_song(direction);
}

extern "C" void reload_song_and_melody(char *song, char *melody)
{
	//printf("reload_song_and_melody(): total chords required = %d\n",fil->chord->total_chords);

	int chords_received = fil->musicbox->get_song_total_chords(song);
	printf("reload_song_and_melody(): total chords received = %d\n",chords_received);

	if(fil->musicbox->total_chords < chords_received) //need to trim the progression
	{
		char *song_ptr = song;
		printf("reload_song_and_melody(): too many chords, trimming to %d\n", fil->musicbox->total_chords);
		for(int i=0;i<fil->musicbox->total_chords;i++)
		{
			song_ptr = strstr(song_ptr+1, ",");
		}
		song_ptr[0] = 0;
		int trimmed_chords = fil->musicbox->get_song_total_chords(song);
		printf("reload_song_and_melody(): trimmed to %d\n", trimmed_chords);
	}
	else if(fil->musicbox->total_chords > chords_received) //need to reduce chords amount in musicbox
	{
		printf("reload_song_and_melody(): not enough chords, reducing settings to %d\n", chords_received);
		fil->musicbox->total_chords = chords_received;
	}

	//printf("reload_song_and_melody(): Free heap [1]: %u\n", xPortGetFreeHeapSize());
	parse_notes(song, fil->musicbox->bases_parsed, fil->musicbox->led_indicators, fil->musicbox->midi_notes);
	//printf("reload_song_and_melody(): Free heap [2]: %u\n", xPortGetFreeHeapSize());
	fil->musicbox->generate(CHORD_MAX_VOICES, 0); //do not allocate memory for freqs
	fil->musicbox->current_chord = 0;
	//printf("reload_song_and_melody(): Free heap [3]: %u\n", xPortGetFreeHeapSize());

	if(melody==NULL)
	{
		printf("reload_song_and_melody(): no melody generated, disabling voices\n");
		//disable melody
		fil->fp.melody_filter_pair = -1;
		fil->musicbox->total_melody_notes = 0;
		fil->musicbox->current_melody_note = 0;
		//fil->chord->use_melody = NULL;
	}

	printf("reload_song_and_melody(): done!\n");

	free(song);
	if(melody!=NULL)
	{
		free(melody);
	}
	//printf("reload_song_and_melody(): Free heap [4]: %u\n", xPortGetFreeHeapSize());
}

extern "C" void randomize_song_and_melody()
{
	printf("randomize_song_and_melody(): total chords required = %d\n",fil->musicbox->total_chords);
	//printf("randomize_song_and_melody(): Free heap [1]: %u\n", xPortGetFreeHeapSize());
	char *progression_str=NULL, *melody_str=NULL;
	int progression_str_length = get_music_from_noise(&progression_str, &melody_str); //this will allocate the memory
	printf("randomize_song_and_melody(): Generated chord progression string length = %d, str = %s\n", progression_str_length, progression_str);
	//printf("randomize_song_and_melody(): Free heap [2]: %u\n", xPortGetFreeHeapSize());
	reload_song_and_melody(progression_str, melody_str); //this will free the allocated memory
	//printf("randomize_song_and_melody(): Free heap [3]: %u\n", xPortGetFreeHeapSize());
}
