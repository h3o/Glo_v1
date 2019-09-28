/*
 * glo_config.h
 *
 *  Created on: Jan 31, 2019
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

#ifndef GLO_CONFIG_H_
#define GLO_CONFIG_H_

#include "hw/init.h"
#include "dsp/Dekrispator.h"
#include "dsp/Antarctica.h"

#define CONFIG_ADDRESS 0x90000
#define CONFIG_SIZE 0x10000

#define CONFIG_LINE_MAX_LENGTH 500

//settings

typedef struct
{
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

	int8_t AGC_ENABLED, AGC_MAX_GAIN, AGC_TARGET_LEVEL, AGC_MAX_GAIN_STEP, AGC_MAX_GAIN_LIMIT;
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

} settings_t;

typedef struct
{
	int ANALOG_VOLUME;
	int ANALOG_VOLUME_updated;
	int DIGITAL_VOLUME;
	int DIGITAL_VOLUME_updated;
	int8_t EQ_BASS;
	int EQ_BASS_updated;
	int8_t EQ_TREBLE;
	int EQ_TREBLE_updated;
	int TEMPO;
	int TEMPO_updated;
	double FINE_TUNING;
	int FINE_TUNING_updated;
	int8_t BEEPS;
	int BEEPS_updated;

	int8_t ALL_CHANNELS_UNLOCKED;
	int ALL_CHANNELS_UNLOCKED_updated;

	int update;

} persistent_settings_t;

//samples

#define SAMPLES_BASE 0xa0000
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

} channels_map_t;

//extern spi_flash_mmap_handle_t mmap_handle_config;
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

typedef struct
{
	char		*name;
	float		position_f;
	uint32_t	position_s;
	uint32_t 	length_s;

} voice_menu_t;

#define VOICE_MENU_ITEMS_MAX 	100
#define VOICE_MENU_ITEM_LENGTH	30

#define BINAURAL_BEATS_AVAILABLE 4 //how many waves are available for cycling through
extern const char *binaural_beats_looping[BINAURAL_BEATS_AVAILABLE];
extern binaural_profile_t *binaural_profile;

extern int channels_found, unlocked_channels_found;

#ifdef __cplusplus
 extern "C" {
#endif

void get_samples_map(int *result);
int get_channels_map(channels_map_t *map, uint64_t channel_filter);
int load_dekrispator_patch(int patch_no, dekrispator_patch_t *patch);
int load_dekrispator_sequence(int id, uint8_t *notes, int *tempo);
void store_dekrispator_patch_nvs(dekrispator_patch_t *patch);
void load_dekrispator_patch_nvs(dekrispator_patch_t *patch);

int load_clouds_patch(int patch, float *params, int expected_params);

int load_song_or_melody(int id, const char *what, char *buffer);

int get_remapped_channels(int *remap, int total_channels);
void move_channel_to_front(int channel_position);

void load_binaural_profile(binaural_profile_t *profile, int binaural_id);
void reload_binaural_profile(binaural_profile_t *profile);

void load_isochronic_def(isochronic_def *def, char *block_name);

int get_voice_menu_items(voice_menu_t *items);

void load_settings(settings_t *settings, const char* block_name);
void load_persistent_settings(persistent_settings_t *settings);
void store_persistent_settings(persistent_settings_t *settings);

void load_all_settings();

void persistent_settings_store_eq();
void persistent_settings_store_tempo();
void persistent_settings_store_tuning();

void factory_reset();

#ifdef __cplusplus
}
#endif

#endif /* GLO_CONFIG_H_ */
