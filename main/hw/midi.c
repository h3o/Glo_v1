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
#include "notes.h"
#include "gpio.h"
#include "leds.h"
#include "ui.h"

#include <math.h>
#include <string.h>

uint8_t current_MIDI_notes[MIDI_OUT_POLYPHONY] = {0,0,0};
char midi_msg_on[3] = {MIDI_MSG_NOTE_ON,0x29,0x64};
char midi_msg_off[3] = {MIDI_MSG_NOTE_OFF,0x29,0x7f};

void send_MIDI_notes(uint8_t *notes, int current_chord, int n_notes)
{
	for(int i=0;i<n_notes;i++)
	{
		if (current_MIDI_notes[i])
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

uint8_t MIDI_last_chord[4] = {0,0,0,0}; //persistently stores last 4 pressed keys, even after released if SUST polyphony mode selected
uint8_t MIDI_last_melody_note; //stores the last pressed key, until released, or another pressed
uint8_t MIDI_last_melody_note_velocity; //stores the velocity of last pressed key
uint8_t MIDI_keys_pressed = 0, MIDI_note_on = 0, MIDI_notes_updated = 0, MIDI_controllers_updated = 0, /*MIDI_controllers_active = 0,*/ MIDI_controllers_active_PB = 0, MIDI_controllers_active_CC = 0;
uint8_t MIDI_ctrl[MIDI_WHEEL_CONTROLLERS] = {0,64}; //default for pitch bend controller is 64

#define MIDI_CLOCK_DIVIDER	24 //MIDI Timing Clock (MIDI Sync) byte 0xF8 is sent 24 times per quarter note

void process_receive_MIDI(void *pvParameters)
{
	printf("process_receive_MIDI(): task running on core ID=%d\n",xPortGetCoreID());

	// Read data from UART.
	//const int uart_num = UART_NUM_2;
	#define MIDI_IN_BUFFER 32
	uint8_t data[MIDI_IN_BUFFER];
	uint16_t midi_clock = 0;
	int length = 0;
	int i;

	int parsing_note_event = MIDI_PARSING_NONE;

	while(1)
	{
		ESP_ERROR_CHECK(uart_get_buffered_data_len(MIDI_UART, (size_t*)&length));
		if (length)
		{
			//printf("UART[%d] received %d bytes: ", MIDI_UART, length);
			length = uart_read_bytes(MIDI_UART, data, length, MIDI_IN_BUFFER);

			if (length==1 && data[0]==MIDI_MSG_TIMING_CLOCK)
			{
				midi_clock++;
			}
			else
			{
				//printf("UART[%d] read %d bytes: ", MIDI_UART, length);
				//for(i=0;i<length;i++)
				//{
					//printf("%02x ",data[i]);
				//}
				//printf("\n");

				//process received bytes
				int r_ptr = -1;

				while(r_ptr < length-1)
				{
					r_ptr++;

					if (parsing_note_event==MIDI_PARSING_NONE)
					{
						if (data[r_ptr]==MIDI_MSG_TIMING_CLOCK)
						{
							midi_clock++;
							continue; //skip the MIDI clock
						}
						else if (data[r_ptr]==MIDI_MSG_NOTE_ON)
						{
							parsing_note_event = MIDI_PARSING_PRESSED;
						}
						else if (data[r_ptr]==MIDI_MSG_NOTE_OFF)
						{
							parsing_note_event = MIDI_PARSING_RELEASED;
						}
						else if (data[r_ptr]==MIDI_MSG_CONT_CTRL)
						{
							parsing_note_event = MIDI_PARSING_CONT_CTRL;
						}
						else if (data[r_ptr]==MIDI_MSG_PITCH_BEND)
						{
							parsing_note_event = MIDI_PARSING_PITCH_BEND;
						}
					}
					else if (parsing_note_event==MIDI_PARSING_PRESSED) //note on
					{
						MIDI_keys_pressed++;
						MIDI_last_melody_note = data[r_ptr];

						if (midi_polyphony==MIDI_POLYPHONY_SUST_CHORD
						|| midi_polyphony==MIDI_POLYPHONY_HOLD_CHORD
						|| (midi_polyphony==MIDI_POLYPHONY_HOLD_COMBINED && MIDI_keys_pressed>1))
						{
							MIDI_last_chord[3] = MIDI_last_chord[2];
							MIDI_last_chord[2] = MIDI_last_chord[1];
							MIDI_last_chord[1] = MIDI_last_chord[0];
							MIDI_last_chord[0] = MIDI_last_melody_note;
						}
						else if (midi_polyphony==MIDI_POLYPHONY_SUST_SINGLE_NOTE || midi_polyphony==MIDI_POLYPHONY_HOLD_SINGLE_NOTE)
						{
							MIDI_last_chord[3] = MIDI_last_melody_note;
							MIDI_last_chord[2] = MIDI_last_melody_note;
							MIDI_last_chord[1] = MIDI_last_melody_note;
							MIDI_last_chord[0] = MIDI_last_melody_note;
						}
						else if (midi_polyphony==MIDI_POLYPHONY_SUST_OCTAVE_UP_DOWN
							 || midi_polyphony==MIDI_POLYPHONY_HOLD_OCTAVE_UP_DOWN
							 || (midi_polyphony==MIDI_POLYPHONY_HOLD_COMBINED && MIDI_keys_pressed==1))
						{
							MIDI_last_chord[3] = MIDI_last_melody_note;
							MIDI_last_chord[2] = MIDI_last_melody_note-12;
							MIDI_last_chord[1] = MIDI_last_melody_note+12;
							MIDI_last_chord[0] = MIDI_last_melody_note;
						}

						parsing_note_event = MIDI_PARSING_VELOCITY;
						MIDI_notes_updated = 1;
						MIDI_note_on = 1;
					}
					else if (parsing_note_event==MIDI_PARSING_VELOCITY) //velocity
					{
						MIDI_last_melody_note_velocity = data[r_ptr]; //update the velocity information
						parsing_note_event = MIDI_PARSING_NONE; //all bytes parsed
					}
					else if (parsing_note_event==MIDI_PARSING_RELEASED) //note off
					{
						if (midi_polyphony==MIDI_POLYPHONY_HOLD_SINGLE_NOTE
						|| midi_polyphony==MIDI_POLYPHONY_HOLD_OCTAVE_UP_DOWN
						|| (midi_polyphony==MIDI_POLYPHONY_HOLD_COMBINED && MIDI_keys_pressed==1))
						{
							MIDI_last_chord[3] = 0;
							MIDI_last_chord[2] = 0;
							MIDI_last_chord[1] = 0;
							MIDI_last_chord[0] = 0;
							MIDI_notes_updated = 1;
						}
						else if (midi_polyphony==MIDI_POLYPHONY_HOLD_CHORD
							 || (midi_polyphony==MIDI_POLYPHONY_HOLD_COMBINED && MIDI_keys_pressed>1))
						{
							//find out where in the chord the released note was, then clear it
							if (MIDI_last_chord[0]==data[r_ptr])
							{
								MIDI_last_chord[0] = MIDI_last_chord[1];
								MIDI_last_chord[1] = MIDI_last_chord[2];
								MIDI_last_chord[2] = MIDI_last_chord[3];
								MIDI_last_chord[3] = 0;
							}
							else if (MIDI_last_chord[1]==data[r_ptr])
							{
								MIDI_last_chord[1] = MIDI_last_chord[2];
								MIDI_last_chord[2] = MIDI_last_chord[3];
								MIDI_last_chord[3] = 0;
							}
							else if (MIDI_last_chord[2]==data[r_ptr])
							{
								MIDI_last_chord[2] = MIDI_last_chord[3];
								MIDI_last_chord[3] = 0;
							}
							else if (MIDI_last_chord[3]==data[r_ptr])
							{
								MIDI_last_chord[3] = 0;
							}

							if(midi_polyphony==MIDI_POLYPHONY_HOLD_COMBINED && MIDI_keys_pressed==2)
							{
								MIDI_last_chord[3] = MIDI_last_melody_note;
								MIDI_last_chord[2] = MIDI_last_melody_note-12;
								MIDI_last_chord[1] = MIDI_last_melody_note+12;
							}

							MIDI_notes_updated = 1;
						}
						else if ((midi_polyphony==MIDI_POLYPHONY_SUST_OCTAVE_UP_DOWN
							   || midi_polyphony==MIDI_POLYPHONY_HOLD_OCTAVE_UP_DOWN) && MIDI_keys_pressed==2)
						{
							MIDI_last_chord[3] = MIDI_last_melody_note;
							MIDI_last_chord[2] = MIDI_last_melody_note-12;
							MIDI_last_chord[1] = MIDI_last_melody_note+12;
						}

						MIDI_keys_pressed--;
						if (!MIDI_keys_pressed) //clear melody note only if all keys released, otherwise legato won't work, neither will work filling up & down octaves at release of all but the last note
						{
							MIDI_last_melody_note = 0;
						}
						parsing_note_event = MIDI_PARSING_VELOCITY;
					}
					else if (parsing_note_event==MIDI_PARSING_CONT_CTRL)
					{
						parsing_note_event = data[r_ptr]==1?MIDI_PARSING_CONT_CTRL_N:MIDI_PARSING_IGNORE;
					}
					else if (parsing_note_event==MIDI_PARSING_PITCH_BEND)
					{
						parsing_note_event = MIDI_PARSING_PITCH_BEND_N;
					}
					else if (parsing_note_event==MIDI_PARSING_CONT_CTRL_N)
					{
						MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC] = data[r_ptr];
						MIDI_controllers_updated = MIDI_WHEEL_CONTROLLER_CC_UPDATED;
						MIDI_controllers_active_CC = 1;
						parsing_note_event = MIDI_PARSING_NONE;
					}
					else if (parsing_note_event==MIDI_PARSING_PITCH_BEND_N)
					{
						MIDI_ctrl[MIDI_WHEEL_CONTROLLER_PB] = data[r_ptr];
						MIDI_controllers_updated = MIDI_WHEEL_CONTROLLER_PB_UPDATED;
						MIDI_controllers_active_PB = 1;
						parsing_note_event = MIDI_PARSING_NONE;
					}
					else if (parsing_note_event==MIDI_PARSING_IGNORE)
					{
						parsing_note_event = MIDI_PARSING_NONE;
					}
				}

				if (parsing_note_event)
				{
					printf("MIDI parser warning: bytes missing to finish parsing, last state=%d\n", parsing_note_event);
				}

				printf("MIDI parser: keys pressed=%d, chord=%d-%d-%d-%d, cc=%d, pb=%d\n", MIDI_keys_pressed, MIDI_last_chord[0], MIDI_last_chord[1], MIDI_last_chord[2], MIDI_last_chord[3], MIDI_ctrl[0], MIDI_ctrl[1]);
			}

			//derive the clock from MIDI
			if (midi_clock%MIDI_CLOCK_DIVIDER==1)
			{
				select_channel_RDY_idle_blink = 0;
				//LED_SIG_ON;
				LED_RDY_ON;
			}
			else if (midi_clock%MIDI_CLOCK_DIVIDER==1+MIDI_CLOCK_DIVIDER/2)
			{
				//LED_SIG_OFF;
				LED_RDY_OFF;
			}
			if (midi_clock==MIDI_CLOCK_DIVIDER*8)
			{
				midi_clock = 0;
			}

		}
		vTaskDelay(10);
	}
}

TaskHandle_t receive_MIDI_task = NULL;

void gecho_start_receive_MIDI()
{
	printf("gecho_start_receive_MIDI(): starting process_receive_MIDI task\n");
	xTaskCreatePinnedToCore((TaskFunction_t)&process_receive_MIDI, "receive_MIDI_task", 2048, NULL, 9, &receive_MIDI_task, 1);
}

void gecho_stop_receive_MIDI()
{
	if (receive_MIDI_task==NULL)
	{
		printf("gecho_stop_receive_MIDI(): task not running\n");
		return;
	}

	printf("gecho_stop_receive_MIDI(): deleting task receive_MIDI_task\n");
	vTaskDelete(receive_MIDI_task);
	receive_MIDI_task = NULL;
	printf("gecho_stop_receive_MIDI(): task deleted\n");

	MIDI_parser_reset();
}

void MIDI_parser_reset()
{
	MIDI_last_chord[0] = 0;
	MIDI_last_chord[1] = 0;
	MIDI_last_chord[2] = 0;
	MIDI_last_chord[3] = 0;
	MIDI_last_melody_note = 0;
	MIDI_last_melody_note_velocity = 0;
	MIDI_keys_pressed = 0;

	MIDI_notes_updated = 0;
	MIDI_controllers_updated = 0;
	MIDI_controllers_active_PB = 0;
	MIDI_controllers_active_CC = 0;
	MIDI_ctrl[0] = 0;
	MIDI_ctrl[1] = 64;
}

float MIDI_note_to_freq(int8_t note)
{
	if (!note)
	{
		return 0;
	}
	return NOTE_FREQ_A4 * pow(HALFTONE_STEP_COEF, note - 33 + persistent_settings.TRANSPOSE);
}

void MIDI_to_LED(int note, int on_off)
{
	#ifdef BOARD_GECHO
	if (note)
	{
		//translate from note to LED number (1=c,2=c#,3=d,...,11=a#,12=b)
		note = (note + persistent_settings.TRANSPOSE + 120)%12;
		KEY_LED_on_off(note+1, on_off);
	}
	#endif
}

int MIDI_to_note(int8_t midi_note, char *buf)
{
	const char *notes[12] = {"c","c#","d","d#","e","f","f#","g","g#","a","a#","b"};

	if (midi_note)
	{
		//translate from MIDI note number to note name
		sprintf(buf,"%s%d", notes[midi_note%12],midi_note/12-1);

		return strlen(buf);
	}
	return 0;
}
