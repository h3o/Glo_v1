#include "sdcard.h"

#include "init.h"
#include "gpio.h"
#include "ui.h"
#include "leds.h"

#include <sys/stat.h>
#include <sys/unistd.h>
#include <string.h>

static const char *TAG = "SD_test";

sdmmc_card_t *card;
uint8_t sd_card_ready = 0, sd_card_present = SD_CARD_PRESENT_UNKNOWN;

#define SD_PLAYBACK_BUFFER 4096 //1024
uint32_t *SD_play_buf;
FILE* SD_f = NULL;

#ifdef BOARD_WHALE
#define SD_CARD_1BIT_MODE
#define SD_CARD_SPI_MODE
#endif

int sd_card_init(int max_files, int speed)
{
	//-----------------------------------------------------
    // SD card test

	#ifdef SD_CARD_SPI_MODE
    ESP_LOGI(TAG, "Using SDSPI peripheral");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	#else
    ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    if(speed) //set faster clock
    {
    	printf("sd_card_init(): speed = 40MHz\n");
    	host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    }
    else
    {
    	printf("sd_card_init(): speed = 20MHz\n");
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

    printf("SD card test in 1-bit mode\n");
	#else
    printf("SD card test in 4-bit mode\n");
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
    	//.allocation_unit_size = 4096,
        .format_if_mount_failed = false,
        .max_files = max_files
    };

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.

	printf("sd_card_init(): mounting file system, free mem = %u\n", xPortGetFreeHeapSize());

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
    	printf("done!\n");
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
    	printf("done!\n");
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

void sd_card_file_list(char *subdir, int recursive)
{
	int total_files = 0;
	//if(subdir==NULL)
	//{
		if(!sd_card_ready)
		{
			printf("sd_card_file_list(): SD Card init...\n");
			int result = sd_card_init(5, persistent_settings.SD_CARD_SPEED);
			if(result!=ESP_OK)
			{
				printf("sd_card_file_list(): problem with SD Card Init, code=%d\n", result);
		    	indicate_error(0x55aa, 8, 80);
				return;
			}
			printf("done!\n");
		}
	//}

	DIR *dp = NULL;
	char dir_name[256];

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
    char filepath[256];
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
        		if(recursive)
        		{
					//printf("de->d_name = %s, type = %d (DIR)\n", de->d_name, de->d_type);
					printf("%s (DIR)\n", de->d_name);

					if(subdir!=NULL)
					{
						sprintf(dir_name,"%s/%s",subdir,de->d_name);
						sd_card_file_list(dir_name, 1);
					}
					else
					{
						sd_card_file_list(de->d_name, 1);
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
        		total_files++;
        	}
        }
    }
    while(de!=NULL);

    printf("[%d files total]\n", total_files);

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
}

void sd_card_download()
{
	char cmd[100];
	int res;

	uart_set_baudrate(UART_NUM_0, 1250000);
	channel_running = 1;

	//start listening to UART
	while(!event_next_channel)
	{
		res = scanf("%s\n", cmd);
		//printf("res = %d, cmd = %s\n",res, cmd);
		if(res!=-1)
		{
			printf("cmd = %s\n", cmd);

			if(cmd[0]=='L')
			{
				printf("--SD_FILE_LIST--\n");
				sd_card_file_list("rec", 0);
				printf("--END--\n");
			}
			if(cmd[0]=='X')
			{
				break;
			}
		}
		Delay(10);
	}
	uart_set_baudrate(UART_NUM_0, 115200);
}

int /*sd_recording_in_progress = 0,*/ sd_ptr = 0, sd_half_ready = 0;
uint32_t *sd_write_buf;//[1024/4];
uint8_t is_recording = 0;

const char *WAV_HEADER = {"RIFF\0\0\0\0WAVEfmt\x20\x10\0\0\0\1\0\2\0\x5d\xc6\0\0\x74\x19\3\0\4\0\x10\0data\0\0\0\0"};
#define WAV_HEADER_SIZE 44

void sd_recording(void *pvParameters)
{
	printf("sd_recording(): task running on core ID=%d\n",xPortGetCoreID());

	char rec_filename[50];

	FILE* f = NULL;
	uint32_t sd_blocks_written = 0;

	if(!sd_card_ready)
    {
	    printf("sd_recording(): SD Card init...\n");

	    printf("sd_recording(): before sd_card_init(), free mem = %u\n", xPortGetFreeHeapSize());
	    int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
	    printf("sd_recording(): after sd_card_init(), free mem = %u\n", xPortGetFreeHeapSize());

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
    		printf("sd_recording(): next file number %u\n",next_no);

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

	printf("sd_recording(): SD card ready!\n");
	if(sd_card_ready)
	{
		#ifndef SD_WRITE_BUFFER_PERMANENT
		sd_write_buf = (uint32_t*)malloc(SD_WRITE_BUFFER);
		#endif

		//sd_recording_in_progress = 1;
		sd_ptr = 0;
		sd_half_ready = 0;

		fwrite(WAV_HEADER,WAV_HEADER_SIZE,1,f);

		uint8_t *b_ptr;

		while(is_recording && channel_running)
		{
			if(sd_half_ready)
			{
				b_ptr = ((uint8_t*)sd_write_buf)+(sd_half_ready-1)*(SD_WRITE_BUFFER/2);
				//printf("sd_recording(): sd_half_ready = %d, writing...\n", sd_half_ready);
				sd_half_ready = 0; //clear this ASAP
				fwrite(b_ptr,SD_WRITE_BUFFER/2,1,f);
				sd_blocks_written++;
				if((sd_blocks_written/16)%2==0)
				{
					LED_RDY_ON;
				}
				else
				{
					LED_RDY_OFF;
				}
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
	printf("sd_recording(): sd_blocks_written = %u, bytes = %u\n", sd_blocks_written, bytes_written);

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

		printf("sd_recording(): closing the file...\n");
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

	printf("sd_recording(): task done, deleting...\n");
	vTaskDelete(NULL);
	printf("sd_recording(): task deleted\n");
}

void sd_write_sample(uint32_t *sample32)
{
	/*
	if(!sd_recording_in_progress)
	{
		return;
	}
	*/

	sd_write_buf[sd_ptr] = sample32[0];
	sd_ptr++;
	if(sd_ptr==SD_WRITE_BUFFER/8)
	{
		sd_half_ready = 1;
		//printf("sd_write_sample(): sd_half_ready = 1\n");
	}
	else if(sd_ptr==SD_WRITE_BUFFER/4)
	{
		sd_half_ready = 2;
		sd_ptr = 0;
		//printf("sd_write_sample(): sd_half_ready = 2\n");
	}
}

uint32_t sd_get_last_rec_file_number()
{
	esp_err_t res;
	nvs_handle handle;
	res = nvs_open("system_cnt", NVS_READONLY, &handle);
	if(res!=ESP_OK)
	{
		printf("sd_get_last_rec_filename(): problem with nvs_open(), error = %d\n", res);
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
		printf("sd_get_next_rec_filename(): problem with nvs_open(), error = %d\n", res);
		return 0;
	}

	uint32_t val_u32;
	res = nvs_get_u32(handle, "rec_cnt", &val_u32);
	if(res!=ESP_OK) { val_u32 = 0; } //if the key does not exist, load default value

	val_u32++;

	res = nvs_set_u32(handle, "rec_cnt", val_u32);
	if(res!=ESP_OK)
	{
		printf("sd_get_next_rec_filename(): problem with nvs_set_u32(), error = %d\n", res);
		return 0;
	}

	nvs_close(handle);
	return val_u32;
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
		is_recording = 1;
		BaseType_t result = xTaskCreatePinnedToCore((TaskFunction_t)&sd_recording, "sd_recording_task", /*2048*2*/1024*2, NULL, 12, NULL, 1);
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

	mixed_sample_buffer = (int16_t*)malloc(SD_PLAYBACK_BUFFER*2); //allocate two 4kB buffers for double buffering

	mixed_sample_buffer_ptr_L = 0;
	mixed_sample_buffer_ptr_R = 0;
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

	BaseType_t result = xTaskCreatePinnedToCore((TaskFunction_t)&load_next_sd_sample_sector, "load_sd_sector", 2048, NULL, 12, NULL, 1);
	if(result != pdPASS)
	{
		printf("xTaskCreatePinnedToCore(): failed to create task, error = %d\n", result);
		return SD_CARD_SAMPLE_NOT_USED;
	}

	return SD_CARD_SAMPLE_OPENED;
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
		while(1)
		{
			if(SD_card_sample_buffer_half)
			{
				//printf("load_next_recorded_sample_sector(): loading SD_card_sample_buffer #%d\n", SD_card_sample_buffer_half);

				if(feof(SD_f)) //rewind back
				{
					//fseek(SD_f,44,SEEK_SET); //skip the header -> not a good idea, will throw off 4kb sencor reads
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

	printf("load_next_recorded_sample_sector(): task done, deleting...\n");
	vTaskDelete(NULL);
	printf("load_next_recorded_sample_sector(): task deleted\n");

}

int get_recording_filename(char *rec_filename, int recording, uint32_t *last_no)
{
	last_no[0] = sd_get_last_rec_file_number();
    printf("get_recording_filename(): last file number %u\n",last_no[0]);

    if(recording == SD_RECORDING_LAST)
	{
	    sprintf(rec_filename, "%s/rec%05u.wav", REC_SUBDIR, last_no[0]);
		printf("get_recording_filename(): file = %s\n", rec_filename);

		return last_no[0];
	}

    sprintf(rec_filename, "%s/rec%05u.wav", REC_SUBDIR, recording);
	printf("get_recording_filename(): file = %s\n", rec_filename);
    return recording;
}

void channel_SD_playback(int recording)
{
	char rec_filename[50];
	int result;

	next_event_lock = 1;

	uint32_t last_no;
	recording = get_recording_filename(rec_filename, recording, &last_no);

	result = sd_card_play(rec_filename, 0); //auto-detect the sampling rate

	if(result == SD_CARD_PLAY_RESULT_PREVIOUS_FILE)
	{
		if(recording==1)
		{
			channel_SD_playback(last_no);
		}
		else
		{
			channel_SD_playback(recording-1);
		}
	}
	else if(result == SD_CARD_PLAY_RESULT_NEXT_FILE)
	{
		if(recording == last_no)
		{
			channel_SD_playback(1);
		}
		else
		{
			channel_SD_playback(recording+1);
		}
	}

	volume_ramp = 1; //to not store new volume level again later
}

int sd_card_open_file(char *file_path)
{
	char filename[50];

	if(!sd_card_ready)
	{
		printf("sd_card_open_file(): SD Card init...\n");
		int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
	    if(result!=ESP_OK)
	    {
	    	printf("sd_card_open_file(): problem with SD Card Init, code=%d\n", result);
	    	indicate_error(0x55aa, 8, 80);
	    	return SD_CARD_PLAY_RESULT_ERROR;
	    }
		printf("done!\n");
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

		while(!feof(SD_f) && !event_next_channel
			&& btn_event_ext!=BUTTON_EVENT_SHORT_PRESS+BUTTON_1
			&& btn_event_ext!=BUTTON_EVENT_SHORT_PRESS+BUTTON_2)
		{
			//printf("channel_SD_playback(): loading buffer #%d\n", sd_blocks_read);
			fread(SD_play_buf,SD_PLAYBACK_BUFFER,1,SD_f);
			sd_blocks_read++;
			i2s_write_bytes(I2S_NUM, (char *)SD_play_buf, SD_PLAYBACK_BUFFER, portMAX_DELAY);
		}

		if(!event_next_channel)
		{
			volume_ramp = 1; //to not store new volume level again
			codec_set_mute(1); //mute the codec
		}

		fclose(SD_f);
		free(SD_play_buf);

		//reset sampling rate back to user-defined value
		if(current_sampling_rate!=persistent_settings.SAMPLING_RATE)
		{
			set_sampling_rate(persistent_settings.SAMPLING_RATE);
		}
	}

	printf("sd_card_play(): total of %d buffers read\n", sd_blocks_read);

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
	FRESULT res = mkdir(dirname, 0755);
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
