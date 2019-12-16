#include "sdcard.h"

#include "init.h"
#include "gpio.h"

#include "fatfs/src/esp_vfs_fat.h"
#include "driver/include/driver/sdmmc_host.h"
#include "driver/include/driver/sdspi_host.h"
#include "sdmmc/include/sdmmc_cmd.h"
#include "vfs/include/sys/dirent.h"
#include <sys/stat.h>
#include <sys/unistd.h>
#include <string.h>

static const char *TAG = "SD_test";

sdmmc_card_t* card;
int sd_card_ready = 0;

#ifdef BOARD_WHALE
#define SD_CARD_1BIT_MODE
#define SD_CARD_SPI_MODE
#endif

int sd_card_init()
{
	//-----------------------------------------------------
    // SD card test

	#ifdef SD_CARD_SPI_MODE
    ESP_LOGI(TAG, "Using SDSPI peripheral");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	#else
    ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    //set faster clock
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

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
        .format_if_mount_failed = false,
        .max_files = 5
    };

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.

    int ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%d). "
                "Make sure SD card lines have pull-up resistors in place.", ret);
        }
        return ret;
    }

    // Card has been initialized, print its properties
    //sdmmc_card_print_info(stdout, card);

    sd_card_ready = 1;

    return ESP_OK;
}

void sd_card_info()
{
	printf("sd_card_info(): SD Card init... ");
    if(!sd_card_ready)
    {
    	int result = sd_card_init();
    	if(result!=ESP_OK)
    	{
    		printf("sd_card_info(): problem with SD Card Init, code=%d\n", result);
    		return;
    	}
    }
	printf("done!\n");

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
	ESP_LOGI(TAG, "Card unmounted");

	sd_card_ready = 0;
}

void sd_card_speed_test()
{
	printf("sd_card_speed_test(): SD Card init... ");
    if(!sd_card_ready)
    {
    	int result = sd_card_init();
    	if(result!=ESP_OK)
    	{
    		printf("sd_card_speed_test(): problem with SD Card Init, code=%d\n", result);
    		return;
    	}
    }
	printf("done!\n");

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
    	ESP_LOGI(TAG, "Card unmounted");

    	sd_card_ready = 0;
    }
    //-----------------------------------------------------
}

void sd_card_file_list()
{
	printf("sd_card_file_list(): SD Card init... ");
	if(!sd_card_ready)
    {
    	int result = sd_card_init();
    	if(result!=ESP_OK)
    	{
    		printf("sd_card_speed_test(): problem with SD Card Init, code=%d\n", result);
    		return;
    	}
    }
	printf("done!\n");

	DIR *dp = NULL;
    dp = opendir("/sdcard/");
	printf("opendir(): dp = %x\n", (uint32_t)dp);

    if(dp==NULL)
    {
    	printf("opendir() returned NULL\n");
    	return;
    }

    struct dirent *de = NULL;
    char filepath[50];
    FILE *f;
    long int filesize;

    do
    {
    	de = readdir(dp);
    	//printf("readdir(): de = %x\n", (uint32_t)de);
        //printf("Dir / opened, de->d_name = %s\n", de->d_name);

        if(de!=NULL)
        {
            sprintf(filepath, "/sdcard/%s", de->d_name);
        	if(de->d_type==DT_DIR)
        	{
        		printf("de->d_name = %s, type = %d (DIR)\n", de->d_name, de->d_type);
        	}
        	else
        	{
        		f = fopen(filepath, "r");
        		if (f == NULL) {
        			printf("Failed to open file %s for reading\n", filepath);
        			return;
        		}
        		fseek(f,0,SEEK_END);
        		filesize = ftell(f);
        		fclose(f);
        		printf("de->d_name = %s, type = %d, size = %ld\n", de->d_name, de->d_type, filesize);
        	}
        }
    } while(de!=NULL);

    if(de==NULL)
    {
    	printf("f_readdir() returned NULL\n");
    	return;
    }

    printf("closedir(): Closing the dir\n");
    closedir(dp);

    //printf("Freeing the dirent structure\n");
    //free(de);

    // All done, unmount partition and disable SDMMC or SPI peripheral
	esp_vfs_fat_sdmmc_unmount();
	ESP_LOGI(TAG, "Card unmounted");

   	sd_card_ready = 0;
}

#define SD_RECORDING_RESOLUTION 100

int sd_recording_in_progress = 0, sd_ptr = 0, sd_half_ready = 0;
uint32_t *sd_write_buf;//[1024/4];

void sd_recording(void *pvParameters)
{
	printf("sd_recording(): task running on core ID=%d\n",xPortGetCoreID());

    char *rec_filename = "/sdcard/rec.raw";
    FILE* f = NULL;

    printf("sd_recording(): SD Card init...\n");
	if(!sd_card_ready)
    {
    	int result = sd_card_init();
    	if(result!=ESP_OK)
    	{
    		printf("sd_recording(): problem with SD Card Init, code=%d\n", result);
    	}
    	else
    	{
    	    printf("sd_recording(): Opening file %s for writing\n", rec_filename);

    	    //t1 = micros();
    	    f = fopen(rec_filename, "w");
    	    if (f == NULL) {
    	        printf("sd_recording(): Failed to open file %s for writing\n", rec_filename);
    	        sd_card_ready = 0;
    	    }
    	    else
    	    {
    	    	/*for(i=0;i<repeat;i++)
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
    	    	ESP_LOGI(TAG, "Card unmounted");

    	    	sd_card_ready = 0;*/
    	    }

    		//printf("done!\n");
    	}
    }
	if(sd_card_ready)
	{
		sd_write_buf = (uint32_t*)malloc(1024);

		sd_recording_in_progress = 1;
		sd_ptr = 0;
		sd_half_ready = 0;

		while(channel_running)
		{
			if(sd_half_ready)
			{
				fwrite(((uint8_t*)sd_write_buf)+(sd_half_ready-1)*512,512,1,f);
			}
			sd_half_ready = 0;

			//need to detect channel ending more often
			for(int i=0;i<10;i++)
			{
				Delay(SD_RECORDING_RESOLUTION/10);
				if(!channel_running)
				{
					break;
				}
			}
		}
		sd_recording_in_progress = 0;
	}

	if(f!=NULL)
	{
		fclose(f);
		Delay(200);
		free(sd_write_buf);
	}
	//t2 = micros();
	//printf("%d bytes written in %f seconds\n",buf_size*repeat,t2-t1);
	//ESP_LOGI(TAG, "File written");

	if(sd_card_ready)
	{
		// All done, unmount partition and disable SDMMC or SPI peripheral
		esp_vfs_fat_sdmmc_unmount();
		ESP_LOGI(TAG, "Card unmounted");

		sd_card_ready = 0;
	}

	printf("sd_recording(): task done, deleting...\n");
	vTaskDelete(NULL);
	printf("sd_recording(): task deleted\n");
}

void sd_write_sample(uint32_t sample32)
{
	if(!sd_recording_in_progress)
	{
		return;
	}

	sd_write_buf[sd_ptr] = sample32;
	sd_ptr++;
	if(sd_ptr==512/4)
	{
		sd_half_ready = 1;
	}
	else if(sd_ptr==1024/4)
	{
		sd_half_ready = 2;
		sd_ptr = 0;
	}
}
