//**************************************************************************
//* File Name          : ch32v208_example.c
//* Description        : AROS-RF 协议栈 CH32V208 使用示例
//**************************************************************************

//**************************************************************************
#include "aros_rf.h"
#include "ch32v20x_it.h"
#include "system_ch32v20x.h"


//**************************************************************************
// 应用配置
//**************************************************************************

// 定义标签 ID (0-20)
#define TAG_ID 5

// 定时器配置
#define TIMER0_PERIOD_US 1000000 // 1 秒
#define TIMER1_PERIOD_US 1500000 // 1.5 秒
#define TIMER2_PERIOD_US 2400000 // 2.4 秒

//**************************************************************************
// 全局变量
//**************************************************************************

static uint16_t tx_counter = 0;

//**************************************************************************
// 回调函数实现
//**************************************************************************

/**
 * @brief 处理函数（在 arf_proc 中调用）
 */
void app_proc(void) {
  // 应用层处理逻辑
  // 可以在这里执行周期性任务
}

/**
 * @brief 新数据回调函数
 * @param dat - 接收到的数据指针（24 字节数据区）
 * @return 0 - 无新数据，非 0 - 有新数据
 */
int app_newdat(uint8_t *dat) {
  // 检查数据有效性
  if (dat == NULL) {
    return 0;
  }

  // 处理接收到的数据
  // dat[0:23] 为数据区
  uint8_t cmd = dat[0];
  uint16_t value = ((uint16_t)dat[1] << 8) | dat[2];

  printf("Received: cmd=0x%02X, value=%d\n", cmd, value);

  return 1; // 表示有新数据
}

/**
 * @brief 定时器 0 回调（1 秒）
 */
void timer0_callback(void) {
  printf("Timer0 triggered: %u\n", (unsigned int)arf_get_tc());
}

/**
 * @brief 定时器 1 回调（1.5 秒）
 */
void timer1_callback(void) {
  printf("Timer1 triggered: %u\n", (unsigned int)arf_get_tc());
}

/**
 * @brief 定时器 2 回调（2.4 秒）
 */
void timer2_callback(void) {
  printf("Timer2 triggered: %u\n", (unsigned int)arf_get_tc());
}

//**************************************************************************
// 发送数据示例
//**************************************************************************

/**
 * @brief 发送测试数据
 */
void send_test_data(void) {
  uint8_t txbuf[ARF_MsgN];

  // 准备发送数据
  memset(txbuf, 0, ARF_MsgN);

  // 填充消息格式：cmd(2b) slot(9b) lv(5b)
  txbuf[0] = (1 << 6) | (0 << 0); // cmd=1, slot=0
  txbuf[1] = 0;                   // slot_low=0, lv=0

  // 填充标签 ID
  arf_u32toa(txbuf + 2, TAG_ID);

  // 填充数据序号
  arf_u16toa(txbuf + 6, tx_counter);
  tx_counter++;

  // 填充测试数据（剩余字节）
  for (int i = 8; i < ARF_MsgN; i++) {
    txbuf[i] = (uint8_t)(i + tx_counter);
  }

  // 发送数据
  printf("Sending data: counter=%u\n", tx_counter);
  arf_TxSend(txbuf, ARF_MsgN);
}

//**************************************************************************
// 主函数
//**************************************************************************

int main(void) {
  //====================================================================
  // 系统初始化
  //====================================================================

  // 1. 初始化系统时钟和延时函数
  arf_SysInit();

  printf("\n=== AROS-RF Protocol Stack - CH32V208 Example ===\n");
  printf("Tag ID: %d\n", TAG_ID);
  printf("Initializing...\n");

  //====================================================================
  // 协议栈初始化
  //====================================================================

  // 2. 初始化 RF 协议栈
  arf_Init();
  printf("RF Protocol stack initialized\n");

  //====================================================================
  // 配置回调函数
  //====================================================================

  // 3. 设置处理函数
  arf_set_procfunc(app_proc);

  // 4. 设置新数据回调函数
  arf_set_newdatfunc(app_newdat);
  printf("Callback functions configured\n");

  //====================================================================
  // 配置定时器
  //====================================================================

  // 5. 设置三个定时器
  arf_set_timer(0, TIMER0_PERIOD_US, timer0_callback); // 1 秒周期
  arf_set_timer(1, TIMER1_PERIOD_US, timer1_callback); // 1.5 秒周期
  arf_set_timer(2, TIMER2_PERIOD_US, timer2_callback); // 2.4 秒周期
  printf("Timers configured\n");

  //====================================================================
  // 配置 LED
  //====================================================================

  // 6. 设置 LED 模式（接收和发送时闪烁）
  arf_led_mod(ARF_LED_RX | ARF_LED_TX | ARF_LED_SEND);

  // 7. 闪烁 LED 显示 ID
  arf_led_id(TAG_ID);
  printf("LED configured\n");

  //====================================================================
  // 延时启动
  //====================================================================

  // 8. 等待系统稳定
  arf_delayms(1000);
  printf("System ready, starting main loop...\n\n");

  //====================================================================
  // 主循环
  //====================================================================

  uint32_t loop_count = 0;
  uint32_t last_send_tc = 0;
  const uint32_t send_interval = 5000000; // 5 秒发送一次

  for (;;) {
    // 处理协议栈事件
    arf_proc();

    // 检查接收数据
    uint8_t *rxbuf;
    while ((rxbuf = arf_isRxFinish()) != NULL) {
      uint8_t rssi = arf_RSSI(rxbuf);
      uint16_t tc = arf_u16toi(rxbuf + ARF_MsgTC);

      printf("RX: RSSI=%d, TC=%u, Data[0]=0x%02X\n", rssi, (unsigned int)tc,
             rxbuf[0]);
    }

    // 周期性发送测试数据
    uint32_t current_tc = arf_get_tc();
    if (current_tc - last_send_tc >= send_interval) {
      send_test_data();
      last_send_tc = current_tc;
    }

    // 每 10000 次循环打印一次状态
    if (++loop_count % 10000 == 0) {
      printf("Loop count: %u, TC: %u\n", (unsigned int)loop_count,
             (unsigned int)current_tc);
    }
  }

  // 永不返回
  return 0;
}

//**************************************************************************
// 中断服务函数（可选）
//**************************************************************************

/**
 * @brief 系统滴答定时器中断
 * 用于提供精确的时间基准
 */
void SysTick_Handler(void) {
  // 如果使用外部时间基准，在这里更新
  // arf_proc() 会在主循环中调用
}

//**************************************************************************
// 硬件抽象层实现（如果 BLE-HAL 未提供）
//**************************************************************************

#ifdef CUSTOM_HAL_IMPLEMENTATION

// LED 控制实现
void LED_on(void) {
  GPIO_WriteBit(GPIOC, GPIO_Pin_9, Bit_RESET); // 低电平点亮
}

void LED_off(void) {
  GPIO_WriteBit(GPIOC, GPIO_Pin_9, Bit_SET); // 高电平熄灭
}

#endif // CUSTOM_HAL_IMPLEMENTATION

//**************************************************************************
// endfile @ ch32v208_example.c
//**************************************************************************