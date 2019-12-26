/*
 * fw_update.c
 *
 *  Created on: Jun 27, 2019
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

#include <fw_update.h>
#include <hw/sdcard.h>
#include <hw/init.h>
#include <hw/ui.h>
#include <hw/leds.h>
//#include "esp_image_format.h"
#include "esp_ota_ops.h"
#include "sha1.h"
#include <string.h>

#ifdef BOARD_GECHO

void configured_and_running_partition_info()
{
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running)
    {
        printf("firmware_update_SD(): Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x", configured->address, running->address);
        printf(" (This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)\n");
    }
    printf("firmware_update_SD(): Running partition type %d subtype %d (offset 0x%08x)\n", running->type, running->subtype, running->address);

    uint8_t offset_H = running->address >> 16;
    indicate_context_setting(byte_bit_reverse(offset_H), 2, 1000);
}

void firmware_update_SD()
{
	printf("firmware_update_SD()\n");
	if(!sd_card_ready)
    {
    	int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
    	if(result!=ESP_OK)
    	{
    		printf("firmware_update_SD(): problem initializing SD card, code=%d\n", result);
	    	indicate_error(0x55aa, 8, 80);
    		return;
    	}
    }

	/*
	DIR *dp = NULL;
    dp = opendir("/sdcard/");
	printf("firmware_update_SD(): opendir() dp = %x\n", (uint32_t)dp);

    if(dp==NULL)
    {
    	printf("firmware_update_SD(): opendir() returned NULL\n");
    	return;
    }
	*/
    char filepath[50];
    FILE *f;
    long int filesize;

    sprintf(filepath, "/sdcard/%s", FW_UPDATE_FILENAME);
    f = fopen(filepath, "rb");
    if (f == NULL)
    {
    	printf("firmware_update_SD(): Failed to open file %s for reading\n", filepath);
    	indicate_error(0x0001, 10, 100);
        return;
	}
    fseek(f,0,SEEK_END);
    filesize = ftell(f);

    if(filesize < FW_UPDATE_MIN_EXPECTED_FILESIZE || filesize > FW_UPDATE_MAX_EXPECTED_FILESIZE)
    {
    	printf("firmware_update_SD(): unexpected file size of %s, should be between %u and %u\n", filepath, FW_UPDATE_MIN_EXPECTED_FILESIZE, FW_UPDATE_MAX_EXPECTED_FILESIZE);
        fclose(f);
        indicate_error(0x0003, 10, 100);
        return;
    }

    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

    configured_and_running_partition_info();

    update_partition = esp_ota_get_next_update_partition(NULL);
    printf("Writing to partition subtype %d at offset 0x%x\n", update_partition->subtype, update_partition->address);
    if(update_partition == NULL)
    {
    	printf("firmware_update_SD(): Could not determine next update partition\n");
        fclose(f);
        indicate_error(0x0007, 10, 100);
        return;
    }

    err = esp_ota_begin(update_partition, filesize, &update_handle);
    if (err != ESP_OK) {
    	printf("firmware_update_SD(): esp_ota_begin failed (%s)\n", esp_err_to_name(err));
        fclose(f);
        indicate_error(0x000f, 10, 100);
        return;
    }

    fseek(f,0,SEEK_SET);
	#define BUFFSIZE	1024
	#define BLOCK_SIZE	16
    char ota_write_data[BUFFSIZE + 1] = { 0 }; //data buffer ready to write to the flash
    int blocks_written = 0;

    while (!feof(f))
    {
        memset(ota_write_data, 0, BUFFSIZE);
        int buff_len = fread(ota_write_data,BLOCK_SIZE,64,f); //read by 16-byte blocks
		printf("%d blocks / %d bytes read, ",buff_len,buff_len*BLOCK_SIZE);

        if (buff_len <= 0) //read error
        {
            printf("firmware_update_SD(): data read error!\n");
        }
        else if (buff_len > 0)
        {
        	err = esp_ota_write(update_handle, (const void *)ota_write_data, buff_len*BLOCK_SIZE);
            if (err != ESP_OK)
            {
                printf("firmware_update_SD(): esp_ota_write failed (%s)!\n", esp_err_to_name(err));
                fclose(f);
                indicate_error(0x001f, 10, 100);
                return;
            }
            else
            {
        		printf("%d bytes written, total of %u blocks / %u bytes written\r", buff_len*BLOCK_SIZE, blocks_written, blocks_written*BLOCK_SIZE);
        		LED_W8_set_byte(blocks_written/64);
        		LED_B5_set_byte(blocks_written/(64*256));
            }
        	blocks_written += buff_len;
        }
    }
	printf("total of %u blocks / %u bytes written\n",blocks_written,blocks_written*BLOCK_SIZE);

	if (esp_ota_end(update_handle) != ESP_OK) {
    	printf("firmware_update_SD(): esp_ota_end failed!\n");
        fclose(f);
        indicate_error(0x003f, 10, 100);
        return;
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
    	printf("firmware_update_SD(): esp_ota_set_boot_partition failed (%s)!\n", esp_err_to_name(err));
        fclose(f);
        indicate_error(0x007f, 10, 100);
        return;
    }

    fclose(f);

    indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_FILL_8_RIGHT, 1, 50);

    // All done, unmount partition and disable SDMMC or SPI peripheral
	esp_vfs_fat_sdmmc_unmount();
	printf("firmware_update_SD(): Card unmounted\n");
   	sd_card_ready = 0;

   	printf("firmware_update_SD(): update finished, restarting\n");
    esp_restart();
}

uint32_t copy_from_SD_to_Flash(FILE *f, uint32_t address, uint32_t size)
{
	//align size to neareset 4kB block (0x1000)

	uint32_t erase_size = ((size/0x1000)+1)*0x1000;
	printf("copy_from_SD_to_Flash(): erasing region at %x of size %x\n", address, erase_size);

	uint32_t t1 = esp_log_timestamp();
	esp_err_t err = spi_flash_erase_range(address, erase_size);
	t1 = esp_log_timestamp() - t1;
	if(err!=ESP_OK)
	{
		printf("copy_from_SD_to_Flash(): problem erasing region at %x of size %x, code = %d\n", address, erase_size, err);
		return 0;
	}
	else
	{
		printf("copy_from_SD_to_Flash(): region erased in %u ms\n", t1);
	}

	uint32_t bytes_copied = 0;
	uint32_t dest_addr = address;
    fseek(f,0,SEEK_SET);

    #define BUFFSIZE	8192 //1024
	#define BLOCK_SIZE	4 //16 //causes cut of the last block in the file if not aligned with block size
    //char data[BUFFSIZE + 1] = { 0 };
    char *data = malloc(BUFFSIZE+1);
    int blocks_written = 0;

	t1 = esp_log_timestamp();
    while (!feof(f))
    {
        memset(data, 0, BUFFSIZE);
        int buff_len = fread(data,BLOCK_SIZE,BUFFSIZE/BLOCK_SIZE,f); //read by 16-byte blocks
		//printf("%d blocks / %d bytes read, ",buff_len,buff_len*BLOCK_SIZE);

		bytes_copied += buff_len*BLOCK_SIZE;

        if (buff_len <= 0) //read error
        {
            printf("copy_from_SD_to_Flash(): data read error!\n");
        }
        else if (buff_len > 0)
        {
        	err = spi_flash_write(dest_addr, data, BUFFSIZE);

            if (err != ESP_OK)
            {
                printf("copy_from_SD_to_Flash(): spi_flash_write failed (%s)!\n", esp_err_to_name(err));
                return 0; //bytes_copied;
            }
            else
            {
        		//printf("%d bytes written\r",buff_len*BLOCK_SIZE);
                printf("%u blocks / %u bytes copied\r",blocks_written,blocks_written*BLOCK_SIZE);
        		if(!(blocks_written&0x001f))
        		{
        			LED_W8_set_byte(blocks_written>>11);
        			LED_B5_set_byte(blocks_written>>16);
        		}
            }

            blocks_written += buff_len;
        	dest_addr += BUFFSIZE;
        }
    }
	t1 = esp_log_timestamp() - t1;
    printf("\n");
    free(data);
    printf("total of %u blocks / %u bytes written in %u ms\n",blocks_written,blocks_written*BLOCK_SIZE,t1);
	return bytes_copied;
}

int get_filepath_and_hash(char *filepart, char *path, char *hash)
{
	printf("get_filepath_and_hash(): filepart = %s\n", filepart);

	DIR *dp = NULL;

	//find last occurence of "/" to determine sub-directory
	char dir[50];
	char file_prefix[20];
	strcpy(dir, "/sdcard/");
	strcat(dir, filepart);
	printf("dir = %s\n", dir);
	char *last = strrchr(dir, '/');
	strcpy(file_prefix,last+1);
	uint32_t last_pos = (uint32_t)last - (uint32_t)dir;
	printf("last '/' found at position %u\n", last_pos);
	last[0] = 0;
	printf("dir = %s\n", dir);
	printf("file prefix = %s\n", file_prefix);

    dp = opendir(dir);
	printf("get_filepath_and_hash(): opendir() dp = %x\n", (uint32_t)dp);

    if(dp==NULL)
    {
    	printf("get_filepath_and_hash(): opendir() returned NULL\n");
    	return 0;
    }

    struct dirent* de;

    while(true)
    {
    	de = readdir(dp);
    	if(!de)
    	{
    		break;
    	}
    	printf("found file: %s\n", de->d_name);
    	if(strstr(de->d_name,file_prefix))
    	{
    		strcpy(path,de->d_name);
    		last = strrchr(path, '.');
    		strcpy(hash,last+1);
    		return 1;
    	}
	}
    path[0] = 0;
    hash[0] = 0;
    return 0;
}

void get_file_sha1(FILE *f, char *hash_buf)
{
    fseek(f,0,SEEK_SET);

	#define BUFFSIZE	1024
	//#define BLOCK_SIZE	4
    char f_buf[BUFFSIZE + 1] = { 0 };

    int total_bytes = 0;

	uint8_t code_sha1[20];

	SHA1_CTX sha;
	SHA1Init(&sha);

	while (!feof(f))
    {
        //int buff_len = fread(f_buf,BLOCK_SIZE,BUFFSIZE/BLOCK_SIZE,f);
        int buff_len = fread(f_buf,1,BUFFSIZE,f);
		//printf("%d blocks / %d bytes read, ",buff_len,buff_len*BLOCK_SIZE);

        if (buff_len <= 0) //read error
        {
            printf("get_file_sha1(): data read error!\n");
        }
        else if (buff_len > 0)
        {
        	//SHA1Update(&sha, (uint8_t *)f_buf, buff_len*BLOCK_SIZE);
        	SHA1Update(&sha, (uint8_t *)f_buf, buff_len);
        	//total_bytes += buff_len*BLOCK_SIZE;
        	total_bytes += buff_len;
        }
    }
	SHA1Final(code_sha1, &sha);
	sha1_to_hex(hash_buf, code_sha1);
	printf("get_file_sha1(): hash_buf=[%s]\n", hash_buf);
	printf("total bytes processed: %d\n",total_bytes);
}

//load samples and config to Flash, at addresses as defined in glo_config.h:
//#define CONFIG_ADDRESS 0x20000
//#define SAMPLES_BASE 0x330000

#define CONFIG_FILENAME "factory/config.txt"
#define SAMPLES_FILENAME "factory/samples.bin"

void factory_data_load_SD()
{
	printf("factory_data_load_SD()\n");
	if(!sd_card_ready)
    {
    	int result = sd_card_init(1, persistent_settings.SD_CARD_SPEED);
    	if(result!=ESP_OK)
    	{
    		printf("factory_data_load_SD(): problem initializing SD card, code=%d\n", result);
	    	indicate_error(0x55aa, 8, 80);
    		return;
    	}
    }

	const esp_partition_t *config_part = esp_partition_find_first(FW_DATA_PART_TYPE, FW_DATA_PART_SUBTYPE_CONFIG, NULL);
	printf("factory_data_load_SD(): esp_partition_find_first returned partition %s of size %x at %x\n", config_part->label,config_part->size,config_part->address);

	const esp_partition_t *samples_part = esp_partition_find_first(FW_DATA_PART_TYPE, FW_DATA_PART_SUBTYPE_SAMPLES, NULL);
	printf("factory_data_load_SD(): esp_partition_find_first returned partition %s of size %x at %x\n", samples_part->label,samples_part->size,samples_part->address);

    char filepath[80], file[60], hash[40];
    FILE *f;
    long int filesize;
    char file_hash[40];
    int success;

    //---------------------------------------------------------------------------------------------------------
    //---CONFIG------------------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------------------------

    //sprintf(filepath, "/sdcard/%s", CONFIG_FILENAME);
    sprintf(filepath, "/sdcard/factory/");
    success = get_filepath_and_hash(CONFIG_FILENAME, file, hash);
    printf("factory_data_load_SD(): get_filepath_and_hash() result = %d, file = [%s], hash = [%s]\n", success, file, hash);
    strcat(filepath, file);

    f = fopen(filepath, "rb");
    if (f == NULL)
    {
    	printf("factory_data_load_SD(): Failed to open file %s for reading\n", filepath);
    	indicate_error(0x0001, 10, 100);
        return;
	}
    fseek(f,0,SEEK_END);
    filesize = ftell(f);

    if(filesize > config_part->size)
    {
    	printf("factory_data_load_SD(): the file %s of %lu bytes does not fit partition (%u bytes)\n", filepath, filesize, config_part->size);
        fclose(f);
        indicate_error(0x0003, 10, 100);
        return;
    }

    //verify hash
    get_file_sha1(f,file_hash);
	printf("factory_data_load_SD(): file_sha1(%s) returned %s\n", filepath, file_hash);

	if(0==strncmp(hash,file_hash,40))
	{
		printf("Hash OK\n");
	}
	else
	{
		printf("Hash mismatch!\n");
        fclose(f);
        indicate_error(0x0007, 10, 100);
        return;
	}

    uint32_t t2 = esp_log_timestamp();
    uint32_t bytes_copied = copy_from_SD_to_Flash(f, config_part->address, filesize);
    printf("factory_data_load_SD(): copy_from_SD_to_Flash() copied %u bytes to address %x\n", bytes_copied, config_part->address);
	t2 = esp_log_timestamp() - t2;
    printf("copy operation completed in %u ms\n",t2);

    fclose(f);

    if(bytes_copied)
    {
    	indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_FILL_8_RIGHT, 1, 50);
    }
    else
    {
		indicate_error(0x000f, 10, 100);
    }

    //---------------------------------------------------------------------------------------------------------
    //---SAMPLES-----------------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------------------------

    //sprintf(filepath, "/sdcard/%s", SAMPLES_FILENAME);
    sprintf(filepath, "/sdcard/factory/");
    success = get_filepath_and_hash(SAMPLES_FILENAME, file, hash);
    printf("factory_data_load_SD(): get_filepath_and_hash() result = %d, file = [%s], hash = [%s]\n", success, file, hash);
    strcat(filepath, file);

    f = fopen(filepath, "rb");
    if (f == NULL)
    {
    	printf("factory_data_load_SD(): Failed to open file %s for reading\n", filepath);
    	indicate_error(0x001f, 10, 100);
        return;
	}
    fseek(f,0,SEEK_END);
    filesize = ftell(f);

    if(filesize > samples_part->size)
    {
    	printf("factory_data_load_SD(): the file %s of %lu bytes does not fit partition (%u bytes)\n", filepath, filesize, samples_part->size);
        fclose(f);
        indicate_error(0x003f, 10, 100);
        return;
    }

    //verify hash
    get_file_sha1(f,file_hash);
	printf("factory_data_load_SD(): file_sha1(%s) returned %s\n", filepath, file_hash);

	if(0==strncmp(hash,file_hash,40))
	{
		printf("Hash OK\n");
	}
	else
	{
		printf("Hash mismatch!\n");
        fclose(f);
        indicate_error(0x007f, 10, 100);
        return;
	}

	t2 = esp_log_timestamp();
    bytes_copied = copy_from_SD_to_Flash(f, samples_part->address, filesize);
    printf("factory_data_load_SD(): copy_from_SD_to_Flash() copied %u bytes to address %x\n", bytes_copied, samples_part->address);
	t2 = esp_log_timestamp() - t2;
    printf("copy operation completed in %u ms\n",t2);

    fclose(f);

    if(bytes_copied)
    {
    	indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_FILL_8_RIGHT, 1, 50);
    }
    else
    {
		indicate_error(0x00ff, 10, 100);
    }

    //---------------------------------------------------------------------------------------------------------
    //---END---------------------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------------------------

    // All done, unmount partition and disable SDMMC or SPI peripheral
	esp_vfs_fat_sdmmc_unmount();
	printf("factory_data_load_SD(): Card unmounted\n");
   	sd_card_ready = 0;

   	printf("factory_data_load_SD(): All done\n");
}

void factory_reset_firmware()
{
	printf("factory_reset_firmware()");

	esp_err_t err = spi_flash_erase_range(OTADATA_ADDRESS, OTADATA_SIZE);
	if(err!=ESP_OK)
	{
		printf("factory_reset_firmware(): problem erasing OTADATA, code = %d\n", err);
		indicate_error(0x55aa, 8, 80);
	}
	else
	{
		printf("factory_reset_firmware(): OTADATA erased, falling back to factory firmware\n");
		indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_CLEAR_8_LEFT, 1, 50);
	}

	printf("factory_reset_firmware(): restarting\n");
    esp_restart();
}

void reload_config()
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

    FILE *f = fopen("/sdcard/config.txt", "rb");
    if (f == NULL)
    {
    	printf("reload_config(): Failed to open config.txt file\n");
    	indicate_error(0x0001, 10, 100);
        return;
	}

    fseek(f,0,SEEK_END);
    int config_len = ftell(f);

    if(config_len > CONFIG_SIZE)
    {
    	printf("reload_config(): the config.txt file does not fit partition\n");
    	indicate_error(0x0003, 10, 100);
        return;
    }

    char *config_buffer = malloc(CONFIG_SIZE+2);
	//int config_len = sd_card_read_file("config.txt", config_buffer, CONFIG_SIZE+2);
    fseek(f,0,SEEK_SET);
    fread(config_buffer,config_len,1,f);

    char *config_ends = strstr(config_buffer, "[end]");
	if(!config_ends)
	{
		printf("reload_config(): end mark not found, possibly corrupt\n");
    	indicate_error(0x0007, 10, 100);
		return;
	}
	printf("reload_config(): config end mark found at %d\n",(int)config_ends - (int)config_buffer);

    uint32_t bytes_copied = copy_from_SD_to_Flash(f, CONFIG_ADDRESS, config_len);
	if(bytes_copied)
	{
		indicate_context_setting(SETTINGS_INDICATOR_ANIMATE_FILL_8_RIGHT, 1, 50);
	    printf("reload_config(): copy_from_SD_to_Flash() copied %u bytes to address %x\n", bytes_copied, CONFIG_ADDRESS);
	}
	else
	{
		indicate_error(0x000f, 10, 100);
	}

    fclose(f);

    esp_vfs_fat_sdmmc_unmount();
	//ESP_LOGI(TAG, "Card unmounted\n");
	sd_card_ready = 0;
}

#endif /* BOARD_GECHO */
