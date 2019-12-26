/*
 * midi.h
 *
 *  Created on: Jul 7, 2019
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

#ifndef SYNC_H_
#define SYNC_H_

#ifdef __cplusplus
extern "C" {
#endif

void gecho_start_receive_sync();
void gecho_stop_receive_sync();

void gecho_start_listen_for_tempo();
void gecho_stop_listen_for_tempo();

#ifdef __cplusplus
}
#endif

#endif
