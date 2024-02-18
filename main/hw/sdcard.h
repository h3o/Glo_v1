/*
 * sdcard.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 16 Feb 2019
 *      Author: mario
 *
 *	  Based on: SD card and FAT filesystem example by Espressif
 *      Source: https://github.com/espressif/esp-idf/blob/master/examples/storage/sd_card/sdspi/main/sd_card_example_main.c
 *
 *  This file is part of the Gecho Loopsynth & Glo Firmware Development Framework.
 *  It can be used within the terms of GNU GPLv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/
 *
 */

#ifndef SDCARD_H_
#define SDCARD_H_

#include <stdint.h>

#define SD_WRITE_ENABLED

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "sys/dirent.h"

extern sdmmc_card_t *card;
extern uint8_t sd_card_ready, sd_card_present;
extern int is_recording;

#define SD_CARD_PRESENT_NONE		0
#define SD_CARD_PRESENT_READONLY	1
#define SD_CARD_PRESENT_WRITEABLE	2
#define SD_CARD_PRESENT_UNKNOWN		3

#define FW_UPDATE_LINK_FILE "/sdcard/update.htm"
#define FW_ACTIVATE_LINK_FILE "/sdcard/activate.htm"
#define FW_UPDATE_LINK_HTTPS "<a href=\"https://gechologic.com/v2/"
#define FW_UPDATE_LINK_EXT "/latest/\">Get the latest firmware</a>"

#define SD_RECORDING_LAST	-1

#define SD_WRITE_BUFFER		8192
//#define SD_WRITE_BUFFER		4096
extern uint32_t *sd_write_buf;

#define SD_PLAYBACK_BUFFER			4096
#define SD_PLAYBACK_BUFFER_SEGMENT	1024
extern uint32_t *SD_play_buf;

extern const char *WAV_HEADER;
#define WAV_HEADER_SIZE 44

#define SD_WRITE_BUFFER_PERMANENT

//Fs=50781.250 -> 203125 b/sec -> 24.795 buffers/sec
//1 buffer = 0.04033 sec, 1 half buffer = 0.02016 = 20ms
//delay between checking for half-buffer ready flag:
#define SD_RECORDING_RESOLUTION		10 //this works well
//#define SD_RECORDING_RESOLUTION	2 //this worked

//delay between checking for half-buffer while playing the sample back:
#define SD_SAMPLE_PLAY_TIMING		10

#define REC_SUBDIR "rec"
#define LOOPER_SUBDIR "looper"

#define SD_CARD_PLAY_RESULT_UNDEFINED		-2
#define SD_CARD_PLAY_RESULT_ERROR			-1
#define SD_CARD_PLAY_RESULT_EXIT			0
#define SD_CARD_PLAY_RESULT_NEXT_FILE		1
#define SD_CARD_PLAY_RESULT_PREVIOUS_FILE	2

#ifdef __cplusplus
extern "C" {
#endif

int sd_card_init(int max_files, int speed);
void sd_card_check(const char *htm_filename, int recalculate_hash);
void sd_card_info();
void sd_card_speed_test();
void sd_card_file_list(char *subdir, int recursive, char **list, int *total_files, int add_dirs_to_list);
void sd_card_download();

void sd_recording(void *pvParameters);
void sd_write_sample(uint32_t *sample32);
void sd_write_samples(uint16_t *buffer, int n_bytes);

uint32_t sd_get_last_rec_file_number();
uint32_t sd_get_next_rec_file_number();
uint32_t sd_get_current_rec_file_number();
void sd_set_current_rec_file_number(int next_fn);

void start_stop_recording();
void use_recorded_sample();
int load_recorded_sample();
void unload_recorded_sample();
void load_next_sd_sample_sector();
int get_recording_filename(char *rec_filename, int recording, uint32_t *last_no);
void channel_SD_playback(int recording, int slicer);
int sd_card_open_file(char *file_path);
int sd_card_play(char *file_path, int sampling_rate);
int sd_card_slicer(char *file_path, int sampling_rate);

int sd_card_write_file(char *filename, char *buffer, int length);

void create_dir_if_not_exists(const char *dir);
uint32_t get_file_size(const char *filename);

void verify_sd_counter_overlap();

#ifdef __cplusplus
}
#endif

#endif
