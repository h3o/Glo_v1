/*
 * glo_config.c
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

#include "glo_config.h"
#include "hw/gpio.h"
#include "hw/ui.h"
#include "hw/leds.h"
#include "hw/midi.h"
#include "hw/init.h"
#include "hw/sdcard.h"
#include "hw/fw_update.h"
#include <string.h>

spi_flash_mmap_handle_t mmap_handle_config;

const char *binaural_beats_looping[BINAURAL_BEATS_AVAILABLE] = {"a:beta","b:delta","d:theta","t:alpha"}; //defines which wave follows next
binaural_profile_t *binaural_profile;

int channels_found, unlocked_channels_found;

void map_config_file(char **buffer) {
	//printf("map_config_file(): Mapping flash memory at %x (+%x length)\n", CONFIG_ADDRESS, CONFIG_SIZE);

	const void *ptr1;
	int esp_result = spi_flash_mmap(CONFIG_ADDRESS, (size_t) CONFIG_SIZE,
			SPI_FLASH_MMAP_DATA, &ptr1, &mmap_handle_config);
	//printf("map_config_file(): result = %d\n", esp_result);
	if (esp_result == ESP_OK) {
		//printf("map_config_file(): mapped destination = %x\n",(unsigned int)ptr1);
		//printf("map_config_file(): handle=%d ptr=%p\n", mmap_handle_config, ptr1);
		buffer[0] = (char*) ptr1;
	} else {
		printf("map_config_file(): could not map memory, result = %d\n",
				esp_result);
		while (1) {
			error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/
		}; //halt
	}
}

void unmap_config_file() {
	//printf("unmap_config_file(): spi_flash_munmap(mmap_handle_config)... ");
	spi_flash_munmap(mmap_handle_config);
	//printf("unmapped!\n");
}

int read_line(char **line_ptr, char *line_buffer) {
	//read a line, look for newline or comment, whichever is found earlier
	//printf("read_line() starting from %p\n", line_ptr[0]);

	char *cr, *lf, *comment, *line_end;
	int line_length;

	cr = strstr(line_ptr[0], "\r"); //0x0D
	lf = strstr(line_ptr[0], "\n"); //0x0A
	comment = strstr(line_ptr[0], "//");

	//printf("read_line() cr=%p, lf=%p, comment=%p\n", cr, lf, comment);

	line_end = NULL;

	if (comment != NULL) {
		line_end = comment;
	} else if (cr != NULL) {
		line_end = cr;
	} else if (lf != NULL) {
		line_end = lf;
	}

	if (comment < line_end && comment != NULL) {
		line_end = comment;
	}
	if (cr < line_end && cr != NULL) {
		line_end = cr;
	}
	if (lf < line_end && lf != NULL) {
		line_end = lf;
	}

	//printf("read_line() line_end=%p\n", line_end);
	//printf("line %d: nearest cr = %p, lf = %p, comment = %p, line end at = %p\n", lines_parsed, cr, lf, comment, line_end);

	if (line_end == NULL) {
		printf("read_line() no line_end found, returning 0\n");
		return 0;
	}

	line_length = (int) (line_end - line_ptr[0]);
	//printf("read_line() line_end found, line_length=%d\n", line_length);

	if (line_length > CONFIG_LINE_MAX_LENGTH) {
		printf(
				"read_line(): detected line length (%d) longer than CONFIG_LINE_MAX_LENGTH (%d)\n",
				line_length, CONFIG_LINE_MAX_LENGTH);
		while (1) {
			error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/
		}; //halt
	}
	strncpy(line_buffer, line_ptr[0], line_length);

	//roll back spaces and tabs
	int space_or_tab_found = 1;
	while (space_or_tab_found) {
		space_or_tab_found = 0;
		while (line_buffer[line_length - 1] == '\t') {
			line_length--;
			space_or_tab_found = 1;
		}
		while (line_buffer[line_length - 1] == ' ') {
			line_length--;
			space_or_tab_found = 1;
		}
	}

	line_buffer[line_length] = 0;
	//printf("read_line() string copied over\n");

	line_ptr[0] = lf + 1; //move the pointer to new line
	return line_length;
}

void get_samples_map(int *result) {
	char *config_buffer;
	map_config_file(&config_buffer);
	//printf("get_samples_map(): map_config_file returned config_buffer=%p\n", config_buffer);

	char *line_ptr = config_buffer;
	int line_length, lines_parsed = 0;
	char *line_buffer = (char*) malloc(CONFIG_LINE_MAX_LENGTH + 2);
	char *lptr;

	//parse data for samples map table
	int done = 0;
	int samples_map_found = 0;
	int sample_n;

	result[0] = 0; //contains number of results found
	result[1] = 0; //contains highest sample number found

	while (!done) {
		//printf("get_samples_map(): reading line %d\n", lines_parsed);
		line_length = read_line(&line_ptr, line_buffer);
		//printf("get_samples_map(): line %d read, length = %d, line = \"%s\"\n", lines_parsed, line_length, line_buffer);

		if (line_length) {
			if (samples_map_found && line_length > 3) {
				if (line_buffer[1] == ':' || line_buffer[2] == ':') //looks like a valid line
						{
					result[0]++;
					//result[result[0]] = atoi(line_buffer+2);
					sample_n = atoi(line_buffer);
					if (sample_n > result[1]) {
						result[1] = sample_n;
					}
					//printf("get_samples_map(): found sample #%d\n", sample_n);

					lptr = line_buffer;
					do {
						lptr++;
					} while (lptr[0] != ':');
					lptr++;
					result[2 * sample_n] = atoi(lptr);
					do {
						lptr++;
					} while (lptr[0] != ',');
					lptr++;
					result[2 * sample_n + 1] = atoi(lptr);
				}
			}

			if (0 == strcmp(line_buffer, "[samples_map]")) {
				samples_map_found = 1;
				//printf("[samples map] found\n");
			} else if (samples_map_found && line_buffer[0] == '[') {
				done = 1;
				//printf("[samples map] block end\n");
			} else if (0 == strcmp(line_buffer, "[end]")) {
				printf("[samples map] block not found in config file!\n");
				while (1) {
					error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/
				}; //halt
			}

		}

		if (result[0] == SAMPLES_MAX) {
			done = 1;
		}

		lines_parsed++;
	}

	//parse done, release the buffer and mapped memory
	free(line_buffer);
	unmap_config_file();
}

int get_channels_map(channels_map_t *map, uint64_t channel_filter) {
	char *config_buffer, *line_buf_ptr;
	map_config_file(&config_buffer);
	//printf("get_channels_map(): map_config_file returned config_buffer=%p\n", config_buffer);

	char *line_ptr = config_buffer;
	int line_length, lines_parsed = 0;
	char *line_buffer = (char*) malloc(CONFIG_LINE_MAX_LENGTH + 2);

	//parse data for channels map table
	int done = 0;
	int channels_map_found = 0;
	int channels_found = 0;

	char filter[30];
	if(channel_filter>0)
	{
		sprintf(filter,"%llu:",channel_filter);
	}
	else
	{
		filter[0]=0;
	}

	while (!done) {
		//printf("get_channels_map(): reading line %d\n", lines_parsed);
		line_length = read_line(&line_ptr, line_buffer);
		line_buf_ptr = line_buffer;
		//printf("get_channels_map(): line %d read, length = %d, line = \"%s\"\n", lines_parsed, line_length, line_buffer);

		if (line_length) {
			if (channels_map_found && line_length > 2 && line_buffer[0] != '[') {
				if(channel_filter==0 || !strncmp(filter,line_buffer,strlen(filter))) //either no filter required or we found the channel
				{
					if(channel_filter)
					{
						printf("get_channels_map(): found channel with prefix [%s], line = \"%s\"\n", filter, line_buffer);
						line_buf_ptr+=strlen(filter);
					}
					map[channels_found].name = (char*) malloc(strlen(line_buf_ptr)+2);//line_length + 2);
					strcpy(map[channels_found].name, line_buf_ptr);

					//reset parameter vars
					map[channels_found].i1 = 0;
					map[channels_found].i2 = 0;
					map[channels_found].i3 = 0;
					map[channels_found].f4 = 0.0f;
					map[channels_found].settings = 0;
					map[channels_found].binaural = 0;
					map[channels_found].str_param = NULL;

					//parse parameters, if any
					char *params = strstr(line_buf_ptr, ":");
					if (params) {
						//printf("parameters found: [%s]\n", params);
						map[channels_found].name[(int) params - (int) line_buf_ptr] = 0;
						params++;
						int params_parsed = 0;
						int param = 1;

						while (!params_parsed) {
							//printf("parsing param %d: [%s]\n", param, params);
							if (params[0] >= '0' && params[0] <= '9') {
								//printf("found digits, converting...\n");
								if (param == 1) {
									map[channels_found].i1 = atoi(params);
									param++;
								} else if (param == 2) {
									map[channels_found].i2 = atoi(params);
									param++;
								} else if (param == 3) {
									map[channels_found].i3 = atoi(params);
									param++;
								} else if (param == 4) {
									map[channels_found].f4 = atof(params);
									param++;
								}

								while ((params[0] >= '0' && params[0] <= '9') || params[0] == '.') {
									//printf("rolling over digits...\n");
									params++;
								}
							}
							else if(!strncmp(params,"settings=",9))
							{
								params+=9;
								map[channels_found].settings = atoi(params);
								//move over the parsed numbers
								while (params[0] >= '0' && params[0] <= '9') params++;
							}
							else if(!strncmp(params,"binaural=",9))
							{
								params+=9;
								map[channels_found].binaural = atoi(params);
								//move over the parsed numbers
								while (params[0] >= '0' && params[0] <= '9') params++;
								//params_parsed = 1; //binaural is the last possible parameter
							}
							else if(!strncmp(params,"str=\"",5))
							{
								params+=5;
								map[channels_found].str_param = (char *)malloc(strlen(params)+1);
								strcpy(map[channels_found].str_param,params);

								//terminate the string at the ending double quotes
								char *p_cut = strstr(map[channels_found].str_param, "\"");
								p_cut[0]=0;

								//move over the parsed string
								params=strstr(params,"\"");
								params++;
								//params_parsed = 1; //str_param is the last possible parameter
							}

							if (params[0] == 0 || (((int) params - (int) line_buf_ptr) > line_length)) {
								//printf("nothing left\n");
								params_parsed = 1; //done
							} else if (params[0] == ',') {
								//printf("comma found, skipping...\n");
								params++;
							}
						}
						map[channels_found].p_count = param - 1;
					}
					else //no parameters
					{
						map[channels_found].p_count = 0;
					}
					//printf("get_channels_map(): storing channel [%s] with %d parameters\n", map[channels_found].name, map[channels_found].p_count);

					channels_found++;

					if(channel_filter)
					{
						//parse done, release the buffer and mapped memory
						free(line_buffer);
						unmap_config_file();

						return channels_found;
					}
				}
			}

			if (0 == strcmp(line_buffer, "[channels]")) {
				channels_map_found = 1;
				//printf("[channels] found\n");
			} else if (channels_map_found && line_buffer[0] == '[') {
				done = 1;
				//printf("[channels] block end\n");
			} else if (0 == strcmp(line_buffer, "[end]")) {
				printf("[channels] block not found in config file!\n");
				while (1) {
					error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/
				}; //halt
			}
		}

		lines_parsed++;
	}

	//parse done, release the buffer and mapped memory
	free(line_buffer);
	unmap_config_file();

	return channels_found;
}

int parse_parameters(char *line, int line_length, int *params, int max_params) {
	//parse parameters
	char *params_ptr = strstr(line, ":");
	int params_parsed = 0;
	params_ptr++;
	//printf("parameters found: [%s]\n", params_ptr);

	while (params_ptr) {

		//printf("parsing param %d: [%s]\n", params_parsed, params_ptr);
		if (params_ptr[0] >= '0' && params_ptr[0] <= '9') {
			//printf("found digits, converting...\n");
			params[params_parsed] = atoi(params_ptr);
			params_ptr++;

			while ((params_ptr[0] >= '0' && params_ptr[0] <= '9')
					|| params_ptr[0] == '.') {
				//printf("rolling over digits...\n");
				params_ptr++;
			}

			if (params_ptr[0] == ',') {
				//printf("comma found, skipping...\n");
				params_ptr++;
			}

			params_parsed++;
		}
		//else if(params_ptr[0]==',') //could be the leftover comma at the end
		//{
		//	params_ptr++;
		//}

		if (params_ptr[0] == 0
				|| (((int) params_ptr - (int) line) > line_length)
				|| params_parsed == max_params) {
			//printf("nothing left\n");
			params_ptr = NULL; //done
		}
		//else if(params[0]==',')
		//{
		//	printf("comma found, skipping...\n");
		//	params_ptr++;
		//}
	}

	return params_parsed;
}

int load_dekrispator_patch(int patch_no, dekrispator_patch_t *patch) {
	char *config_buffer;
	map_config_file(&config_buffer);
	//printf("load_dekrispator_patch(): map_config_file returned config_buffer=%p\n", config_buffer);

	char *line_ptr = config_buffer;
	int line_length, lines_parsed = 0;
	char *line_buffer = (char*) malloc(CONFIG_LINE_MAX_LENGTH + 2);

	int done = 0;
	//int line_done = 0;
	int patches_map_found = 0;
	int patches_found = 0;
	int patch_no_found = 0;
	//int fx_found = 0;
	int params_parsed;

	while (!done) {
		//printf("load_dekrispator_patch(): reading line %d\n", lines_parsed);
		line_length = read_line(&line_ptr, line_buffer);
		//printf("load_dekrispator_patch(): line %d read, length = %d, line = \"%s\"\n", lines_parsed, line_length, line_buffer);

		//if(0==strcmp(line_buffer,"[end]"))
		//{
		//	printf("config file parsed to the end\n");
		//	done = 1;
		//	break;
		//}

		if (!done && line_length) {
			//line_done = 0;
			if (patches_map_found && line_length > 2 && line_buffer[0] != '[') {
				if (line_buffer[0] == 'M' && line_buffer[1] == 'a'
						&& line_buffer[2] == 'g' && line_buffer[3] == 'i'
						&& line_buffer[4] == 'c') //looks like a valid line
								{
					if (line_buffer[5] == 'P') //MagicPatch
							{
						patches_found++;
						//printf("found the MagicPatch #%d\n", patches_found);

						if (patches_found == patch_no) //found the one we are looking for, patches are numbered from 1
								{
							//printf("found the MagicPatch #%d: [%s]\n", patches_found, line_buffer);
							patch->patch_no = patch_no;
							patch_no_found = 1;

							params_parsed = parse_parameters(line_buffer,
									line_length, patch->patch, MP_PARAMS);
							if (params_parsed == MP_PARAMS) {
								//printf("MagicPatch(): found %d parameters as expected\n", params_parsed);
								//line_done = 1;
							} else {
								printf(
										"MagicPatch(): expected %d parameters, found %d\n",
										MP_PARAMS, params_parsed);
								while (1) {
									error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/
								}; //halt
							}
						}
					}
					if (patch_no_found && line_buffer[5] == 'F') //MagicFX
							{
						//printf("found the MagicFX #%d: [%s]\n", patches_found, line_buffer);

						params_parsed = parse_parameters(line_buffer,
								line_length, patch->fx, MF_PARAMS);
						if (params_parsed == MF_PARAMS) {
							//printf("MagicFX(): found %d parameters as expected\n", params_parsed);
							//line_done = 1;
							done = 1;
							break;
						} else {
							printf(
									"MagicFX(): expected %d parameters, found %d\n",
									MF_PARAMS, params_parsed);
							while (1) {
								error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/
							}; //halt
						}
					}
				}
			}

			//printf("moving on...\n");
			//Delay(10);

			//if(!line_done && !done)
			//{
			if (0 == strcmp(line_buffer, "[dekrispator_patches]")) {
				patches_map_found = 1;
				//printf("[dekrispator_patches] found\n");
			} else if (patches_map_found && line_buffer[0] == '[') {
				//printf("[dekrispator_patches] block end\n");
				done = 1;
				break;
			} else if (0 == strcmp(line_buffer, "[end]")) {
				printf(
						"[dekrispator_patches] block not found in config file!\n");
				while (1) {
					error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/
				}; //halt
			}
			//}
		}
		//else
		//{
		//	printf("this line was empty...\n");
		//	Delay(10);
		//}

		lines_parsed++;
	}
	//printf("load_dekrispator_patch(): done parsing\n");

	//parse done, release the buffer and mapped memory
	free(line_buffer);
	unmap_config_file();

	return patches_found;
}

int calculate(char *formula)
{
	char buf[20];
	strcpy(buf,formula);
	char *cc = strstr(buf, ",");
	if(cc)
	{
		cc[0]=0;
	}
	//printf("calculate(): [%s] ", buf);

	int a=0,b=0,c=0,d=0;
	char op1=0,op2=0,op3=0;
	cc=buf;
	a=atoi(cc);
	while(cc[0]>='0'&&cc[0]<='9')cc++;
	op1=cc[0];
	if(op1)
	{
		cc++;
		b=atoi(cc);
		while(cc[0]>='0'&&cc[0]<='9')cc++;
		op2=cc[0];
		if(op2)
		{
			cc++;
			c=atoi(cc);
			while(cc[0]>='0'&&cc[0]<='9')cc++;
			op3=cc[0];
			if(op3)
			{
				cc++;
				d=atoi(cc);
			}
		}
	}
	//printf("a=%d,b=%d,c=%d,d=%d,op1=%c,op2=%c,op3=%c\n",a,b,c,d,op1,op2,op3);

	if(!op2)
	{
		if(op1=='+') return a+b;
		if(op1=='-') return a-b;
		if(op1=='*') return a*b;
		if(op1=='/') return a/b;
	}
	else if(op1=='+'&&op2=='-'&&!op3)
	{
		return a+b-c;
	}
	else if(op1=='-'&&op2=='+'&&!op3)
	{
		return a-b+c;
	}
	else if(op1=='+'&&op2=='*'&&!op3)
	{
		return a+b*c;
	}
	else if(op1=='*'&&op2=='+'&&!op3)
	{
		return a*b+c;
	}
	else if(op1=='+'&&op2=='*'&&op3=='+')
	{
		return a+b*c+d;
	}
	else if(op1=='+'&&op2=='*'&&op3=='-')
	{
		return a+b*c-d;
	}
	else if(op1=='-'&&op2=='*'&&op3=='+')
	{
		return a-b*c+d;
	}
	else if(op1=='-'&&op2=='*'&&op3=='-')
	{
		return a-b*c-d;
	}

	printf("calculate(): unexpected format\n");
	return 0;
}

int load_dekrispator_sequence(int id, uint8_t *notes, int *tempo) {
	//printf("load_dekrispator_sequence(%d,...)\n", id);
	char *config_buffer;
	map_config_file(&config_buffer);
	//printf("load_dekrispator_sequence(): map_config_file returned config_buffer=%p\n", config_buffer);

	char *line_ptr = config_buffer;
	int line_length, lines_parsed = 0;
	char *line_buffer = (char*) malloc(CONFIG_LINE_MAX_LENGTH + 2);

	int done = 0;
	int seq_block_found = 0;
	int seq_found = 0;
	int seq_no_found = 0;
	int sequence_notes = 0;
	//int buffer_ptr = 0;
	char *eq;

	while (!done) {
		//printf("load_dekrispator_sequence(): reading line %d\n", lines_parsed);
		line_length = read_line(&line_ptr, line_buffer);
		//printf("load_dekrispator_sequence(): line %d read, length = %d, line = \"%s\"\n", lines_parsed, line_length, line_buffer);

		if (!done && line_length)
		{
			if (seq_block_found && line_length > 2 && line_buffer[0] != '[')
			{
				if (!seq_no_found && 0 == strncmp(line_buffer, "seq=", 4)) //sequence identifier
				{
					//printf("found the sequence record #%d: ", seq_found);
					seq_found++;

					eq = strstr(line_buffer, "=");
					int seq_id = atoi(eq + 1);
					//printf("id = %d\n", seq_id);

					if (seq_id == id) //found the one we were looking for
					{
						//printf("found the desired sequence\n");
						seq_no_found = seq_id;

						eq = strstr(line_buffer, "tempo=");
						tempo[0] = atoi(eq + 6);
						//printf("tempo = %d\n", tempo[0]);
					}
				}
				else if (seq_no_found)
				{
					//printf("new line from the record id = %d: %s\n", seq_no_found, line_buffer);
					if (line_buffer[0] == '[' || line_buffer[0] == 's') //reached end of block or new record
					{
						//printf("[dekrispator_sequences] block ended on \"%c\" (case #1)\n", line_buffer[0]);
						done = 1;
						break;
					} else {
						//strcpy(buffer + buffer_ptr, line_buffer);
						//buffer_ptr += strlen(line_buffer);
						//buffer[buffer_ptr] = 0; //terminate the string here
						eq = line_buffer;
						int note;
						do
						{
							note = atoi(eq);
							//printf("decoded note = %d\n", note);
							if(note)
							{
								notes[sequence_notes] = note;

								if(((strstr(eq, ",")>strstr(eq, "+")) && strstr(eq, "+")!=NULL)
								||((strstr(eq, ",")>strstr(eq, "-")) && strstr(eq, "-")!=NULL)
								||((strstr(eq, ",")>strstr(eq, "*")) && strstr(eq, "*")!=NULL)
								||((strstr(eq, ",")>strstr(eq, "/")) && strstr(eq, "/")!=NULL))
								{
									//printf("math found\n");
									notes[sequence_notes] = calculate(eq);
									//printf("result[%d] = %d\n", sequence_notes, notes[sequence_notes]);
								}
								//printf("note %d stored = %d\n", sequence_notes, notes[sequence_notes]);

								sequence_notes++;
								eq = strstr(eq, ",");
								if(!eq) {note = 0; break; }
								eq++;
							}
							//else
							//{
							//	print("EOL\n");
							//	break;
							//}
						}
						while(note);
						//printf("EOL\n");
					}
				}
			} else if (0 == strcmp(line_buffer, "[dekrispator_sequences]")) {
				seq_block_found = 1;
				//printf("[dekrispator_sequences] found\n");
			} else if (seq_block_found && line_buffer[0] == '[') {
				printf("[dekrispator_sequences] block ended on \"[\" (case #2)\n");
				done = 1;
				break;
			} else if (0 == strcmp(line_buffer, "[end]")) {
				printf("[dekrispator_sequences] block not found in config file!\n");
				while (1) {
					error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/
				}; //halt
			}
		}
		lines_parsed++;
	}
	//printf("load_dekrispator_sequence(): done parsing\n");

	//parse done, release the buffer and mapped memory
	free(line_buffer);
	unmap_config_file();

	return sequence_notes;
}

void store_dekrispator_patch_nvs(dekrispator_patch_t *patch)
{
	uint8_t packed_patch[MP_PARAMS+MF_PARAMS];
	for(int i=0;i<MP_PARAMS;i++)
	{
		packed_patch[i] = patch->patch[i];
	}
	for(int i=0;i<MF_PARAMS;i++)
	{
		packed_patch[MP_PARAMS+i] = patch->fx[i];
	}

	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("glo_settings", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("store_dekrispator_patch_nvs(): problem with nvs_open(), error = %d\n", res);
		return;
	}

	printf("store_dekrispator_patch_nvs(): updating key \"dkr_patch\" with blob of bytes: ");
	for(int i=0;i<MP_PARAMS+MF_PARAMS;i++)
	{
		printf("%d,", packed_patch[i]);
	}
	printf("\n");

	res = nvs_set_blob(handle, "dkr_patch", packed_patch, MP_PARAMS+MF_PARAMS);
	if(res!=ESP_OK) //problem writing data
	{
		printf("store_dekrispator_patch_nvs(): problem with nvs_set_blob() while updating key \"dkr_patch\", error = %d\n", res);
		nvs_close(handle);
		return;
	}
	res = nvs_commit(handle);
	if(res!=ESP_OK) //problem writing data
	{
		printf("store_dekrispator_patch_nvs(): problem with nvs_commit() while updating key \"dkr_patch\", error = %d\n", res);
	}

	nvs_close(handle);
}

void load_dekrispator_patch_nvs(dekrispator_patch_t *patch)
{
	uint8_t packed_patch[MP_PARAMS+MF_PARAMS];

	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("glo_settings", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("load_dekrispator_patch_nvs(): problem with nvs_open(), error = %d\n", res);
		return;
	}

	printf("load_dekrispator_patch_nvs(): reading key \"dkr_patch\" (a blob of bytes)\n");

	size_t bytes_loaded = MP_PARAMS+MF_PARAMS;
	res = nvs_get_blob(handle, "dkr_patch", packed_patch, &bytes_loaded);
	if(res!=ESP_OK) //problem reading data
	{
		printf("load_dekrispator_patch_nvs(): problem with nvs_get_blob() while reading key \"dkr_patch\", error = %d\n", res);
		nvs_close(handle);
		return;
	}
	nvs_close(handle);

	if(bytes_loaded != MP_PARAMS+MF_PARAMS)
	{
		printf("load_dekrispator_patch_nvs(): unexpected amount of bytes loaded from key \"dkr_patch\": %d vs %d (MP_PARAMS+MF_PARAMS)\n", bytes_loaded, MP_PARAMS+MF_PARAMS);
		return;
	}

	for(int i=0;i<MP_PARAMS;i++)
	{
		patch->patch[i] = packed_patch[i];
	}
	for(int i=0;i<MF_PARAMS;i++)
	{
		patch->fx[i] = packed_patch[MP_PARAMS+i];
	}

	printf("load_dekrispator_patch_nvs(): key \"dkr_patch\" contains bytes: ");

	for(int i=0;i<MP_PARAMS+MF_PARAMS;i++)
	{
		printf("%d,", packed_patch[i]);
	}
	printf("\n");
}

int load_clouds_patch(int patch, float *params, int expected_params, const char *block_name)
{
	char *config_buffer;
	map_config_file(&config_buffer);
	//printf("load_clouds_patch(): map_config_file returned config_buffer=%p\n", config_buffer);

	char *line_ptr = config_buffer;
	int line_length, lines_parsed = 0;
	char *line_buffer = (char*) malloc(CONFIG_LINE_MAX_LENGTH + 2);

	int done = 0;
	int patches_map_found = 0;
	int patches_found = 0;
	int patch_no_found = 0;
	int params_parsed;

	while (!done) {
		//printf("load_clouds_patch(): reading line %d\n", lines_parsed);
		line_length = read_line(&line_ptr, line_buffer);
		//printf("load_clouds_patch(): line %d read, length = %d, line = \"%s\"\n", lines_parsed, line_length, line_buffer);

		if (!done && line_length) {
			if (patches_map_found && line_length > 2 && line_buffer[0] != '[') {
				if (line_buffer[0] >= '0' && line_buffer[0] <= '9') //looks like a valid line
				{
					patches_found++;
					//printf("found patch #%d\n", patches_found);

					if (patches_found == patch) //found the one we are looking for, patches are numbered from 1
					{
						//printf("found patch #%d: [%s]\n", patches_found, line_buffer);
						patch_no_found = 1;

						params_parsed = 0;
						char *p_ptr = line_buffer;
						while(params_parsed<expected_params)
						{
							params[params_parsed] = atof(p_ptr);
							params_parsed++;
							if(params_parsed==expected_params) break;
							while(p_ptr[0]!=',') p_ptr++;
							p_ptr++;
						}
					}
				}
			}

			//if(!line_done && !done)
			//{
			if (0 == strcmp(line_buffer, block_name)) {
				patches_map_found = 1;
				//printf("[clouds_patches] found\n");
			} else if (patches_map_found && line_buffer[0] == '[') {
				//printf("[clouds_patches] block end\n");
				done = 1;
				break;
			} else if (0 == strcmp(line_buffer, "[end]")) {
				printf(
						"%s block not found in config file!\n", block_name);
				while (1) {
					error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/
				}; //halt
			}
		}
		lines_parsed++;
	}
	//printf("load_clouds_patch(): done parsing\n");

	//parse done, release the buffer and mapped memory
	free(line_buffer);
	unmap_config_file();

	return patches_found;
}

int load_song_or_melody(int id, const char *what, char *buffer) {
	//printf("load_song_or_melody(%d,%s,...)\n", id, what);
	char *config_buffer;
	map_config_file(&config_buffer);
	//printf("load_song_or_melody(): map_config_file returned config_buffer=%p\n", config_buffer);

	char *line_ptr = config_buffer;
	int line_length, lines_parsed = 0;
	char *line_buffer = (char*) malloc(CONFIG_LINE_MAX_LENGTH + 2);

	int done = 0;
	int songs_block_found = 0;
	int songs_found = 0;
	int song_no_found = 0;
	int buffer_ptr = 0;

	while (!done) {
		//printf("load_song_or_melody(): reading line %d\n", lines_parsed);
		line_length = read_line(&line_ptr, line_buffer);
		//printf("load_song_or_melody(): line %d read, length = %d, line = \"%s\"\n", lines_parsed, line_length, line_buffer);

		if (!done && line_length) {
			if (songs_block_found && line_length > 2 && line_buffer[0] != '[') {
				if (!song_no_found
						&& 0 == strncmp(line_buffer, what, strlen(what))) //song or melody identifier
								{
					//printf("found the %s record #%d: ", what, songs_found);
					songs_found++;

					char *eq = strstr(line_buffer, "=");
					int song_id = atoi(eq + 1);
					//printf("id = %d\n", song_id);

					if (song_id == id) //found the one we were looking for
							{
						//printf("found the desired %s\n", what);
						song_no_found = song_id;
					}
				} else if (song_no_found) {
					//printf("new line from the %s record id = %d: %s\n", what, song_no_found, line_buffer);
					if (line_buffer[0] == '[' || line_buffer[0] == 's'
							|| line_buffer[0] == 'm') //reached end of block or new record
									{
						//printf("[songs] block ended on \"%c\" (case #1)\n", line_buffer[0]);
						done = 1;
						break;
					} else {
						strcpy(buffer + buffer_ptr, line_buffer);
						buffer_ptr += strlen(line_buffer);

						if(0 == strncmp("melody", what, strlen(what))) //song or melody identifier
						{
							buffer[buffer_ptr] = ' '; //append space to separate moultiple lines
							buffer_ptr++;
						}
						buffer[buffer_ptr] = 0; //terminate the string here
					}
				}
			} else if (0 == strcmp(line_buffer, "[songs]")) {
				songs_block_found = 1;
				//printf("[songs] found\n");
			} else if (songs_block_found && line_buffer[0] == '[') {
				//printf("[songs] block ended on \"[\" (case #2)\n");
				done = 1;
				break;
			} else if (0 == strcmp(line_buffer, "[end]")) {
				printf("[songs] block not found in config file!\n");
				while (1) {
					error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/
				}; //halt
			}
		}
		lines_parsed++;
	}
	//printf("load_song_or_melody(): done parsing\n");

	//parse done, release the buffer and mapped memory
	free(line_buffer);
	unmap_config_file();

	return buffer_ptr;
}

#define NVS_STR_BUF_LEN 256

int get_remapped_channels(int *remap, int total_channels)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("glo_settings", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("get_remapped_channels(): problem with nvs_open(), error = %d\n", res);
		remap[0] = res;
		return -1;
	}
	//nvs_erase_all(handle); //testing

	char buf[NVS_STR_BUF_LEN];
	size_t got_length;

	//check if the key exists and how long the data is
	res = nvs_get_str(handle, "channel_order", NULL, &got_length);
	printf("get_remapped_channels(): nvs_get_str() call #1 returned length = %d, res = %d\n", got_length, res);

	if(res==ESP_ERR_NVS_NOT_FOUND) //the key may not be there yet, try to create it
	{
		printf("get_remapped_channels(): \"channel_order\" key does not exist yet\n");

		buf[0]=0;
		for(int i=0;i<total_channels;i++)
		{
			sprintf(buf+strlen(buf),"%d,",i);
			remap[i] = i;
		}
		buf[strlen(buf)-1] = 0;

		printf("get_remapped_channels(): creating new key \"channel_order\" with content [%s]\n", buf);
		res = nvs_set_str(handle, "channel_order", buf);
		if(res!=ESP_OK) //problem writing data
		{
			printf("get_remapped_channels(): problem with nvs_set_str() while creating key %s, error = %d\n", buf, res);
			remap[0] = res;
			nvs_close(handle);
			return -2;
		}
		res = nvs_commit(handle);
		if(res!=ESP_OK) //problem writing data
		{
			printf("get_remapped_channels(): problem with nvs_commit() while creating key %s, error = %d\n", buf, res);
			remap[0] = res;
			nvs_close(handle);
			return -3;
		}
		nvs_close(handle);

		//new key created
		return total_channels;
	}
	else if(res!=ESP_OK) //other problem
	{
		printf("get_remapped_channels(): problem with nvs_get_str(), error = %d\n", res);
		remap[0] = res;
		return -4;
	}

	if(got_length >= NVS_STR_BUF_LEN)
	{
		printf("get_remapped_channels(): nvs_get_str() buffer overflow (%d >= %d)\n", got_length, NVS_STR_BUF_LEN);
		remap[0] = got_length;
		return -5;
	}

	got_length = NVS_STR_BUF_LEN;
	res = nvs_get_str(handle, "channel_order", buf, &got_length);
	printf("get_remapped_channels(): nvs_get_str() call #2 returned length = %d, res = %d\n", got_length, res);

	if(res!=ESP_OK)
	{
		printf("get_remapped_channels(): nvs_get_str() failed\n");
		remap[0] = res;
		return -6;
	}

	printf("get_remapped_channels(): key \"channel_order\" found, content = [%s], length = %d\n", buf, got_length);

	//parse the string

	int remapped_channels_found = 0;

	if(got_length>0)
	{
		char *ptr = buf;
		while(ptr<buf+strlen(buf) && ptr[0])
		{
			remap[remapped_channels_found] = atoi(ptr);
			while(ptr[0]!=',' && ptr[0]) ptr++;
			if(ptr[0])ptr++;
			remapped_channels_found++;
		}
	}

	return remapped_channels_found;
}

void update_remapped_channels()
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("glo_settings", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("update_remapped_channels(): problem with nvs_open(), error = %d\n", res);
		return;
	}

	char buf[NVS_STR_BUF_LEN];

	buf[0]=0;
	for(int i=0;i<remapped_channels_found;i++)
	{
		sprintf(buf+strlen(buf),"%d,",remapped_channels[i]);
	}
	buf[strlen(buf)-1] = 0;

	printf("update_remapped_channels(): updating key \"channel_order\" with content [%s]\n", buf);
	res = nvs_set_str(handle, "channel_order", buf);
	if(res!=ESP_OK) //problem writing data
	{
		printf("update_remapped_channels(): problem with nvs_set_str() while updating key %s, error = %d\n", buf, res);
		nvs_close(handle);
		return;
	}
	res = nvs_commit(handle);
	if(res!=ESP_OK) //problem writing data
	{
		printf("update_remapped_channels(): problem with nvs_commit() while updating key %s, error = %d\n", buf, res);
	}

	nvs_close(handle);
}

void move_channel_to_front(int channel_position)
{
    printf("move_channel_to_front(): old order = ");
    for(int i=0;i<remapped_channels_found;i++) { printf("%d ", remapped_channels[i]); } printf("\n");

	if(!channel_position)
	{
		return; //nothing to do
	}
	int channel_at_position = remapped_channels[channel_position];

	//shift all up
	for(int i=channel_position;i>0;i--)
	{
		remapped_channels[i] = remapped_channels[i-1];
	}
	remapped_channels[0] = channel_at_position;

	printf("move_channel_to_front(): new order = ");
    for(int i=0;i<remapped_channels_found;i++) { printf("%d ", remapped_channels[i]); } printf("\n");

    update_remapped_channels();
}

char *get_line_in_block(const char *block_name, char *prefix)
{
	printf("get_line_in_block(): looking for record with prefix \"%s\" in block %s\n", prefix, block_name);

	char *line = NULL;

	char *config_buffer;
	map_config_file(&config_buffer);
	//printf("get_line_in_block(): map_config_file returned config_buffer=%p\n", config_buffer);

	char *line_ptr = config_buffer;
	int line_length, lines_parsed = 0;
	char *line_buffer = (char*) malloc(CONFIG_LINE_MAX_LENGTH + 2);

	int done = 0;
	int block_found = 0;
	//int prefix_found = 0;

	while (!done) {
		//printf("get_line_in_block(): reading line %d\n", lines_parsed);
		line_length = read_line(&line_ptr, line_buffer);
		//printf("get_line_in_block(): line %d read, length = %d, line = \"%s\"\n", lines_parsed, line_length, line_buffer);

		if (!done && line_length) {

			if (block_found && line_length > 2 && line_buffer[0] != '[') {

				if (!strncmp(line_buffer,prefix,strlen(prefix))) { //prefix found
					printf("get_line_in_block(): prefix \"%s\" found\n", prefix);

					line = malloc(line_length+2);
					strcpy(line,line_buffer);
					done = 1;
					break;
				}
			}

			if (0 == strncmp(line_buffer, block_name, strlen(block_name))) {
				block_found = 1;
				printf("get_line_in_block(): block %s found\n", block_name);
			} else if (block_found && line_buffer[0] == '[') {
				printf("get_line_in_block(): block %s ended\n", block_name);
				done = 1;
				break;
			} else if (0 == strcmp(line_buffer, "[end]")) {
				printf("get_line_in_block(): block %s not found in config file!\n", block_name);
				while (1) { error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/ }; //halt
			}
		}

		lines_parsed++;
	}
	//printf("get_line_in_block(): done parsing\n");

	//parse done, release the buffer and mapped memory
	free(line_buffer);
	unmap_config_file();

	return line;
}

int get_binaural_def(int binaural_id, char **buffer)
{
	char prefix[20];
	sprintf(prefix,"%d:",binaural_id);
	char *line = get_line_in_block("[binaural_beats_by_channel]",prefix); //this will allocate memory for line
	if(line==NULL)
	{
		printf("get_binaural_def(): [binaural_beats_by_channel] record with prefix \"%s\" not found\n", prefix);
		while(1);
	}
	strcpy(prefix,line+strlen(prefix));
	free(line);

	buffer[0] = get_line_in_block("[binaural_beats_program]",prefix); //this will allocate memory for buffer
	if(buffer[0]==NULL)
	{
		printf("get_binaural_def(): [binaural_beats_program] record with prefix \"%s\" not found\n", prefix);
		while(1);
	}

	//number of coords is easy to tell by how many semicolons there are
	int coords_found = 0;
	char *pch=strchr(buffer[0],';');
	while (pch!=NULL)
	{
		coords_found++;
	    pch=strchr(pch+1,';');
	}

	coords_found++; //there is expected to be one more params than semicolons

	printf("get_binaural_def(): line with %d coords found in config: \"%s\"\n", coords_found, buffer[0]);

	return coords_found;
}

int get_binaural_def_by_wave(char *binaural_wave, char **buffer)
{
	char prefix[20];
	sprintf(prefix,"%s:",binaural_wave);
	buffer[0] = get_line_in_block("[binaural_beats_program]",prefix); //this will allocate memory for buffer
	if(buffer[0]==NULL)
	{
		printf("get_binaural_def_by_wave(): [binaural_beats_program] record with prefix \"%s\" not found\n", prefix);
		while(1);
	}

	//number of coords is easy to tell by how many semicolons there are
	int coords_found = 0;
	char *pch=strchr(buffer[0],';');
	while (pch!=NULL)
	{
		coords_found++;
	    pch=strchr(pch+1,';');
	}

	coords_found++; //there is expected to be one more params than semicolons

	printf("get_binaural_def_by_wave(): line with %d coords found in config: \"%s\"\n", coords_found, buffer[0]);

	return coords_found;
}

void parse_binaural_def(binaural_profile_t *profile, char *buffer)
{
	strncpy(profile->name,buffer,10);
	((char*)strstr(profile->name,":"))[0] = 0;

	//parse data in buffer
	char *ptr = strstr(buffer,":[") + 2;

	for(int i=0;i<profile->total_coords;i++)
	{
		profile->coord_x[i] = atoi(ptr);
		while(ptr[0]!=',')ptr++;
		ptr++;
		profile->coord_y[i] = atof(ptr);
		if(i==profile->total_coords-1)
		{
			break;
		}
		while(ptr[0]!=';')ptr++;
		ptr++;
		while(ptr[0]==' ')ptr++;
	}

	//last time coordinate is effectively a total lenght of the binaural session
	profile->total_length = profile->coord_x[profile->total_coords-1];
}

void load_binaural_profile(binaural_profile_t *profile, int binaural_id)
{
	profile->id = binaural_id;
	profile->name = malloc(20);

	char *buffer;
	profile->total_coords = get_binaural_def(binaural_id, &buffer); //this will allocate memory for buffer
	profile->coord_x = malloc((1+profile->total_coords) * sizeof(int));
	profile->coord_y = malloc((1+profile->total_coords) * sizeof(float));
	parse_binaural_def(profile, buffer);
	free(buffer);
}

void reload_binaural_profile(binaural_profile_t *profile) //reload with next available wave
{
	printf("reload_binaural_profile(): current wave = %s\n", profile->name);

	char next_wave[10];
	for(int i=0;i<BINAURAL_BEATS_AVAILABLE;i++)
	{
		printf("looking for wave record starting with %c, comparing with %c\n", binaural_beats_looping[i][0], profile->name[0]);
		if(binaural_beats_looping[i][0]==profile->name[0])
		{
			strcpy(next_wave,binaural_beats_looping[i]+2);
			printf("reload_binaural_profile(): next available wave = %s\n", next_wave);
		}
	}

	free(profile->coord_x);
	free(profile->coord_y);

	char *buffer;
	profile->total_coords = get_binaural_def_by_wave(next_wave, &buffer); //this will allocate memory for buffer
	profile->coord_x = malloc((1+profile->total_coords) * sizeof(int));
	profile->coord_y = malloc((1+profile->total_coords) * sizeof(float));
	parse_binaural_def(profile, buffer);
	free(buffer);


	/*
	char *buffer;
	profile->total_coords = get_binaural_def(binaural_id, &buffer); //this will allocate memory for buffer

	profile->coord_x = malloc((1+profile->total_coords) * sizeof(int));
	profile->coord_y = malloc((1+profile->total_coords) * sizeof(float));

	//parse data in buffer
	char *ptr = strstr(buffer,":[") + 2;

	for(int i=0;i<profile->total_coords;i++)
	{
		profile->coord_x[i] = atoi(ptr);
		while(ptr[0]!=',')ptr++;
		ptr++;
		profile->coord_y[i] = atof(ptr);
		if(i==profile->total_coords-1)
		{
			break;
		}
		while(ptr[0]!=';')ptr++;
		ptr++;
		while(ptr[0]==' ')ptr++;
	}

	//last time coordinate is effectively a total lenght of the binaural session
	profile->total_length = profile->coord_x[profile->total_coords-1];

	free(buffer);
	*/
}

void load_isochronic_def(isochronic_def *def, char *block_name)
{
	char *config_buffer;
	map_config_file(&config_buffer);
	//printf("load_isochronic_def(): map_config_file returned config_buffer=%p\n", config_buffer);

	char *line_ptr = config_buffer;
	int line_length, lines_parsed = 0;
	char *line_buffer = (char*) malloc(CONFIG_LINE_MAX_LENGTH + 2);

	int done = 0;
	int block_found = 0;
	//float total_length = 0, correction = 0;

	while (!done) {
		//printf("load_isochronic_def(): reading line %d\n", lines_parsed);
		line_length = read_line(&line_ptr, line_buffer);
		//printf("load_isochronic_def(): line %d read, length = %d, line = \"%s\"\n", lines_parsed, line_length, line_buffer);

		if (line_length) {
			if (block_found && line_length > 2 && line_buffer[0] != '[') {

				int item_name_length = 0;
				while(line_buffer[item_name_length]!=' ' && line_buffer[item_name_length]!='\t') item_name_length++;

				if(!strncmp(line_buffer,"ALPHA_LOW",9)) { def->ALPHA_LOW = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"ALPHA_HIGH",10)) { def->ALPHA_HIGH = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"BETA_LOW",8)) { def->BETA_LOW = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"BETA_HIGH",9)) { def->BETA_HIGH = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"DELTA_LOW",9)) { def->DELTA_LOW = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"DELTA_HIGH",10)) { def->DELTA_HIGH = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"THETA_LOW",9)) { def->THETA_LOW = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"THETA_HIGH",10)) { def->THETA_HIGH = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"PROGRAM_LENGTH",14)) { def->PROGRAM_LENGTH = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"TONE_LENGTH_MAX",15)) { def->TONE_LENGTH_MAX = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"TONE_LENGTH_STEP",16)) { def->TONE_LENGTH_STEP = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"TONE_LENGTH",11)) { def->TONE_LENGTH = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"TONE_VOLUME_MAX",15)) { def->TONE_VOLUME_MAX = atof(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"TONE_VOLUME_STEP",16)) { def->TONE_VOLUME_STEP = atof(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"TONE_VOLUME",11)) { def->TONE_VOLUME = atof(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"TONE_CUTOFF_MAX",15)) { def->TONE_CUTOFF_MAX = atof(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"TONE_CUTOFF_STEP",16)) { def->TONE_CUTOFF_STEP = atof(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"TONE_CUTOFF",11)) { def->TONE_CUTOFF = atof(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"WIND_VOLUME",11)) { def->WIND_VOLUME = atof(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"SIGNAL_LIMIT_MIN",16)) { def->SIGNAL_LIMIT_MIN = atof(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"SIGNAL_LIMIT_MAX",16)) { def->SIGNAL_LIMIT_MAX = atof(line_buffer+item_name_length); }
			}

			if (0 == strcmp(line_buffer, block_name)) {
				block_found = 1;
				//printf("[voice_menu] found\n");
			} else if (block_found && line_buffer[0] == '[') {
				done = 1;
				//printf("[voice_menu] block end\n");
			} else if (0 == strcmp(line_buffer, "[end]")) {
				printf("%s block not found in config file!\n", block_name);
				while (1) {
					error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/
				}; //halt
			}
		}

		lines_parsed++;
	}

	//parse done, release the buffer and mapped memory
	free(line_buffer);
	unmap_config_file();
}

int load_settings(settings_t *settings, const char* block_name)
{
	char *config_buffer;
	map_config_file(&config_buffer);
	//printf("load_settings(): map_config_file returned config_buffer=%p\n", config_buffer);

	if(config_buffer[0]==0xff && config_buffer[1]==0xff && config_buffer[2]==0xff && config_buffer[3]==0xff &&
	   config_buffer[4]==0xff && config_buffer[5]==0xff && config_buffer[6]==0xff && config_buffer[7]==0xff)
	{
		return -1;
	}

	char *line_ptr = config_buffer;
	int line_length, lines_parsed = 0;
	char *line_buffer = (char*) malloc(CONFIG_LINE_MAX_LENGTH + 2);

	int done = 0;
	int block_found = 0;

	while (!done) {
		//printf("load_settings(): reading line %d\n", lines_parsed);
		line_length = read_line(&line_ptr, line_buffer);
		//printf("load_settings(): line %d read, length = %d, line = \"%s\"\n", lines_parsed, line_length, line_buffer);

		if (line_length) {
			if (block_found && line_length > 2 && line_buffer[0] != '[') {

				int item_name_length = 0;
				while(line_buffer[item_name_length]!=' ' && line_buffer[item_name_length]!='\t') item_name_length++;

				if(!strncmp(line_buffer,"AUTO_POWER_OFF_ONLY_IF_NO_MOTION",32)) { settings->AUTO_POWER_OFF_ONLY_IF_NO_MOTION = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"AUTO_POWER_OFF_TIMEOUT",22)) { settings->AUTO_POWER_OFF_TIMEOUT = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"DEFAULT_ACCESSIBLE_CHANNELS",27)) { settings->DEFAULT_ACCESSIBLE_CHANNELS = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"TUNING_DEFAULT",14)) { settings->TUNING_DEFAULT = atof(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"TUNING_MIN",10)) { settings->TUNING_MIN = atof(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"TUNING_MAX",10)) { settings->TUNING_MAX = atof(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"TUNING_INCREASE_COEFF",21)) { settings->TUNING_INCREASE_COEFF = atof(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"GRANULAR_DETUNE_COEFF_SET",25)) { settings->GRANULAR_DETUNE_COEFF_SET = atof(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"GRANULAR_DETUNE_COEFF_MUL",25)) { settings->GRANULAR_DETUNE_COEFF_MUL = atof(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"GRANULAR_DETUNE_COEFF_MAX",25)) { settings->GRANULAR_DETUNE_COEFF_MAX = atof(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"TEMPO_BPM_DEFAULT",17)) { settings->TEMPO_BPM_DEFAULT = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"TEMPO_BPM_MIN",13)) { settings->TEMPO_BPM_MIN = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"TEMPO_BPM_MAX",13)) { settings->TEMPO_BPM_MAX = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"TEMPO_BPM_STEP",14)) { settings->TEMPO_BPM_STEP = atoi(line_buffer+item_name_length); }

				if(!strncmp(line_buffer,"AGC_ENABLED",11)) { settings->AGC_ENABLED = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"AGC_MAX_GAIN_STEP",17)) { settings->AGC_MAX_GAIN_STEP = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"AGC_MAX_GAIN_LIMIT",18)) { settings->AGC_MAX_GAIN_LIMIT = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"AGC_MAX_GAIN",12)) { settings->AGC_MAX_GAIN = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"AGC_TARGET_LEVEL",16)) { settings->AGC_TARGET_LEVEL = atoi(line_buffer+item_name_length); }
				else if(!strncmp(line_buffer,"MIC_BIAS",8)) { settings->MIC_BIAS = atoi(line_buffer+item_name_length); }

				if(!strncmp(line_buffer,"CODEC_ANALOG_VOLUME_DEFAULT",27)) { settings->CODEC_ANALOG_VOLUME_DEFAULT = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"CODEC_DIGITAL_VOLUME_DEFAULT",28)) { settings->CODEC_DIGITAL_VOLUME_DEFAULT = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"CLOUDS_HARD_LIMITER_POSITIVE",28)) { settings->CLOUDS_HARD_LIMITER_POSITIVE = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"CLOUDS_HARD_LIMITER_NEGATIVE",28)) { settings->CLOUDS_HARD_LIMITER_NEGATIVE = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"CLOUDS_HARD_LIMITER_MAX",23)) { settings->CLOUDS_HARD_LIMITER_MAX = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"CLOUDS_HARD_LIMITER_STEP",24)) { settings->CLOUDS_HARD_LIMITER_STEP = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"DRUM_THRESHOLD_ON",17)) { settings->DRUM_THRESHOLD_ON = atof(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"DRUM_THRESHOLD_OFF",18)) { settings->DRUM_THRESHOLD_OFF = atof(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"DRUM_LENGTH1",12)) { settings->DRUM_LENGTH1 = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"DRUM_LENGTH2",12)) { settings->DRUM_LENGTH2 = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"DRUM_LENGTH3",12)) { settings->DRUM_LENGTH3 = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"DRUM_LENGTH4",12)) { settings->DRUM_LENGTH4 = atoi(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"DCO_ADC_SAMPLE_GAIN",19)) { settings->DCO_ADC_SAMPLE_GAIN = atof(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"KS_FREQ_CORRECTION",18)) { settings->KS_FREQ_CORRECTION = atof(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"IDLE_SET_RST_SHORTCUT",21)) { settings->IDLE_SET_RST_SHORTCUT = atol(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"IDLE_RST_SET_SHORTCUT",21)) { settings->IDLE_RST_SET_SHORTCUT = atol(line_buffer+item_name_length); }
				if(!strncmp(line_buffer,"IDLE_LONG_SET_SHORTCUT",22)) { settings->IDLE_LONG_SET_SHORTCUT = atol(line_buffer+item_name_length); }

				if(!strncmp(line_buffer,"CONFIG_FW_VERSION",17)) { settings->CONFIG_FW_VERSION = fw_version_to_uint16(line_buffer+item_name_length); }
			}

			if (0 == strcmp(line_buffer, block_name)) {
				block_found = 1;
				//printf("[voice_menu] found\n");
			} else if (block_found && line_buffer[0] == '[') {
				done = 1;
				//printf("[voice_menu] block end\n");
			} else if (0 == strcmp(line_buffer, "[end]")) {
				printf("%s block not found in config file!\n", block_name);
				while (1) {
					error_blink(15, 60, 5, 30, 5, 30); /*RGB:count,delay*/
				}; //halt
			}
		}

		lines_parsed++;
	}

	//parse done, release the buffer and mapped memory
	free(line_buffer);
	unmap_config_file();

	return block_found;
}

void load_persistent_settings(persistent_settings_t *settings)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("glo_settings", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("load_persistent_settings(): problem with nvs_open() in ");
		printf("R/O mode, error = %d\n", res);

		//maybe it was not created yet, will try
		res = nvs_open("glo_settings", NVS_READWRITE, &handle);
		if(res!=ESP_OK)
		{
			printf("load_persistent_settings(): problem with nvs_open() in ");
			printf("R/W mode, error = %d\n", res);
			return;
		}
	}

	int8_t val_i8;
	uint8_t val_u8;
	int16_t val_i16;
	//uint16_t val_u16;
	uint64_t val_u64;

	res = nvs_get_i16(handle, "AV", &val_i16);
	if(res!=ESP_OK) { val_i16 = global_settings.CODEC_ANALOG_VOLUME_DEFAULT; } //if the key does not exist, load default value
	settings->ANALOG_VOLUME = val_i16;

	res = nvs_get_i16(handle, "DV", &val_i16);
	if(res!=ESP_OK) { val_i16 = global_settings.CODEC_DIGITAL_VOLUME_DEFAULT; } //if the key does not exist, load default value
	settings->DIGITAL_VOLUME = val_i16;

	res = nvs_get_u8(handle, "ADC", &val_u8);
	if(res!=ESP_OK) { val_u8 = ADC_INPUT_DEFAULT; } //if the key does not exist, load default value
	settings->ADC_INPUT_SELECT = val_u8;

	res = nvs_get_i8(handle, "EQ_BASS", &val_i8);
	if(res!=ESP_OK) { val_i8 = EQ_BASS_DEFAULT; }
	settings->EQ_BASS = val_i8;

	res = nvs_get_i8(handle, "EQ_TREBLE", &val_i8);
	if(res!=ESP_OK) { val_i8 = EQ_TREBLE_DEFAULT; }
	settings->EQ_TREBLE = val_i8;

	res = nvs_get_i16(handle, "TEMPO", &val_i16);
	if(res!=ESP_OK) {
		val_i16 = global_settings.TEMPO_BPM_DEFAULT;
		printf("load_persistent_settings(): TEMPO not found, loading default = %d\n", val_i16);
	}
	settings->TEMPO = val_i16;

	res = nvs_get_u64(handle, "TUNING", &val_u64);
	if(res!=ESP_OK) {
		settings->FINE_TUNING = global_settings.TUNING_DEFAULT;
		printf("load_persistent_settings(): FINE_TUNING(TUNING) not found, loading default = %f\n", global_settings.TUNING_DEFAULT);
	}
	else
	{
		settings->FINE_TUNING = ((double*)&val_u64)[0];
	}

	res = nvs_get_i8(handle, "TRN", &val_i8);
	if(res!=ESP_OK) {
		val_i8 = 0;
		printf("load_persistent_settings(): TRANSPOSE not found, loading default = %d\n", val_i8);
	}
	settings->TRANSPOSE = val_i8;

	res = nvs_get_i8(handle, "BEEPS", &val_i8);
	if(res!=ESP_OK) { val_i8 = 1; }
	settings->BEEPS = val_i8;

	res = nvs_get_i8(handle, "XTRA_CHNL", &val_i8);
	if(res!=ESP_OK) {
		val_i8 = 0;
		printf("load_persistent_settings(): ALL_CHANNELS_UNLOCKED(XTRA_CHNL) not found, loading default = %d\n", val_i8);
	}
	settings->ALL_CHANNELS_UNLOCKED = val_i8;

	res = nvs_get_i8(handle, "MIDI_SYNC", &val_i8);
	if(res!=ESP_OK) { val_i8 = MIDI_SYNC_MODE_DEFAULT; }
	settings->MIDI_SYNC_MODE = val_i8;

	res = nvs_get_i8(handle, "MIDI_POLY", &val_i8);
	if(res!=ESP_OK) { val_i8 = MIDI_POLYPHONY_DEFAULT; }
	settings->MIDI_POLYPHONY = val_i8;

	res = nvs_get_i8(handle, "SENSORS", &val_i8);
	if(res!=ESP_OK) { val_i8 = PARAMETER_CONTROL_SENSORS_DEFAULT; }
	settings->PARAMS_SENSORS = val_i8;

	res = nvs_get_i8(handle, "AGC_PGA", &val_i8);
	if(res!=ESP_OK) { val_i8 = global_settings.AGC_ENABLED ? global_settings.AGC_TARGET_LEVEL : 0; }
	settings->AGC_ENABLED_OR_PGA = val_i8;

	res = nvs_get_i8(handle, "AGC_G", &val_i8);
	if(res!=ESP_OK) { val_i8 = global_settings.AGC_MAX_GAIN; }
	settings->AGC_MAX_GAIN = val_i8;

	res = nvs_get_i8(handle, "MIC_B", &val_i8);
	if(res!=ESP_OK) { val_i8 = global_settings.MIC_BIAS; }
	settings->MIC_BIAS = val_i8;

	res = nvs_get_i8(handle, "LEDS_OFF", &val_i8);
	if(res!=ESP_OK) { val_i8 = 0; }
	settings->ALL_LEDS_OFF = val_i8;

	res = nvs_get_i8(handle, "PWR_OFF", &val_i8);
	if(res!=ESP_OK) { val_i8 = 6; /* x10 minutes = 1 hour */ }
	settings->AUTO_POWER_OFF = val_i8;

	res = nvs_get_i16(handle, "FS", &val_i16);
	if(res!=ESP_OK) { val_i16 = SAMPLE_RATE_DEFAULT; } //if the key does not exist, load default value
	settings->SAMPLING_RATE = val_i16;

	res = nvs_get_i8(handle, "SD_CLK", &val_i8);
	if(res!=ESP_OK) { val_i8 = 1; /*high speed*/ }
	settings->SD_CARD_SPEED = val_i8;

	res = nvs_get_i8(handle, "ACC_O", &val_i8);
	if(res!=ESP_OK) { val_i8 = ACC_ORIENTATION_DEFAULT; }
	settings->ACC_ORIENTATION = val_i8;

	res = nvs_get_i8(handle, "ACC_I", &val_i8);
	if(res!=ESP_OK) { val_i8 = ACC_INVERT_DEFAULT; }
	settings->ACC_INVERT = val_i8;

	/*
	res = nvs_get_u16(handle, "FW_VER", &val_u16);
	if(res!=ESP_OK) { val_u16 = 0; }
	settings->CONFIG_FW_VERSION = val_u16;
	*/

	nvs_close(handle);
}

void store_persistent_settings(persistent_settings_t *settings)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("glo_settings", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("store_persistent_settings(): problem with nvs_open(), error = %d\n", res);
		return;
	}
	//nvs_erase_all(handle); //testing

	if(settings->ANALOG_VOLUME_updated)
	{
		printf("store_persistent_settings(): ANALOG_VOLUME(AV) updated to %d\n", settings->ANALOG_VOLUME);
		settings->ANALOG_VOLUME_updated = 0;
		res = nvs_set_i16(handle, "AV", settings->ANALOG_VOLUME);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i16(), error = %d\n", res);
		}
	}
	if(settings->DIGITAL_VOLUME_updated)
	{
		printf("store_persistent_settings(): DIGITAL_VOLUME(DV) updated to %d\n", settings->DIGITAL_VOLUME);
		settings->DIGITAL_VOLUME_updated = 0;
		res = nvs_set_i16(handle, "DV", settings->DIGITAL_VOLUME);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i16(), error = %d\n", res);
		}
	}
	if(settings->ADC_INPUT_SELECT_updated)
	{
		printf("store_persistent_settings(): ADC_INPUT_SELECT(ADC) updated to %d\n", settings->ADC_INPUT_SELECT);
		settings->ADC_INPUT_SELECT_updated = 0;
		res = nvs_set_u8(handle, "ADC", settings->ADC_INPUT_SELECT);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_u8(), error = %d\n", res);
		}
	}
	if(settings->EQ_BASS_updated)
	{
		printf("store_persistent_settings(): EQ_BASS updated to %d\n", settings->EQ_BASS);
		settings->EQ_BASS_updated = 0;
		res = nvs_set_i8(handle, "EQ_BASS", settings->EQ_BASS);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}
	if(settings->EQ_TREBLE_updated)
	{
		printf("store_persistent_settings(): EQ_TREBLE updated to %d\n", settings->EQ_TREBLE);
		settings->EQ_TREBLE_updated = 0;
		res = nvs_set_i8(handle, "EQ_TREBLE", settings->EQ_TREBLE);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}
	if(settings->TEMPO_updated)
	{
		printf("store_persistent_settings(): TEMPO updated to %d\n", settings->TEMPO);
		settings->TEMPO_updated = 0;
		res = nvs_set_i16(handle, "TEMPO", settings->TEMPO);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i16(), error = %d\n", res);
		}
	}
	if(settings->FINE_TUNING_updated)
	{
		printf("store_persistent_settings(): FINE_TUNING(TUNING) updated to %f\n", settings->FINE_TUNING);
		settings->FINE_TUNING_updated = 0;
		uint64_t *val_u64 = (uint64_t*)&settings->FINE_TUNING;
		res = nvs_set_u64(handle, "TUNING", val_u64[0]);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i16(), error = %d\n", res);
		}
	}
	if(settings->TRANSPOSE_updated)
	{
		printf("store_persistent_settings(): TRANSPOSE updated to %d\n", settings->TRANSPOSE);
		settings->TRANSPOSE_updated = 0;
		res = nvs_set_i8(handle, "TRN", settings->TRANSPOSE);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}
	if(settings->BEEPS_updated)
	{
		printf("store_persistent_settings(): BEEPS updated to %d\n", settings->BEEPS);
		settings->BEEPS_updated = 0;
		res = nvs_set_i8(handle, "BEEPS", settings->BEEPS);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}
	if(settings->ALL_CHANNELS_UNLOCKED_updated)
	{
		printf("store_persistent_settings(): ALL_CHANNELS_UNLOCKED(XTRA_CHNL) updated to %d\n", settings->ALL_CHANNELS_UNLOCKED);
		settings->ALL_CHANNELS_UNLOCKED_updated = 0;
		res = nvs_set_i8(handle, "XTRA_CHNL", settings->ALL_CHANNELS_UNLOCKED);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}
	if(settings->MIDI_SYNC_MODE_updated)
	{
		printf("store_persistent_settings(): MIDI_SYNC_MODE updated to %d\n", settings->MIDI_SYNC_MODE);
		settings->MIDI_SYNC_MODE_updated = 0;
		res = nvs_set_i8(handle, "MIDI_SYNC", settings->MIDI_SYNC_MODE);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}
	if(settings->MIDI_POLYPHONY_updated)
	{
		printf("store_persistent_settings(): MIDI_POLYPHONY_MODE updated to %d\n", settings->MIDI_POLYPHONY);
		settings->MIDI_POLYPHONY_updated = 0;
		res = nvs_set_i8(handle, "MIDI_POLY", settings->MIDI_POLYPHONY);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}
	if(settings->PARAMS_SENSORS_updated)
	{
		printf("store_persistent_settings(): PARAMS_SENSORS updated to %d\n", settings->PARAMS_SENSORS);
		settings->PARAMS_SENSORS_updated = 0;
		res = nvs_set_i8(handle, "SENSORS", settings->PARAMS_SENSORS);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}

	if(settings->AGC_ENABLED_OR_PGA_updated)
	{
		printf("store_persistent_settings(): AGC_ENABLED_OR_PGA updated to %d\n", settings->AGC_ENABLED_OR_PGA);
		settings->AGC_ENABLED_OR_PGA_updated = 0;
		res = nvs_set_i8(handle, "AGC_PGA", settings->AGC_ENABLED_OR_PGA);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}

	if(settings->AGC_MAX_GAIN_updated)
	{
		printf("store_persistent_settings(): AGC_MAX_GAIN updated to %d\n", settings->AGC_MAX_GAIN);
		settings->AGC_MAX_GAIN_updated = 0;
		res = nvs_set_i8(handle, "AGC_G", settings->AGC_MAX_GAIN);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}

	if(settings->MIC_BIAS_updated)
	{
		printf("store_persistent_settings(): MIC_BIAS updated to %d\n", settings->MIC_BIAS);
		settings->MIC_BIAS_updated = 0;
		res = nvs_set_i8(handle, "MIC_B", settings->MIC_BIAS);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}

	if(settings->ALL_LEDS_OFF_updated)
	{
		printf("store_persistent_settings(): ALL_LEDS_OFF updated to %d\n", settings->ALL_LEDS_OFF);
		settings->ALL_LEDS_OFF_updated = 0;
		res = nvs_set_i8(handle, "LEDS_OFF", settings->ALL_LEDS_OFF);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}

	if(settings->AUTO_POWER_OFF_updated)
	{
		printf("store_persistent_settings(): AUTO_POWER_OFF updated to %d\n", settings->AUTO_POWER_OFF);
		settings->AUTO_POWER_OFF_updated = 0;
		res = nvs_set_i8(handle, "PWR_OFF", settings->AUTO_POWER_OFF);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}

	if(settings->SAMPLING_RATE_updated)
	{
		printf("store_persistent_settings(): SAMPLING_RATE updated to %d\n", settings->SAMPLING_RATE);
		settings->SAMPLING_RATE_updated = 0;
		res = nvs_set_i16(handle, "FS", settings->SAMPLING_RATE);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i16(), error = %d\n", res);
		}
	}

	if(settings->SD_CARD_SPEED_updated)
	{
		printf("store_persistent_settings(): SD_CARD_SPEED updated to %d\n", settings->SD_CARD_SPEED);
		settings->SD_CARD_SPEED_updated = 0;
		res = nvs_set_i8(handle, "SD_CLK", settings->SD_CARD_SPEED);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}

	if(settings->ACC_ORIENTATION_updated)
	{
		printf("store_persistent_settings(): ACC_ORIENTATION updated to %d\n", settings->ACC_ORIENTATION);
		settings->ACC_ORIENTATION_updated = 0;
		res = nvs_set_i8(handle, "ACC_O", settings->ACC_ORIENTATION);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}

	if(settings->ACC_INVERT_updated)
	{
		printf("store_persistent_settings(): ACC_INVERT updated to %d\n", settings->ACC_INVERT);
		settings->ACC_INVERT_updated = 0;
		res = nvs_set_i8(handle, "ACC_I", settings->ACC_INVERT);
		if(res!=ESP_OK)
		{
			printf("store_persistent_settings(): problem with nvs_set_i8(), error = %d\n", res);
		}
	}

	res = nvs_commit(handle);
	if(res!=ESP_OK) //problem writing data
	{
		printf("store_persistent_settings(): problem with nvs_commit(), error = %d\n", res);
	}
	nvs_close(handle);
}

int load_all_settings()
{
    int settings_block_found = load_settings(&global_settings, "[global_settings]");
    printf("Global settings loaded: CONFIG_FW_VERSION=%d, AUTO_POWER_OFF_TIMEOUT=%d, AUTO_POWER_OFF_ONLY_IF_NO_MOTION=%d, DEFAULT_ACCESSIBLE_CHANNELS=%d, "
    		"TUNING_DEFAULT=%f, TUNING_MAX=%f, TUNING_MIN=%f, TUNING_INCREASE_COEFF=%f, "
    		"GRANULAR_DETUNE_COEFF_SET=%f, GRANULAR_DETUNE_COEFF_MUL=%f, GRANULAR_DETUNE_COEFF_MAX=%f, "
    		"TEMPO_BPM_DEFAULT=%d, TEMPO_BPM_MIN=%d, TEMPO_BPM_MAX=%d, TEMPO_BPM_STEP=%d, "
    		"AGC_ENABLED=%d, AGC_MAX_GAIN=%d, AGC_TARGET_LEVEL=%d, MIC_BIAS=%d, CODEC_ANALOG_VOLUME_DEFAULT=%d, CODEC_DIGITAL_VOLUME_DEFAULT=%d, "
    		"CLOUDS_HARD_LIMITER_POSITIVE=%d, CLOUDS_HARD_LIMITER_NEGATIVE=%d, CLOUDS_HARD_LIMITER_MAX=%d, CLOUDS_HARD_LIMITER_STEP=%d, "
    		"DRUM_THRESHOLD_ON=%f, DRUM_THRESHOLD_OFF=%f, DRUM_LENGTH1=%d, DRUM_LENGTH2=%d, DRUM_LENGTH3=%d, DRUM_LENGTH4=%d, "
    		"DCO_ADC_SAMPLE_GAIN=%f, KS_FREQ_CORRECTION=%f, "
    		"IDLE_SET_RST_SHORTCUT=%llu, IDLE_RST_SET_SHORTCUT=%llu, IDLE_LONG_SET_SHORTCUT=%llu\n",
			global_settings.CONFIG_FW_VERSION,
			global_settings.AUTO_POWER_OFF_TIMEOUT,
			global_settings.AUTO_POWER_OFF_ONLY_IF_NO_MOTION,
			global_settings.DEFAULT_ACCESSIBLE_CHANNELS,
			global_settings.TUNING_DEFAULT,
			global_settings.TUNING_MAX,
			global_settings.TUNING_MIN,
			global_settings.TUNING_INCREASE_COEFF,
			global_settings.GRANULAR_DETUNE_COEFF_SET,
			global_settings.GRANULAR_DETUNE_COEFF_MUL,
			global_settings.GRANULAR_DETUNE_COEFF_MAX,
			global_settings.TEMPO_BPM_DEFAULT,
			global_settings.TEMPO_BPM_MIN,
			global_settings.TEMPO_BPM_MAX,
			global_settings.TEMPO_BPM_STEP,
			global_settings.AGC_ENABLED,
			global_settings.AGC_MAX_GAIN,
			global_settings.AGC_TARGET_LEVEL,
			global_settings.MIC_BIAS,
			global_settings.CODEC_ANALOG_VOLUME_DEFAULT,
			global_settings.CODEC_DIGITAL_VOLUME_DEFAULT,
			global_settings.CLOUDS_HARD_LIMITER_POSITIVE,
			global_settings.CLOUDS_HARD_LIMITER_NEGATIVE,
			global_settings.CLOUDS_HARD_LIMITER_MAX,
			global_settings.CLOUDS_HARD_LIMITER_STEP,
			global_settings.DRUM_THRESHOLD_ON,
			global_settings.DRUM_THRESHOLD_OFF,
			global_settings.DRUM_LENGTH1,
			global_settings.DRUM_LENGTH2,
			global_settings.DRUM_LENGTH3,
			global_settings.DRUM_LENGTH4,
			global_settings.DCO_ADC_SAMPLE_GAIN,
			global_settings.KS_FREQ_CORRECTION,
			global_settings.IDLE_SET_RST_SHORTCUT,
			global_settings.IDLE_RST_SET_SHORTCUT,
			global_settings.IDLE_LONG_SET_SHORTCUT
    );

    tempo_bpm = global_settings.TEMPO_BPM_DEFAULT;
    //global_tuning = global_settings.TUNING_DEFAULT;

    //codec_analog_volume = global_settings.CODEC_ANALOG_VOLUME_DEFAULT;
    codec_digital_volume = global_settings.CODEC_DIGITAL_VOLUME_DEFAULT;
    //codec_volume_user = global_settings.CODEC_DIGITAL_VOLUME_DEFAULT;

    load_persistent_settings(&persistent_settings);
    printf("Persistent settings loaded: ANALOG_VOLUME=%d, DIGITAL_VOLUME=%d, ADC_INPUT_SELECT=%d, EQ_BASS=%d, EQ_TREBLE=%d, TEMPO=%d, TRANSPOSE=%d, FINE_TUNING=%f, BEEPS=%d, "
    		"ALL_CHANNELS_UNLOCKED=%d, MIDI_SYNC_MODE=%d, MIDI_POLYPHONY_MODE=%d, PARAMS_SENSORS=%d, AGC_ENABLED_OR_PGA=%d, AGC_MAX_GAIN=%d, "
    		"ALL_LEDS_OFF=%d, AUTO_POWER_OFF=%d, SAMPLING_RATE=%u, SD_CARD_SPEED=%d, ACC_ORIENTATION=%d, ACC_INVERT=%d\n",
    		persistent_settings.ANALOG_VOLUME,
    		persistent_settings.DIGITAL_VOLUME,
			persistent_settings.ADC_INPUT_SELECT,
			persistent_settings.EQ_BASS,
			persistent_settings.EQ_TREBLE,
			persistent_settings.TEMPO,
			persistent_settings.TRANSPOSE,
			persistent_settings.FINE_TUNING,
			persistent_settings.BEEPS,
			persistent_settings.ALL_CHANNELS_UNLOCKED,
			persistent_settings.MIDI_SYNC_MODE,
			persistent_settings.MIDI_POLYPHONY,
			persistent_settings.PARAMS_SENSORS,
			persistent_settings.AGC_ENABLED_OR_PGA,
			persistent_settings.AGC_MAX_GAIN,
			persistent_settings.ALL_LEDS_OFF,
			persistent_settings.AUTO_POWER_OFF,
			persistent_settings.SAMPLING_RATE,
			persistent_settings.SD_CARD_SPEED,
			persistent_settings.ACC_ORIENTATION,
			persistent_settings.ACC_INVERT);

    codec_analog_volume = persistent_settings.ANALOG_VOLUME;
    codec_volume_user = persistent_settings.DIGITAL_VOLUME;
    EQ_bass_setting = persistent_settings.EQ_BASS;
    EQ_treble_setting = persistent_settings.EQ_TREBLE;
    tempo_bpm = persistent_settings.TEMPO;
    global_tuning = persistent_settings.FINE_TUNING;
    beeps_enabled = persistent_settings.BEEPS;

    //hide extra channels unless unlocked (only used in whale)
    if(!persistent_settings.ALL_CHANNELS_UNLOCKED)
    {
    	channels_found = global_settings.DEFAULT_ACCESSIBLE_CHANNELS;
    }

    midi_sync_mode = persistent_settings.MIDI_SYNC_MODE;
    midi_polyphony = persistent_settings.MIDI_POLYPHONY;
    use_acc_or_ir_sensors = persistent_settings.PARAMS_SENSORS;
    ADC_input_select = persistent_settings.ADC_INPUT_SELECT;

    return settings_block_found;
}

void persistent_settings_store_eq()
{
	persistent_settings.EQ_BASS = EQ_bass_setting;
	persistent_settings.EQ_BASS_updated = 1;
	persistent_settings.EQ_TREBLE = EQ_treble_setting;
	persistent_settings.EQ_TREBLE_updated = 1;
	persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
}

void persistent_settings_store_tempo()
{
	persistent_settings.TEMPO = tempo_bpm;
	persistent_settings.TEMPO_updated = 1;
	persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
}

void persistent_settings_store_tuning()
{
	persistent_settings.FINE_TUNING = global_tuning;
	persistent_settings.FINE_TUNING_updated = 1;
	persistent_settings.update = PERSISTENT_SETTINGS_UPDATE_TIMER;
}

int nvs_erase_namespace(char *namespace)
{
	esp_err_t res;
	nvs_handle handle;

	res = nvs_open(namespace, NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("nvs_erase_namespace(): problem with nvs_open(), error = %d\n", res);
		indicate_error(0x0001, 10, 100);
		return 1;
	}
	res = nvs_erase_all(handle);
	if(res!=ESP_OK)
	{
		printf("nvs_erase_namespace(): problem with nvs_erase_all(), error = %d\n", res);
		indicate_error(0x0003, 10, 100);
		return 2;
	}
	res = nvs_commit(handle);
	if(res!=ESP_OK)
	{
		printf("nvs_erase_namespace(): problem with nvs_commit(), error = %d\n", res);
		indicate_error(0x0007, 10, 100);
		return 3;
	}
	nvs_close(handle);

	return 0;
}

void settings_reset()
{
	LEDs_all_OFF();

	if(!nvs_erase_namespace("glo_settings")) //if no error
	{
		indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_LEFT_8, 4, 50);
		whale_restart();
	}
}

void sd_rec_counter_reset()
{
	LEDs_all_OFF();

	if(!nvs_erase_namespace("system_cnt")) //if no error
	{
		indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_LEFT_8, 4, 50);
		whale_restart();
	}
}

extern int use_acc_or_ir_sensors;

void set_controls(int settings, int instant_update)
{
	printf("set_controls(): settings = %d\n", settings);

	if(settings==3 || settings==PARAMETER_CONTROL_SENSORS_ACCELEROMETER)
	{
		use_acc_or_ir_sensors = PARAMETER_CONTROL_SENSORS_ACCELEROMETER;
		indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_3DS, 3, 100);
	}
	if(settings==4 || settings==PARAMETER_CONTROL_SENSORS_IRS)
	{
		use_acc_or_ir_sensors = PARAMETER_CONTROL_SENSORS_IRS;
		indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_IRS, 2, 40);
	}
	printf("set_controls(): use_acc_or_ir_sensors set to %d\n", use_acc_or_ir_sensors);

	persistent_settings.PARAMS_SENSORS = use_acc_or_ir_sensors;
	persistent_settings.PARAMS_SENSORS_updated = 1;
	persistent_settings.update = 0; //to avoid duplicate update if timer already running down
	if(instant_update)
	{
		store_persistent_settings(&persistent_settings);
	}
}

void service_menu_action(int command)
{
	printf("service_menu_action(): command = %d\n",command);
	if(command == SERVICE_MENU_WRITE_CONFIG)
	{
		write_config();
	}

	if(command == SERVICE_MENU_RELOAD_CONFIG)
	{
		reload_config();
	}
	if(command == SERVICE_MENU_FACTORY_FW)
	{
		factory_reset_firmware();
	}
	if(command == SERVICE_MENU_REC_COUNTER_RST)
	{
		sd_rec_counter_reset();
	}
}

void write_config()
{
	char *config_buffer;
	map_config_file(&config_buffer);

	char *config_ends = strstr(config_buffer, "[end]");

	if(!config_ends)
	{
		printf("write_config(): end mark not found\n");
		indicate_error(0x0001, 10, 100);
		return;
	}
	int config_len = (int)config_ends - (int)config_buffer;
	printf("write_config(): config end mark found at %d\n",config_len);

	int res = sd_card_write_file("config.txt", config_buffer, config_len+9+20*39); //add length of [end], 2 newlines and 20 lines of 39x '-'
	if(res>0)
	{
		indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_FILL_8_LEFT, 1, 50);
	}
	else if(res!=-1) //if other error than just SD not present
	{
		indicate_error(0x0003, 10, 100);
	}

	unmap_config_file();
}

void service_menu()
{
	printf("service_menu()\n");
	int service_menu = 1;
	int blink_cnt = 0;

	//check for held RST, if need to invoke service menu
	if(BUTTON_RST_ON)
	{
		printf("service_menu(): BUTTON_RST_ON\n");

		while(service_menu)
		{
			if(blink_cnt%50==0)
			{
				LED_R8_set_byte(0x55);
			}
			else if(blink_cnt%50==25)
			{
				LED_R8_set_byte(0xaa);
			}
			Delay(10);

			LED_O4_set_byte((0x01<<WHICH_USER_BUTTON_ON)>>1);

			if(service_menu==1 && !BUTTON_RST_ON)
			{
				service_menu=2;
			}
			if(service_menu==2 && BUTTON_RST_ON)
			{
				service_menu=0; //exit without any action
			}
			if(service_menu==2 && BUTTON_SET_ON)
			{
				//exectute action, if an user button (1-4) pressed at the same time
				if(USER_BUTTON_ON)
				{
					service_menu_action(WHICH_USER_BUTTON_ON);
				}
				service_menu=0; //exit without any action
			}
			blink_cnt++;
		}
	}
	else if(BUTTON_SET_ON)
	{
		printf("service_menu(): BUTTON_SET_ON\n");

		uint8_t fm[10];
		esp_efuse_mac_get_default(fm);
		//memcpy(fm+6,GLO_HASH,2);
		sscanf(GLO_HASH, "%2hhx", &fm[6]);
		sscanf(GLO_HASH+2, "%2hhx", &fm[7]);
		printf("Factory MAC & 2B Glo hash = %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", fm[0],fm[1],fm[2],fm[3],fm[4],fm[5],fm[6],fm[7]);

		while(service_menu)
		{
			Delay(10);
			LED_O4_set_byte((0x01<<WHICH_USER_BUTTON_ON)>>1);

			if(USER_BUTTON_ON)
			{
				//display bytes from mac address on red/white LEDs

				LED_R8_set_byte(byte_bit_reverse(fm[2*(WHICH_USER_BUTTON_ON-1)]));
				LED_W8_set_byte(byte_bit_reverse(fm[2*(WHICH_USER_BUTTON_ON-1)+1]));
			}
			else
			{
				LED_R8_all_OFF();
				if(blink_cnt%50==0)
				{
					LED_W8_set_byte(0x55);
				}
				else if(blink_cnt%50==25)
				{
					LED_W8_set_byte(0xaa);
				}
			}

			if(service_menu==1 && !BUTTON_SET_ON)
			{
				service_menu=2;
			}
			if(service_menu==2 && BUTTON_RST_ON)
			{
				service_menu=0; //exit without any action
			}
			if(service_menu==2 && BUTTON_SET_ON)
			{
				//exectute action, if an user button (1-4) pressed at the same time
				if(BUTTON_U1_ON)
				{
					LED_R8_all_OFF();
					LED_W8_all_OFF();
					factory_data_load_SD();
				}
				if(BUTTON_U2_ON)
				{
					LED_R8_all_OFF();
					LED_W8_all_OFF();
					firmware_update_SD();
				}
				if(BUTTON_U3_ON && BUTTON_U4_ON)
				{
					LED_R8_all_OFF();
					LED_W8_all_OFF();

					//write activate.htm file to SD card with a correct link
					sd_card_check(FW_ACTIVATE_LINK_FILE, 1);
					if(sd_card_present==SD_CARD_PRESENT_WRITEABLE)
					{
						indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_FILL_8_RIGHT, 1, 50);
					}
					else
					{
						indicate_error(0x55aa, 8, 80);
					}
				}
				//service_menu=0; //exit without any action
			}
			blink_cnt++;
		}
	}
	LEDs_all_OFF();
}
