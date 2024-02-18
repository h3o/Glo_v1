/*
 * fw_update.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: Jun 27, 2019
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

#ifndef FW_UPDATE_H_
#define FW_UPDATE_H_

#include "glo_config.h"
#include "board.h"

#ifdef BOARD_GECHO

#define FW_UPDATE_FILENAME "gecho_fw.bin"
#define FW_UPDATE_MIN_EXPECTED_FILESIZE 500000 //in bytes
#define FW_UPDATE_MAX_EXPECTED_FILESIZE 1024*1024 //in bytes

/* default partition table:

# Name,   Type, SubType, Offset,  Size, Flags
# Note: if you change the phy_init or app partition offset, make sure to change the offset in Kconfig.projbuild
nvs,      data, nvs,     0x9000,   0x14000,
otadata,  data, ota,     0x1d000,  0x2000
phy_init, data, phy,     0x1f000,  0x1000,
config,	  0x40, 0x01,    0x20000,  0x10000,
factory,  app,  factory, 0x30000,  0x100000,
ota_0,    app,  ota_0,   0x130000, 0x100000,
ota_1,    app,  ota_1,   0x230000, 0x100000,
samples,  0x40, 0x02,    0x330000, 0xcd0000,
*/

//this is configured in glo_config.h:
//#define CONFIG_ADDRESS 0x20000
//#define CONFIG_SIZE 0x10000
//#define SAMPLES_BASE 0x330000

#define OTADATA_ADDRESS		0x1d000
#define OTADATA_SIZE		0x2000

#define FW_DATA_PART_TYPE				0x40
#define FW_DATA_PART_SUBTYPE_CONFIG		0x01
#define FW_DATA_PART_SUBTYPE_SAMPLES	0x02

#ifdef __cplusplus
 extern "C" {
#endif

void configured_and_running_partition_info();
void firmware_update_SD();
//uint32_t copy_from_SD_to_Flash(FILE *f, uint32_t address, uint32_t size);
void factory_data_load_SD();
void factory_reset_firmware();
int reload_config();

#ifdef __cplusplus
}
#endif

#endif /* BOARD_GECHO */

#endif /* FW_UPDATE_H_ */
