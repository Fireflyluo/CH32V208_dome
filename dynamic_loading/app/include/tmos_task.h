#ifndef _TMOS_TASK_H_
#define _TMOS_TASK_H_

#include "wchble.h"
#include "stdint.h"
#include <string.h> 
#include "debug.h"

#define LED_PIN GET_PIN(C, 9)

// 自定义消息结构
typedef struct {
    tmos_event_hdr_t hdr;
    uint8_t data[]; // 实际使用时需动态分配
} tmos_var_msg_t;

void led_task_init(void);

// led 任务事件定义
#define DEMO_TASK_TMOS_EVT_TEST_1 (0x0001 << 0)
#define DEMO_TASK_TMOS_EVT_TEST_2 (0x0001 << 1)
#define DEMO_TASK_TMOS_EVT_TEST_3 (0x0001 << 2)
#define DEMO_TASK_TMOS_EVT_TEST_4 (0x0001 << 3)
#define DEMO_TASK_TMOS_EVT_TEST_5 (0x0001 << 4)

// ledMsg 任务事件定义
#define TEST_MSG_EVENT_1 (0x0001 << 0)
#define TEST_MSG_EVENT_2 (0x0001 << 1)

    void demo_task_init(void);

#endif
