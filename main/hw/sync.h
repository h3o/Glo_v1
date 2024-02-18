/*
 * midi.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Jul 7, 2019
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

#ifndef SYNC_H_
#define SYNC_H_

#include "hw/init.h"

extern uint8_t tempo_detect_success, start_channel_by_sync_pulse;

#ifdef __cplusplus
extern "C" {
#endif

void gecho_start_receive_sync();
void gecho_stop_receive_sync();

void gecho_start_listen_for_tempo();
void gecho_stop_listen_for_tempo();
void gecho_delete_listen_for_tempo_task();

#ifdef __cplusplus
}
#endif

#endif
