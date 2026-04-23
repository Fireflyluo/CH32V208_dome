// 协议测试-windows
#include "../utils/data_protocol.h"

#include <stdio.h>
#include <string.h>

static int test_base64_tail(void) {
  const uint8_t in1[1] = {0xAB};
  const uint8_t in2[2] = {0xAB, 0xCD};
  char enc[16];
  uint8_t dec[16];
  int16_t n;

  if (base64_encode(in1, 1, enc) != 2) {
    return -1;
  }
  n = base64_decode(enc, (uint16_t)strlen(enc), dec);
  if (n != 1 || dec[0] != 0xAB) {
    return -2;
  }

  if (base64_encode(in2, 2, enc) != 3) {
    return -3;
  }
  n = base64_decode(enc, (uint16_t)strlen(enc), dec);
  if (n != 2 || dec[0] != 0xAB || dec[1] != 0xCD) {
    return -4;
  }

  return 0;
}

static int test_param_roundtrip(void) {
  param_data_decoded_t in = {
      .T1 = 1000u,
      .T2 = 2000u,
      .T3 = 3000u,
      .T4 = 4000u,
      .threshold_high = 3456u,
      .threshold_low = 1234u,
      .master_time = 0x123456789ull,
  };
  char frame[256];
  char content[128];
  uint16_t content_len = 0;
  uint32_t crc = 0;
  uint8_t seq = 0;
  msg_type_t type = MSG_UNKNOWN;
  param_data_decoded_t out;
  frame_status_t st;

  uint16_t len = build_set_params_msg(frame, sizeof(frame), 7u, &in);
  if (len == 0) {
    return -10;
  }

  st = parse_frame(frame, len, &type, &seq, content, &content_len, &crc);
  if (st != FRAME_OK || type != MSG_SET_PARAMS || seq != 7u) {
    return -11;
  }

  if (!parse_param_data(content, content_len, &out)) {
    return -12;
  }

  if ((out.T1 != in.T1) || (out.T2 != in.T2) || (out.T3 != in.T3) ||
      (out.T4 != in.T4) || (out.threshold_high != (in.threshold_high & 0x0FFFu)) ||
      (out.threshold_low != (in.threshold_low & 0x0FFFu)) ||
      (out.master_time != (in.master_time & 0xFFFFFFFFFull))) {
    return -13;
  }

  return 0;
}

int main(void) {
  int rc;

  rc = test_base64_tail();
  if (rc != 0) {
    printf("test_base64_tail failed: %d\n", rc);
    return 1;
  }

  rc = test_param_roundtrip();
  if (rc != 0) {
    printf("test_param_roundtrip failed: %d\n", rc);
    return 1;
  }

  printf("protocol_windows_smoke: PASS\n");
  return 0;
}
