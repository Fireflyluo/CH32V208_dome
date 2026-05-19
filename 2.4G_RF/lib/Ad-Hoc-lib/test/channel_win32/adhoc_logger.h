#ifndef ADHOC_LOGGER_H
#define ADHOC_LOGGER_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ADHOC_LOGGER_MAX_SLOTS 64

void adhoc_logger_init(const char *logs_dir);
void adhoc_logger_open_slot(uint8_t slot_idx, const char *file_name);
void adhoc_logger_close_slot(uint8_t slot_idx);
void adhoc_logger_log(uint8_t slot_idx, const char *format, ...);
void adhoc_logger_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ADHOC_LOGGER_H */
