/**
 * @file data_protocol.c
 * @brief 数据协议处理实现
 * 
 * 实现了数据协议相关的编码、解码、校验等功能
 */
#include "data_protocol.h"

#include <stdio.h>

#define CRC24Q_POLY 0x1864CFBu      ///< CRC24Q多项式
#define CRC24Q_INIT 0x000000u       ///< CRC24Q初始值

#define PROTOCOL_MAX_REPORT_CONTENT 240u    ///< 报告内容最大长度
#define PARAM_CONTENT_LEN 26u               ///< 参数内容长度

/**
 * @brief 将Base64字符转换为对应的数值
 * 
 * @param c Base64字符
 * @return 对应的数值，如果字符无效则返回-1
 */
static int8_t base64_char_to_val(char c) {
  static int8_t table[256];     // 存储Base64字符到数值的映射表
  static bool inited = false;   // 标记映射表是否已初始化
  const char *base = get_base64_table();    // 获取Base64编码表
  uint16_t i;

  // 如果映射表未初始化，则进行初始化
  if (!inited) {
    for (i = 0; i < 256u; ++i) {
      table[i] = -1;            // 初始化所有字符对应的值为-1(无效)
    }
    for (i = 0; i < 64u; ++i) {
      table[(uint8_t)base[i]] = (int8_t)i;  // 建立Base64字符到数值的映射
    }
    inited = true;
  }

  return table[(uint8_t)c];
}

/**
 * @brief 将数值转换为对应的Base64字符
 * 
 * @param v 数值(0-63)
 * @return 对应的Base64字符
 */
static char base64_val_to_char(uint8_t v) {
  if (v >= 64u) {
    return '?';     // 如果数值超出范围，返回'?'
  }
  return get_base64_table()[v];   // 返回对应位置的Base64字符
}

/**
 * @brief 将12位数值编码为Base64字符
 * 
 * @param v 12位数值
 * @param out 输出字符数组(至少2个字符)
 */
static void encode_u12(uint16_t v, char out[2]) {
  v &= 0x0FFFu;                             // 确保只保留低12位
  out[0] = base64_val_to_char((uint8_t)((v >> 6) & 0x3Fu));  // 取高6位
  out[1] = base64_val_to_char((uint8_t)(v & 0x3Fu));         // 取低6位
}

/**
 * @brief 解码12位数值
 * 
 * @param in 输入字符数组(2个字符)
 * @param v 输出数值指针
 * @return 是否解码成功
 */
static bool decode_u12(const char in[2], uint16_t *v) {
  int8_t a = base64_char_to_val(in[0]);     // 第一个字符转换为数值
  int8_t b = base64_char_to_val(in[1]);     // 第二个字符转换为数值

  if (a < 0 || b < 0 || v == NULL) {
    return false;   // 如果任一字符无效或输出指针为空，则解码失败
  }
  *v = (uint16_t)(((uint16_t)a << 6) | (uint16_t)b);    // 合并高低6位
  return true;
}

/**
 * @brief 将18位数值编码为Base64字符
 * 
 * @param v 18位数值
 * @param out 输出字符数组(至少3个字符)
 */
static void encode_u18(uint32_t v, char out[3]) {
  v &= 0x3FFFFu;                            // 确保只保留低18位
  out[0] = base64_val_to_char((uint8_t)((v >> 12) & 0x3Fu)); // 取高6位
  out[1] = base64_val_to_char((uint8_t)((v >> 6) & 0x3Fu));  // 取中间6位
  out[2] = base64_val_to_char((uint8_t)(v & 0x3Fu));         // 取低6位
}

/**
 * @brief 解码18位数值
 * 
 * @param in 输入字符数组(3个字符)
 * @param v 输出数值指针
 * @return 是否解码成功
 */
static bool decode_u18(const char in[3], uint32_t *v) {
  int8_t a = base64_char_to_val(in[0]);     // 第一个字符转换为数值
  int8_t b = base64_char_to_val(in[1]);     // 第二个字符转换为数值
  int8_t c = base64_char_to_val(in[2]);     // 第三个字符转换为数值

  if (a < 0 || b < 0 || c < 0 || v == NULL) {
    return false;   // 如果任一字符无效或输出指针为空，则解码失败
  }
  *v = ((uint32_t)a << 12) | ((uint32_t)b << 6) | (uint32_t)c;  // 合并三个6位值
  return true;
}

/**
 * @brief 将24位数值编码为Base64字符
 * 
 * @param v 24位数值
 * @param out 输出字符数组(至少4个字符)
 */
static void encode_u24(uint32_t v, char out[4]) {
  v &= 0xFFFFFFu;                           // 确保只保留低24位
  out[0] = base64_val_to_char((uint8_t)((v >> 18) & 0x3Fu)); // 取最高6位
  out[1] = base64_val_to_char((uint8_t)((v >> 12) & 0x3Fu)); // 取次高6位
  out[2] = base64_val_to_char((uint8_t)((v >> 6) & 0x3Fu));  // 取次低6位
  out[3] = base64_val_to_char((uint8_t)(v & 0x3Fu));         // 取最低6位
}

/**
 * @brief 解码24位数值
 * 
 * @param in 输入字符数组(4个字符)
 * @param v 输出数值指针
 * @return 是否解码成功
 */
static bool decode_u24(const char in[4], uint32_t *v) {
  int8_t a = base64_char_to_val(in[0]);     // 第一个字符转换为数值
  int8_t b = base64_char_to_val(in[1]);     // 第二个字符转换为数值
  int8_t c = base64_char_to_val(in[2]);     // 第三个字符转换为数值
  int8_t d = base64_char_to_val(in[3]);     // 第四个字符转换为数值

  if (a < 0 || b < 0 || c < 0 || d < 0 || v == NULL) {
    return false;   // 如果任一字符无效或输出指针为空，则解码失败
  }
  *v = ((uint32_t)a << 18) | ((uint32_t)b << 12) | ((uint32_t)c << 6) | (uint32_t)d;  // 合并四个6位值
  return true;
}

/**
 * @brief 将36位数值编码为Base64字符
 * 
 * @param v 36位数值
 * @param out 输出字符数组(至少6个字符)
 */
static void encode_u36(uint64_t v, char out[6]) {
  v &= 0xFFFFFFFFFu;                        // 确保只保留低36位
  out[0] = base64_val_to_char((uint8_t)((v >> 30) & 0x3Fu)); // 取最高6位
  out[1] = base64_val_to_char((uint8_t)((v >> 24) & 0x3Fu)); // 取次高6位
  out[2] = base64_val_to_char((uint8_t)((v >> 18) & 0x3Fu)); // 取高6位
  out[3] = base64_val_to_char((uint8_t)((v >> 12) & 0x3Fu)); // 取中6位
  out[4] = base64_val_to_char((uint8_t)((v >> 6) & 0x3Fu));  // 取低6位
  out[5] = base64_val_to_char((uint8_t)(v & 0x3Fu));         // 取最低6位
}

/**
 * @brief 解码36位数值
 * 
 * @param in 输入字符数组(6个字符)
 * @param v 输出数值指针
 * @return 是否解码成功
 */
static bool decode_u36(const char in[6], uint64_t *v) {
  int8_t a = base64_char_to_val(in[0]);     // 第一个字符转换为数值
  int8_t b = base64_char_to_val(in[1]);     // 第二个字符转换为数值
  int8_t c = base64_char_to_val(in[2]);     // 第三个字符转换为数值
  int8_t d = base64_char_to_val(in[3]);     // 第四个字符转换为数值
  int8_t e = base64_char_to_val(in[4]);     // 第五个字符转换为数值
  int8_t f = base64_char_to_val(in[5]);     // 第六个字符转换为数值

  if (a < 0 || b < 0 || c < 0 || d < 0 || e < 0 || f < 0 || v == NULL) {
    return false;   // 如果任一字符无效或输出指针为空，则解码失败
  }

  *v = ((uint64_t)a << 30) | ((uint64_t)b << 24) | ((uint64_t)c << 18) | 
       ((uint64_t)d << 12) | ((uint64_t)e << 6) | (uint64_t)f; // 合并六个6位值
  return true;
}

/**
 * @brief 获取Base64编码表
 * 
 * @return Base64编码表字符串指针
 */
const char *get_base64_table(void) {
  static const char table[65] = "0123456789"
                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "-_";   // 使用RFC 4648标准的Base64表，但用'-'和'_'替换'+','/'
  return table;
}

/**
 * @brief Base64编码
 * 
 * @param input 输入数据指针
 * @param input_len 输入数据长度
 * @param output 输出字符串指针
 * @return 编码后字符串长度
 */
uint16_t base64_encode(const uint8_t *input, uint16_t input_len, char *output) {
  uint16_t i = 0u;        // 输入索引
  uint16_t j = 0u;        // 输出索引

  // 检查输入参数有效性
  if (input == NULL || output == NULL || input_len == 0u) {
    if (output != NULL) {
      output[0] = '\0';   // 空输出时设置字符串结束符
    }
    return 0u;
  }

  // 每3个字节处理为4个Base64字符
  while ((uint16_t)(input_len - i) >= 3u) {
    uint32_t triple = ((uint32_t)input[i] << 16) | ((uint32_t)input[i + 1] << 8) |
                      (uint32_t)input[i + 2]; // 组合3个字节为24位整数
    output[j++] = base64_val_to_char((uint8_t)((triple >> 18) & 0x3Fu));  // 高6位
    output[j++] = base64_val_to_char((uint8_t)((triple >> 12) & 0x3Fu));  // 中高6位
    output[j++] = base64_val_to_char((uint8_t)((triple >> 6) & 0x3Fu));   // 中低6位
    output[j++] = base64_val_to_char((uint8_t)(triple & 0x3Fu));          // 低6位
    i = (uint16_t)(i + 3u);   // 移动到下一组3字节
  }

  // 处理剩余不足3字节的数据
  if ((uint16_t)(input_len - i) == 1u) {      // 剩余1字节
    uint16_t v = (uint16_t)((uint16_t)input[i] << 4);   // 左移4位
    output[j++] = base64_val_to_char((uint8_t)((v >> 6) & 0x3Fu));   // 高6位
    output[j++] = base64_val_to_char((uint8_t)(v & 0x3Fu));          // 低6位
  } else if ((uint16_t)(input_len - i) == 2u) {   // 剩余2字节
    uint32_t v = ((uint32_t)input[i] << 10) | ((uint32_t)input[i + 1] << 2); // 组合2字节
    output[j++] = base64_val_to_char((uint8_t)((v >> 12) & 0x3Fu));  // 高6位
    output[j++] = base64_val_to_char((uint8_t)((v >> 6) & 0x3Fu));   // 中6位
    output[j++] = base64_val_to_char((uint8_t)(v & 0x3Fu));          // 低6位
  }

  output[j] = '\0';   // 添加字符串结束符
  return j;           // 返回编码后长度
}

/**
 * @brief Base64解码
 * 
 * @param input 输入字符串指针
 * @param input_len 输入字符串长度
 * @param output 输出数据指针
 * @return 解码后数据长度，失败返回负值
 */
int16_t base64_decode(const char *input, uint16_t input_len, uint8_t *output) {
  uint16_t i;                 // 循环计数器
  uint16_t o = 0u;            // 输出索引
  uint16_t full_quads;        // 完整四元组数量
  uint16_t rem;               // 剩余字符数量

  // 检查输入参数有效性
  if (input == NULL || output == NULL || input_len == 0u) {
    return -1;
  }

  // 计算剩余字符数，Base64长度除以4的余数不能是1
  rem = (uint16_t)(input_len % 4u);
  if (rem == 1u) {
    return -1;    // 余数为1是无效的Base64格式
  }

  // 计算完整的四元组数量
  full_quads = (uint16_t)(input_len / 4u);
  
  // 处理每个完整的四元组
  for (i = 0u; i < full_quads; ++i) {
    int8_t a = base64_char_to_val(input[i * 4u + 0u]);    // 第1个字符
    int8_t b = base64_char_to_val(input[i * 4u + 1u]);    // 第2个字符
    int8_t c = base64_char_to_val(input[i * 4u + 2u]);    // 第3个字符
    int8_t d = base64_char_to_val(input[i * 4u + 3u]);    // 第4个字符
    uint32_t triple;                                      // 3字节数据

    // 检查所有字符是否有效
    if (a < 0 || b < 0 || c < 0 || d < 0) {
      return -2;    // 包含无效字符
    }

    // 从4个Base64字符重构3个原始字节
    triple = ((uint32_t)a << 18) | ((uint32_t)b << 12) | ((uint32_t)c << 6) | (uint32_t)d;
    output[o++] = (uint8_t)((triple >> 16) & 0xFFu);   // 第1字节
    output[o++] = (uint8_t)((triple >> 8) & 0xFFu);    // 第2字节
    output[o++] = (uint8_t)(triple & 0xFFu);           // 第3字节
  }

  // 处理剩余的字符（可能有2个或3个）
  if (rem == 2u) {      // 剩余2个字符，可解码出1个字节
    int8_t a = base64_char_to_val(input[input_len - 2u]);   // 倒数第2个字符
    int8_t b = base64_char_to_val(input[input_len - 1u]);   // 倒数第1个字符

    if (a < 0 || b < 0) {
      return -2;        // 包含无效字符
    }

    output[o++] = (uint8_t)((((uint16_t)a << 6) | (uint16_t)b) >> 4);  // 组合后右移4位
  } else if (rem == 3u) {   // 剩余3个字符，可解码出2个字节
    int8_t a = base64_char_to_val(input[input_len - 3u]);   // 倒数第3个字符
    int8_t b = base64_char_to_val(input[input_len - 2u]);   // 倒数第2个字符
    int8_t c = base64_char_to_val(input[input_len - 1u]);   // 倒数第1个字符
    uint32_t v;                                             // 临时变量

    if (a < 0 || b < 0 || c < 0) {
      return -2;        // 包含无效字符
    }

    v = ((uint32_t)a << 12) | ((uint32_t)b << 6) | (uint32_t)c;   // 组合3个字符
    output[o++] = (uint8_t)((v >> 10) & 0xFFu);    // 高8位
    output[o++] = (uint8_t)((v >> 2) & 0xFFu);     // 中8位
  }

  return (int16_t)o;    // 返回解码后数据长度
}

/**
 * @brief 计算CRC24Q校验值
 * 
 * @param data 数据指针
 * @param length 数据长度
 * @return CRC24Q校验值
 */
uint32_t crc24q_calculate(const uint8_t *data, uint16_t length) {
  uint32_t crc = CRC24Q_INIT;   // 初始化CRC值
  uint16_t i;                   // 外循环计数器
  uint8_t j;                    // 内循环计数器

  if (data == NULL) {
    return 0u;      // 数据指针为空则返回0
  }

  // 对每个字节进行CRC计算
  for (i = 0u; i < length; ++i) {
    crc ^= ((uint32_t)data[i] << 16);   // 将当前字节移到高位并与CRC异或
    // 对字节的每一位进行处理
    for (j = 0u; j < 8u; ++j) {
      crc <<= 1;                      // CRC左移一位
      if ((crc & 0x1000000u) != 0u) { // 检查最高位是否溢出
        crc ^= CRC24Q_POLY;           // 溢出则与多项式异或
      }
    }
  }

  return (crc & 0xFFFFFFu);           // 只保留低24位
}

/**
 * @brief 将CRC值转换为Base64编码字符串
 * 
 * @param crc CRC值
 * @param output 输出字符串指针
 */
void crc_to_base64(uint32_t crc, char *output) {
  if (output == NULL) {
    return;     // 输出指针为空则直接返回
  }

  encode_u24(crc, output);    // 将24位CRC值编码为Base64字符
  output[4] = '\0';           // 添加字符串结束符
}

/**
 * @brief 从Base64编码字符串解析CRC值
 * 
 * @param input 输入字符串指针
 * @return CRC值，失败返回0xFFFFFFFFu
 */
uint32_t crc_from_base64(const char *input) {
  uint32_t v = 0u;            // 临时存储解析出的值

  if (input == NULL) {
    return 0xFFFFFFFFu;       // 输入指针为空则返回错误值
  }

  if (!decode_u24(input, &v)) {   // 尝试解码24位值
    return 0xFFFFFFFFu;       // 解码失败则返回错误值
  }
  return v;                   // 返回解码结果
}

/**
 * @brief 构建协议帧
 * 
 * @param frame 帧输出缓冲区
 * @param frame_size 帧缓冲区大小
 * @param type 消息类型
 * @param seq 序列号
 * @param content 内容字符串
 * @param content_len 内容长度
 * @return 实际构建的帧长度
 */
uint16_t build_frame(char *frame, uint16_t frame_size, msg_type_t type,
                     uint8_t seq, const char *content, uint16_t content_len) {
  uint16_t pos = 0u;          // 当前写入位置
  uint32_t crc;               // CRC校验值
  char crc_str[5];            // CRC编码字符串

  // 检查参数有效性
  if (frame == NULL) {
    return 0u;
  }
  if (frame_size < (uint16_t)(content_len + 8u + 1u)) {  // # + type + seq + content + crc + $
    return 0u;
  }
  if (content_len > PROTOCOL_MAX_FRAME_LEN) {
    return 0u;
  }

  // 写入帧起始标记
  frame[pos++] = '#';
  // 写入消息类型
  frame[pos++] = (char)type;
  // 写入序列号的Base64编码
  frame[pos++] = base64_val_to_char((uint8_t)(seq & 0x3Fu));

  // 写入内容部分
  if (content != NULL && content_len > 0u) {
    DATA_PROTOCOL_MEMCPY(&frame[pos], content, content_len);
    pos = (uint16_t)(pos + content_len);
  }

  // 计算并写入CRC校验
  crc = crc24q_calculate((const uint8_t *)frame, pos);  // 计算帧起始到内容结束的CRC
  crc_to_base64(crc, crc_str);                         // 将CRC转换为Base64编码
  DATA_PROTOCOL_MEMCPY(&frame[pos], crc_str, 4u);      // 写入4个字符的CRC
  pos = (uint16_t)(pos + 4u);

  // 写入帧结束标记
  frame[pos++] = '$';
  frame[pos] = '\0';    // 添加字符串结束符
  return pos;           // 返回帧总长度
}

/**
 * @brief 解析协议帧
 * 
 * @param frame 帧输入缓冲区
 * @param frame_len 帧长度
 * @param type 消息类型输出
 * @param seq 序列号输出
 * @param content_buf 内容缓冲区
 * @param content_len 内容长度输出
 * @param crc_received 接收的CRC值
 * @return 帧解析状态
 */
frame_status_t parse_frame(const char *frame, uint16_t frame_len,
                           msg_type_t *type, uint8_t *seq, char *content_buf,
                           uint16_t *content_len, uint32_t *crc_received) {
  uint16_t content_start = 3u;    // 内容开始位置：'#' + type + seq = 3个字符
  uint16_t content_end;           // 内容结束位置
  uint32_t crc_calc;              // 计算出的CRC值
  char crc_str[4];                // 接收到的CRC字符串
  int8_t seq_v;                   // 解析出的序列号值

  // 检查参数有效性
  if (frame == NULL || type == NULL || seq == NULL || content_len == NULL ||
      crc_received == NULL) {
    return FRAME_ERR_INCOMPLETE;
  }

  // 检查帧长度和起始/结束标记
  if (frame_len < 7u) {                           // 最小帧长度：# + type + seq + crc(4) + $ = 7
    return FRAME_ERR_INCOMPLETE;
  }
  if (frame[0] != '#') {                          // 检查起始标记
    return FRAME_ERR_INVALID_START;
  }
  if (frame[frame_len - 1u] != '$') {             // 检查结束标记
    return FRAME_ERR_INVALID_END;
  }

  *type = (msg_type_t)frame[1];                   // 提取消息类型

  seq_v = base64_char_to_val(frame[2]);           // 解析序列号
  if (seq_v < 0) {                                // 检查序列号字符是否有效
    return FRAME_ERR_INVALID_CHAR;
  }
  *seq = (uint8_t)seq_v;

  content_end = (uint16_t)(frame_len - 5u);       // 计算内容结束位置：去掉起始3字符+crc 4字符+结束1字符
  if (content_end < content_start) {              // 检查内容长度是否合理
    return FRAME_ERR_INCOMPLETE;
  }

  *content_len = (uint16_t)(content_end - content_start);     // 计算内容长度
  if (content_buf != NULL && *content_len > 0u) {             // 如果需要提取内容
    DATA_PROTOCOL_MEMCPY(content_buf, &frame[content_start], *content_len);
    content_buf[*content_len] = '\0';             // 添加字符串结束符
  }

  DATA_PROTOCOL_MEMCPY(crc_str, &frame[content_end], 4u);     // 提取接收到的CRC
  *crc_received = crc_from_base64(crc_str);       // 解析CRC值

  crc_calc = crc24q_calculate((const uint8_t *)frame, content_end);  // 计算帧的CRC
  if (*crc_received != crc_calc) {                // 比较接收到的CRC和计算出的CRC
    return FRAME_ERR_CRC;
  }

  return FRAME_OK;                                // 解析成功
}

/**
 * @brief 检查帧是否完整
 * 
 * @param frame 帧输入缓冲区
 * @param frame_len 帧长度
 * @param frame_start 帧起始位置输出
 * @param frame_end 帧结束位置输出
 * @return 是否完整
 */
bool is_frame_complete(const char *frame, uint16_t frame_len,
                       uint16_t *frame_start, uint16_t *frame_end) {
  uint16_t i;         // 起始位置搜索索引
  uint16_t j;         // 结束位置搜索索引

  if (frame == NULL) {
    return false;     // 输入指针为空则返回false
  }

  // 遍历查找帧的起始和结束标记
  for (i = 0u; i < frame_len; ++i) {
    if (frame[i] != '#') {          // 不是起始标记则继续寻找
      continue;
    }

    for (j = (uint16_t)(i + 1u); j < frame_len; ++j) {
      if (frame[j] == '$' && (uint16_t)(j - i + 1u) >= 7u) {   // 找到结束标记且长度足够
        if (frame_start != NULL) {
          *frame_start = i;         // 输出起始位置
        }
        if (frame_end != NULL) {
          *frame_end = j;           // 输出结束位置
        }
        return true;                // 帧完整
      }
    }
  }

  return false;                     // 未找到完整帧
}

/**
 * @brief 构建查询消息
 * 
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param seq 序列号
 * @return 消息长度
 */
uint16_t build_query_msg(char *buffer, uint16_t buffer_size, uint8_t seq) {
  return build_frame(buffer, buffer_size, MSG_QUERY, seq, NULL, 0u);  // 构建无内容的查询帧
}

/**
 * @brief 构建重复查询消息
 * 
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param seq 序列号
 * @return 消息长度
 */
uint16_t build_repeat_query_msg(char *buffer, uint16_t buffer_size,
                                uint8_t seq) {
  return build_frame(buffer, buffer_size, MSG_REPEAT_QUERY, seq, NULL, 0u);  // 构建无内容的重复查询帧
}

/**
 * @brief 构建设置参数消息
 * 
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param seq 序列号
 * @param params 参数数据
 * @return 消息长度
 */
uint16_t build_set_params_msg(char *buffer, uint16_t buffer_size, uint8_t seq,
                              const param_data_decoded_t *params) {
  char content[PARAM_CONTENT_LEN + 1u];   // 参数内容字符串
  uint16_t pos = 0u;                      // 当前写入位置
  char tmp[6];                            // 临时存储编码结果

  // 检查参数有效性
  if (buffer == NULL || params == NULL) {
    return 0u;
  }

  // 编码T1参数为4个Base64字符
  encode_u24(params->T1, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 4u);
  pos = (uint16_t)(pos + 4u);

  // 编码T2参数为4个Base64字符
  encode_u24(params->T2, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 4u);
  pos = (uint16_t)(pos + 4u);

  // 编码T3参数为4个Base64字符
  encode_u24(params->T3, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 4u);
  pos = (uint16_t)(pos + 4u);

  // 编码T4参数为4个Base64字符
  encode_u24(params->T4, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 4u);
  pos = (uint16_t)(pos + 4u);

  // 编码高阈值为2个Base64字符
  encode_u12(params->threshold_high, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 2u);
  pos = (uint16_t)(pos + 2u);

  // 编码低阈值为2个Base64字符
  encode_u12(params->threshold_low, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 2u);
  pos = (uint16_t)(pos + 2u);

  // 编码主时间为6个Base64字符
  encode_u36(params->master_time, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 6u);
  pos = (uint16_t)(pos + 6u);

  content[pos] = '\0';    // 添加字符串结束符
  return build_frame(buffer, buffer_size, MSG_SET_PARAMS, seq, content, pos);  // 构建参数设置帧
}

/**
 * @brief 构建报告消息
 * 
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param seq 序列号
 * @param type 消息类型
 * @param data 报告数据
 * @return 消息长度
 */
uint16_t build_report_msg(char *buffer, uint16_t buffer_size, uint8_t seq,
                          msg_type_t type, const report_data_decoded_t *data) {
  char content[PROTOCOL_MAX_REPORT_CONTENT + 1u];   // 报告内容字符串
  uint16_t pos = 0u;                                  // 当前写入位置
  uint16_t i;                                         // 循环计数器
  char tmp[6];                                        // 临时存储编码结果
  uint16_t fixed_pairs = 0u;                          // 固定字段对数量

  // 检查参数有效性
  if (buffer == NULL || data == NULL) {
    return 0u;
  }

  // 编码标签ID为6个Base64字符
  encode_u36(data->tag_id, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 6u);
  pos = (uint16_t)(pos + 6u);

  // 编码开始时间（小时、分钟、秒）
  content[pos++] = base64_val_to_char((uint8_t)(data->start_hour & 0x3Fu));
  content[pos++] = base64_val_to_char((uint8_t)(data->start_minute & 0x3Fu));
  content[pos++] = base64_val_to_char((uint8_t)(data->start_second & 0x3Fu));

  // 编码序列号为3个Base64字符
  encode_u18(data->sequence, tmp);
  DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 3u);
  pos = (uint16_t)(pos + 3u);

  // 如果包含温湿度数据
  if (data->has_temp_humidity) {
    // 编码温度为2个Base64字符
    encode_u12(data->temperature, tmp);
    DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 2u);
    pos = (uint16_t)(pos + 2u);

    // 编码湿度为2个Base64字符
    encode_u12(data->humidity, tmp);
    DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 2u);
    pos = (uint16_t)(pos + 2u);

    fixed_pairs = 2u;   // 标记已有2个固定字段对（温度、湿度）
  }

  // 编码加速度数据
  for (i = 0u; i < data->accel_count; ++i) {
    if (data->acceleration == NULL) {     // 检查加速度数组是否为空
      return 0u;
    }
    // 检查是否会超出最大报告内容长度
    if ((uint16_t)(12u + (fixed_pairs + i + 1u) * 2u) > PROTOCOL_MAX_REPORT_CONTENT) {
      return 0u;
    }
    // 编码单个加速度值为2个Base64字符
    encode_u12(data->acceleration[i], tmp);
    DATA_PROTOCOL_MEMCPY(&content[pos], tmp, 2u);
    pos = (uint16_t)(pos + 2u);
  }

  content[pos] = '\0';    // 添加字符串结束符
  return build_frame(buffer, buffer_size, type, seq, content, pos);  // 构建报告帧
}

/**
 * @brief 构建确认消息
 * 
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param seq 序列号
 * @param original_content 原始内容
 * @param content_len 内容长度
 * @return 消息长度
 */
uint16_t build_ack_msg(char *buffer, uint16_t buffer_size, uint8_t seq,
                       const char *original_content, uint16_t content_len) {
  return build_frame(buffer, buffer_size, MSG_ACK_PARAMS, seq, original_content,
                     content_len);   // 构建参数确认帧
}

/**
 * @brief 解析报告数据
 * 
 * @param encoded_data 编码的数据
 * @param data_len 数据长度
 * @param report 报告数据结构体输出
 * @return 是否解析成功
 */
bool parse_report_data(const char *encoded_data, uint16_t data_len,
                       report_data_decoded_t *report) {
  uint16_t pos = 0u;        // 当前读取位置
  uint16_t remain;          // 剩余数据长度
  uint16_t i;               // 循环计数器

  // 检查参数有效性
  if (encoded_data == NULL || report == NULL || data_len < 12u) {
    return false;           // 数据长度至少要能容纳基础字段
  }

  // 解码标签ID（36位，6个Base64字符）
  if (!decode_u36(&encoded_data[pos], &report->tag_id)) {
    return false;
  }
  pos = (uint16_t)(pos + 6u);

  // 解码开始时间（小时、分钟、秒，各占1个Base64字符）
  {
    int8_t h = base64_char_to_val(encoded_data[pos + 0u]);  // 小时
    int8_t m = base64_char_to_val(encoded_data[pos + 1u]);  // 分钟
    int8_t s = base64_char_to_val(encoded_data[pos + 2u]);  // 秒
    if (h < 0 || m < 0 || s < 0) {
      return false;         // 时间字符无效
    }
    report->start_hour = (uint8_t)h;
    report->start_minute = (uint8_t)m;
    report->start_second = (uint8_t)s;
  }
  pos = (uint16_t)(pos + 3u);

  // 解码序列号（18位，3个Base64字符）
  if (!decode_u18(&encoded_data[pos], &report->sequence)) {
    return false;
  }
  pos = (uint16_t)(pos + 3u);

  remain = (uint16_t)(data_len - pos);    // 计算剩余数据长度

  // 初始化报告数据结构
  report->temperature = 0u;
  report->humidity = 0u;
  report->accel_count = 0u;
  report->has_temp_humidity = false;

  // 检查是否有温湿度数据（需要至少4个字符，温度2个+湿度2个）
  if (remain >= 4u && (remain % 2u) == 0u) {
    uint16_t t, h;
    if (decode_u12(&encoded_data[pos], &t) &&       // 解码温度
        decode_u12(&encoded_data[pos + 2u], &h)) {  // 解码湿度
      report->temperature = t;
      report->humidity = h;
      report->has_temp_humidity = true;             // 标记包含温湿度数据
      pos = (uint16_t)(pos + 4u);                   // 移动到加速度数据位置
      remain = (uint16_t)(data_len - pos);
    }
  }

  // 检查剩余数据长度是否为偶数（加速度数据每项占2个字符）
  if ((remain % 2u) != 0u) {
    return false;
  }

  report->accel_count = (uint16_t)(remain / 2u);    // 计算加速度数据项数
  if (report->accel_count > 0u && report->acceleration != NULL) {
    // 如果提供了加速度数组指针，解码所有加速度值
    for (i = 0u; i < report->accel_count; ++i) {
      if (!decode_u12(&encoded_data[pos + (uint16_t)(i * 2u)],    // 每项占2个字符
                      &report->acceleration[i])) {
        return false;       // 解码失败
      }
    }
  }

  return true;              // 解析成功
}

/**
 * @brief 解析参数数据
 * 
 * @param encoded_data 编码的数据
 * @param data_len 数据长度
 * @param params 参数数据结构体输出
 * @return 是否解析成功
 */
bool parse_param_data(const char *encoded_data, uint16_t data_len,
                      param_data_decoded_t *params) {
  uint16_t pos = 0u;        // 当前读取位置

  // 检查参数有效性（参数数据长度应为26个字符）
  if (encoded_data == NULL || params == NULL || data_len != PARAM_CONTENT_LEN) {
    return false;
  }

  // 解码T1参数（24位，4个Base64字符）
  if (!decode_u24(&encoded_data[pos], &params->T1)) {
    return false;
  }
  pos = (uint16_t)(pos + 4u);

  // 解码T2参数（24位，4个Base64字符）
  if (!decode_u24(&encoded_data[pos], &params->T2)) {
    return false;
  }
  pos = (uint16_t)(pos + 4u);

  // 解码T3参数（24位，4个Base64字符）
  if (!decode_u24(&encoded_data[pos], &params->T3)) {
    return false;
  }
  pos = (uint16_t)(pos + 4u);

  // 解码T4参数（24位，4个Base64字符）
  if (!decode_u24(&encoded_data[pos], &params->T4)) {
    return false;
  }
  pos = (uint16_t)(pos + 4u);

  // 解码高阈值（12位，2个Base64字符）
  if (!decode_u12(&encoded_data[pos], &params->threshold_high)) {
    return false;
  }
  pos = (uint16_t)(pos + 2u);

  // 解码低阈值（12位，2个Base64字符）
  if (!decode_u12(&encoded_data[pos], &params->threshold_low)) {
    return false;
  }
  pos = (uint16_t)(pos + 2u);

  // 解码主时间（36位，6个Base64字符）
  if (!decode_u36(&encoded_data[pos], &params->master_time)) {
    return false;
  }

  return true;              // 解析成功
}

/**
 * @brief 温度值转换为12位表示
 * 
 * @param temp_celsius 摄氏温度值
 * @return 12位温度表示
 */
uint16_t temperature_to_12bit(float temp_celsius) {
  // 将摄氏温度转换为0-4000的整数表示(-200°C至+200°C)
  int32_t temp_deci = (int32_t)((temp_celsius + 200.0f) * 10.0f);
  if (temp_deci < 0) {
    temp_deci = 0;          // 限制最小值
  }
  if (temp_deci > 4000) {
    temp_deci = 4000;       // 限制最大值
  }
  return (uint16_t)temp_deci;   // 返回12位表示
}

/**
 * @brief 从12位表示转换为温度值
 * 
 * @param data 12位温度表示
 * @return 摄氏温度值
 */
float temperature_from_12bit(uint16_t data) {
  // 将12位表示转换回摄氏温度值
  return ((float)(data & 0x0FFFu) / 10.0f) - 200.0f;
}

/**
 * @brief 湿度值转换为12位表示
 * 
 * @param humidity 湿度值
 * @return 12位湿度表示
 */
uint16_t humidity_to_12bit(float humidity) {
  // 将湿度值转换为0-1000的整数表示(0%至100%)
  int32_t hum_deci = (int32_t)(humidity * 10.0f);
  if (hum_deci < 0) {
    hum_deci = 0;           // 限制最小值
  }
  if (hum_deci > 1000) {
    hum_deci = 1000;        // 限制最大值
  }
  return (uint16_t)hum_deci;    // 返回12位表示
}

/**
 * @brief 从12位表示转换为湿度值
 * 
 * @param data 12位湿度表示
 * @return 湿度值
 */
float humidity_from_12bit(uint16_t data) {
  // 将12位表示转换回湿度值
  return (float)(data & 0x0FFFu) / 10.0f;
}

/**
 * @brief 加速度值转换为12位表示
 * 
 * @param accel_g 加速度(g单位)
 * @param full_scale 满量程范围
 * @return 12位加速度表示
 */
uint16_t acceleration_to_12bit(float accel_g, float full_scale) {
  float normalized;         // 归一化后的值
  uint16_t value;           // 最终12位值

  if (full_scale <= 0.0f) {
    return 0u;              // 满量程无效则返回0
  }

  // 将加速度归一化到0-4095范围内
  normalized = (accel_g / full_scale) * 4095.0f;
  if (normalized < 0.0f) {
    normalized = 0.0f;      // 限制最小值
  }
  if (normalized > 4095.0f) {
    normalized = 4095.0f;   // 限制最大值
  }

  value = (uint16_t)normalized;
  return (uint16_t)(value & 0x0FFFu);   // 确保只有低12位有效
}

/**
 * @brief 从12位表示转换为加速度值
 * 
 * @param data 12位加速度表示
 * @param full_scale 满量程范围
 * @return 加速度(g单位)
 */
float acceleration_from_12bit(uint16_t data, float full_scale) {
  if (full_scale <= 0.0f) {
    return 0.0f;            // 满量程无效则返回0
  }
  // 将12位表示转换回实际加速度值
  return ((float)(data & 0x0FFFu) / 4095.0f) * full_scale;
}

/**
 * @brief 以十六进制形式打印数据
 * 
 * @param data 数据指针
 * @param length 数据长度
 */
void hex_dump(const uint8_t *data, uint16_t length) {
#if PROTOCOL_ENABLE_DEBUG_PRINT
  uint16_t i;
  if (data == NULL) {
    return;
  }
  for (i = 0u; i < length; ++i) {
    printf("%02X ", data[i]);    // 打印两位十六进制数
    if (((uint16_t)(i + 1u) % 16u) == 0u) {   // 每16个字节换行
      printf("\n");
    }
  }
  printf("\n");                 // 最后换行
#else
  (void)data;
  (void)length;
#endif
}

/**
 * @brief 打印帧内容
 * 
 * @param frame 帧内容
 * @param length 帧长度
 */
void print_frame(const char *frame, uint16_t length) {
#if PROTOCOL_ENABLE_DEBUG_PRINT
  uint16_t i;
  if (frame == NULL) {
    return;
  }

  printf("Frame (%u bytes): ", length);
  for (i = 0u; i < length; ++i) {
    char c = frame[i];
    if (c >= 32 && c <= 126) {      // 打印可打印字符
      printf("%c", c);
    } else {                        // 不可打印字符用点代替
      printf(".");
    }
  }
  printf("\nHex: ");
  for (i = 0u; i < length; ++i) {
    printf("%02X ", (uint8_t)frame[i]);   // 打印十六进制表示
  }
  printf("\n");
#else
  (void)frame;
  (void)length;
#endif
}

/**
 * @brief 打印报告数据
 * 
 * @param report 报告数据
 */
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

/**
 * @brief 打印参数数据
 * 
 * @param params 参数数据
 */
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