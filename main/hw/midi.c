/*
 * midi.c
 *
 *  Created on: Mar 25, 2019
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

#include "midi.h"
#include "init.h"

uint8_t current_MIDI_notes[MIDI_POLYPHONY] = {0,0,0};
char midi_msg_on[3] = {0x90,0x29,0x64};
char midi_msg_off[3] = {0x80,0x29,0x7f};

void send_MIDI_notes(uint8_t *notes, int current_chord, int n_notes)
{
	for(int i=0;i<n_notes;i++)
	{
		if(current_MIDI_notes[i])
		{
			//send note off
			midi_msg_off[1] = current_MIDI_notes[i];

			int length = uart_write_bytes(MIDI_UART, midi_msg_off, 3);
			//printf("UART[%d] sent MIDI note OFF message, %d bytes: %02x %02x %02x\n",MIDI_UART, length, midi_msg_off[0], midi_msg_off[1], midi_msg_off[2]);
		}
	}
	for(int i=0;i<n_notes;i++)
	{
		//send note on
		current_MIDI_notes[i] = notes[3*current_chord+i];
		midi_msg_on[1] = current_MIDI_notes[i];

		int length = uart_write_bytes(MIDI_UART, midi_msg_on, 3);
		//printf("UART[%d] sent MIDI note ON message, %d bytes: %02x %02x %02x\n",MIDI_UART, length, midi_msg_on[0], midi_msg_on[1], midi_msg_on[2]);
	}
}
