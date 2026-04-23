/**
 * @file data_protocol.h
 * @brief 数据协议处理头文件
 * 
 * 定义了数据协议相关的常量、枚举、结构体和函数接口
 */

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

/**
 * @enum msg_type_t
 * @brief 消息类型枚举
 */
typedef enum {
  MSG_UNKNOWN = 0,          /**< 未知消息类型 */

  MSG_QUERY = 'S',          /**< 查询请求 */
  MSG_REPEAT_QUERY = 'E',   /**< 重复查询请求 */
  MSG_SET_PARAMS = 'P',     /**< 设置参数 */

  MSG_REPORT_FIRST = 'i',   /**< 首次报告 */
  MSG_REPORT_MIDDLE = 'm',  /**< 中间报告 */
  MSG_REPORT_LAST = 'h',    /**< 最后报告 */
  MSG_REPORT_NONE = 'n',    /**< 无报告 */
  MSG_ACK_PARAMS = 'q',     /**< 参数确认响应 */
} msg_type_t;

/**
 * @enum frame_status_t
 * @brief 帧解析状态枚举
 */
typedef enum {
  FRAME_OK = 0,                 /**< 帧解析成功 */
  FRAME_ERR_INVALID_START = -1, /**< 帧起始标记无效 */
  FRAME_ERR_INVALID_END = -2,   /**< 帧结束标记无效 */
  FRAME_ERR_CRC = -3,           /**< CRC校验失败 */
  FRAME_ERR_TOO_LONG = -4,      /**< 帧长度超限 */
  FRAME_ERR_INVALID_CHAR = -5,  /**< 帧中包含非法字符 */
  FRAME_ERR_INCOMPLETE = -6     /**< 帧不完整 */
} frame_status_t;

/**
 * @struct frame_header_t
 * @brief 帧头部结构体
 */
typedef struct PACKED {
  char start_mark;              /**< 起始标记 */
  msg_type_t msg_type;          /**< 消息类型 */
  char seq_char;                /**< 序列号字符 */
  char *content;                /**< 内容指针 */
  char crc[4];                  /**< CRC校验码 */
  char end_mark;                /**< 结束标记 */
} frame_header_t;

/**
 * @struct report_data_decoded_t
 * @brief 解码后的报告数据结构体
 */
typedef struct {
  uint64_t tag_id;              /**< 标签ID，低36位有效 */
  uint8_t start_hour;           /**< 开始小时，6位 */
  uint8_t start_minute;         /**< 开始分钟，6位 */
  uint8_t start_second;         /**< 开始秒数，6位 */
  uint32_t sequence;            /**< 序列号，低18位有效 */
  uint16_t temperature;         /**< 温度值，12位 */
  uint16_t humidity;            /**< 湿度值，12位 */
  uint16_t *acceleration;       /**< 加速度数组，每项12位 */
  uint16_t accel_count;         /**< 加速度数据数量 */
  bool has_temp_humidity;       /**< 是否包含温湿度数据 */
} report_data_decoded_t;

/**
 * @struct param_data_decoded_t
 * @brief 解码后的参数数据结构体
 */
typedef struct {
  uint32_t T1;                  /**< 时间参数T1，低24位有效 */
  uint32_t T2;                  /**< 时间参数T2，低24位有效 */
  uint32_t T3;                  /**< 时间参数T3，低24位有效 */
  uint32_t T4;                  /**< 时间参数T4，低24位有效 */
  uint16_t threshold_high;      /**< 高阈值，低12位有效 */
  uint16_t threshold_low;       /**< 低阈值，低12位有效 */
  uint64_t master_time;         /**< 主时间，低36位有效 */
} param_data_decoded_t;

#ifndef __GNUC__
#pragma pack(pop)
#endif

/**
 * @brief 获取Base64编码表
 * 
 * @return Base64编码表字符串指针
 */
const char *get_base64_table(void);

/**
 * @brief Base64编码
 * 
 * @param input 输入数据指针
 * @param input_len 输入数据长度
 * @param output 输出字符串指针
 * @return 编码后字符串长度
 */
uint16_t base64_encode(const uint8_t *input, uint16_t input_len, char *output);

/**
 * @brief Base64解码
 * 
 * @param input 输入字符串指针
 * @param input_len 输入字符串长度
 * @param output 输出数据指针
 * @return 解码后数据长度，失败返回负值
 */
int16_t base64_decode(const char *input, uint16_t input_len, uint8_t *output);

/**
 * @brief 计算CRC24Q校验值
 * 
 * @param data 数据指针
 * @param length 数据长度
 * @return CRC24Q校验值
 */
uint32_t crc24q_calculate(const uint8_t *data, uint16_t length);

/**
 * @brief 将CRC值转换为Base64编码字符串
 * 
 * @param crc CRC值
 * @param output 输出字符串指针
 */
void crc_to_base64(uint32_t crc, char *output);

/**
 * @brief 从Base64编码字符串解析CRC值
 * 
 * @param input 输入字符串指针
 * @return CRC值，失败返回0xFFFFFFFFu
 */
uint32_t crc_from_base64(const char *input);

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
                     uint8_t seq, const char *content, uint16_t content_len);

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
                           uint16_t *content_len, uint32_t *crc_received);

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
                       uint16_t *frame_start, uint16_t *frame_end);

/**
 * @brief 构建查询消息
 * 
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param seq 序列号
 * @return 消息长度
 */
uint16_t build_query_msg(char *buffer, uint16_t buffer_size, uint8_t seq);

/**
 * @brief 构建重复查询消息
 * 
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param seq 序列号
 * @return 消息长度
 */
uint16_t build_repeat_query_msg(char *buffer, uint16_t buffer_size,
                                uint8_t seq);

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
                              const param_data_decoded_t *params);

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
                          msg_type_t type, const report_data_decoded_t *data);

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
                       const char *original_content, uint16_t content_len);

/**
 * @brief 解析报告数据
 * 
 * @param encoded_data 编码的数据
 * @param data_len 数据长度
 * @param report 报告数据结构体输出
 * @return 是否解析成功
 */
bool parse_report_data(const char *encoded_data, uint16_t data_len,
                       report_data_decoded_t *report);

/**
 * @brief 解析参数数据
 * 
 * @param encoded_data 编码的数据
 * @param data_len 数据长度
 * @param params 参数数据结构体输出
 * @return 是否解析成功
 */
bool parse_param_data(const char *encoded_data, uint16_t data_len,
                      param_data_decoded_t *params);

/**
 * @brief 温度值转换为12位表示
 * 
 * @param temp_celsius 摄氏温度值
 * @return 12位温度表示
 */
uint16_t temperature_to_12bit(float temp_celsius);

/**
 * @brief 从12位表示转换为温度值
 * 
 * @param data 12位温度表示
 * @return 摄氏温度值
 */
float temperature_from_12bit(uint16_t data);

/**
 * @brief 湿度值转换为12位表示
 * 
 * @param humidity 湿度值
 * @return 12位湿度表示
 */
uint16_t humidity_to_12bit(float humidity);

/**
 * @brief 从12位表示转换为湿度值
 * 
 * @param data 12位湿度表示
 * @return 湿度值
 */
float humidity_from_12bit(uint16_t data);

/**
 * @brief 加速度值转换为12位表示
 * 
 * @param accel_g 加速度(g单位)
 * @param full_scale 满量程范围
 * @return 12位加速度表示
 */
uint16_t acceleration_to_12bit(float accel_g, float full_scale);

/**
 * @brief 从12位表示转换为加速度值
 * 
 * @param data 12位加速度表示
 * @param full_scale 满量程范围
 * @return 加速度(g单位)
 */
float acceleration_from_12bit(uint16_t data, float full_scale);

/**
 * @brief 以十六进制形式打印数据
 * 
 * @param data 数据指针
 * @param length 数据长度
 */
void hex_dump(const uint8_t *data, uint16_t length);

/**
 * @brief 打印帧内容
 * 
 * @param frame 帧内容
 * @param length 帧长度
 */
void print_frame(const char *frame, uint16_t length);

/**
 * @brief 打印报告数据
 * 
 * @param report 报告数据
 */
void print_report_data(const report_data_decoded_t *report);

/**
 * @brief 打印参数数据
 * 
 * @param params 参数数据
 */
void print_param_data(const param_data_decoded_t *params);

#endif // DATA_PROTOCOL_H