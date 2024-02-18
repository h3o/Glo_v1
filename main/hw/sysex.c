/*
 * sysex.c
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Apr 29, 2020
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

#include <string.h>

#include "sysex.h"
#include "midi.h"
#include "sdcard.h"
#include "gpio.h"
#include "leds.h"
#include "ui.h"

int find_file(int file_n, char **list, int list_len)
{
	printf("find_file(): file_n=%d, list_len=%d\n", file_n, list_len);
	char prefix[12];
	sprintf(prefix,"%02d",file_n+1);
	printf("find_file(): looking for prefix %s\n", prefix);

	for(int i=0;i<list_len;i++)
	{
		if(!strncmp(prefix,list[i],2))
		{
			printf("find_file(): found file %s at position %d\n", list[i], i);
			return i;
		}
	}
	printf("find_file(): not found\n");
	return -1;
}

void MIDI_sysex_sender()
{
	char *files_list[100];
	int files_found;
	sd_card_file_list("sysex/out", 0, files_list, &files_found, 0);

	printf("MIDI_sysex_sender(): found %d files\n", files_found);

	for(int i=0;i<files_found;i++)
	{
		printf("%s,",files_list[i]);
	}
	printf("\n--END--\n");

	channel_running = 1;

	int file_selected = 0, file_found;
	#define SYSEX_MAX_FILES	32

	file_found = find_file(file_selected, files_list, files_found);

	int indicators_loop = 0;
	#define INDICATORS_DELAY 		5	//5ms delay per cycle
	#define INDICATORS_BLINK_FAST	50	//250ms period

	gecho_deinit_MIDI(); //in case driver already running with a different configuration
	gecho_init_MIDI(MIDI_UART, 1); //midi out enabled
	Delay(500);

	while(!event_next_channel)
	{
		ui_command = 0;

		#define SYSEX_UI_CMD_NEXT_FILE		1
		#define SYSEX_UI_CMD_PREV_FILE		2
		#define SYSEX_UI_CMD_SEND_FILE		3
		#define SYSEX_UI_CMD_RECV_FILE		4
		#define SYSEX_UI_CMD_CHANGE_SPEED	5

		//map UI commands
		#ifdef BOARD_GECHO
		if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1) { ui_command = SYSEX_UI_CMD_PREV_FILE; btn_event_ext = 0; }
		if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2) { ui_command = SYSEX_UI_CMD_NEXT_FILE; btn_event_ext = 0; }
		if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_SET) { ui_command = SYSEX_UI_CMD_SEND_FILE; btn_event_ext = 0; }
		if(btn_event_ext==BUTTON_EVENT_SET_PLUS+BUTTON_1) { ui_command = SYSEX_UI_CMD_RECV_FILE; btn_event_ext = 0; }
		if(btn_event_ext==BUTTON_EVENT_SET_PLUS+BUTTON_2) { ui_command = SYSEX_UI_CMD_CHANGE_SPEED; btn_event_ext = 0; }
		#endif

		if(ui_command==SYSEX_UI_CMD_NEXT_FILE)
		{
			file_selected++;
			if(file_selected>=SYSEX_MAX_FILES)
			{
				file_selected = 0;
			}
			file_found = find_file(file_selected, files_list, files_found);
		}
		if(ui_command==SYSEX_UI_CMD_PREV_FILE)
		{
			file_selected--;
			if(file_selected<0)
			{
				file_selected = SYSEX_MAX_FILES-1; //files are numbered 0..31, filenames 01..32
			}
			file_found = find_file(file_selected, files_list, files_found);
		}
		if(ui_command==SYSEX_UI_CMD_SEND_FILE)
		{
			if(file_found==-1)
			{
				printf("MIDI_sysex_sender(): slot #%d empty\n", file_selected);
				indicate_error(0x55aa, 8, 80);
			}
			else
			{
				printf("MIDI_sysex_sender(): sending file from slot #%d\n", file_selected);

			    char filepath[50];
			    FILE *f;
			    long int filesize;

			    sprintf(filepath, "/sdcard/sysex/out/%s", files_list[file_found]);
			    printf("MIDI_sysex_sender(): opening file %s\n", filepath);
			    f = fopen(filepath, "rb");
			    if (f == NULL)
			    {
			    	printf("MIDI_sysex_sender(): Failed to open file %s for reading\n", filepath);
			    	indicate_error(0x0001, 10, 100);
			        break;
				}
			    fseek(f,0,SEEK_END);
			    filesize = ftell(f);

			    printf("MIDI_sysex_sender(): file size of %s = %lu\n", filepath, filesize);

			    fseek(f,0,SEEK_SET);

				#define BUFFSIZE	1024*4
				#define BLOCK_SIZE	64
				#define BLOCKS_READ	64

				char *fread_data;
				fread_data = malloc(BUFFSIZE + 1);
				int blocks_read = 0;
				int buff_len, i;

				printf("---BEGIN_BINARY_DUMP---");

				while (!feof(f))
				{
					buff_len = fread(fread_data,BLOCK_SIZE,BLOCKS_READ,f); //read by larger blocks
					//printf("%d blocks / %d bytes read\n",buff_len,buff_len*BLOCK_SIZE);

					/*if (buff_len < 0) //read error
					{
						printf("sd_card_download(): data read error!\n");
					}
					else */
					if (buff_len > 0)
					{
						if(buff_len<BLOCKS_READ)
						{
							//printf("\n");
						}
						blocks_read += buff_len;
						printf("%d blocks / %d bytes read, total of %u blocks / %u bytes read\r", buff_len, buff_len*BLOCK_SIZE, blocks_read, blocks_read*BLOCK_SIZE);
						//LED_W8_set_byte(blocks_read/BLOCKS_READ);
						LED_B5_set_byte(blocks_read/BLOCKS_READ);

						for(i=0;i<buff_len*BLOCK_SIZE;i++)
						{
							if(fread_data[i]==0xf0)
							{
								Delay(2000); //delay between individual sysex chunks
							}
							//printf("sending byte: %c\n", fread_data[i]);
							/*int length = */uart_write_bytes(MIDI_UART, &fread_data[i], 1);
							LED_W8_set_byte(i/(BLOCK_SIZE/4));
							//putchar(fread_data[i]);
							Delay(1);
						}
					}
				}
				printf("\ntotal of %u blocks / %u bytes read\n",blocks_read,blocks_read*BLOCK_SIZE);

				fseek(f,blocks_read*BLOCK_SIZE,SEEK_SET);
				printf("reading last %ld bytes...",filesize-blocks_read*BLOCK_SIZE);
				buff_len = fread(fread_data,1,filesize-blocks_read*BLOCK_SIZE,f); //read by 1-byte blocks
				printf("%d bytes read\n",buff_len);

				for(i=0;i<buff_len;i++)
				{
					/*int length = */uart_write_bytes(MIDI_UART, &fread_data[i], 1);
					//putchar(fread_data[i]);
					Delay(1);
				}

				fclose(f);
				free(fread_data);

				printf("---END_BINARY_DUMP---\n");
			}
		}

		LED_O4_set_byte(1<<(file_selected/8));

		if(file_found>-1)
		{
			LED_R8_set_byte(1<<file_selected%8);
		}
		else
		{
			if(indicators_loop % INDICATORS_BLINK_FAST == 0)
			{
				LED_R8_set_byte(1<<file_selected%8);
			}
			else if(indicators_loop % INDICATORS_BLINK_FAST == INDICATORS_BLINK_FAST/2)
			{
				LED_R8_set_byte(0);
			}
		}

		indicators_loop++;
		Delay(INDICATORS_DELAY);
	}

	//release the memory allocated for file list, by function sd_card_file_list()
	for(int i=0;i<files_found;i++)
	{
		free(files_list[i]);
	}
}
