//**************************************************************************
//* File Name          : pc_sim_example.c
//* Description        : AROS-RF 协议栈 PC 模拟示例
//**************************************************************************

//**************************************************************************
#include "aros_rf.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


//**************************************************************************
// 应用配置
//**************************************************************************

#define TAG_ID 10
#define SIM_DURATION 30       // 模拟持续时间（秒）
#define SEND_INTERVAL 5000000 // 发送间隔（微秒）

//**************************************************************************
// 全局变量
//**************************************************************************

static uint16_t tx_counter = 0;
static FILE *log_file = NULL;

//**************************************************************************
// 辅助函数
//**************************************************************************

/**
 * @brief 打印带时间戳的日志
 */
void log_print(const char *format, ...) {
  char time_buf[64];
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);

  strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
  printf("[%s] ", time_buf);

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  if (log_file != NULL) {
    fprintf(log_file, "[%s] ", time_buf);
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    fflush(log_file);
  }
}

//**************************************************************************
// 回调函数实现
//**************************************************************************

/**
 * @brief 处理函数（在 arf_proc 中调用）
 */
void app_proc(void) {
  // 模拟应用层处理
  // 可以在这里添加周期性任务
}

/**
 * @brief 新数据回调函数
 * @param dat - 接收到的数据指针
 * @return 0 - 无新数据，非 0 - 有新数据
 */
int app_newdat(uint8_t *dat) {
  if (dat == NULL) {
    return 0;
  }

  uint8_t cmd = dat[0];
  uint16_t value = ((uint16_t)dat[1] << 8) | dat[2];
  uint32_t tag_id = arf_u32toi(dat + 2);
  uint16_t no = arf_u16toi(dat + 6);

  log_print("New data: cmd=0x%02X, tag_id=%u, no=%u, value=%u\n", cmd,
            (unsigned int)tag_id, no, value);

  return 1;
}

/**
 * @brief 定时器回调
 */
void timer_callback(void) {
  log_print("Timer triggered at TC=%u\n", (unsigned int)arf_get_tc());
}

//**************************************************************************
// 发送数据示例
//**************************************************************************

/**
 * @brief 发送测试数据
 */
void send_test_data(void) {
  uint8_t txbuf[ARF_MsgN];

  memset(txbuf, 0, ARF_MsgN);

  // 填充消息格式
  txbuf[0] = (1 << 6) | (0 << 0); // cmd=1, slot=0
  txbuf[1] = 0;                   // lv=0

  // 填充标签 ID
  arf_u32toa(txbuf + 2, TAG_ID);

  // 填充序号
  arf_u16toa(txbuf + 6, tx_counter);
  tx_counter++;

  // 填充测试数据
  for (int i = 8; i < ARF_MsgN; i++) {
    txbuf[i] = (uint8_t)(i + tx_counter);
  }

  log_print("Sending data: counter=%u\n", tx_counter);
  arf_TxSend(txbuf, ARF_MsgN);
}

/**
 * @brief 模拟接收数据（PC 模拟用）
 */
void simulate_receive(void) {
  static uint8_t simulated_rx[ARF_MsgN];
  static uint16_t sim_counter = 0;

  memset(simulated_rx, 0, ARF_MsgN);
  simulated_rx[0] = 0x02; // cmd=2
  simulated_rx[1] = 0;
  arf_u32toa(simulated_rx + 2, sim_counter % 100);
  arf_u16toa(simulated_rx + 6, sim_counter);
  sim_counter++;

  log_print("Simulated RX: counter=%u\n", sim_counter);

  // 在 PC 模拟模式下，可以通过 arf_newdatfunc 处理
  if (arf_newdatfunc != NULL) {
    (*arf_newdatfunc)(simulated_rx + 8);
  }
}

//**************************************************************************
// 主函数
//**************************************************************************

int main(int argc, char *argv[]) {
  //====================================================================
  // 初始化日志
  //====================================================================

  log_file = fopen("aros_rf_sim.log", "w");
  if (log_file == NULL) {
    printf("Warning: Failed to open log file\n");
  }

  printf("\n");
  printf("==============================================================\n");
  printf("  AROS-RF Protocol Stack - PC Simulation Example\n");
  printf("==============================================================\n");
  printf("Tag ID: %d\n", TAG_ID);
  printf("Simulation duration: %d seconds\n", SIM_DURATION);
  printf("==============================================================\n\n");

  //====================================================================
  // 系统初始化
  //====================================================================

  log_print("Initializing system...\n");
  arf_SysInit();

  //====================================================================
  // 协议栈初始化
  //====================================================================

  log_print("Initializing RF protocol stack...\n");
  arf_Init();

  //====================================================================
  // 配置回调函数
  //====================================================================

  arf_set_procfunc(app_proc);
  arf_set_newdatfunc(app_newdat);
  log_print("Callback functions configured\n");

  //====================================================================
  // 配置定时器
  //====================================================================

  arf_set_timer(0, 2000000, timer_callback); // 2 秒定时器
  log_print("Timer configured (2 seconds interval)\n");

  //====================================================================
  // 配置 LED
  //====================================================================

  arf_led_mod(ARF_LED_RX | ARF_LED_TX | ARF_LED_SEND);
  log_print("LED mode configured\n");

  //====================================================================
  // 延时启动
  //====================================================================

  arf_delayms(500);
  log_print("System ready, starting simulation...\n\n");

  //====================================================================
  // 主循环
  //====================================================================

  uint32_t start_tc = arf_get_tc();
  uint32_t last_send_tc = 0;
  uint32_t last_sim_rx_tc = 0;
  const uint32_t sim_rx_interval = 3000000; // 3 秒模拟一次接收
  uint32_t loop_count = 0;
  uint32_t duration_us = SIM_DURATION * 1000000;

  while (arf_get_tc() - start_tc < duration_us) {
    uint32_t current_tc = arf_get_tc();

    // 处理协议栈事件
    arf_proc();

    // 模拟接收数据（仅 PC 模拟）
    if (current_tc - last_sim_rx_tc >= sim_rx_interval) {
      simulate_receive();
      last_sim_rx_tc = current_tc;
    }

    // 周期性发送测试数据
    if (current_tc - last_send_tc >= SEND_INTERVAL) {
      send_test_data();
      last_send_tc = current_tc;
    }

    // 每隔一定时间打印状态
    if (++loop_count % 5000 == 0) {
      uint32_t elapsed = (current_tc - start_tc) / 1000000;
      log_print("Status: elapsed=%u sec, TC=%u\n", (unsigned int)elapsed,
                (unsigned int)current_tc);
    }

    // 短暂延时（避免 CPU 占用过高）
    arf_delayms(1);
  }

  //====================================================================
  // 结束
  //====================================================================

  log_print("\nSimulation completed\n");
  log_print("Total loops: %u\n", (unsigned int)loop_count);
  log_print("Total TX: %u\n", tx_counter);
  log_print("Final TC: %u\n", (unsigned int)arf_get_tc());

  printf("\n==============================================================\n");
  printf("  Simulation completed successfully!\n");
  printf("  Log file: aros_rf_sim.log\n");
  printf("==============================================================\n\n");

  if (log_file != NULL) {
    fclose(log_file);
  }

  return 0;
}

//**************************************************************************
// endfile @ pc_sim_example.c
//**************************************************************************