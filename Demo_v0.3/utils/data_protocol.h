#ifndef DATA_PROTOCOL_H
#define DATA_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef PROTOCOL_MAX_FRAME_LEN
#define PROTOCOL_MAX_FRAME_LEN 256
#endif

#ifndef PROTOCOL_BUFFER_SIZE
#define PROTOCOL_BUFFER_SIZE 512
#endif

#ifndef PROTOCOL_ENABLE_DEBUG_PRINT
#define PROTOCOL_ENABLE_DEBUG_PRINT 1
#endif

#ifndef DATA_PROTOCOL_USE_CUSTOM_MEMOPS
#define DATA_PROTOCOL_USE_CUSTOM_MEMOPS 0
#endif

#if DATA_PROTOCOL_USE_CUSTOM_MEMOPS
#ifndef DATA_PROTOCOL_MEMSET
#error "DATA_PROTOCOL_USE_CUSTOM_MEMOPS=1 requires DATA_PROTOCOL_MEMSET"
#endif
#ifndef DATA_PROTOCOL_MEMCPY
#error "DATA_PROTOCOL_USE_CUSTOM_MEMOPS=1 requires DATA_PROTOCOL_MEMCPY"
#endif
#else
#ifndef DATA_PROTOCOL_MEMSET
#define DATA_PROTOCOL_MEMSET memset
#endif
#ifndef DATA_PROTOCOL_MEMCPY
#define DATA_PROTOCOL_MEMCPY memcpy
#endif
#endif

#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#define PACKED
#pragma pack(push, 1)
#endif

typedef enum {
  MSG_UNKNOWN = 0,

  MSG_QUERY = 'S',
  MSG_REPEAT_QUERY = 'E',
  MSG_SET_PARAMS = 'P',

  MSG_REPORT_FIRST = 'i',
  MSG_REPORT_MIDDLE = 'm',
  MSG_REPORT_LAST = 'h',
  MSG_REPORT_NONE = 'n',
  MSG_ACK_PARAMS = 'q',
} msg_type_t;

typedef enum {
  FRAME_OK = 0,
  FRAME_ERR_INVALID_START = -1,
  FRAME_ERR_INVALID_END = -2,
  FRAME_ERR_CRC = -3,
  FRAME_ERR_TOO_LONG = -4,
  FRAME_ERR_INVALID_CHAR = -5,
  FRAME_ERR_INCOMPLETE = -6
} frame_status_t;

typedef struct PACKED {
  char start_mark;
  msg_type_t msg_type;
  char seq_char;
  char *content;
  char crc[4];
  char end_mark;
} frame_header_t;

typedef struct {
  uint64_t tag_id;        /* lower 36 bits valid */
  uint8_t start_hour;     /* 6-bit */
  uint8_t start_minute;   /* 6-bit */
  uint8_t start_second;   /* 6-bit */
  uint32_t sequence;      /* lower 18 bits valid */
  uint16_t temperature;   /* 12-bit */
  uint16_t humidity;      /* 12-bit */
  uint16_t *acceleration; /* each sample is 12-bit */
  uint16_t accel_count;
  bool has_temp_humidity;
} report_data_decoded_t;

typedef struct {
  uint32_t T1;            /* lower 24 bits valid */
  uint32_t T2;            /* lower 24 bits valid */
  uint32_t T3;            /* lower 24 bits valid */
  uint32_t T4;            /* lower 24 bits valid */
  uint16_t threshold_high;/* lower 12 bits valid */
  uint16_t threshold_low; /* lower 12 bits valid */
  uint64_t master_time;   /* lower 36 bits valid */
} param_data_decoded_t;

#ifndef __GNUC__
#pragma pack(pop)
#endif

const char *get_base64_table(void);
uint16_t base64_encode(const uint8_t *input, uint16_t input_len, char *output);
int16_t base64_decode(const char *input, uint16_t input_len, uint8_t *output);

uint32_t crc24q_calculate(const uint8_t *data, uint16_t length);
void crc_to_base64(uint32_t crc, char *output);
uint32_t crc_from_base64(const char *input);

uint16_t build_frame(char *frame, uint16_t frame_size, msg_type_t type,
                     uint8_t seq, const char *content, uint16_t content_len);

frame_status_t parse_frame(const char *frame, uint16_t frame_len,
                           msg_type_t *type, uint8_t *seq, char *content_buf,
                           uint16_t *content_len, uint32_t *crc_received);

bool is_frame_complete(const char *frame, uint16_t frame_len,
                       uint16_t *frame_start, uint16_t *frame_end);

uint16_t build_query_msg(char *buffer, uint16_t buffer_size, uint8_t seq);
uint16_t build_repeat_query_msg(char *buffer, uint16_t buffer_size,
                                uint8_t seq);
uint16_t build_set_params_msg(char *buffer, uint16_t buffer_size, uint8_t seq,
                              const param_data_decoded_t *params);
uint16_t build_report_msg(char *buffer, uint16_t buffer_size, uint8_t seq,
                          msg_type_t type, const report_data_decoded_t *data);
uint16_t build_ack_msg(char *buffer, uint16_t buffer_size, uint8_t seq,
                       const char *original_content, uint16_t content_len);

bool parse_report_data(const char *encoded_data, uint16_t data_len,
                       report_data_decoded_t *report);
bool parse_param_data(const char *encoded_data, uint16_t data_len,
                      param_data_decoded_t *params);

uint16_t temperature_to_12bit(float temp_celsius);
float temperature_from_12bit(uint16_t data);
uint16_t humidity_to_12bit(float humidity);
float humidity_from_12bit(uint16_t data);
uint16_t acceleration_to_12bit(float accel_g, float full_scale);
float acceleration_from_12bit(uint16_t data, float full_scale);

void hex_dump(const uint8_t *data, uint16_t length);
void print_frame(const char *frame, uint16_t length);
void print_report_data(const report_data_decoded_t *report);
void print_param_data(const param_data_decoded_t *params);

#endif // DATA_PROTOCOL_H
