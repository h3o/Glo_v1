#ifndef SDCARD_H_
#define SDCARD_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int sd_card_init();
void sd_card_info();
void sd_card_speed_test();
void sd_card_file_list();

void sd_recording(void *pvParameters);
void sd_write_sample(uint32_t sample32);

#ifdef __cplusplus
}
#endif

#endif
