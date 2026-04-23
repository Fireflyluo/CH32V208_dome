#include "data_protocol.h"

#include <stdio.h>

#define CRC24Q_POLY 0x1864CFBu
#define CRC24Q_INIT 0x000000u

#define PROTOCOL_MAX_REPORT_CONTENT 240u
#define PARAM_CONTENT_LEN 26u

static int8_t base64_char_to_val(char c) {
  static int8_t table[256];
  static bool inited = false;
  const char *base = get_base64_table();
  uint16_t i;

  if (!inited) {
    for (i = 0; i < 256u; ++i) {
      table[i] = -1;
    }
    for (i = 0; i < 64u; ++i) {
      table[(uint8_t)base[i]] = (int8_t)i;
    }
    inited = true;
  }

  return table[(uint8_t)c];
}

static char base64_val_to_char(uint8_t v) {
  if (v >= 64u) {
    return '?';
  }
  return get_base64_table()[v];
}

static void encode_u12(uint16_t v, char out[2]) {
  v &= 0x0FFFu;
  out[0] = base64_val_to_char((uint8_t)((v >> 6) & 0x3Fu));
  out[1] = base64_val_to_char((uint8_t)(v & 0x3Fu));
}

static bool decode_u12(const char in[2], uint16_t *v) {
  int8_t a = base64_char_to_val(in[0]);
  int8_t b = base64_char_to_val(in[1]);

  if (a < 0 || b < 0 || v == NULL) {
    return false;
  }
  *v = (uint16_t)(((uint16_t)a << 6) | (uint16_t)b);
  return true;
}

static void encode_u18(uint32_t v, char out[3]) {
  v &= 0x3FFFFu;
  out[0] = base64_val_to_char((uint8_t)((v >> 12) & 0x3Fu));
  out[1] = base64_val_to_char((uint8_t)((v >> 6) & 0x3Fu));
  out[2] = base64_val_to_char((uint8_t)(v & 0x3Fu));
}

static bool decode_u18(const char in[3], uint32_t *v) {
  int8_t a = base64_char_to_val(in[0]);
  int8_t b = base64_char_to_val(in[1]);
  int8_t c = base64_char_to_val(in[2]);

  if (a < 0 || b < 0 || c < 0 || v == NULL) {
    return false;
  }
  *v = ((uint32_t)a << 12) | ((uint32_t)b << 6) | (uint32_t)c;
  return true;
}

static void encode_u24(uint32_t v, char out[4]) {
  v &= 0xFFFFFFu;
  out[0] = base64_val_to_char((uint8_t)((v >> 18) & 0x3Fu));
  out[1] = base64_val_to_char((uint8_t)((v >> 12) & 0x3Fu));
  out[2] = base64_val_to_char((uint8_t)((v >> 6) & 0x3Fu));
  out[3] = base64_val_to_char((uint8_t)(v & 0x3Fu));
}

static bool decode_u24(const char in[4], uint32_t *v) {
  int8_t a = base64_char_to_val(in[0]);
  int8_t b = base64_char_to_val(in[1]);
  int8_t c = base64_char_to_val(in[2]);
  int8_t d = base64_char_to_val(in[3]);

  if (a < 0 || b < 0 || c < 0 || d < 0 || v == NULL) {
    return false;
  }
  *v = ((uint32_t)a << 18) | ((uint32_t)b << 12) | ((uint32_t)c << 6) |
       (uint32_t)d;
  return true;
}

static void encode_u36(uint64_t v, char out[6]) {
  v &= 0xFFFFFFFFFu;
  out[0] = base64_val_to_char((uint8_t)((v >> 30) & 0x3Fu));
  out[1] = base64_val_to_char((uint8_t)((v >> 24) & 0x3Fu));
  out[2] = base64_val_to_char((uint8_t)((v >> 18) & 0x3Fu));
  out[3] = base64_val_to_char((uint8_t)((v >> 12) & 0x3Fu));
  out[4] = base64_val_to_char((uint8_t)((v >> 6) & 0x3Fu));
  out[5] = base64_val_to_char((uint8_t)(v & 0x3Fu));
}

static bool decode_u36(const char in[6], uint64_t *v) {
  int8_t a = base64_char_to_val(in[0]);
  int8_t b = base64_char_to_val(in[1]);
  int8_t c = base64_char_to_val(in[2]);
  int8_t d = base64_char_to_val(in[3]);
  int8_t e = base64_char_to_val(in[4]);
  int8_t f = base64_char_to_val(in[5]);

  if (a < 0 || b < 0 || c < 0 || d < 0 || e < 0 || f < 0 || v == NULL) {
    return false;
  }

  *v = ((uint64_t)a << 30) | ((uint64_t)b << 24) | ((uint64_t)c << 18) |
       ((uint64_t)d << 12) | ((uint64_t)e << 6) | (uint64_t)f;
  return true;
}

const char *get_base64_table(void) {
  static const char table[65] = "0123456789"
                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "-_";
  return table;
}

uint16_t base64_encode(const uint8_t *input, uint16_t input_len, char *output) {
  uint16_t i = 0u;
  uint16_t j = 0u;

  if (input == NULL || output == NULL || input_len == 0u) {
    if (output != NULL) {
      output[0] = '\0';
    }
    return 0u;
  }

  while ((uint16_t)(input_len - i) >= 3u) {
    uint32_t triple = ((uint32_t)input[i] << 16) | ((uint32_t)input[i + 1] << 8) |
                      (uint32_t)input[i + 2];
    output[j++] = base64_val_to_char((uint8_t)((triple >> 18) & 0x3Fu));
    output[j++] = base64_val_to_char((uint8_t)((triple >> 12) & 0x3Fu));
    output[j++] = base64_val_to_char((uint8_t)((triple >> 6) & 0x3Fu));
    output[j++] = base64_val_to_char((uint8_t)(triple & 0x3Fu));
    i = (uint16_t)(i + 3u);
  }

  if ((uint16_t)(input_len - i) == 1u) {
    uint16_t v = (uint16_t)((uint16_t)input[i] << 4);
    output[j++] = base64_val_to_char((uint8_t)((v >> 6) & 0x3Fu));
    output[j++] = base64_val_to_char((uint8_t)(v & 0x3Fu));
  } else if ((uint16_t)(input_len - i) == 2u) {
    uint32_t v = ((uint32_t)input[i] << 10) | ((uint32_t)input[i + 1] << 2);
    output[j++] = base64_val_to_char((uint8_t)((v >> 12) & 0x3Fu));
    output[j++] = base64_val_to_char((uint8_t)((v >> 6) & 0x3Fu));
    output[j++] = base64_val_to_char((uint8_t)(v & 0x3Fu));
  }

  output[j] = '\0';
  return j;
}

int16_t base64_decode(const char *input, uint16_t input_len, uint8_t *output) {
  uint16_t i;
  uint16_t o = 0u;
  uint16_t full_quads;
  uint16_t rem;

  if (input == NULL || output == NULL || input_len == 0u) {
    return -1;
  }

  rem = (uint16_t)(input_len % 4u);
  if (rem == 1u) {
    return -1;
  }

  full_quads = (uint16_t)(input_len / 4u);
  for (i = 0u; i < full_quads; ++i) {
    int8_t a = base64_char_to_val(input[i * 4u + 0u]);
    int8_t b = base64_char_to_val(input[i * 4u + 1u]);
    int8_t c = base64_char_to_val(input[i * 4u + 2u]);
    int8_t d = base64_char_to_val(input[i * 4u + 3u]);
    uint32_t triple;

    if (a < 0 || b < 0 || c < 0 || d < 0) {
      return -2;
    }

    triple = ((uint32_t)a << 18) | ((uint32_t)b << 12) | ((uint32_t)c << 6) |
             (uint32_t)d;
    output[o++] = (uint8_t)((triple >> 16) & 0xFFu);
    output[o++] = (uint8_t)((triple >> 8) & 0xFFu);
    output[o++] = (uint8_t)(triple & 0xFFu);
  }

  if (rem == 2u) {
    int8_t a = base64_char_to_val(input[input_len - 2u]);
    int8_t b = base64_char_to_val(input[input_len - 1u]);

    if (a < 0 || b < 0) {
      return -2;
    }

    output[o++] = (uint8_t)((((uint16_t)a << 6) | (uint16_t)b) >> 4);
  } else if (rem == 3u) {
    int8_t a = base64_char_to_val(input[input_len - 3u]);
    int8_t b = base64_char_to_val(input[input_len - 2u]);
    int8_t c = base64_char_to_val(input[input_len - 1u]);
    uint32_t v;

    if (a < 0 || b < 0 || c < 0) {
      return -2;
    }

    v = ((uint32_t)a << 12) | ((uint32_t)b << 6) | (uint32_t)c;
    output[o++] = (uint8_t)((v >> 10) & 0xFFu);
    output[o++] = (uint8_t)((v >> 2) & 0xFFu);
  }

  return (int16_t)o;
}

uint32_t crc24q_calculate(const uint8_t *data, uint16_t length) {
  uint32_t crc = CRC24Q_INIT;
  uint16_t i;
  uint8_t j;

  if (data == NULL) {
    return 0u;
  }

  for (i = 0u; i < length; ++i) {
    crc ^= ((uint32_t)data[i] << 16);
    for (j = 0u; j < 8u; ++j) {
      crc <<= 1;
      if ((crc & 0x1000000u) != 0u) {
        crc ^= CRC24Q_POLY;
      }
    }
  }

  return (crc & 0xFFFFFFu);
}

void crc_to_base64(uint32_t crc, char *output) {
  if (output == NULL) {
    return;
  }

  encode_u24(crc, output);
  output[4] = '\0';
}

uint32_t crc_from_base64(const char *input) {
  uint32_t v = 0u;

  if (input == NULL) {
    return 0xFFFFFFFFu;
  }

  if (!decode_u24(input, &v)) {
    return 0xFFFFFFFFu;
  }
  return v;
}

uint16_t build_frame(char *frame, uint16_t frame_size, msg_type_t type,
                     uint8_t seq, const char *content, uint16_t content_len) {
  uint16_t pos = 0u;
  uint32_t crc;
  char crc_str[5];

  if (frame == NULL) {
    return 0u;
  }
  if (frame_size < (uint16_t)(content_len + 8u + 1u)) {
    return 0u;
  }
  if (content_len > PROTOCOL_MAX_FRAME_LEN) {
    return 0u;
  }

  frame[pos++] = '#';
  frame[pos++] = (char)type;
  frame[pos++] = base64_val_to_char((uint8_t)(seq & 0x3Fu));

  if (content != NULL && content_len > 0u) {
    DATA_PROTOCOL_MEMCPY(&frame[pos], content, content_len);
    pos = (uint16_t)(pos + content_len);
  }

  crc = crc24q_calculate((const uint8_t *)frame, pos);
  crc_to_base64(crc, crc_str);
  DATA_PROTOCOL_MEMCPY(&frame[pos], crc_str, 4u);
  pos = (uint16_t)(pos + 4u);

  frame[pos++] = '$';
  frame[pos] = '\0';
  return pos;
}

frame_status_t parse_frame(const char *frame, uint16_t frame_len,
                           msg_type_t *type, uint8_t *seq, char *content_buf,
                           uint16_t *content_len, uint32_t *crc_received) {
  uint16_t content_start = 3u;
  uint16_t content_end;
  uint32_t crc_calc;
  char crc_str[4];
  int8_t seq_v;

  if (frame == NULL || type == NULL || seq == NULL || content_len == NULL ||
      crc_received == NULL) {
    return FRAME_ERR_INCOMPLETE;
  }

  if (frame_len < 7u) {
    return FRAME_ERR_INCOMPLETE;
  }
  if (frame[0] != '#') {
    return FRAME_ERR_INVALID_START;
  }
  if (frame[frame_len - 1u] != '$') {
    return FRAME_ERR_INVALID_END;
  }

  *type = (msg_type_t)frame[1];

  seq_v = base64_char_to_val(frame[2]);
  if (seq_v < 0) {
    return FRAME_ERR_INVALID_CHAR;
  }
  *seq = (uint8_t)seq_v;

  content_end = (uint16_t)(frame_len - 5u);
  if (content_end < content_start) {
    return FRAME_ERR_INCOMPLETE;
  }

  *content_len = (uint16_t)(content_end - content_start);
  if (content_buf != NULL && *content_len > 0u) {
    DATA_PROTOCOL_MEMCPY(content_buf, &frame[content_start], *content_len);
    content_buf[*content_len] = '\0';
  }

  DATA_PROTOCOL_MEMCPY(crc_str, &frame[content_end], 4u);
  *crc_received = crc_from_base64(crc_str);

  crc_calc = crc24q_calculate((const uint8_t *)frame, content_end);
  if (*crc_received != crc_calc) {
    return FRAME_ERR_CRC;
  }

  return FRAME_OK;
}

bool is_frame_complete(const char *frame, uint16_t frame_len,
                       uint16_t *frame_start, uint16_t *frame_end) {
  uint16_t i;
  uint16_t j;

  if (frame == NULL) {
    return false;
  }

  for (i = 0u; i < frame_len; ++i) {
    if (frame[i] != '#') {
      continue;
    }

    for (j = (uint16_t)(i + 1u); j < frame_len; ++j) {
      if (frame[j] == '$' && (uint16_t)(j - i + 1u) >= 7u) {
        if (frame_start != NULL) {
          *frame_start = i;
        }
        if (frame_end != NULL) {
          *frame_end = j;
        }
        return true;
      }
    }
  }

  return false;
}

uint16_t build_query_msg(char *buffer, uint16_t buffer_size, uint8_t seq) {
  return build_frame(buffer, buffer_size, MSG_QUERY, seq, NULL, 0u);
}

uint16_t build_repeat_query_msg(char *buffer, uint16_t buffer_size,
                                uint8_t seq) {
  return build_frame(buffer, buffer_size, MSG_REPEAT_QUERY, seq, NULL, 0u);
}

uint16_t build_set_params_msg(char *buffer, uint16_t buffer_size, uint8_t seq,
                              const param_data_decoded_t *params) {
  char content[PARAM_CONTENT_LEN + 1u];
  uint16_t pos = 0u;
  char tmp[6];

  if (buffer == NULL || params == NULL) {
    return 0u;
  }

  encode_u24(params->T1, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 4u);
  pos = (uint16_t)(pos + 4u);

  encode_u24(params->T2, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 4u);
  pos = (uint16_t)(pos + 4u);

  encode_u24(params->T3, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 4u);
  pos = (uint16_t)(pos + 4u);

  encode_u24(params->T4, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 4u);
  pos = (uint16_t)(pos + 4u);

  encode_u12(params->threshold_high, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 2u);
  pos = (uint16_t)(pos + 2u);

  encode_u12(params->threshold_low, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 2u);
  pos = (uint16_t)(pos + 2u);

  encode_u36(params->master_time, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 6u);
  pos = (uint16_t)(pos + 6u);

  content[pos] = '\0';
  return build_frame(buffer, buffer_size, MSG_SET_PARAMS, seq, content, pos);
}

uint16_t build_report_msg(char *buffer, uint16_t buffer_size, uint8_t seq,
                          msg_type_t type, const report_data_decoded_t *data) {
  char content[PROTOCOL_MAX_REPORT_CONTENT + 1u];
  uint16_t pos = 0u;
  uint16_t i;
  char tmp[6];
  uint16_t fixed_pairs = 0u;

  if (buffer == NULL || data == NULL) {
    return 0u;
  }

  encode_u36(data->tag_id, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 6u);
  pos = (uint16_t)(pos + 6u);

  content[pos++] = base64_val_to_char((uint8_t)(data->start_hour & 0x3Fu));
  content[pos++] = base64_val_to_char((uint8_t)(data->start_minute & 0x3Fu));
  content[pos++] = base64_val_to_char((uint8_t)(data->start_second & 0x3Fu));

  encode_u18(data->sequence, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 3u);
  pos = (uint16_t)(pos + 3u);

  if (data->has_temp_humidity) {
    encode_u12(data->temperature, tmp);
    DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 2u);
    pos = (uint16_t)(pos + 2u);

    encode_u12(data->humidity, tmp);
    DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 2u);
    pos = (uint16_t)(pos + 2u);

    fixed_pairs = 2u;
  }

  for (i = 0u; i < data->accel_count; ++i) {
    if (data->acceleration == NULL) {
      return 0u;
    }
    if ((uint16_t)(12u + (fixed_pairs + i + 1u) * 2u) > PROTOCOL_MAX_REPORT_CONTENT) {
      return 0u;
    }
    encode_u12(data->acceleration[i], tmp);
    DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 2u);
    pos = (uint16_t)(pos + 2u);
  }

  content[pos] = '\0';
  return build_frame(buffer, buffer_size, type, seq, content, pos);
}

uint16_t build_ack_msg(char *buffer, uint16_t buffer_size, uint8_t seq,
                       const char *original_content, uint16_t content_len) {
  return build_frame(buffer, buffer_size, MSG_ACK_PARAMS, seq, original_content,
                     content_len);
}

bool parse_report_data(const char *encoded_data, uint16_t data_len,
                       report_data_decoded_t *report) {
  uint16_t pos = 0u;
  uint16_t remain;
  uint16_t i;

  if (encoded_data == NULL || report == NULL || data_len < 12u) {
    return false;
  }

  if (!decode_u36(&encoded_data[pos], &report->tag_id)) {
    return false;
  }
  pos = (uint16_t)(pos + 6u);

  {
    int8_t h = base64_char_to_val(encoded_data[pos + 0u]);
    int8_t m = base64_char_to_val(encoded_data[pos + 1u]);
    int8_t s = base64_char_to_val(encoded_data[pos + 2u]);
    if (h < 0 || m < 0 || s < 0) {
      return false;
    }
    report->start_hour = (uint8_t)h;
    report->start_minute = (uint8_t)m;
    report->start_second = (uint8_t)s;
  }
  pos = (uint16_t)(pos + 3u);

  if (!decode_u18(&encoded_data[pos], &report->sequence)) {
    return false;
  }
  pos = (uint16_t)(pos + 3u);

  remain = (uint16_t)(data_len - pos);

  report->temperature = 0u;
  report->humidity = 0u;
  report->accel_count = 0u;
  report->has_temp_humidity = false;

  if (remain >= 4u && (remain % 2u) == 0u) {
    uint16_t t;
    uint16_t h;
    if (decode_u12(&encoded_data[pos], &t) &&
        decode_u12(&encoded_data[pos + 2u], &h)) {
      report->temperature = t;
      report->humidity = h;
      report->has_temp_humidity = true;
      pos = (uint16_t)(pos + 4u);
      remain = (uint16_t)(data_len - pos);
    }
  }

  if ((remain % 2u) != 0u) {
    return false;
  }

  report->accel_count = (uint16_t)(remain / 2u);
  if (report->accel_count > 0u && report->acceleration != NULL) {
    for (i = 0u; i < report->accel_count; ++i) {
      if (!decode_u12(&encoded_data[pos + (uint16_t)(i * 2u)],
                      &report->acceleration[i])) {
        return false;
      }
    }
  }

  return true;
}

bool parse_param_data(const char *encoded_data, uint16_t data_len,
                      param_data_decoded_t *params) {
  uint16_t pos = 0u;

  if (encoded_data == NULL || params == NULL || data_len != PARAM_CONTENT_LEN) {
    return false;
  }

  if (!decode_u24(&encoded_data[pos], &params->T1)) {
    return false;
  }
  pos = (uint16_t)(pos + 4u);

  if (!decode_u24(&encoded_data[pos], &params->T2)) {
    return false;
  }
  pos = (uint16_t)(pos + 4u);

  if (!decode_u24(&encoded_data[pos], &params->T3)) {
    return false;
  }
  pos = (uint16_t)(pos + 4u);

  if (!decode_u24(&encoded_data[pos], &params->T4)) {
    return false;
  }
  pos = (uint16_t)(pos + 4u);

  if (!decode_u12(&encoded_data[pos], &params->threshold_high)) {
    return false;
  }
  pos = (uint16_t)(pos + 2u);

  if (!decode_u12(&encoded_data[pos], &params->threshold_low)) {
    return false;
  }
  pos = (uint16_t)(pos + 2u);

  if (!decode_u36(&encoded_data[pos], &params->master_time)) {
    return false;
  }

  return true;
}

uint16_t temperature_to_12bit(float temp_celsius) {
  int32_t temp_deci = (int32_t)((temp_celsius + 200.0f) * 10.0f);
  if (temp_deci < 0) {
    temp_deci = 0;
  }
  if (temp_deci > 4000) {
    temp_deci = 4000;
  }
  return (uint16_t)temp_deci;
}

float temperature_from_12bit(uint16_t data) {
  return ((float)(data & 0x0FFFu) / 10.0f) - 200.0f;
}

uint16_t humidity_to_12bit(float humidity) {
  int32_t hum_deci = (int32_t)(humidity * 10.0f);
  if (hum_deci < 0) {
    hum_deci = 0;
  }
  if (hum_deci > 1000) {
    hum_deci = 1000;
  }
  return (uint16_t)hum_deci;
}

float humidity_from_12bit(uint16_t data) { return (float)(data & 0x0FFFu) / 10.0f; }

uint16_t acceleration_to_12bit(float accel_g, float full_scale) {
  float normalized;
  uint16_t value;

  if (full_scale <= 0.0f) {
    return 0u;
  }

  normalized = (accel_g / full_scale) * 4095.0f;
  if (normalized < 0.0f) {
    normalized = 0.0f;
  }
  if (normalized > 4095.0f) {
    normalized = 4095.0f;
  }

  value = (uint16_t)normalized;
  return (uint16_t)(value & 0x0FFFu);
}

float acceleration_from_12bit(uint16_t data, float full_scale) {
  if (full_scale <= 0.0f) {
    return 0.0f;
  }
  return ((float)(data & 0x0FFFu) / 4095.0f) * full_scale;
}

void hex_dump(const uint8_t *data, uint16_t length) {
#if PROTOCOL_ENABLE_DEBUG_PRINT
  uint16_t i;
  if (data == NULL) {
    return;
  }
  for (i = 0u; i < length; ++i) {
    printf("%02X ", data[i]);
    if (((uint16_t)(i + 1u) % 16u) == 0u) {
      printf("\n");
    }
  }
  printf("\n");
#else
  (void)data;
  (void)length;
#endif
}

void print_frame(const char *frame, uint16_t length) {
#if PROTOCOL_ENABLE_DEBUG_PRINT
  uint16_t i;
  if (frame == NULL) {
    return;
  }

  printf("Frame (%u bytes): ", length);
  for (i = 0u; i < length; ++i) {
    char c = frame[i];
    if (c >= 32 && c <= 126) {
      printf("%c", c);
    } else {
      printf(".");
    }
  }
  printf("\nHex: ");
  for (i = 0u; i < length; ++i) {
    printf("%02X ", (uint8_t)frame[i]);
  }
  printf("\n");
#else
  (void)frame;
  (void)length;
#endif
}

void print_report_data(const report_data_decoded_t *report) {
#if PROTOCOL_ENABLE_DEBUG_PRINT
  if (report == NULL) {
    return;
  }

  printf("=== Report Data ===\n");
  printf("Tag ID: 0x%09lX\n", (unsigned long)(report->tag_id & 0xFFFFFFFFFu));
  printf("Start Time: %02u:%02u:%02u\n", report->start_hour, report->start_minute,
         report->start_second);
  printf("Sequence: %lu\n", (unsigned long)(report->sequence & 0x3FFFFu));
  if (report->has_temp_humidity) {
    printf("Temperature: %.1f C\n", temperature_from_12bit(report->temperature));
    printf("Humidity: %.1f %%\n", humidity_from_12bit(report->humidity));
  }
  printf("Acceleration points: %u\n", report->accel_count);
#else
  (void)report;
#endif
}

void print_param_data(const param_data_decoded_t *params) {
#if PROTOCOL_ENABLE_DEBUG_PRINT
  if (params == NULL) {
    return;
  }

  printf("=== Parameter Data ===\n");
  printf("T1: %lu ms\n", (unsigned long)(params->T1 & 0xFFFFFFu));
  printf("T2: %lu ms\n", (unsigned long)(params->T2 & 0xFFFFFFu));
  printf("T3: %lu ms\n", (unsigned long)(params->T3 & 0xFFFFFFu));
  printf("T4: %lu ms\n", (unsigned long)(params->T4 & 0xFFFFFFu));
  printf("Threshold High: %u\n", (unsigned int)(params->threshold_high & 0x0FFFu));
  printf("Threshold Low: %u\n", (unsigned int)(params->threshold_low & 0x0FFFu));
  printf("Master Time: %lu\n", (unsigned long)(params->master_time & 0xFFFFFFFFFu));
#else
  (void)params;
#endif
}
