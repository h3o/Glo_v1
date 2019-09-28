/*
 * Interface.cpp
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

#include <Interface.h>
#include <dsp/Filters.h>
#include <hw/codec.h>
#include <hw/gpio.h>
#include <hw/signals.h>
#include <InitChannels.h>
#include <MusicBox.h>
#include <Chaos.h>
#include <notes.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <hw/init.h>

unsigned long seconds;

int progressive_rhythm_factor;
float noise_volume, noise_volume_max, noise_boost_by_sensor, mixed_sample_volume;
int special_effect, selected_song, selected_melody;

bool PROG_enable_filters,
	 PROG_enable_rhythm,
	 PROG_enable_chord_loop,
     PROG_add_echo,
     PROG_add_plain_noise,
	 PROG_noise_effects;

int FILTERS_TYPE_AND_ORDER;
int ACTIVE_FILTERS_PAIRS;
int PROGRESS_UPDATE_FILTERS_RATE;
//int DEFAULT_ARPEGGIATOR_FILTER_PAIR;

int SHIFT_CHORD_INTERVAL;

int direct_update_filters_id[FILTERS];
float direct_update_filters_freq[FILTERS];

/*
#if WIND_FILTERS == 0
int wind_freqs[] = {};
float wind_cutoff[WIND_FILTERS] = {};
float wind_cutoff_limit[2] = {0.079818594,0.319274376};
#endif
*/

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
int use_midi = 0;

void filters_and_signals_init(float resonance)
{
	//printf("filters_and_signals_init()\n");
	//printf("selected_song = %d\n", selected_song);

	if(PROG_enable_filters)
	{
		//initialize filters
		printf("fil = new Filters(selected_song=%d,resonance=%f)... ", selected_song, resonance);
		fil = new Filters(selected_song, resonance);
		set_tuning_all(global_tuning);

		//printf("OK\n");
	    printf("[new Filters]Free heap: %u\n", xPortGetFreeHeapSize());

	    if(selected_melody > 0)
		{
			printf("fil->set_melody_voice(DEFAULT_MELODY_VOICE=%d, selected_melody)... ", DEFAULT_MELODY_VOICE);
			fil->set_melody_voice(DEFAULT_MELODY_VOICE, selected_melody);
			//printf("OK\n");
		}

		printf("fil->setup(FILTERS_TYPE_AND_ORDER=%d)... ",FILTERS_TYPE_AND_ORDER);
		fil->setup(FILTERS_TYPE_AND_ORDER);
		//printf("OK\n");
	    printf("[fil->setup]Free heap: %u\n", xPortGetFreeHeapSize());
	}

    if(PROG_add_echo)
    {
    	//printf("clear echo buffer (size=%d)...",sizeof(echo_buffer));
    	//memset(echo_buffer,0,sizeof(echo_buffer));
    	printf("clear echo buffer (size=%d)...",ECHO_BUFFER_LENGTH*sizeof(int16_t));
    	memset(echo_buffer,0,ECHO_BUFFER_LENGTH*sizeof(int16_t));
    	echo_buffer_ptr0 = 0;
    	printf("done!\n");
    }

	#if WIND_FILTERS > 0

    if(wind_voices>0)
    {
    	printf("filters_and_signals_init(): wind_voices used, n=%d\n",wind_voices);

    	for(int i=0;i<WIND_FILTERS;i++)
    	{
    		fil->iir2[i]->setResonance(0.970); //value from Antarctica.cpp
    		fil->iir2[i]->setCutoff((float)wind_freqs[i%(WIND_FILTERS/2)] / (float)I2S_AUDIOFREQ * 2 * 2);
    		//mixing_volumes[i]=1.0f;
    		//mixing_deltas[i]=0;
    	}

    	/*
    	cutoff[0] = 0.159637183;
    	cutoff[1] = 0.159637183;
    	cutoff[2] = 0.159637183;
    	cutoff[3] = 0.159637183;

    	cutoff_limit[0] = 0.079818594;
    	cutoff_limit[1] = 0.319274376;
    	*/

    	//feedback boundaries
    	//feedback = 0.96;
    	//feedback_limit[0] = 0.5;
    	//feedback_limit[1] = 1.0;
    }
	#endif
}

void program_settings_reset()
{
	//run_program = 1; //enable the main program loop
	noise_volume_max = 1.0f;
	noise_boost_by_sensor = 1.0f;
	special_effect = 0;
	mixed_sample_volume = 1.0f;

	//selected_song = 1;
	selected_melody = 0;
	wind_voices = 0;

	PROG_enable_filters = true;
	PROG_enable_rhythm = true;
	PROG_enable_chord_loop = true;
	//PROG_enable_LED_indicators_sequencer = true;
	//PROG_enable_LED_indicators_IR_sensors = true;
    //PROG_enable_S1_control_noise_boost = false;
    //PROG_enable_S2_control_noise_attenuation = false;
	//PROG_enable_S3_control_resonance = false;
	//PROG_enable_S4_control_arpeggiator = false;
	//PROG_add_OpAmp_ADC12_signal = true;
	PROG_add_echo = true;
	PROG_add_plain_noise = true;
	//PROG_audio_input_microphones = true;
    //PROG_audio_input_pickups = false;
    //PROG_audio_input_IR_sensors = false;
    //PROG_buttons_controls_during_play = true;
    PROG_noise_effects = false;
    //PROG_drum_kit = false;
    //PROG_drum_kit_with_echo = false,
    //PROG_wavetable_sample = false;
    //PROG_mix_sample_from_flash = false;

	//TEST_enable_V1_control_voice = false;
    //TEST_enable_V2_control_drum = false;

    FILTERS_TYPE_AND_ORDER = DEFAULT_FILTERS_TYPE_AND_ORDER;

    ACTIVE_FILTERS_PAIRS = ACTIVE_FILTER_PAIRS_ALL; //normally, all filter pairs are active

	PROGRESS_UPDATE_FILTERS_RATE = 100; //every 10 ms, at 0.129 ms (57/441)
    //DEFAULT_ARPEGGIATOR_FILTER_PAIR = 0;

    SHIFT_CHORD_INTERVAL = 2; //default every 2 seconds

    ECHO_MIXING_GAIN_MUL = 2; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 3; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

    //ECHO_MIXING_GAIN_MUL = 4; //amount of signal to feed back to echo loop, expressed as a fragment
    //ECHO_MIXING_GAIN_DIV = 5; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

    //echo_length_updated = 0;
    echo_dynamic_loop_length0 = echo_dynamic_loop_length;
    echo_skip_samples = 0;
    echo_skip_samples_from = 0;

	seconds = 0;
    sampleCounter = 0;
    arpeggiator_loop = 0;
	progressive_rhythm_factor = 1;
    noise_volume = noise_volume_max;

    //reset counters and events to default values
    short_press_volume_minus = 0;
    short_press_volume_plus = 0;
    short_press_volume_both = 0;
    long_press_volume_plus = 0;
    long_press_volume_minus = 0;
	long_press_volume_both = 0;

	#ifdef BOARD_WHALE
	short_press_button_play = 0;
    play_button_cnt = 0;
    short_press_sequence = 0;
	#endif

	#ifdef BOARD_GECHO
    short_press_rst_button = 0;
    long_press_rst_button = 0;
	#endif

    sample_lpf[0] = 0;
    sample_lpf[1] = 0;

    /*
    if(progression_str != NULL)
    {
    	free(progression_str);
    	progression_str = NULL;
    }
    */

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

extern "C" void voice_say(const char *segment)
{
	//this causes guru meditation error
	//printf("voice_say(): channel_deinit()\n");
	//channel_deinit();
	printf("voice_say(): channel_init(10,...)\n");
	use_binaural = 0;
	use_midi = 0;
	//channel_init(10, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0); //map the voice menu sample from Flash to RAM
	channel_init(5, 0, 0, FILTERS_TYPE_NO_FILTERS, 0, 0, 0, 0);
	printf("voice_say(): channel_init() done!\n");

	//let it play at default sample rate, even if sample was recorded at a slightly different one
	//#if SAMPLE_RATE_VOICE_MENU != SAMPLE_RATE_DEFAULT
	//set_sampling_rate(SAMPLE_RATE_VOICE_MENU);
	//#endif

    volume_ramp = 0; //to not interfere with codec_set_mute() in voice_menu_say()

	//settings_menu_active = 1;

	voice_menu_t voice_menu_items[VOICE_MENU_ITEMS_MAX];
	int items_found = get_voice_menu_items(voice_menu_items);

	printf("voice_say(): get_voice_menu_items(): %d items found:\n", items_found);
	for(int i=0;i<items_found;i++)
	{
		printf("%s -> %f / %d - %d\n", voice_menu_items[i].name, voice_menu_items[i].position_f, voice_menu_items[i].position_s, voice_menu_items[i].length_s);
	}

	//voice_menu_say("settings_menu", voice_menu_items, items_found);
	//voice_menu_say("this_is_glo", voice_menu_items, items_found);
	//voice_menu_say("upgrade_your_perception", voice_menu_items, items_found);
	//voice_menu_say("pitch_and_roll", voice_menu_items, items_found);

	voice_menu_say(segment, voice_menu_items, items_found);

	volume_ramp = 1; //to not overwrite stored user volume setting

	//this causes guru meditation error
	//printf("voice_say(): channel_deinit()\n");
	//channel_deinit();

	//#if SAMPLE_RATE_VOICE_MENU != SAMPLE_RATE_DEFAULT
	//set_sampling_rate(SAMPLE_RATE_DEFAULT);
	//#endif

	//release memory
	for(int i=0;i<items_found;i++)
	{
		free(voice_menu_items[i].name);
	}
}

extern "C" void reload_song_and_melody(char *song, char *melody)
{
	//printf("reload_song_and_melody(): total chords required = %d\n",fil->chord->total_chords);

	int chords_received = fil->chord->get_song_total_chords(song);
	printf("reload_song_and_melody(): total chords received = %d\n",chords_received);

	if(fil->chord->total_chords < chords_received) //need to trim the progression
	{
		char *song_ptr = song;
		printf("reload_song_and_melody(): too many chords, trimming to %d\n", fil->chord->total_chords);
		for(int i=0;i<fil->chord->total_chords;i++)
		{
			song_ptr = strstr(song_ptr+1, ",");
		}
		song_ptr[0] = 0;
		int trimmed_chords = fil->chord->get_song_total_chords(song);
		printf("reload_song_and_melody(): trimmed to %d\n", trimmed_chords);
	}
	else if(fil->chord->total_chords > chords_received) //need to reduce chords amount in musicbox
	{
		printf("reload_song_and_melody(): not enough chords, reducing settings to %d\n", chords_received);
		fil->chord->total_chords = chords_received;
	}

	//printf("reload_song_and_melody(): Free heap [1]: %u\n", xPortGetFreeHeapSize());
	parse_notes(song, fil->chord->bases_parsed, fil->chord->led_indicators, fil->chord->midi_notes);
	//printf("reload_song_and_melody(): Free heap [2]: %u\n", xPortGetFreeHeapSize());
	fil->chord->generate(CHORD_MAX_VOICES, 0); //do not allocate memory for freqs
	fil->chord->current_chord = 0;
	//printf("reload_song_and_melody(): Free heap [3]: %u\n", xPortGetFreeHeapSize());

	if(melody==NULL)
	{
		printf("reload_song_and_melody(): no melody generated, disabling voices\n");
		//disable melody
		fil->fp.melody_filter_pair = -1;
		fil->chord->total_melody_notes = 0;
		fil->chord->current_melody_note = 0;
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
	printf("randomize_song_and_melody(): total chords required = %d\n",fil->chord->total_chords);
	//printf("randomize_song_and_melody(): Free heap [1]: %u\n", xPortGetFreeHeapSize());
	char *progression_str=NULL, *melody_str=NULL;
	int progression_str_length = get_music_from_noise(&progression_str, &melody_str); //this will allocate the memory
	printf("randomize_song_and_melody(): Generated chord progression string length = %d, str = %s\n", progression_str_length, progression_str);
	//printf("randomize_song_and_melody(): Free heap [2]: %u\n", xPortGetFreeHeapSize());
	reload_song_and_melody(progression_str, melody_str); //this will free the allocated memory
	//printf("randomize_song_and_melody(): Free heap [3]: %u\n", xPortGetFreeHeapSize());
}
