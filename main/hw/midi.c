/*
 * midi.c
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Mar 25, 2019
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

#include <math.h>
#include <string.h>

#include "midi.h"
#include "init.h"
#include "notes.h"
#include "gpio.h"
#include "leds.h"
#include "ui.h"

uint8_t current_MIDI_notes[MIDI_OUT_POLYPHONY] = {0,0,0};
char midi_msg_on[3] = {MIDI_MSG_NOTE_ON,0x29,0x64}; //default velocity = 0x64 (100)
char midi_msg_off[3] = {MIDI_MSG_NOTE_OFF,0x29,0x7f};

void send_MIDI_notes(uint8_t *notes, int current_chord, int n_notes)
{
	for(int i=0;i<n_notes;i++)
	{
		if (current_MIDI_notes[i])
		{
			//send note off
			midi_msg_off[1] = current_MIDI_notes[i];

			/*int length =*/ uart_write_bytes(MIDI_UART, midi_msg_off, 3);
			//printf("UART[%d] sent MIDI note OFF message, %d bytes: %02x %02x %02x\n",MIDI_UART, length, midi_msg_off[0], midi_msg_off[1], midi_msg_off[2]);
		}
	}
	for(int i=0;i<n_notes;i++)
	{
		//send note on
		current_MIDI_notes[i] = notes[3*current_chord+i];
		midi_msg_on[1] = current_MIDI_notes[i];

		/*int length =*/ uart_write_bytes(MIDI_UART, midi_msg_on, 3);
		//printf("UART[%d] sent MIDI note ON message, %d bytes: %02x %02x %02x\n",MIDI_UART, length, midi_msg_on[0], midi_msg_on[1], midi_msg_on[2]);
	}
}

uint8_t MIDI_last_chord[4] = {0,0,0,0}; //persistently stores last 4 pressed keys, even after released if SUST polyphony mode selected
uint8_t MIDI_last_melody_note; //stores the last pressed key, until released, or another pressed
uint8_t MIDI_last_melody_note_velocity; //stores the velocity of last pressed key
uint8_t MIDI_last_note_off = 0; //stores the last released key, until another one released
uint8_t MIDI_keys_pressed = 0, MIDI_note_on = 0, MIDI_notes_updated = 0, MIDI_controllers_updated = 0, /*MIDI_controllers_active = 0,*/ MIDI_controllers_active_PB = 0, MIDI_controllers_active_CC = 0;
uint8_t MIDI_ctrl[MIDI_WHEEL_CONTROLLERS] = {0,64}; //default for pitch bend controller is 64
uint8_t MIDI_tempo_event = 0;

#define MIDI_CLOCK_DIVIDER	24 //MIDI Timing Clock (MIDI Sync) byte 0xF8 is sent 24 times per quarter note

void process_receive_MIDI(void *pvParameters)
{
	printf("process_receive_MIDI(): task running on core ID=%d\n",xPortGetCoreID());

	// Read data from UART.
	//const int uart_num = UART_NUM_2;
	#define MIDI_IN_BUFFER 32
	uint8_t data[MIDI_IN_BUFFER];
	uint16_t midi_clock = 0;
	int length = 0;//, i;

	int parsing_note_event = MIDI_PARSING_NONE;

	while(1)
	{
		ESP_ERROR_CHECK(uart_get_buffered_data_len(MIDI_UART, (size_t*)&length));
		if (length)
		{
			//printf("UART[%d] received %d bytes\n", MIDI_UART, length);

			length = uart_read_bytes(MIDI_UART, data, length, MIDI_IN_BUFFER);

			/*
			printf("UART[%d] read %d bytes: ", MIDI_UART, length);
			for(i=0;i<length;i++)
			{
				printf("%02x ",data[i]);
			}
			printf("\n");
			*/

			if (length==1 && data[0]==MIDI_MSG_TIMING_CLOCK)
			{
				midi_clock++;
				//printf("c:%d\n", midi_clock);
			}
			else
			{
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
							//printf("c:%d\n", midi_clock);
							continue; //skip the MIDI clock
						}
						else if ((data[r_ptr]&0xf0)==MIDI_MSG_NOTE_ON)
						{
							parsing_note_event = MIDI_PARSING_PRESSED;
						}
						else if ((data[r_ptr]&0xf0)==MIDI_MSG_NOTE_OFF)
						{
							parsing_note_event = MIDI_PARSING_RELEASED;
						}
						else if ((data[r_ptr]&0xf0)==MIDI_MSG_CONT_CTRL)
						{
							//printf("(data[r_ptr]&0xf0)==MIDI_MSG_CONT_CTRL\n");
							parsing_note_event = MIDI_PARSING_CONT_CTRL;
						}
						else if ((data[r_ptr]&0xf0)==MIDI_MSG_PITCH_BEND)
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

						//look up if this is some of the pre-defined key/pad controls

						for(int i=0;i<midi_ctrl_keys;i++)
						{
							if(midi_ctrl_keys_map[i] == data[r_ptr]+(data[r_ptr-1]<<8))
							{
								//printf("i=%d,midi_ctrl_key_map[i] == data[r_ptr]+(data[r_ptr-1]<<8\n", i);

								midi_ctrl_keys_values[i] = data[r_ptr];
								midi_ctrl_keys_updated[i] = 1;
								midi_ctrl_keys_active = 1;

								//printf("midi_ctrl_key_values[%d] => %d\n", i, midi_ctrl_keys_values[i]);
								break;
							}
						}
					}
					else if (parsing_note_event==MIDI_PARSING_VELOCITY) //velocity
					{
						MIDI_last_melody_note_velocity = data[r_ptr]; //update the velocity information
						parsing_note_event = MIDI_PARSING_NONE; //all bytes parsed
					}
					else if (parsing_note_event==MIDI_PARSING_RELEASED) //note off
					{
						MIDI_last_note_off = data[r_ptr];

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
						parsing_note_event = /*data[r_ptr]==1?*/MIDI_PARSING_CONT_CTRL_N/*:MIDI_PARSING_IGNORE*/;
					}
					else if (parsing_note_event==MIDI_PARSING_PITCH_BEND)
					{
						parsing_note_event = MIDI_PARSING_PITCH_BEND_N;
					}
					else if (parsing_note_event==MIDI_PARSING_CONT_CTRL_N)
					{
						//printf("MIDI_PARSING_CONT_CTRL_N: %02x-%02x-%02x\n", data[r_ptr-2], data[r_ptr-1], data[r_ptr]);
						MIDI_ctrl[MIDI_WHEEL_CONTROLLER_CC] = data[r_ptr];
						MIDI_controllers_updated = MIDI_WHEEL_CONTROLLER_CC_UPDATED;
						MIDI_controllers_active_CC = 1;

						//look up if this is some of the pre-defined cc wheels/knobs

						for(int i=0;i<midi_ctrl_cc;i++)
						{
							/*
							if(midi_ctrl_cc_map[i] == data[r_ptr-2]+(data[r_ptr-1]<<8))
							{
								printf("i=%d,midi_ctrl_cc_map[i] == data[r_ptr-2]+data[r_ptr-1]<<8\n", i);
							}
							*/
							if(midi_ctrl_cc_msg_size[i] == 3 && midi_ctrl_cc_map[i] == data[r_ptr-1]+(data[r_ptr-2]<<8))
							{
								//printf("i=%d,midi_ctrl_cc_map[i] == data[r_ptr-1]+data[r_ptr-2]<<8\n", i);

								midi_ctrl_cc_values[i] = data[r_ptr];
								midi_ctrl_cc_updated[i] = 1;
								midi_ctrl_cc_active = 1;

								//printf("midi_ctrl_cc_values[%d] => %d (msg_size=3)\n", i, midi_ctrl_cc_values[i]);
								break;
							}
							if(midi_ctrl_cc_msg_size[i] == 6 && midi_ctrl_cc_map[i] == data[r_ptr-1]+(data[r_ptr-2]<<8))
							{
								if(data[r_ptr]!=0x40)
								{
									//printf("i=%d,midi_ctrl_cc_map[i] == data[r_ptr-1]+data[r_ptr-2]<<8\n", i);

									int new_value = midi_ctrl_cc_values[i] + data[r_ptr] - 0x40;
									if(new_value<0)
									{
										new_value = 0;
									}
									if(new_value>127)
									{
										new_value = 127;
									}
									if(midi_ctrl_cc_values[i] != new_value)
									{
										midi_ctrl_cc_values[i] = new_value;
										midi_ctrl_cc_updated[i] = 1;
										midi_ctrl_cc_active = 1;
										//printf("midi_ctrl_cc_values[%d] => %d (msg_size=6)\n", i, midi_ctrl_cc_values[i]);
									}
								}
								break;
							}
						}

						parsing_note_event = MIDI_PARSING_NONE;
					}
					else if (parsing_note_event==MIDI_PARSING_PITCH_BEND_N)
					{
						//printf("MIDI_PARSING_PITCH_BEND_N: %02x-%02x-%02x\n", data[r_ptr-2], data[r_ptr-1], data[r_ptr]);
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

				//printf("MIDI parser: keys pressed=%d, chord=%d-%d-%d-%d, cc=%d, pb=%d\n", MIDI_keys_pressed, MIDI_last_chord[0], MIDI_last_chord[1], MIDI_last_chord[2], MIDI_last_chord[3], MIDI_ctrl[0], MIDI_ctrl[1]);
			}

			//derive the clock from MIDI
			if (midi_clock%MIDI_CLOCK_DIVIDER==1)
			{
				select_channel_RDY_idle_blink = 0;
				//LED_SIG_ON;
				LED_RDY_ON;

				MIDI_tempo_event = 1;
			}
			else if (midi_clock%MIDI_CLOCK_DIVIDER==1+MIDI_CLOCK_DIVIDER/2)
			{
				//LED_SIG_OFF;
				LED_RDY_OFF;

				MIDI_tempo_event = 2;
			}
			if (midi_clock==MIDI_CLOCK_DIVIDER*8)
			{
				midi_clock = 0;
			}

		}
		vTaskDelay(1);
	}
}

TaskHandle_t receive_MIDI_task = NULL;

void gecho_start_receive_MIDI()
{
	printf("gecho_start_receive_MIDI(): starting process_receive_MIDI task\n");
	xTaskCreatePinnedToCore((TaskFunction_t)&process_receive_MIDI, "receive_MIDI_task", 2048, NULL, PRIORITY_RECEIVE_MIDI_TASK, &receive_MIDI_task, CPU_CORE_RECEIVE_MIDI_TASK);
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
	uart_flush(MIDI_UART);

	MIDI_last_chord[0] = 0;
	MIDI_last_chord[1] = 0;
	MIDI_last_chord[2] = 0;
	MIDI_last_chord[3] = 0;
	MIDI_last_melody_note = 0;
	MIDI_last_melody_note_velocity = 0;
	MIDI_last_note_off = 0;
	MIDI_keys_pressed = 0;
	MIDI_note_on = 0;
	MIDI_notes_updated = 0;
	MIDI_controllers_updated = 0;
	MIDI_controllers_active_PB = 0;
	MIDI_controllers_active_CC = 0;
	MIDI_ctrl[0] = 0;
	MIDI_ctrl[1] = 64;
	MIDI_tempo_event = 0;

	midi_ctrl_cc_active = 0;
	midi_ctrl_keys_active = 0;
}

float MIDI_note_to_freq(int8_t note)
{
	if (!note)
	{
		return 0;
	}
	return NOTE_FREQ_A4 * pow(HALFTONE_STEP_COEF, note - 69 + persistent_settings.TRANSPOSE); //A4 is MIDI note #69
}

float MIDI_note_to_freq_ft(float note)
{
	return NOTE_FREQ_A4 * pow(HALFTONE_STEP_COEF, note - 69.0f + (float)persistent_settings.TRANSPOSE); //A4 is MIDI note #69
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

int MIDI_to_note(int8_t midi_note, char *buf) //note and octave
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

int MIDI_to_note_only(int8_t midi_note, char *buf) //only note, without octave
{
	const char *notes[12] = {"c","c#","d","d#","e","f","f#","g","g#","a","a#","b"};

	if (midi_note)
	{
		//translate from MIDI note number to note name
		sprintf(buf,"%s", notes[midi_note%12]);

		return strlen(buf);
	}
	return 0;
}

uint8_t midi_ctrl_cc_msg_size[MIDI_CTRL_MAX_CC];
int midi_ctrl_keys = 0, midi_ctrl_cc = 0;
uint16_t midi_ctrl_keys_map[MIDI_CTRL_MAX_KEYS], midi_ctrl_cc_map[MIDI_CTRL_MAX_CC];

uint8_t midi_ctrl_cc_values[MIDI_CTRL_MAX_CC], midi_ctrl_keys_values[MIDI_CTRL_MAX_KEYS];
int midi_ctrl_cc_updated[MIDI_CTRL_MAX_CC], midi_ctrl_keys_updated[MIDI_CTRL_MAX_KEYS];
int midi_ctrl_cc_active = 0, midi_ctrl_keys_active = 0;

void setup_MIDI_controls()
{
	gecho_deinit_MIDI(); //in case driver already running with a different configuration
	gecho_init_MIDI(MIDI_UART, 0); //no midi out

	// Read data from UART.
	//const int uart_num = MIDI_UART;
	uint8_t data[128], infinite_encoder_cc_display = 0, cc_found_msg_size[MIDI_CTRL_MAX_CC];
	uint16_t keys_found_map[MIDI_CTRL_MAX_KEYS], cc_found_map[MIDI_CTRL_MAX_CC];

	memset(cc_found_msg_size,0,sizeof(cc_found_msg_size));
	memset(keys_found_map,0,sizeof(keys_found_map));
	memset(cc_found_map,0,sizeof(cc_found_map));
	printf("setup_MIDI_controls(): sizeof cc_found_msg_size / keys_found_map / cc_found_map = %d/%d/%d\n", sizeof(cc_found_msg_size), sizeof(keys_found_map), sizeof(cc_found_map));

	int length = 0;
	int i, result;
	channel_running = 1;

	int n_keys_found = 0, n_cc_found = 0, already_known, active_cc_blink = -1, active_cc_blink0 = -1, loops = 0, activity_timeout = -1;

	#define CC_LOOP_DELAY			1		//1 millisecond
	#define CC_ACTIVITY_TIMEOUT		1000	//1 second - multiples of CC_LOOP_DELAY
	#define CC_ACTIVITY_BLINK_DIV	100		//blink speed as a divider

	LEDs_all_OFF();

	int exit_reason = 0;
	#define MIDI_CFG_EXIT_REASON_COMPLETE	1
	#define MIDI_CFG_EXIT_REASON_SET		2
	#define MIDI_CFG_EXIT_REASON_RST		3

	while(1)
	{
		ESP_ERROR_CHECK(uart_get_buffered_data_len(MIDI_UART, (size_t*)&length));
		if(length)
		{
			printf("UART[%d] received %d bytes: ", MIDI_UART, length);
			length = uart_read_bytes(MIDI_UART, data, length, 100);
			//printf("UART[%d] read %d bytes: ", MIDI_UART, length);
			for(i=0;i<length;i++)
			{
				printf("%02x ",data[i]);
			}
			printf("\n");

			if(length==3)
			{
				if((data[0]&0xf0)==0x90) //note on
				{
					activity_timeout = CC_ACTIVITY_TIMEOUT;

					//look up if already known
					already_known = 0;
					for(i=0;i<n_keys_found;i++)
					{
						if(keys_found_map[i]==(uint16_t)data[1]+((uint16_t)data[0]<<8))
						{
							already_known = 1;
						}
					}

					if(!already_known && n_keys_found<MIDI_CTRL_MAX_KEYS)
					{
						LED_W8_set(n_keys_found, 1);
						keys_found_map[n_keys_found] = (uint16_t)data[1]+((uint16_t)data[0]<<8);
						printf("setup_MIDI_controls(): new NOTE_ON (#%d): %02x-%02x (%04x)\n", n_keys_found, data[0], data[1], keys_found_map[n_keys_found]);
						n_keys_found++;
					}

					LED_B5_all_ON();
				}

				if((data[0]&0xf0)==0x80/* && n_keys_found<MIDI_CTRL_MAX_KEYS*/) //note off
				{
					activity_timeout = CC_ACTIVITY_TIMEOUT;

					LED_B5_all_OFF();
				}

				if((data[0]&0xf0)==0xb0) //continuous controller
				{
					activity_timeout = CC_ACTIVITY_TIMEOUT;

					//look up if already known
					already_known = 0;
					for(i=0;i<n_cc_found;i++)
					{
						if(cc_found_map[i]==(uint16_t)data[1]+((uint16_t)data[0]<<8))
						{
							already_known = 1;
							active_cc_blink0 = active_cc_blink;
							active_cc_blink = i;

							if(cc_found_msg_size[i]!=length)
							{
								printf("this control was previously detected as sending %d byte messages, now received %d byte msg!\n", cc_found_msg_size[i], length);
								if(cc_found_msg_size[i]>length)
								{
									printf("updating cc_found_msg_size: %d => %d\n", cc_found_msg_size[i], length);
									cc_found_msg_size[i] = length;
								}
							}
						}
					}

					if(!already_known && n_cc_found<MIDI_CTRL_MAX_CC)
					{
						//if(n_cc_found<8) LED_R8_set(n_cc_found, 1); else LED_O4_set(n_cc_found-8, 1);
						active_cc_blink0 = active_cc_blink;
						active_cc_blink = n_cc_found;
						cc_found_map[n_cc_found] = (uint16_t)data[1]+((uint16_t)data[0]<<8);
						cc_found_msg_size[n_cc_found] = length;
						printf("setup_MIDI_controls(): new CC (#%d) with %d byte msg: %02x-%02x (%04x)\n", n_cc_found, cc_found_msg_size[n_cc_found], data[0], data[1], cc_found_map[n_cc_found]);
						n_cc_found++;
					}
					else if(!already_known && n_cc_found==MIDI_CTRL_MAX_CC)
					{
						active_cc_blink0 = active_cc_blink;
						active_cc_blink = -1;
					}

					LED_B5_set_byte(data[2]>>2);
				}
			}

			if(length==6)
			{
				if(((data[0]&0xf0)==0xb0) && ((data[3]&0xf0)==0xb0) && data[1]==data[4]) //continuous controller with infinite encoders (worlde mini)
				{
					activity_timeout = CC_ACTIVITY_TIMEOUT;

					//look up if already known
					already_known = 0;
					for(i=0;i<n_cc_found;i++)
					{
						if(cc_found_map[i]==(uint16_t)data[1]+((uint16_t)data[0]<<8))
						{
							already_known = 1;
							active_cc_blink0 = active_cc_blink;
							active_cc_blink = i;
						}
					}

					if(!already_known && n_cc_found<MIDI_CTRL_MAX_CC)
					{
						//if(n_cc_found<8) LED_R8_set(n_cc_found, 1); else LED_O4_set(n_cc_found-8, 1);
						active_cc_blink0 = active_cc_blink;
						active_cc_blink = n_cc_found;
						cc_found_map[n_cc_found] = (uint16_t)data[1]+((uint16_t)data[0]<<8);
						cc_found_msg_size[n_cc_found] = length;
						printf("setup_MIDI_controls(): new CC (#%d) with %d byte msg: %02x-%02x (%04x)\n", n_cc_found, cc_found_msg_size[n_cc_found], data[0], data[1], cc_found_map[n_cc_found]);
						n_cc_found++;
					}
					else if(!already_known && n_cc_found==MIDI_CTRL_MAX_CC)
					{
						active_cc_blink0 = active_cc_blink;
						active_cc_blink = -1;
					}

					infinite_encoder_cc_display += data[5]-data[2];
					LED_B5_set_byte(infinite_encoder_cc_display);
				}
			}
		}

		if(n_keys_found==MIDI_CTRL_MAX_KEYS && n_cc_found==MIDI_CTRL_MAX_CC)
		{
			exit_reason = MIDI_CFG_EXIT_REASON_COMPLETE;
			break;
		}

		if(BUTTON_RST_ON) //cancel and exit the menu
		{
			/*
			while(BUTTON_RST_ON) //wait till the button released
			{
				Delay(10);
			}
			*/
			exit_reason = MIDI_CFG_EXIT_REASON_RST;
			printf("setup_MIDI_controls(): RST pressed, exiting\n");
			break;
		}

		if(BUTTON_SET_ON)
		{
			/*
			while(BUTTON_SET_ON) //wait till the button released
			{
				Delay(10);
			}
			*/
			exit_reason = MIDI_CFG_EXIT_REASON_SET;
			printf("setup_MIDI_controls(): SET pressed, saving the configuration\n");
			break;
		}

		if(active_cc_blink0 != active_cc_blink) //controls changed while some of the LEDs was still blinking
		{
			if(active_cc_blink0<8) LED_R8_set(active_cc_blink0, 1); else LED_O4_set(active_cc_blink0-8, 1);
			active_cc_blink0 = active_cc_blink;
		}

		if(active_cc_blink>-1)
		{
			if(active_cc_blink<8) LED_R8_set(active_cc_blink, (loops/CC_ACTIVITY_BLINK_DIV)%2); else LED_O4_set(active_cc_blink-8, (loops/CC_ACTIVITY_BLINK_DIV)%2);
		}

		Delay(CC_LOOP_DELAY);
		loops++;

		if(activity_timeout>-1) activity_timeout--;
		if(!activity_timeout)
		{
			if(active_cc_blink>-1)
			{
				if(active_cc_blink<8) LED_R8_set(active_cc_blink, 1); else LED_O4_set(active_cc_blink-8, 1);
				active_cc_blink = -1;
				active_cc_blink0 = -1;
			}
		}
	}
	LED_B5_all_OFF();

	printf("exit reason = %d, keys_found_map[%d] = ", exit_reason, n_keys_found);
	for(i=0;i<MIDI_CTRL_MAX_KEYS;i++)
	{
		printf("%02x ",keys_found_map[i]);
	}
	printf("\n");

	printf("cc_found_map[%d] = ", n_cc_found);
	for(i=0;i<MIDI_CTRL_MAX_CC;i++)
	{
		printf("%02x ",cc_found_map[i]);
	}
	printf("\n");

	if(exit_reason != MIDI_CFG_EXIT_REASON_RST)
	{
		//save the configuration to permanent storage
		printf("setup_MIDI_controls(): saving the new configuration...\n");

		midi_ctrl_keys = n_keys_found;
		midi_ctrl_cc = n_cc_found;

		memcpy(midi_ctrl_cc_msg_size, cc_found_msg_size, MIDI_CTRL_MAX_CC*sizeof(uint8_t));	//uint8_t midi_ctrl_cc_msg_size[MIDI_CTRL_MAX_CC]
		memcpy(midi_ctrl_keys_map, keys_found_map, MIDI_CTRL_MAX_KEYS*sizeof(uint16_t));	//uint16_t midi_ctrl_keys_map[MIDI_CTRL_MAX_KEYS]
		memcpy(midi_ctrl_cc_map, cc_found_map, MIDI_CTRL_MAX_CC*sizeof(uint16_t));			//uint16_t midi_ctrl_cc_map[MIDI_CTRL_MAX_CC]

		result = store_midi_controller_configuration(); //returns number of keys successfully written
		printf("setup_MIDI_controls(): store_midi_controller_configuration() returned %d\n", result);

		Delay(500);
	}

	printf("setup_MIDI_controls(): [end] restarting...\n");
	whale_restart();
}

int load_midi_controller_configuration()
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("midi_ctrl", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("load_midi_controller_configuration(): problem with nvs_open(), error = %d\n", res);
		return 0;
	}

	uint8_t val_u8;
	int ret = 0;
	size_t bytes_loaded;

	res = nvs_get_u8(handle, "keys", &val_u8);
	if(res==ESP_OK) //if the key does not exist, do not load anything
	{
		printf("load_midi_controller_configuration(): \"keys\" found: midi_ctrl_keys => %d\n", val_u8);
		midi_ctrl_keys = val_u8;
		ret += val_u8;
		ret<<=8;
	}

	res = nvs_get_u8(handle, "ccs", &val_u8);
	if(res==ESP_OK) //if the key does not exist, do not load anything
	{
		printf("load_midi_controller_configuration(): \"ccs\" found: midi_ctrl_cc => %d\n", val_u8);
		midi_ctrl_cc = val_u8;
		ret += val_u8;
	}

	bytes_loaded = MIDI_CTRL_MAX_CC*sizeof(uint8_t);//sizeof(midi_ctrl_cc_msg_size); //there must be a non-zero value for nvs_get_blob() to actually load data
	res = nvs_get_blob(handle, "cc_size", midi_ctrl_cc_msg_size, &bytes_loaded);
	if(res==ESP_OK)
	{
		printf("load_midi_controller_configuration(): \"cc_size\" found, %d/%d bytes loaded\n", bytes_loaded, MIDI_CTRL_MAX_CC*sizeof(uint8_t));//sizeof(midi_ctrl_cc_msg_size));
	}
	else
	{
		printf("load_midi_controller_configuration(): \"cc_size\" load returned error = %d\n", res);
	}

	bytes_loaded = MIDI_CTRL_MAX_KEYS*sizeof(uint16_t); //sizeof(midi_ctrl_keys_map); //there must be a non-zero value for nvs_get_blob() to actually load data
	res = nvs_get_blob(handle, "k_map", midi_ctrl_keys_map, &bytes_loaded);
	if(res==ESP_OK)
	{
		printf("load_midi_controller_configuration(): \"k_map\" found, %d/%d bytes loaded\n", bytes_loaded, MIDI_CTRL_MAX_KEYS*sizeof(uint16_t));//sizeof(midi_ctrl_keys_map));
	}
	else
	{
		printf("load_midi_controller_configuration(): \"k_map\" load returned error = %d\n", res);
	}

	bytes_loaded = MIDI_CTRL_MAX_CC*sizeof(uint16_t);//sizeof(midi_ctrl_cc_map); //there must be a non-zero value for nvs_get_blob() to actually load data
	res = nvs_get_blob(handle, "cc_map", midi_ctrl_cc_map, &bytes_loaded);
	if(res==ESP_OK)
	{
		printf("load_midi_controller_configuration(): \"cc_map\" found, %d/%d bytes loaded\n", bytes_loaded, MIDI_CTRL_MAX_CC*sizeof(uint16_t));//sizeof(midi_ctrl_cc_map));
	}
	else
	{
		printf("load_midi_controller_configuration(): \"cc_map\" load returned error = %d\n", res);
	}

	nvs_close(handle);
	return ret;
}

int store_midi_controller_configuration() //returns number of keys successfully written
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("midi_ctrl", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("store_midi_controller_configuration(): problem with nvs_open(), error = %d\n", res);
		return 0;
	}

	int ret = 0;

	res = nvs_set_u8(handle, "keys", (uint8_t)midi_ctrl_keys);
	if(res==ESP_OK)
	{
		printf("store_midi_controller_configuration(): \"keys\" stored, midi_ctrl_keys => %d\n", (uint8_t)midi_ctrl_keys);
		ret++;
	}

	res = nvs_set_u8(handle, "ccs", (uint8_t)midi_ctrl_cc);
	if(res==ESP_OK)
	{
		printf("store_midi_controller_configuration(): \"ccs\" stored, midi_ctrl_cc => %d\n", (uint8_t)midi_ctrl_cc);
		ret++;
	}

	res = nvs_set_blob(handle, "cc_size", midi_ctrl_cc_msg_size, sizeof(midi_ctrl_cc_msg_size));
	if(res==ESP_OK)
	{
		printf("store_midi_controller_configuration(): \"cc_size\" stored, %d bytes total\n", sizeof(midi_ctrl_cc_msg_size));
		ret++;
	}

	res = nvs_set_blob(handle, "k_map", midi_ctrl_keys_map, sizeof(midi_ctrl_keys_map));
	if(res==ESP_OK)
	{
		printf("store_midi_controller_configuration(): \"k_map\" stored, %d bytes total\n", sizeof(midi_ctrl_keys_map));
		ret++;
	}

	res = nvs_set_blob(handle, "cc_map", midi_ctrl_cc_map, sizeof(midi_ctrl_cc_map));
	if(res==ESP_OK)
	{
		printf("store_midi_controller_configuration(): \"cc_map\" stored, %d bytes total\n", sizeof(midi_ctrl_cc_map));
		ret++;
	}

	res = nvs_commit(handle);
	if(res!=ESP_OK)
	{
		printf("store_midi_controller_configuration(): problem with nvs_commit(), error = %d\n", res);
		return 0;
	}

	nvs_close(handle);
	return ret;
}

void reset_MIDI_controls()
{
	esp_err_t res;
	nvs_handle handle;

	res = nvs_open("midi_ctrl", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("reset_MIDI_controls(): problem with nvs_open(), error = %d\n", res);
		return;
	}
	res = nvs_erase_all(handle);
	if(res!=ESP_OK)
	{
		printf("reset_MIDI_controls(): problem with nvs_erase_all(), error = %d\n", res);
		return;
	}
	res = nvs_commit(handle);
	if(res!=ESP_OK)
	{
		printf("reset_MIDI_controls(): problem with nvs_commit(), error = %d\n", res);
		return;
	}
	nvs_close(handle);

	Delay(500);

	printf("setup_MIDI_controls(): [end] restarting...\n");
	whale_restart();
}
