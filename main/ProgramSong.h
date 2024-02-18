/*
 * ProgramSong.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Nov 08, 2019
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

#ifndef PROGRAM_SONG_H_
#define PROGRAM_SONG_H_

#define PROGRAM_SONG_ACTION_NEW		1
#define PROGRAM_SONG_ACTION_LOAD	2
#define PROGRAM_SONG_ACTION_STORE	3
#define PROGRAM_SONG_ACTION_CLEAR	4

void program_song(int action);
int edit_chords_by_buttons();

#endif /* PROGRAM_SONG_H_ */
