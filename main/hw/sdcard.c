/*
 * sdcard.c
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

#include <sys/stat.h>
#include <sys/unistd.h>
#include <string.h>

#include "sdcard.h"

#include "init.h"
#include "gpio.h"
#include "ui.h"
#include "leds.h"
#include "signals.h"
#include "midi.h"

static const char *TAG = "SD_test";

sdmmc_card_t *card;
uint8_t sd_card_ready = 0, sd_card_present = SD_CARD_PRESENT_UNKNOWN;

uint32_t *SD_play_buf;
FILE* SD_f = NULL;

#ifdef BOARD_WHALE
#define SD_CARD_1BIT_MODE
#define SD_CARD_SPI_MODE
#endif

int sd_card_init(int max_files, int speed)
{
	printf("sd_card_init(%d, %d)\n", max_files, speed);

	//-----------------------------------------------------
    // SD card test

	#ifdef SD_CARD_SPI_MODE
    ESP_LOGI(TAG, "Using SDSPI peripheral");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	#else
    //ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

	if(speed) //set faster clock
    {
    	//printf("sd_card_init(): speed = 40MHz\n");
    	host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    }
    else
    {
    	//printf("sd_card_init(): speed = 20MHz\n");
    	host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    }

	#endif

    #ifdef SD_CARD_1BIT_MODE
    // To use 1-line SD mode, uncomment the following line:

	#ifdef SD_CARD_SPI_MODE
    host.flags = SDMMC_HOST_FLAG_SPI;
    host.slot = VSPI_HOST;
    host.max_freq_khz = 8000;
    #else
    host.flags = SDMMC_HOST_FLAG_1BIT;
	#endif

    //printf("SD card test in 1-bit mode\n");
	#else
    //printf("SD card test in 4-bit mode\n");
	#endif

	//#ifdef SD_CARD_1BIT_MODE
	#ifdef SD_CARD_SPI_MODE

    gpio_set_pull_mode(23, GPIO_PULLUP_ONLY);	// CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(19, GPIO_PULLUP_ONLY);	// D0, needed in 4- and 1-line modes
    //gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);	// D1, needed in 4-line mode only
    //gpio_set_pull_mode(33, GPIO_PULLUP_ONLY);	// D2, needed in 4-line mode only
    gpio_set_pull_mode(5, GPIO_PULLUP_ONLY);	// D3, needed in 4- and 1-line modes

	sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
	slot_config.gpio_miso = 19; //D0
	slot_config.gpio_mosi = 23; //CMD
	slot_config.gpio_sck  = 18; //CLK
	slot_config.gpio_cs   = 5;	//CD

	#else

	// This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

	// GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
    // Internal pull-ups are not sufficient. However, enabling internal pull-ups
    // does make a difference some boards, so we do that here.
    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
    gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
    gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes
	#endif

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    	//.allocation_unit_size = 0,//4096,
        .format_if_mount_failed = false,
        .max_files = max_files
    };

	// Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.

	//printf("sd_card_init(): mounting file system, free mem = %u\n", xPortGetFreeHeapSize());

    int ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. Partition can not be mounted. ");
                //"If you want the card to be formatted, set format_if_mount_failed = true.");
        }
        else if (ret == ESP_ERR_INVALID_STATE)
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%d): esp_vfs_fat_sdmmc_mount was already called.", ret);
                //"Make sure SD card lines have pull-up resistors in place.", ret);
        }
        else if (ret == ESP_ERR_NO_MEM)
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%d): memory can not be allocated.", ret);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%d): unexpected error.", ret);
        }
        return ret;
    }

	// Card has been initialized, print its properties
    //sdmmc_card_print_info(stdout, card);

    sd_card_ready = 1;

    return ESP_OK;
}

void sd_card_check(const char *htm_filename, int recalculate_hash)
{
    printf("sd_card_check(): SD Card init...\n");
    int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
    if(result!=ESP_OK)
    {
    	printf("sd_card_check(): problem with SD Card Init, code=%d\n", result);
		sd_card_present = SD_CARD_PRESENT_NONE;
    	return;
    }

    sd_card_present = SD_CARD_PRESENT_WRITEABLE;

    char update_link[200], update_link_file[200];
    sprintf(update_link, "%s%s%s\r\n", FW_UPDATE_LINK_HTTPS, GLO_HASH, FW_UPDATE_LINK_EXT);

    //check if update link file exists
    FILE *f;
    struct stat st;
    if (stat(htm_filename, &st) == 0)
    {
    	//check if content is correct
        //ESP_LOGI(TAG, "Reading file");
        f = fopen(htm_filename, "r"); //open file for reading
        if (f == NULL) {
            //ESP_LOGE(TAG, "Failed to open file for reading");
        	printf("sd_card_check(): problem reading the existing link file\n");
    		sd_card_present = SD_CARD_PRESENT_NONE;
            return;
        }
        fgets(update_link_file, sizeof(update_link_file), f);
        fclose(f);

    	if(strcmp(update_link,update_link_file)) //if not the same
    	{
        	printf("sd_card_check(): the existing link file is incorrect\n");
    		//delete it
    		unlink(htm_filename);
    	}
    	else //we're done
    	{
        	printf("sd_card_check(): the existing link file is OK\n");
    	    // All done, unmount partition and disable SDMMC or SPI peripheral
    	    esp_vfs_fat_sdmmc_unmount();
    	    //ESP_LOGI(TAG, "Card unmounted");
    	    sd_card_ready = 0;
    		return;
    	}
    }

    //ESP_LOGI(TAG, "Opening file");
    f = fopen(htm_filename, "w");
    if (f == NULL)
    {
    	//ESP_LOGE(TAG, "Failed to open file for writing");
    	printf("sd_card_check(): problem writing the link file\n");
    	sd_card_present = SD_CARD_PRESENT_READONLY;
    }
    else
    {
    	fprintf(f, "%s", update_link);
    	fclose(f);
    	//ESP_LOGI(TAG, "File written");
    }

    // All done, unmount partition and disable SDMMC or SPI peripheral
    esp_vfs_fat_sdmmc_unmount();
    //ESP_LOGI(TAG, "Card unmounted");
    sd_card_ready = 0;
}

void sd_card_info()
{
    if(!sd_card_ready)
    {
    	printf("sd_card_info(): SD Card init... ");
    	int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
    	if(result!=ESP_OK)
    	{
    		printf("sd_card_info(): problem with SD Card Init, code=%d\n", result);
	    	indicate_error(0x55aa, 8, 80);
    		return;
    	}
    	//printf("done!\n");
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    FATFS *fs;
    DWORD fre_clust;
    uint64_t fre_sect, tot_sect;

    /* Get volume information and free clusters of sdcard */
    int res = f_getfree("/sdcard/", &fre_clust, &fs);
    if (res) {
		printf("sd_card_info(): problem with SD Card f_getfree(), code=%d\n", res);
		return;
    }

    /* Get total sectors and free sectors */
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = (uint64_t)fre_clust * fs->csize;
    printf("sd_card_info(): total secors = %llu, free secors = %llu, cluster size = %u sectors, total capacity = %lluMB, free capacity = %lluMB\n", tot_sect, fre_sect, fs->csize, (tot_sect * 512) / (1024 * 1024), (fre_sect * 512) / (1024 * 1024));

    //uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    //printf("SD_card_mount(): SD Card Size = %lluMB\n", cardSize);
    //printf("SD_card_mount(): Total space = %lluMB\n", SD.totalBytes() / (1024 * 1024));
    //printf("SD_card_mount(): Used space = %lluMB\n", SD.usedBytes() / (1024 * 1024));

	esp_vfs_fat_sdmmc_unmount();
	//ESP_LOGI(TAG, "Card unmounted\n");
	sd_card_ready = 0;
}

void sd_card_speed_test()
{
    if(!sd_card_ready)
    {
    	printf("sd_card_speed_test(): SD Card init...\n");
    	int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
    	if(result!=ESP_OK)
    	{
    		printf("sd_card_speed_test(): problem with SD Card Init, code=%d\n", result);
	    	indicate_error(0x55aa, 8, 80);
    		return;
    	}
    	//printf("done!\n");
    }

    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen("/sdcard/hello.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Hello %s!\n", card->cid.name);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    // Check if destination file exists before renaming
    struct stat st;
    if (stat("/sdcard/foo.txt", &st) == 0) {
        // Delete it if it exists
        unlink("/sdcard/foo.txt");
    }

    // Rename original file
    ESP_LOGI(TAG, "Renaming file");
    if (rename("/sdcard/hello.txt", "/sdcard/foo.txt") != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }

    // Open renamed file for reading
    ESP_LOGI(TAG, "Reading file");
    f = fopen("/sdcard/foo.txt", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char* pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    // speed test - read

    int i,buf_size = 2048, repeat = 1000;
    char *buf = (char*)malloc(buf_size);
    float t1,t2;

    ESP_LOGI(TAG, "Opening file for READ speed test");

    t1 = micros();
    f = fopen("/sdcard/speed.dat", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file speed.dat for reading");
        //return;
    }
    else
    {
    	for(i=0;i<repeat;i++)
    	{
    		fread(buf,buf_size,1,f);
    		printf(".");
    	}
    	fclose(f);
    	t2 = micros();
    	printf("%d bytes read in %f seconds\n",buf_size*repeat,t2-t1);
    }

    // speed test - write
    for(i=0;i<buf_size;i++)
    {
    	buf[i]=(uint8_t)i;
    }

    ESP_LOGI(TAG, "Opening file for WRITE speed test");

    t1 = micros();
    f = fopen("/sdcard/speed.dat", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file speed.dat for writing");
        //return;
    }
    else
    {
    	for(i=0;i<repeat;i++)
    	{
    		fwrite(buf,buf_size,1,f);
    		printf(".");
    	}
    	fclose(f);
    	t2 = micros();
    	printf("%d bytes written in %f seconds\n",buf_size*repeat,t2-t1);
    	ESP_LOGI(TAG, "File written");

    	// All done, unmount partition and disable SDMMC or SPI peripheral
    	esp_vfs_fat_sdmmc_unmount();
    	//ESP_LOGI(TAG, "Card unmounted");
    	sd_card_ready = 0;
    }
    //-----------------------------------------------------
}

void sd_card_file_list(char *subdir, int recursive, char **list, int *total_files, int add_dirs_to_list)
{
	//total_files[0] = 0;
	//if(subdir==NULL)
	//{
		if(!sd_card_ready)
		{
			printf("sd_card_file_list(): SD Card init...\n");
			int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
			if(result!=ESP_OK)
			{
				printf("sd_card_file_list(): problem with SD Card Init, code=%d\n", result);
		    	indicate_error(0x55aa, 8, 80);
				return;
			}
			//printf("done!\n");
		}
	//}

	DIR *dp = NULL;
	char dir_name[270];

	if(subdir!=NULL)
	{
		sprintf(dir_name,"/sdcard/%s",subdir);
	}
	else
	{
		sprintf(dir_name,"/sdcard/");
	}

	dp = opendir(dir_name);
	//printf("opendir(): dp = %x\n", (uint32_t)dp);

    if(dp==NULL)
    {
    	printf("opendir(\"%s\") returned NULL\n",dir_name);
    	return;
    }

    struct dirent *de = NULL;
    char filepath[270];
    FILE *f;
    long int filesize;

	do
    {
    	de = readdir(dp);
    	//printf("readdir(): de = %x\n", (uint32_t)de);
        //printf("Dir / opened, de->d_name = %s\n", de->d_name);

        if(de!=NULL)
        {
            if(subdir!=NULL && recursive)
            {
            	printf("-> ");
            }

        	if(de->d_type==DT_DIR)
        	{
        		if(add_dirs_to_list)
        		{
					if(list!=NULL)
					{
						list[total_files[0]] = malloc(strlen(de->d_name)+1);
						strcpy(list[total_files[0]],de->d_name);
						total_files[0]++;
					}
        		}

        		if(recursive)
        		{
					//printf("de->d_name = %s, type = %d (DIR)\n", de->d_name, de->d_type);
					printf("%s (DIR)\n", de->d_name);

					if(subdir!=NULL)
					{
						sprintf(dir_name,"%s/%s",subdir,de->d_name);
						sd_card_file_list(dir_name, 1, list, total_files, 0);
					}
					else
					{
						sd_card_file_list(de->d_name, 1, list, total_files, 0);
					}
        		}
        	}
        	else
        	{
        		if(subdir!=NULL)
        		{
        			sprintf(filepath, "/sdcard/%s/%s", subdir, de->d_name);
        		}
        		else
        		{
        			sprintf(filepath, "/sdcard/%s", de->d_name);
        		}

                f = fopen(filepath, "r");
        		if (f == NULL) {
        			printf("Failed to open file %s for reading\n", filepath);
        			return;
        		}
        		fseek(f,0,SEEK_END);
        		filesize = ftell(f);
        		fclose(f);
        		//printf("de->d_name = %s, type = %d, size = %ld\n", de->d_name, de->d_type, filesize);
        		printf("%s\t%ld B\n", de->d_name, filesize);
        		if(list!=NULL)
        		{
        			list[total_files[0]] = malloc(strlen(de->d_name)+1);
        			strcpy(list[total_files[0]],de->d_name);
            		total_files[0]++;
        		}
        	}
        }
    }
    while(de!=NULL);

    //printf("[%d files total]\n", total_files);

    //if(de==NULL)
    //{
    	//printf("f_readdir() returned NULL\n");
    	//return;
    //}

    //printf("closedir(): Closing the dir\n");
    closedir(dp);

    //printf("Freeing the dirent structure\n");
    //free(de);

	if(subdir==NULL)
	{
		// All done, unmount partition and disable SDMMC or SPI peripheral
		esp_vfs_fat_sdmmc_unmount();
		//ESP_LOGI(TAG, "Card unmounted");
		sd_card_ready = 0;
	}

	//return total_files;
}

#define SD_SYNC_IDLE			0
#define SD_SYNC_GET_FILE		1
#define SD_SYNC_GOT_FILE_NAME	2
#define SD_SYNC_EOL_CHAR		";" //character to terminate parameter input

void sd_card_download()
{
	char cmd[100],param[100],*p;
	int res;//, param_ptr;
	int state = SD_SYNC_IDLE;

	uart_set_baudrate(UART_NUM_0, 1250000);
	channel_running = 1;

	//start listening to UART
	while(!event_next_channel)
	{
		res = scanf("%s", cmd);
		//printf("res = %d, cmd = %s\n",res, cmd);

		if(res!=-1)
		{
			printf("cmd = \"%s\", state = %d\n", cmd, state);

			if(state==SD_SYNC_IDLE || state==SD_SYNC_GOT_FILE_NAME)
			{
				if(cmd[0]=='L') //list all files
				{
					printf("--SD_FILE_LIST--\n");
					sd_card_file_list("rec", 0, NULL, NULL, 0);
					printf("--END--\n");
				}
				if(cmd[0]=='X') //exit the channel
				{
					break;
				}
				if(cmd[0]=='G') //get file, set up file name
				{
					printf("--GET_FILE--\n");
					state = SD_SYNC_GET_FILE;
					//param_ptr = 0;
					param[0] = 0;
				}
				if(cmd[0]=='P') //return current parameter (e.g. selected file name)
				{
					printf("--SELECTED_PARAMETER--\n");
					printf("parameter=\"%s\"\n",param);
				}
				if(cmd[0]=='D' && state==SD_SYNC_GOT_FILE_NAME) //initiate file download
				{
					printf("--DOWNLOAD_FILE--\n");
					printf("filename=\"%s\"\n",param);

					if(!sd_card_ready)
				    {
				    	int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
				    	if(result!=ESP_OK)
				    	{
				    		printf("sd_card_download(): problem initializing SD card, code=%d\n", result);
					    	indicate_error(0x55aa, 8, 80);
				    		break;
				    	}
				    }

				    char filepath[150];
				    FILE *f;
				    long int filesize;

				    sprintf(filepath, "/sdcard/rec/%s", param);
				    f = fopen(filepath, "rb");
				    if (f == NULL)
				    {
				    	printf("sd_card_download(): Failed to open file %s for reading\n", filepath);
				    	indicate_error(0x0001, 10, 100);
				        break;
					}
				    fseek(f,0,SEEK_END);
				    filesize = ftell(f);

				    printf("sd_card_download(): file size of %s = %lu\n", filepath, filesize);

				    fseek(f,0,SEEK_SET);

				    #define BUFFSIZE	1024*4
					#define BLOCK_SIZE	64
					#define BLOCKS_READ	64

				    char *fread_data;//[BUFFSIZE + 1] = { 0 }; //data buffer ready to write to the flash
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
				        	//printf("%d blocks / %d bytes read, total of %u blocks / %u bytes read\r", buff_len, buff_len*BLOCK_SIZE, blocks_read, blocks_read*BLOCK_SIZE);
				        	LED_W8_set_byte(blocks_read/BLOCKS_READ);
				        	LED_B5_set_byte(blocks_read/(BLOCKS_READ*256));

				        	for(i=0;i<buff_len*BLOCK_SIZE;i++)
				        	{
				        		putchar(fread_data[i]);
				        	}
				        }
				    }
					//printf("\ntotal of %u blocks / %u bytes read\n",blocks_read,blocks_read*BLOCK_SIZE);

					fseek(f,blocks_read*BLOCK_SIZE,SEEK_SET);
					//printf("reading last %ld bytes...",filesize-blocks_read*BLOCK_SIZE);
			        buff_len = fread(fread_data,1,filesize-blocks_read*BLOCK_SIZE,f); //read by 1-byte blocks
					//printf("%d bytes read\n",buff_len);

		        	for(i=0;i<buff_len;i++)
		        	{
		        		putchar(fread_data[i]);
		        	}

		        	fclose(f);
				    free(fread_data);

					printf("---END_BINARY_DUMP---\n");

				    /*
				    // All done, unmount partition and disable SDMMC or SPI peripheral
					esp_vfs_fat_sdmmc_unmount();
					printf("sd_card_download(): Card unmounted\n");
				   	sd_card_ready = 0;
				   	*/
				}
			}
			else if(state==SD_SYNC_GET_FILE)
			{
				strcat(param,cmd);
				//param_ptr+=strlen(cmd);
				//printf("param=\"%s\",param_ptr=%d\n",param,param_ptr);
				printf("param=\"%s\"\n",param);
				if((p=strstr(param,SD_SYNC_EOL_CHAR))!=NULL)
				{
					p[0]=0;
					state=SD_SYNC_GOT_FILE_NAME;
					printf("--GOT_FILE_NAME--\n");
					printf("parameter=\"%s\"\n",param);
				}
			}

		}
		Delay(10);
	}
	uart_set_baudrate(UART_NUM_0, 115200);
}

int /*sd_recording_in_progress = 0,*/ sd_ptr = 0, sd_half_ready = 0;
uint32_t *sd_write_buf;//[1024/4];
int is_recording = 0;

//#define COUNT_SKIPPED_BUFFERS

#ifdef COUNT_SKIPPED_BUFFERS
uint32_t SD_buffers_ready = 0; //to check skipped sectors
#endif

const char *WAV_HEADER = {"RIFF\0\0\0\0WAVEfmt\x20\x10\0\0\0\1\0\2\0\x5d\xc6\0\0\x74\x19\3\0\4\0\x10\0data\0\0\0\0"};

//#define SD_RECORDING_DEBUG

void sd_recording(void *pvParameters)
{
	#ifdef SD_RECORDING_DEBUG
	printf("sd_recording(): task running on core ID=%d\n",xPortGetCoreID());
	#endif

	char rec_filename[30]; //typically 24 chars, e.g. "/sdcard/rec/rec00001.wav"

	FILE* f = NULL;
	uint32_t sd_blocks_written = 0;
	#ifdef COUNT_SKIPPED_BUFFERS
	SD_buffers_ready = 0;
	#endif

	if(!sd_card_ready)
    {
		#ifdef SD_RECORDING_DEBUG
		printf("sd_recording(): SD Card init...\n");
	    printf("sd_recording(): before sd_card_init(), free mem = %u\n", xPortGetFreeHeapSize());
		#endif
	    int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
		#ifdef SD_RECORDING_DEBUG
	    printf("sd_recording(): after sd_card_init(), free mem = %u\n", xPortGetFreeHeapSize());
		#endif

	    if(result!=ESP_OK)
    	{
    		printf("sd_recording(): problem with SD Card Init, code=%d\n", result);
    		indicate_error(0x55aa, 8, 80);
    		is_recording = 0;
    	}
    	else
    	{
			create_dir_if_not_exists(REC_SUBDIR);

			uint32_t next_no = sd_get_next_rec_file_number();
			#ifdef SD_RECORDING_DEBUG
			printf("sd_recording(): next file number %u\n",next_no);
			#endif

    		sprintf(rec_filename, "/sdcard/%s/rec%05u.wav", REC_SUBDIR, next_no);
			printf("sd_recording(): Opening file %s for writing\n", rec_filename);

			//t1 = micros();
    	    f = fopen(rec_filename, "w");
    	    if (f == NULL) {
    	        printf("sd_recording(): Failed to open file %s for writing\n", rec_filename);
    	        sd_card_ready = 0;

    	        indicate_error(0x55aa, 8, 80);
        		is_recording = 0;
    	    }
    	    /*else
    	    {
    	    	for(i=0;i<repeat;i++)
    	    	{
    	    		fwrite(buf,buf_size,1,f);
    	    		printf(".");
    	    	}
    	    	fclose(f);
    	    	t2 = micros();
    	    	printf("%d bytes written in %f seconds\n",buf_size*repeat,t2-t1);
    	    	ESP_LOGI(TAG, "File written");

    	    	// All done, unmount partition and disable SDMMC or SPI peripheral
    	    	esp_vfs_fat_sdmmc_unmount();
    	    	//ESP_LOGI(TAG, "Card unmounted");
    	    	sd_card_ready = 0;
    	    }*/

    		//printf("done!\n");
    	}
    }

	if(sd_card_ready)
	{
		#ifdef SD_RECORDING_DEBUG
		printf("sd_recording(): SD card ready!\n");
		#endif

		#ifndef SD_WRITE_BUFFER_PERMANENT
		sd_write_buf = (uint32_t*)malloc(SD_WRITE_BUFFER);
		#endif

		uint8_t *b_ptr;

		//sd_recording_in_progress = 1;
		sd_ptr = 0;
		sd_half_ready = 0;

		//instead of writing the header separately...
		fwrite(WAV_HEADER,WAV_HEADER_SIZE,1,f);

		/*
		//...we will write the whole block, zero padded --> well, this does not work at all for unknown reasons
		memset(sd_write_buf,0xff,SD_WRITE_BUFFER/2); //clear the 1st half of the buffer
		memcpy(sd_write_buf,WAV_HEADER,WAV_HEADER_SIZE);
		b_ptr = (uint8_t*)sd_write_buf;
		fwrite(b_ptr,SD_WRITE_BUFFER/2,1,f);
		sd_blocks_written = 1;
		printf("sd_recording(): header sector written, sd_blocks_written = %d, file pos = %ld\n", sd_blocks_written, ftell(f));
		*/

		is_recording = 1;

		while(is_recording && channel_running)
		{
			if(sd_half_ready)
			{
				b_ptr = ((uint8_t*)sd_write_buf)+(sd_half_ready-1)*(SD_WRITE_BUFFER/2);
				#ifdef SD_RECORDING_DEBUG
				printf("sd_recording(): sd_half_ready = %d, writing ", sd_half_ready);
				#endif
				sd_half_ready = 0; //clear this ASAP

				if(sd_blocks_written)
				{
					fwrite(b_ptr,SD_WRITE_BUFFER/2,1,f);
					#ifdef SD_RECORDING_DEBUG
					printf("%d bytes\n", SD_WRITE_BUFFER/2);
					#endif
				}
				else
				{
					fwrite(b_ptr+WAV_HEADER_SIZE,SD_WRITE_BUFFER/2-WAV_HEADER_SIZE,1,f);
					#ifdef SD_RECORDING_DEBUG
					printf("%d bytes\n", SD_WRITE_BUFFER/2-WAV_HEADER_SIZE);
					#endif
				}

				sd_blocks_written++;

				if((sd_blocks_written/16)%2==0)
				{
					LED_RDY_ON;
				}
				else
				{
					LED_RDY_OFF;
				}

				#ifdef COUNT_SKIPPED_BUFFERS
				printf("rdy:%u - wr:%u = skip:%u\n", SD_buffers_ready, sd_blocks_written, SD_buffers_ready - sd_blocks_written);
				#endif
			}
			else
			{
				vTaskDelay(SD_RECORDING_RESOLUTION);
			}

			/*
			//need to detect channel ending more often
			for(int i=0;i<10;i++)
			{
				Delay(SD_RECORDING_RESOLUTION/10);
				if(!channel_running)
				{
					break;
				}
			}
			*/
		}
		//sd_recording_in_progress = 0;
		is_recording = 0; //if stopped by exiting the channel, need to clear this flag
		LED_RDY_ON;
	}

	uint32_t bytes_written = sd_blocks_written * SD_WRITE_BUFFER/2;
	bytes_written -= WAV_HEADER_SIZE; //the very first block was shorter by WAV header

	printf("sd_recording(): %u blocks written (%u bytes)\n", sd_blocks_written, bytes_written);

	if(f!=NULL)
	{
		if(sd_blocks_written)
		{
			//update the wav header, as explained at http://soundfile.sapp.org/doc/WaveFormat/

			bytes_written += 0x24;

			fseek(f,4,SEEK_SET); //Chunk Size
			fwrite(&bytes_written,4,1,f);

			//update sample rate info
			uint32_t sampl_rate32 = current_sampling_rate; //the variable needs to be 4 bytes
			fseek(f,24,SEEK_SET); //Sample rate
			fwrite(&sampl_rate32,4,1,f);

			bytes_written -= 0x24;

			fseek(f,40,SEEK_SET); //Subchunk 2 Size
			fwrite(&bytes_written,4,1,f);
		}

		#ifdef SD_RECORDING_DEBUG
		printf("sd_recording(): closing the file...\n");
		#endif
		fclose(f);
		//Delay(200);

		#ifndef SD_WRITE_BUFFER_PERMANENT
		free(sd_write_buf);
		#endif
	}
	//t2 = micros();
	//printf("%d bytes written in %f seconds\n",buf_size*repeat,t2-t1);
	//ESP_LOGI(TAG, "File written");

	if(sd_card_ready)
	{
		// All done, unmount partition and disable SDMMC or SPI peripheral
		esp_vfs_fat_sdmmc_unmount();
		//ESP_LOGI(TAG, "Card unmounted");
		sd_card_ready = 0;
	}

	#ifdef SD_RECORDING_DEBUG
	printf("sd_recording(): task done, deleting...\n");
	#endif
	vTaskDelete(NULL);
	#ifdef SD_RECORDING_DEBUG
	printf("sd_recording(): task deleted\n");
	#endif
}

IRAM_ATTR void sd_write_sample(uint32_t *sample32)
{
	if(!is_recording)//sd_recording_in_progress)
	{
		return;
	}

	sd_write_buf[sd_ptr] = sample32[0];
	sd_ptr++;
	if(sd_ptr==SD_WRITE_BUFFER/8)
	{
		while(sd_half_ready) { vTaskDelay(5); }
		sd_half_ready = 1;
		//printf("sd_write_sample(): sd_half_ready = 1\n");
		#ifdef COUNT_SKIPPED_BUFFERS
		SD_buffers_ready++;
		#endif
	}
	else if(sd_ptr==SD_WRITE_BUFFER/4)
	{
		while(sd_half_ready) { vTaskDelay(5); }
		sd_half_ready = 2;
		sd_ptr = 0;
		//printf("sd_write_sample(): sd_half_ready = 2\n");
		#ifdef COUNT_SKIPPED_BUFFERS
		SD_buffers_ready++;
		#endif
	}
}

IRAM_ATTR void sd_write_samples(uint16_t *buffer, int n_bytes)
{
	if(!is_recording)
	{
		return;
	}

	memcpy(sd_write_buf+sd_ptr, buffer, n_bytes);
	sd_ptr += n_bytes/4;//sizeof(uint32_t);
	if(sd_ptr==SD_WRITE_BUFFER/8)
	{
		while(sd_half_ready) { vTaskDelay(5); }
		sd_half_ready = 1;
		//printf("sd_write_sample(): sd_half_ready = 1\n");
		#ifdef COUNT_SKIPPED_BUFFERS
		SD_buffers_ready++;
		#endif
	}
	else if(sd_ptr==SD_WRITE_BUFFER/4)
	{
		while(sd_half_ready) { vTaskDelay(5); }
		sd_half_ready = 2;
		sd_ptr = 0;
		//printf("sd_write_sample(): sd_half_ready = 2\n");
		#ifdef COUNT_SKIPPED_BUFFERS
		SD_buffers_ready++;
		#endif
	}
}

uint32_t sd_get_last_rec_file_number()
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("system_cnt", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("sd_get_last_rec_file_number(): problem with nvs_open(), error = %d\n", res);
		return 0;
	}

	uint32_t val_u32;
	res = nvs_get_u32(handle, "rec_cnt", &val_u32);
	if(res!=ESP_OK) { return 0; } //if the key does not exist, load default value
	nvs_close(handle);
	return val_u32;
}

uint32_t sd_get_next_rec_file_number()
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("system_cnt", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("sd_get_next_rec_file_number(): problem with nvs_open(), error = %d\n", res);
		return 0;
	}

	uint32_t val_u32;
	res = nvs_get_u32(handle, "rec_cnt", &val_u32);
	if(res!=ESP_OK) { val_u32 = 0; } //if the key does not exist, load default value

	val_u32++;

	res = nvs_set_u32(handle, "rec_cnt", val_u32);
	if(res!=ESP_OK)
	{
		printf("sd_get_next_rec_file_number(): problem with nvs_set_u32(), error = %d\n", res);
		return 0;
	}

	nvs_close(handle);
	return val_u32;
}

uint32_t sd_get_current_rec_file_number()
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("system_cnt", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("sd_get_current_rec_file_number(): problem with nvs_open(), error = %d\n", res);
		return 0;
	}

	uint32_t val_u32;
	res = nvs_get_u32(handle, "rec_cnt", &val_u32);
	if(res!=ESP_OK) { val_u32 = 0; } //if the key does not exist, load default value

	nvs_close(handle);
	return val_u32;
}

void sd_set_current_rec_file_number(int current_fn)
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("system_cnt", NVS_READWRITE, &handle);
	if(res!=ESP_OK)
	{
		printf("sd_set_next_rec_file_number(): problem with nvs_open(), error = %d\n", res);
		return;
	}

	res = nvs_set_u32(handle, "rec_cnt", current_fn);
	if(res!=ESP_OK)
	{
		printf("sd_set_next_rec_file_number(): problem with nvs_set_u32(), error = %d\n", res);
		//return;
	}

	nvs_close(handle);
}

void start_stop_recording()
{
	if(!channel_running || sd_playback_channel)
	{
		printf("start_stop_recording(): channel not running or it is a SD playback\n");
		return;
	}
	printf("start_stop_recording(): is_recording = %d\n", is_recording);
	if(!is_recording)
	{
		BaseType_t result = xTaskCreatePinnedToCore((TaskFunction_t)&sd_recording, "sd_recording_task", /*2048*2*/1024*3, NULL, PRIORITY_SD_RECORDING_TASK, NULL, CPU_CORE_SD_RECORDING_TASK);
		if(result != pdPASS)
		{
			printf("xTaskCreatePinnedToCore(): failed to create task, error = %d\n", result);
		}

	}
	else
	{
		is_recording = 0;
	}
}

extern int mixed_sample_buffer_ptr_L, mixed_sample_buffer_ptr_R;
extern int16_t *mixed_sample_buffer;
extern int MIXED_SAMPLE_BUFFER_LENGTH;
extern float mixed_sample_volume_coef_ext;

void use_recorded_sample()
{
	if(!channel_running || sd_playback_channel)
	{
		printf("use_recorded_sample(): channel not running or it is a SD playback\n");
		return;
	}
	if(is_recording)
	{
		printf("use_recorded_sample(): currently recording, cannot use recorded samples\n");
		return;
	}
	printf("use_recorded_sample(): OK\n");

	load_SD_card_sample = SD_CARD_SAMPLE_OPENING;
}

extern spi_flash_mmap_handle_t mmap_handle_samples;

int load_recorded_sample()
{
	char rec_filename[50];
	uint32_t last_no;
	get_recording_filename(rec_filename, SD_RECORDING_LAST, &last_no);
	printf("load_recorded_sample(): opening file %s\n", rec_filename);

	int sd_open_res = sd_card_open_file(rec_filename);

	if(sd_open_res != ESP_OK)
	{
		printf("load_recorded_sample(): problem opening file %s\n", rec_filename);
		return SD_CARD_SAMPLE_NOT_USED;
	}

	if(mmap_handle_samples)//!=NULL)
	{
		printf("spi_flash_munmap(mmap_handle_samples) ... ");
		spi_flash_munmap(mmap_handle_samples);
		mmap_handle_samples = 0;//NULL;
		printf("unmapped!\n");
	}

	mixed_sample_buffer = (int16_t*)malloc(SD_PLAYBACK_BUFFER*2); //allocate two 4kB buffers for double buffering

	mixed_sample_buffer_ptr_R = 0;
	mixed_sample_buffer_ptr_L = 1; //files on SD are in stereo, every 2nd sample is the other channel

	mixed_sample_volume_coef_ext *= 2; //boost the sample volume up

	MIXED_SAMPLE_BUFFER_LENGTH = SD_PLAYBACK_BUFFER; //length of double buffer in 16-bit words
	printf("load_recorded_sample(): Buffer allocated at 0x%x, length = %d samples\n", (unsigned int)mixed_sample_buffer, MIXED_SAMPLE_BUFFER_LENGTH);

	/*
	while(!feof(SD_f) && !event_next_channel
	//printf("channel_SD_playback(): loading buffer #%d\n", sd_blocks_read);
	fread(SD_play_buf,SD_PLAYBACK_BUFFER,1,SD_f);
	sd_blocks_read++;
	i2s_write_bytes(I2S_NUM, (char *)SD_play_buf, SD_PLAYBACK_BUFFER, portMAX_DELAY);

	fclose(SD_f);
	free(SD_play_buf);
	*/

	BaseType_t result = xTaskCreatePinnedToCore((TaskFunction_t)&load_next_sd_sample_sector, "load_sd_sector", 2048, NULL, PRIORITY_SD_SECTOR_LOAD_TASK, NULL, CPU_CORE_SD_SECTOR_LOAD_TASK);
	if(result != pdPASS)
	{
		printf("xTaskCreatePinnedToCore(): failed to create task, error = %d\n", result);
		return SD_CARD_SAMPLE_NOT_USED;
	}

	return SD_CARD_SAMPLE_OPENED;
}

void unload_recorded_sample()
{
	printf("unload_recorded_sample()\n");
	if(load_SD_card_sample)
	{
		printf("unload_recorded_sample(): load_SD_card_sample -> SD_CARD_SAMPLE_NOT_USED\n");
		load_SD_card_sample = SD_CARD_SAMPLE_NOT_USED;
		Delay(100); //so there is time for the task to close
	}
}

void load_next_sd_sample_sector(void *pvParameters)
{
	int16_t minL, maxL, minR, maxR, cnt;
	printf("load_next_recorded_sample_sector(): task running on core ID=%d\n",xPortGetCoreID());

	if(mixed_sample_buffer==NULL)
	{
		printf("load_next_recorded_sample_sector(): mixed_sample_buffer not allocated!\n");
	}
	else
	{
		while(load_SD_card_sample) //unless SD_CARD_SAMPLE_NOT_USED
		{
			if(SD_card_sample_buffer_half)
			{
				//printf("load_next_recorded_sample_sector(): loading SD_card_sample_buffer #%d\n", SD_card_sample_buffer_half);

				if(feof(SD_f)) //rewind back
				{
					//fseek(SD_f,44,SEEK_SET); //skip the header -> not a good idea, will throw off 4kb sector reads
					fseek(SD_f,0,SEEK_SET);
				}

				fread((char*)mixed_sample_buffer+SD_PLAYBACK_BUFFER*(SD_card_sample_buffer_half-1),SD_PLAYBACK_BUFFER,1,SD_f);

				minL = 32000;
				maxL = -32000;
				minR = 32000;
				maxR = -32000;
				cnt = 0;

				for(int i=SD_PLAYBACK_BUFFER*(SD_card_sample_buffer_half-1)/2;i<SD_PLAYBACK_BUFFER*SD_card_sample_buffer_half/2;i+=2)
				{
					if(mixed_sample_buffer[i]<minL)
					{
						minL = mixed_sample_buffer[i];
					}
					else if(mixed_sample_buffer[i]>maxL)
					{
						maxL = mixed_sample_buffer[i];
					}
					if(mixed_sample_buffer[i+1]<minR)
					{
						minR = mixed_sample_buffer[i+1];
					}
					else if(mixed_sample_buffer[i+1]>maxR)
					{
						maxR = mixed_sample_buffer[i+1];
					}
					cnt++;
				}
				printf("SD sample peaks: %d/%d L, %d/%d R out of %d samples\n", minL, maxL, minR, maxR, cnt);

				//sample_f[0] += ( (float)(32768 - (int16_t)mixed_sample_buffer[mixed_sample_buffer_ptr_L++]) / 32768.0f) * mixed_sample_volume;

				SD_card_sample_buffer_half = 0;
			}
			vTaskDelay(SD_SAMPLE_PLAY_TIMING);
		}
	}

	fclose(SD_f);
	printf("load_next_recorded_sample_sector(): SD file closed\n");
	free(mixed_sample_buffer);
	printf("load_next_recorded_sample_sector(): mixed_sample_buffer memory released\n");
	mixed_sample_buffer=NULL;

	printf("load_next_recorded_sample_sector(): task done, deleting...\n");
	vTaskDelete(NULL);
	printf("load_next_recorded_sample_sector(): task deleted\n");
}

int get_recording_filename(char *rec_filename, int recording, uint32_t *last_no)
{
	last_no[0] = sd_get_last_rec_file_number();
    //printf("get_recording_filename(): last file number %u\n",last_no[0]);

    if(recording == SD_RECORDING_LAST)
	{
	    sprintf(rec_filename, "%s/rec%05u.wav", REC_SUBDIR, last_no[0]);
		//printf("get_recording_filename(): file = %s\n", rec_filename);

		return last_no[0];
	}

    sprintf(rec_filename, "%s/rec%05u.wav", REC_SUBDIR, recording);
	//printf("get_recording_filename(): file = %s\n", rec_filename);
    return recording;
}

void channel_SD_playback(int recording, int slicer)
{
	char rec_filename[50];
	int result = SD_CARD_PLAY_RESULT_UNDEFINED;
	uint32_t last_no;
	uint8_t browsing_direction = SD_CARD_PLAY_RESULT_NEXT_FILE;

	//ui_ignore_events = 1; //disable context menu
	ui_button3_enabled = 0;
	ui_button4_enabled = 0;
	context_menu_enabled = 0;

	while(result!=SD_CARD_PLAY_RESULT_EXIT)
	{
		//printf("channel_SD_playback() loop: recording = %d, slicer = %d, result = %d\n", recording, slicer, result);
		if(recording==SD_RECORDING_LAST)
		{
			browsing_direction = SD_CARD_PLAY_RESULT_PREVIOUS_FILE; //browse backwards
		}

		next_event_lock = 1;

		recording = get_recording_filename(rec_filename, recording, &last_no);

		if(slicer)
		{
			//play back segments from most recent recorded file in a randomized chopped way
			result = sd_card_slicer(rec_filename, 0); //auto-detect the sampling rate
		}
		else
		{
			result = sd_card_play(rec_filename, 0); //auto-detect the sampling rate
		}

		if(result == SD_CARD_PLAY_RESULT_ERROR)
		{
			//skip the file, keep browsing in the direction
			result = browsing_direction;
			//printf("channel_SD_playback(): exited with error, setting direction to %d\n", browsing_direction);
		}

		if(result == SD_CARD_PLAY_RESULT_PREVIOUS_FILE)
		{
			browsing_direction = SD_CARD_PLAY_RESULT_PREVIOUS_FILE; //browsing backwards
			//printf("channel_SD_playback(): exited with selecting PREVIOUS file, setting direction to %d\n", browsing_direction);

			if(recording==1)
			{
				recording = last_no;
			}
			else
			{
				recording --;
			}
		}
		else if(result == SD_CARD_PLAY_RESULT_NEXT_FILE)
		{
			browsing_direction = SD_CARD_PLAY_RESULT_NEXT_FILE; //browsing forward
			//printf("channel_SD_playback(): exited with selecting NEXT file, setting direction to %d\n", browsing_direction);

			if(recording == last_no)
			{
				recording = 1;
			}
			else
			{
				recording++;
			}
		}
	}

	volume_ramp = 1; //to not store new volume level again later
}

int sd_card_open_file(char *file_path)
{
	char filename[50];

	if(!sd_card_ready)
	{
		//printf("sd_card_open_file(): SD Card init...\n");
		int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
	    if(result!=ESP_OK)
	    {
	    	printf("sd_card_open_file(): problem with SD Card Init, code=%d\n", result);
	    	indicate_error(0x55aa, 8, 80);
	    	return SD_CARD_PLAY_RESULT_ERROR;
	    }
		//printf("done!\n");
	}

	sprintf(filename, "/sdcard/%s", file_path);
	printf("sd_card_open_file(): Opening file %s for reading\n", filename);

	SD_f = fopen(filename, "rb");
	if (SD_f == NULL) {
		printf("sd_card_open_file(): Failed to open file %s for reading\n", filename);
		return SD_CARD_PLAY_RESULT_ERROR;
	}
	//fseek(SD_f,44,SEEK_SET); //skip the header -> not a good idea, will throw off 4kb sencor reads
	return ESP_OK;
}

int sd_card_play(char *file_path, int sampling_rate)
{
	unsigned int sd_blocks_read = 0;
	int ret = SD_CARD_PLAY_RESULT_EXIT;

	int sd_open_res = sd_card_open_file(file_path);

	if(sd_open_res != ESP_OK)
	{
		ret = sd_open_res;
	}
	else
	{
		SD_play_buf = (uint32_t*)malloc(SD_PLAYBACK_BUFFER); //only allocate as many bytes as SD_PLAYBACK_BUFFER but in a 32-bit structure

		fseek(SD_f,0,SEEK_END);
		unsigned long filesize = ftell(SD_f);
		fseek(SD_f,0,SEEK_SET);

		if(filesize<1024)
		{
			printf("sd_card_play(): file seems corrupt\n");
			ret = SD_CARD_PLAY_RESULT_ERROR;
		}
		else
		{
			if(sampling_rate==0) //if sapling rate auto-detect from wav file enabled
			{
				//read sample rate info
				fseek(SD_f,24,SEEK_SET); //Sample rate
				uint32_t sampl_rate32;
				fread(&sampl_rate32,sizeof(sampl_rate32),1,SD_f);
				fseek(SD_f,0,SEEK_SET);

				sampling_rate = sampl_rate32;

				if(!is_valid_sampling_rate(sampling_rate))
				{
					sampling_rate = persistent_settings.SAMPLING_RATE;
				}
			}

			if(sampling_rate!=current_sampling_rate)
			{
				set_sampling_rate(sampling_rate);
			}

			printf("sd_card_play(): playback loop starting\n");

			channel_running = 1;
			sd_playback_channel = 1; //to disallow recording from this channel

			volume_ramp = 0;
			//instead of volume ramp, set the codec volume instantly to un-mute the codec
			codec_digital_volume=codec_volume_user;
			codec_set_digital_volume();
			//printf("sd_card_play(): codec unmuted\n");

			while(!feof(SD_f) && !event_next_channel
				&& btn_event_ext!=BUTTON_EVENT_SHORT_PRESS+BUTTON_1
				&& btn_event_ext!=BUTTON_EVENT_SHORT_PRESS+BUTTON_2)
			{
				//printf("channel_SD_playback(): loading buffer #%d\n", sd_blocks_read);

				fread(SD_play_buf,SD_PLAYBACK_BUFFER,1,SD_f);

				if(!sd_blocks_read) //if the very first sector...
				{
					memset(SD_play_buf,0,WAV_HEADER_SIZE); //...overwrite the header with zeroes
				}

				sd_blocks_read++;
				//i2s_write_bytes(I2S_NUM, (char *)SD_play_buf, SD_PLAYBACK_BUFFER, portMAX_DELAY);
				i2s_write(I2S_NUM, (void*)SD_play_buf, SD_PLAYBACK_BUFFER, &i2s_bytes_rw, portMAX_DELAY);
			}

			if(!event_next_channel)
			{
				printf("sd_card_play(): event_next_channel\n");
				volume_ramp = 1; //to not store new volume level again
			}
			codec_set_mute(1); //mute the codec
			//printf("sd_card_play(): codec muted\n");
			channel_running = 0;
			volume_ramp = 0;
		}

		fclose(SD_f);
		free(SD_play_buf);

		//reset sampling rate back to user-defined value
		if(current_sampling_rate!=persistent_settings.SAMPLING_RATE)
		{
			set_sampling_rate(persistent_settings.SAMPLING_RATE);
		}
	}

	//printf("sd_card_play(): total of %d buffers read\n", sd_blocks_read);

	// All done, unmount partition and disable SDMMC or SPI peripheral
	esp_vfs_fat_sdmmc_unmount();
	//ESP_LOGI(TAG, "Card unmounted");
	sd_card_ready = 0;

	if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1)
	{
		ret = SD_CARD_PLAY_RESULT_PREVIOUS_FILE;
	}
	else if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2)
	{
		ret = SD_CARD_PLAY_RESULT_NEXT_FILE;
	}

	btn_event_ext = 0;
	event_next_channel = 0;

	return ret;
}

int sd_card_write_file(char *filename, char *buffer, int length)
{
	printf("sd_card_write_file(): storing file %s of size %d\n",filename,length);

	char fn[50];
	FILE* f = NULL;

	if(!sd_card_ready)
	{
	    printf("sd_card_write_file(): SD Card init...\n");
		int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
		if(result!=ESP_OK)
		{
			printf("sd_card_write_file(): problem with SD Card Init, code=%d\n", result);
	    	indicate_error(0x55aa, 8, 80);
	    	return -1;
		}
	}

	int ret = 0;

	if(sd_card_ready)
	{
	    sprintf(fn, "/sdcard/%s", filename);
		printf("sd_card_write_file(): Opening file %s for writing\n", fn);

		f = fopen(fn, "w");
		if (f == NULL) {
			printf("sd_card_write_file(): Failed to open file %s for writing\n", fn);
			sd_card_ready = 0;
	    	indicate_error(0x55aa, 8, 80);
	    	ret = -2;
		}
		else
		{
			printf("sd_card_write_file(): writing data...\n");
			ret = fwrite(buffer,length,1,f);
			printf("sd_card_write_file(): closing the file...\n");
			fclose(f);
	    }

		// All done, unmount partition and disable SDMMC or SPI peripheral
		esp_vfs_fat_sdmmc_unmount();
		//ESP_LOGI(TAG, "Card unmounted");
		sd_card_ready = 0;
	}

	return ret;
}


void create_dir_if_not_exists(const char *dir)
{
	char dirname[50];
	sprintf(dirname, "/sdcard/%s", dir);
	//printf("create_dir_if_not_exists(): Creating directory %s\n", dirname);
	/*FRESULT res =*/ mkdir(dirname, 0755);
	//printf("create_dir_if_not_exists(): mkdir() result = %d\n", res);
}

uint32_t get_file_size(const char *filename)
{
	if(!sd_card_ready)
	{
		printf("get_file_size(%s): SD card not ready!\n", filename);
		return 0;
	}
	FILE *f = fopen(filename, "r");
	if (f == NULL) {
		printf("get_file_size(): Failed to open file %s for reading\n", filename);
		return 0;
	}
	fseek(f,0,SEEK_END);
	uint32_t filesize = ftell(f);
	fclose(f);
	return filesize;
}

void sd_slicer_generate_segments(unsigned long *segments, int n_seg, unsigned long filesize)
{
	for(int i=0;i<n_seg;i++)
	{
		segments[i] = (PseudoRNG1a_next_float_new()+0.5) * (filesize-50000); //safety margin to not get over the EOF
		segments[i] &= 0xffffff00;
	}
}

void sd_slicer_shift_segment(unsigned long *segments, int seg_ptr, int direction_and_step, unsigned long filesize)
{
	printf("sd_slicer_shift_segment(segment=%d): shift by %d sectors, %lu => ", seg_ptr, direction_and_step, segments[seg_ptr]);
	segments[seg_ptr] += SD_PLAYBACK_BUFFER * direction_and_step;
	printf("%lu\n", segments[seg_ptr]);

	if(segments[seg_ptr]>filesize-50000)
	{
		segments[seg_ptr] = filesize-50000;
	}
	segments[seg_ptr] &= 0xffffff00;

	fseek(SD_f,segments[seg_ptr],SEEK_SET);
}


int sd_card_slicer(char *file_path, int sampling_rate)
{
	unsigned int sd_blocks_read = 0;
	int ret = SD_CARD_PLAY_RESULT_EXIT;

	#define SD_SLICER_MIN_SEGMENTS			1	//segments per loop
	#define SD_SLICER_MAX_SEGMENTS			64
	#define SD_SLICER_DEFAULT_SEGMENTS		8

	#define SD_SLICER_MIN_SEG_LENGTH		1	//length in 64th of a second at basic speed setting
	#define SD_SLICER_MAX_SEG_LENGTH		64	//one full second
	#define SD_SLICER_DEFAULT_SEG_LENGTH	16	//one fourth of a second

	int sd_slicer_segments = SD_SLICER_DEFAULT_SEGMENTS, sd_slicer_seg_length = SD_SLICER_DEFAULT_SEG_LENGTH;

	unsigned long segments[SD_SLICER_MAX_SEGMENTS];
	long int filesize;
	int seg_ptr = 0, seg_length, i, j;

	//in 4kB sectors, or 1024 samples
	#define SEGMENT_SHIFT_COARSE	32
	#define SEGMENT_SHIFT_FINE		8

	int segments_reduction_by_sensor = 0, oversample_by_sensor = 0;
	float seg_length_reduction_by_sensor = 1.0f;

	float s_lpf[4] = {0,0,0,0};
	#define S_LPF_ALPHA	0.06f

	#define MIDI_OVERRIDE_NONE		0
	#define MIDI_OVERRIDE_TEMPO		1
	#define MIDI_OVERRIDE_KEYS		2

	int midi_override = MIDI_OVERRIDE_NONE, midi_next_segment = 0;
	MIDI_parser_reset();

	sampleCounter = 0;

	int sd_open_res = sd_card_open_file(file_path);

	if(sd_open_res != ESP_OK)
	{
		ret = sd_open_res;
	}
	else
	{
		SD_play_buf = (uint32_t*)malloc(SD_PLAYBACK_BUFFER); //only allocate as many bytes as SD_PLAYBACK_BUFFER but in a 32-bit structure

		fseek(SD_f,0,SEEK_END);
		filesize = ftell(SD_f);
		fseek(SD_f,0,SEEK_SET);

		if(filesize<1024)
		{
			printf("sd_card_slicer(): file seems corrupt\n");
			ret = SD_CARD_PLAY_RESULT_ERROR;
		}
		else
		{
			if(sampling_rate==0) //if sapling rate auto-detect from wav file enabled
			{
				//read sample rate info
				fseek(SD_f,24,SEEK_SET); //Sample rate
				uint32_t sampl_rate32;
				fread(&sampl_rate32,sizeof(sampl_rate32),1,SD_f);
				fseek(SD_f,0,SEEK_SET);

				sampling_rate = sampl_rate32;

				if(!is_valid_sampling_rate(sampling_rate))
				{
					sampling_rate = persistent_settings.SAMPLING_RATE;
				}
			}

			if(sampling_rate!=current_sampling_rate)
			{
				set_sampling_rate(sampling_rate);
			}

			//printf("sd_card_slicer(): generating segments\n");

			seg_length = sampling_rate / 64 * sd_slicer_seg_length; //length in samples (L+R)

			sd_slicer_generate_segments(segments, SD_SLICER_MAX_SEGMENTS, filesize);
			printf("sd_card_slicer(): file size = %lu, segments = %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n", filesize, segments[0], segments[1], segments[2], segments[3], segments[4], segments[5], segments[6], segments[7]);

			//printf("sd_card_slicer(): chopped playback starting\n");

			channel_running = 1;
			sd_playback_channel = 1; //to disallow recording from this channel

			sensors_active = 1;

			volume_ramp = 0;
			//instead of volume ramp, set the codec volume instantly to un-mute the codec
			codec_digital_volume=codec_volume_user;
			codec_set_digital_volume();

			LED_R8_set_byte(0x01);
			LED_O4_set_byte(0x01);

			while(!feof(SD_f) && !event_next_channel
				//&& btn_event_ext!=BUTTON_EVENT_SHORT_PRESS+BUTTON_1
				//&& btn_event_ext!=BUTTON_EVENT_SHORT_PRESS+BUTTON_2)
				&& btn_event_ext!=BUTTON_EVENT_SET_PLUS+BUTTON_1
				&& btn_event_ext!=BUTTON_EVENT_SET_PLUS+BUTTON_2)
			{
				//printf("channel_SD_playback(): loading buffer #%d\n", sd_blocks_read);
				fread(SD_play_buf,SD_PLAYBACK_BUFFER,1,SD_f);
				sd_blocks_read++;
				if(oversample_by_sensor==0)
				{
					//i2s_write_bytes(I2S_NUM, (char *)SD_play_buf, SD_PLAYBACK_BUFFER, portMAX_DELAY);
					i2s_write(I2S_NUM, (void*)SD_play_buf, SD_PLAYBACK_BUFFER, &i2s_bytes_rw, portMAX_DELAY);
				}
				else if(oversample_by_sensor>0)
				{
					for(i=0;i<SD_PLAYBACK_BUFFER;i+=4)
					{
						for(j=0;j<=oversample_by_sensor;j++)
						{
							//i2s_write_bytes(I2S_NUM, ((char *)SD_play_buf)+i, 4, portMAX_DELAY);
							i2s_write(I2S_NUM, ((void*)SD_play_buf)+i, 4, &i2s_bytes_rw, portMAX_DELAY);
						}
					}
				}
				else if(oversample_by_sensor<0)
				{
					for(i=0;i<SD_PLAYBACK_BUFFER;i+=(4*(-oversample_by_sensor+1)))
					{
						//i2s_write_bytes(I2S_NUM, ((char *)SD_play_buf)+i, 4, portMAX_DELAY);
						i2s_write(I2S_NUM, ((void*)SD_play_buf)+i, 4, &i2s_bytes_rw, portMAX_DELAY);
					}
				}

				if(feof(SD_f))
				{
					printf("sd_card_slicer(): EOF, rewind\n");
					fseek(SD_f,WAV_HEADER_SIZE,SEEK_SET);
				}

				//one buffer = 4096 Bytes = 1024 stereo samples
				sampleCounter += 1024;

				for(i=0;i<4;i++)
				{
					s_lpf[i] = s_lpf[i] + S_LPF_ALPHA * (ir_res[i] - s_lpf[i]);
				}

				if((!midi_override && sampleCounter > (uint16_t)((float)seg_length*seg_length_reduction_by_sensor)) || midi_next_segment || MIDI_note_on)
				{
					sampleCounter = 0;

					if(MIDI_note_on)
					{
						MIDI_note_on = 0;

						seg_ptr = MIDI_last_chord[0] % sd_slicer_segments;
						//printf("sd_card_slicer(): segment %d by MIDI note %d\n", seg_ptr, MIDI_last_chord[0]);
					}
					else
					{
						seg_ptr++;

						if(!segments_reduction_by_sensor)
						{
							if(seg_ptr>=sd_slicer_segments)
							{
								seg_ptr = 0;
							}
						}
						else
						{
							if(sd_slicer_segments<=4)
							{
								if(seg_ptr>sd_slicer_segments-segments_reduction_by_sensor)
								{
									seg_ptr = 0;
								}
							}
							else
							{
								//printf("seg_ptr = %d, sd_slicer_segments = %d, segments_reduction_by_sensor = %d\n", seg_ptr, sd_slicer_segments, segments_reduction_by_sensor);
								if(segments_reduction_by_sensor==1 && seg_ptr>=sd_slicer_segments/2) { seg_ptr = 0; }
								if(segments_reduction_by_sensor==2 && seg_ptr>=sd_slicer_segments/4) { seg_ptr = 0; }
								if(segments_reduction_by_sensor==3 && seg_ptr>=sd_slicer_segments/8) { seg_ptr = 0; }
								if(segments_reduction_by_sensor==4 && seg_ptr>=sd_slicer_segments/16) { seg_ptr = 0; }
							}
						}
					}
					//printf("sd_card_slicer(): loading segment #%d, position = %lu\n", seg_ptr, segments[seg_ptr]);
					fseek(SD_f,segments[seg_ptr],SEEK_SET);

					if(!SENSOR_THRESHOLD_RED_1)
					{
						LED_R8_set_byte(1<<(seg_ptr%8));
					}
					if(!SENSOR_THRESHOLD_ORANGE_1)
					{
						if(seg_ptr<32)
						{
							LED_O4_set_byte(1<<(seg_ptr/8));
						}
						else
						{
							//LED_O4_set_byte(0xef>>(8-seg_ptr/8));
							LED_O4_set_byte(0x38>>(8-seg_ptr/8));
						}
					}

					midi_next_segment = 0;
				}

				if(SENSOR_THRESHOLD_ORANGE_1)
				{
					if(SENSOR_THRESHOLD_ORANGE_4) { segments_reduction_by_sensor = 4; }
					else if(SENSOR_THRESHOLD_ORANGE_3) { segments_reduction_by_sensor = 3; }
					else if(SENSOR_THRESHOLD_ORANGE_2) { segments_reduction_by_sensor = 2; }
					else segments_reduction_by_sensor = 1;
				}
				else { segments_reduction_by_sensor = 0; }

				if(SENSOR_THRESHOLD_RED_1)
				{
					//seg_length_reduction_by_sensor = 1 - ir_res[0];
					seg_length_reduction_by_sensor = 1 - s_lpf[0];
				}
				else
				{
					seg_length_reduction_by_sensor = 1.0f;
				}

				if(SENSOR_THRESHOLD_BLUE_5)
				{
					if(SENSOR_THRESHOLD_BLUE_1) { oversample_by_sensor = 3; }
					else if(SENSOR_THRESHOLD_BLUE_2) { oversample_by_sensor = 2; }
					else if(SENSOR_THRESHOLD_BLUE_3) { oversample_by_sensor = 2; }
					else if(SENSOR_THRESHOLD_BLUE_4) { oversample_by_sensor = 1; }
					else oversample_by_sensor = 1;
				}
				else {

					if(SENSOR_THRESHOLD_WHITE_8)
					{
						if(SENSOR_THRESHOLD_WHITE_1) { oversample_by_sensor = -4; }
						else if(SENSOR_THRESHOLD_WHITE_2) { oversample_by_sensor = -4; }
						else if(SENSOR_THRESHOLD_WHITE_3) { oversample_by_sensor = -3; }
						else if(SENSOR_THRESHOLD_WHITE_4) { oversample_by_sensor = -3; }
						else if(SENSOR_THRESHOLD_WHITE_5) { oversample_by_sensor = -2; }
						else if(SENSOR_THRESHOLD_WHITE_6) { oversample_by_sensor = -2; }
						else if(SENSOR_THRESHOLD_WHITE_7) { oversample_by_sensor = -1; }
						else oversample_by_sensor = -1;
					}
					else { oversample_by_sensor = 0; }
				}

				/*
				if(btn_event_ext)
				{
					printf("sd_card_slicer(): btn_event_ext = %d\n", btn_event_ext);
				}
				*/
				//if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_SET)
				if(event_channel_options)
				{
					event_channel_options = 0;
					//btn_event_ext = 0;
					sd_slicer_generate_segments(segments, SD_SLICER_MAX_SEGMENTS, filesize);
					printf("sd_card_slicer(): file size = %lu, segments = %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n", filesize, segments[0], segments[1], segments[2], segments[3], segments[4], segments[5], segments[6], segments[7]);
					seg_ptr = 0;
					LED_R8_set_byte(0x01);
					LED_O4_set_byte(0x01);
				}

				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_1)
				{
					btn_event_ext = 0;

					if(sd_slicer_segments>SD_SLICER_MIN_SEGMENTS)
					{
						sd_slicer_segments /= 2;
					}

					seg_length = sampling_rate / 64 * sd_slicer_seg_length; //length in samples (L+R)
					printf("sd_card_slicer(): segments decreased to %d, length = %d\n", sd_slicer_segments, seg_length);
					//seg_ptr = 0;
				}

				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_2)
				{
					btn_event_ext = 0;

					if(sd_slicer_segments<SD_SLICER_MAX_SEGMENTS)
					{
						sd_slicer_segments *= 2;
					}

					seg_length = sampling_rate / 64 * sd_slicer_seg_length; //length in samples (L+R)
					printf("sd_card_slicer(): segments increased to %d, length = %d\n", sd_slicer_segments, seg_length);
					//seg_ptr = 0;
				}

				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_3)
				{
					btn_event_ext = 0;

					if(!midi_override)
					{
						if(sd_slicer_seg_length>SD_SLICER_MIN_SEG_LENGTH)
						{
							sd_slicer_seg_length /= 2;
						}

						seg_length = sampling_rate / 64 * sd_slicer_seg_length; //length in samples (L+R)
						printf("sd_card_slicer(): segments decreased to %d, length = %d\n", sd_slicer_segments, seg_length);
						//seg_ptr = 0;
					}
					else
					{
						/*
						segments[seg_ptr] -= 4096 * SEGMENT_SHIFT_COARSE; //shift by x sectors

						if(segments[seg_ptr]>filesize-50000)
						{
							segments[seg_ptr] = filesize-50000;
						}
						segments[seg_ptr] &= 0xffffff00;

						fseek(SD_f,segments[seg_ptr],SEEK_SET);
						*/
						sd_slicer_shift_segment(segments, seg_ptr, -SEGMENT_SHIFT_COARSE, filesize);
					}
				}

				if(btn_event_ext==BUTTON_EVENT_SHORT_PRESS+BUTTON_4)
				{
					btn_event_ext = 0;

					if(!midi_override)
					{
						if(sd_slicer_seg_length<SD_SLICER_MAX_SEG_LENGTH)
						{
							sd_slicer_seg_length *= 2;
						}

						seg_length = sampling_rate / 64 * sd_slicer_seg_length; //length in samples (L+R)
						printf("sd_card_slicer(): segments increased to %d, length = %d\n", sd_slicer_segments, seg_length);
						//seg_ptr = 0;
					}
					else
					{
						/*
						segments[seg_ptr] += 4096 * SEGMENT_SHIFT_COARSE; //shift by x sectors

						if(segments[seg_ptr]>filesize-50000)
						{
							segments[seg_ptr] = filesize-50000;
						}
						segments[seg_ptr] &= 0xffffff00;

						fseek(SD_f,segments[seg_ptr],SEEK_SET);
						*/
						sd_slicer_shift_segment(segments, seg_ptr, SEGMENT_SHIFT_COARSE, filesize);
					}
				}

				if(btn_event_ext==BUTTON_EVENT_SET_PLUS+BUTTON_3)
				{
					btn_event_ext = 0;

					if(midi_override)
					{
						sd_slicer_shift_segment(segments, seg_ptr, -SEGMENT_SHIFT_FINE, filesize);
					}
				}

				if(btn_event_ext==BUTTON_EVENT_SET_PLUS+BUTTON_4)
				{
					btn_event_ext = 0;

					if(midi_override)
					{
						sd_slicer_shift_segment(segments, seg_ptr, SEGMENT_SHIFT_FINE, filesize);
					}
				}

				if(midi_override!=MIDI_OVERRIDE_KEYS && MIDI_keys_pressed)
				{
					printf("sd_card_slicer: MIDI keys active, will receive notes\n");
					midi_override = MIDI_OVERRIDE_KEYS;
				}
				if(!midi_override && MIDI_tempo_event)
				{
					printf("sd_card_slicer: MIDI clock active, will receive tempo\n");
					midi_override = MIDI_OVERRIDE_TEMPO;
				}
				if(midi_override==MIDI_OVERRIDE_TEMPO && MIDI_tempo_event)
				{
					MIDI_tempo_event = 0;
					midi_next_segment = 1;
					//printf("sd_card_slicer(): MIDI_tempo_event -> midi_next_segment\n");
				}
			}

			if(!event_next_channel)
			{
				volume_ramp = 1; //to not store new volume level again
			}
			codec_set_mute(1); //mute the codec
			channel_running = 0;
			volume_ramp = 0;
		}

		fclose(SD_f);
		free(SD_play_buf);

		//reset sampling rate back to user-defined value
		if(current_sampling_rate!=persistent_settings.SAMPLING_RATE)
		{
			set_sampling_rate(persistent_settings.SAMPLING_RATE);
		}
	}

	//printf("sd_card_play(): total of %d buffers read\n", sd_blocks_read);

	// All done, unmount partition and disable SDMMC or SPI peripheral
	esp_vfs_fat_sdmmc_unmount();
	//ESP_LOGI(TAG, "Card unmounted");
	sd_card_ready = 0;

	if(btn_event_ext==BUTTON_EVENT_SET_PLUS+BUTTON_1)
	{
		ret = SD_CARD_PLAY_RESULT_PREVIOUS_FILE;
	}
	else if(btn_event_ext==BUTTON_EVENT_SET_PLUS+BUTTON_2)
	{
		ret = SD_CARD_PLAY_RESULT_NEXT_FILE;
	}
	else if(event_next_channel)
	{
		ret = SD_CARD_PLAY_RESULT_EXIT;
	}

	btn_event_ext = 0;
	event_next_channel = 0;

	return ret;
}

void verify_sd_counter_overlap() //prevents overwriting existing wav files if counter set too low (i.e. card swapped from another unit)
{
    printf("verify_sd_counter_overlap(): free mem [begin] = %u\n", xPortGetFreeHeapSize());

    if(!sd_card_ready)
    {
    	printf("verify_sd_counter_overlap(): SD Card init...\n");
    	int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
    	if(result!=ESP_OK)
    	{
    		printf("verify_sd_counter_overlap(): problem with SD Card Init, code=%d\n", result);
    		sd_card_present = SD_CARD_PRESENT_NONE;
    		return;
    	}
    }

    char **files_list = malloc(200 * sizeof(char*));
	int files_found = 0;
    sd_card_file_list("rec", 0, files_list, &files_found, 0); //non recursive, do not add dirs

    int f_n, max_fn = 0;
    printf("verify_sd_counter_overlap(): found %d files\n", files_found);
	for(int i=0;i<files_found;i++)
	{
		f_n = atoi((files_list[i])+3);
		if(f_n > max_fn)
		{
			max_fn = f_n;
		}
		printf("%s - %d\n", files_list[i], f_n);
		free(files_list[i]);
	}
	free(files_list);

	uint32_t current_rfn = sd_get_current_rec_file_number();

	printf("verify_sd_counter_overlap(): highest WAV file number found = %d, current WAV recording file number = %d\n", max_fn, current_rfn);

	if(current_rfn<max_fn)
	{
		printf("verify_sd_counter_overlap(): current_rfn < max_fn => setting current WAV recording file number to %d\n", max_fn);
		sd_set_current_rec_file_number(max_fn);
	}

    // All done, unmount partition and disable SDMMC or SPI peripheral
    esp_vfs_fat_sdmmc_unmount();
    //ESP_LOGI(TAG, "Card unmounted");
    sd_card_ready = 0;

    printf("verify_sd_counter_overlap(): free mem [end] = %u\n", xPortGetFreeHeapSize());
}
