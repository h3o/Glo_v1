/*
 * glo_config.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Jan 31, 2019
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

#ifndef GLO_CONFIG_H_
#define GLO_CONFIG_H_

#include "hw/init.h"
#include "dsp/Dekrispator.h"
#include "dsp/Antarctica.h"

#ifdef BOARD_WHALE
#define CONFIG_ADDRESS 0x90000
#define CONFIG_SIZE 0x10000
#endif

#ifdef BOARD_GECHO
#define CONFIG_ADDRESS 0x20000
#define CONFIG_SIZE 0x10000
#endif

#define CONFIG_LINE_MAX_LENGTH 500

//config.txt settings structure

typedef struct
{
	int16_t CONFIG_FW_VERSION;
	float TUNING_DEFAULT;
	float TUNING_MIN;
	float TUNING_MAX;
	double TUNING_INCREASE_COEFF;

	float GRANULAR_DETUNE_COEFF_SET;
	float GRANULAR_DETUNE_COEFF_MUL;
	float GRANULAR_DETUNE_COEFF_MAX;

	int TEMPO_BPM_DEFAULT;
	int TEMPO_BPM_MIN;
	int TEMPO_BPM_MAX;
	int TEMPO_BPM_STEP;

	int AUTO_POWER_OFF_TIMEOUT;
	uint8_t AUTO_POWER_OFF_ONLY_IF_NO_MOTION;

	uint8_t DEFAULT_ACCESSIBLE_CHANNELS;

	int8_t AGC_ENABLED, AGC_MAX_GAIN, AGC_TARGET_LEVEL, AGC_MAX_GAIN_STEP, AGC_MAX_GAIN_LIMIT, MIC_BIAS;
	uint8_t CODEC_ANALOG_VOLUME_DEFAULT;
	uint8_t CODEC_DIGITAL_VOLUME_DEFAULT;

	int CLOUDS_HARD_LIMITER_POSITIVE;
	int CLOUDS_HARD_LIMITER_NEGATIVE;
	int CLOUDS_HARD_LIMITER_MAX;
	int CLOUDS_HARD_LIMITER_STEP;

	float DRUM_THRESHOLD_ON;
	float DRUM_THRESHOLD_OFF;
	int DRUM_LENGTH1;
	int DRUM_LENGTH2;
	int DRUM_LENGTH3;
	int DRUM_LENGTH4;

	float DCO_ADC_SAMPLE_GAIN;
	float KS_FREQ_CORRECTION;

	uint64_t IDLE_SET_RST_SHORTCUT;
	uint64_t IDLE_RST_SET_SHORTCUT;
	uint64_t IDLE_LONG_SET_SHORTCUT;

} settings_t;

//persistent settings structure

typedef struct
{
	int ANALOG_VOLUME;
	int8_t ANALOG_VOLUME_updated;

	int DIGITAL_VOLUME;
	int8_t DIGITAL_VOLUME_updated;

	uint8_t ADC_INPUT_SELECT;
	int8_t ADC_INPUT_SELECT_updated;

	int8_t EQ_BASS;
	int8_t EQ_BASS_updated;
	int8_t EQ_TREBLE;
	int8_t EQ_TREBLE_updated;

	int TEMPO;
	int8_t TEMPO_updated;

	double FINE_TUNING;
	int8_t FINE_TUNING_updated;

	int8_t TRANSPOSE;
	int8_t TRANSPOSE_updated;

	int8_t BEEPS;
	int8_t BEEPS_updated;

	int8_t ALL_CHANNELS_UNLOCKED;
	int8_t ALL_CHANNELS_UNLOCKED_updated;

	int8_t MIDI_SYNC_MODE;
	int8_t MIDI_SYNC_MODE_updated;

	int8_t MIDI_POLYPHONY;
	int8_t MIDI_POLYPHONY_updated;

	int8_t PARAMS_SENSORS;
	int8_t PARAMS_SENSORS_updated;

	int8_t AGC_ENABLED_OR_PGA;
	int8_t AGC_ENABLED_OR_PGA_updated;

	int8_t AGC_MAX_GAIN;
	int8_t AGC_MAX_GAIN_updated;

	int8_t MIC_BIAS;
	int8_t MIC_BIAS_updated;

	int8_t ALL_LEDS_OFF;
	int8_t ALL_LEDS_OFF_updated;

	int8_t AUTO_POWER_OFF;
	int8_t AUTO_POWER_OFF_updated;

	int8_t SD_CARD_SPEED;
	int8_t SD_CARD_SPEED_updated;

	uint16_t SAMPLING_RATE;
	int8_t SAMPLING_RATE_updated;

	int8_t ACC_ORIENTATION;
	int8_t ACC_ORIENTATION_updated;

	int8_t ACC_INVERT;
	int8_t ACC_INVERT_updated;

	int update;

} persistent_settings_t;

//samples

#define SAMPLES_BASE 0x330000
#define SAMPLES_MAX 32

typedef struct
{
	char	*name;
	int		p_count;
	int		i1;
	int		i2;
	int		i3;
	float	f4;
	int 	settings;
	int 	binaural;
	char 	*str_param;

} channels_map_t;

extern int remapped_channels[], remapped_channels_found;

typedef struct
{
	int		id;
	char	*name;
	int		total_coords;
	int		*coord_x;
	float	*coord_y;
	int 	total_length;

} binaural_profile_t;

#define BINAURAL_BEATS_AVAILABLE 4 //how many waves are available for cycling through
extern const char *binaural_beats_looping[BINAURAL_BEATS_AVAILABLE];
extern binaural_profile_t *binaural_profile;

extern int channels_found, unlocked_channels_found;

#define SERVICE_MENU_WRITE_CONFIG		1
#define SERVICE_MENU_RELOAD_CONFIG		2
#define SERVICE_MENU_FACTORY_FW			3
#define SERVICE_MENU_REC_COUNTER_RST	4

#ifdef __cplusplus
 extern "C" {
#endif

void get_samples_map(int *result);
int get_channels_map(channels_map_t *map, uint64_t channel_filter);
int load_dekrispator_patch(int patch_no, dekrispator_patch_t *patch);
int load_dekrispator_sequence(int id, uint8_t *notes, int *tempo);
void store_dekrispator_patch_nvs(dekrispator_patch_t *patch);
void load_dekrispator_patch_nvs(dekrispator_patch_t *patch);

int load_clouds_patch(int patch, float *params, int expected_params, const char *block_name);

int load_song_or_melody(int id, const char *what, char *buffer);

int get_remapped_channels(int *remap, int total_channels);
void move_channel_to_front(int channel_position);

void load_binaural_profile(binaural_profile_t *profile, int binaural_id);
void reload_binaural_profile(binaural_profile_t *profile);

void load_isochronic_def(isochronic_def *def, char *block_name);

int load_settings(settings_t *settings, const char* block_name);
void load_persistent_settings(persistent_settings_t *settings);
void store_persistent_settings(persistent_settings_t *settings);

int load_all_settings();

void persistent_settings_store_eq();
void persistent_settings_store_tempo();
void persistent_settings_store_tuning();

void settings_reset();
void sd_rec_counter_reset();
void set_controls(int settings, int instant_update);
void service_menu_action(int command);
void write_config();

void service_menu();

#ifdef __cplusplus
}
#endif

#endif /* GLO_CONFIG_H_ */
