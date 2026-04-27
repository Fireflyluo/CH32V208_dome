#include "drv_gpio.h"
#include "tmos_task.h"

#define MSG_EVENT 0x10
#define MSG_TEST_EVENT 0x11
// 存储当前task id 的全局变量
tmosTaskID led_task_id = INVALID_TASK_ID;
tmosTaskID ledMsg_task_id = INVALID_TASK_ID;

uint8_t test_data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

static uint16_t led_task_process_event(uint8_t task_id, uint16_t events);
static uint16_t ledMsg_task_process_event(uint8_t task_id, uint16_t events);

// 初始化task
// 包括注册函数,可以注册后去开启event
void led_task_init(void)
{
    // 注册task id,同时把该task的event处理函数传进去
    led_task_id = TMOS_ProcessEventRegister(led_task_process_event);
    ledMsg_task_id = TMOS_ProcessEventRegister(ledMsg_task_process_event);
    // 立即开始一个event
    tmos_set_event(led_task_id, DEMO_TASK_TMOS_EVT_TEST_1);
    // 开始一个定时event,1s后产生,当前语句只会产生一次event
    // 可以在event产生后去开启event,可以是别的task的,也可以是当前task的event
    // 延时时间：1600*625us
    tmos_start_task(led_task_id, DEMO_TASK_TMOS_EVT_TEST_2, 1600);
    tmos_start_reload_task(ledMsg_task_id, TEST_MSG_EVENT_1, 3200);
}
// 消息处理的函数
static void demo_task_process_TMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch (pMsg->event)
    {
    case MSG_EVENT:
        tmos_set_event(ledMsg_task_id, TEST_MSG_EVENT_2);
        PRINT("已收到 led_task 发送的不带数据消息。\r\n");
        PRINT("pMsg->event=%x，pMsg->status=%x\r\n", pMsg->event, pMsg->status);
        break;

    default:
        PRINT("pMsg->event %04x\r\n", pMsg->event);
        break;
    }
}
static void demo_task_data_process_TMOSMsg(tmos_var_msg_t *pMsg)
{
    switch (pMsg->hdr.event)
    {
    case MSG_TEST_EVENT:
        PRINT("已收到 ledMsg_task 发送的带数据消息。\r\n");
        PRINT("pMsg->event=%x，pMsg->status=%x\r\n", pMsg->hdr.event, pMsg->hdr.status);
        PRINT("data:");
        for (int i = 0; i < 10; i++)
        {
            PRINT(" %d", ((tmos_var_msg_t *)pMsg)->data[i]);
        }
        break;

    default:
        PRINT("pMsg->event %04x\r\n", pMsg->hdr.event);
        break;
    }
}
// task的event处理回调函数,需要在注册task时候,传进去
static uint16_t led_task_process_event(uint8_t task_id, uint16_t events)
{
    uint8_t *msgPtr;
    tmos_event_hdr_t *test_message;
    // 处理系统消息事件,该事件由tmos_msg_send发送消息时产生
    if (events & SYS_EVENT_MSG)
    {
        msgPtr = tmos_msg_receive(task_id);
        if (msgPtr)
        {
            // 消息处理
            demo_task_data_process_TMOSMsg((tmos_var_msg_t *)msgPtr);
            tmos_msg_deallocate(msgPtr);
        }
        return events ^ SYS_EVENT_MSG;
    }
    // event 处理
    if (events & DEMO_TASK_TMOS_EVT_TEST_1)
    {
        gpio_write(LED_PIN, GPIO_PIN_SET);
        tmos_start_task(led_task_id, DEMO_TASK_TMOS_EVT_TEST_2, 800);
        PRINT("打开 led,500ms 后关闭 \r\n");
        return (events ^ DEMO_TASK_TMOS_EVT_TEST_1); // 异或的方式清除该事件运行标志，并返回未运行的事件标志
    }
    // event 处理
    if (events & DEMO_TASK_TMOS_EVT_TEST_2)
    {
        gpio_write(LED_PIN, GPIO_PIN_RESET);
        PRINT("关闭 led,1000ms 后开启 \r\n");
        tmos_start_task(led_task_id, DEMO_TASK_TMOS_EVT_TEST_1, 1600);
        tmos_set_event(led_task_id, DEMO_TASK_TMOS_EVT_TEST_3);
        return (events ^ DEMO_TASK_TMOS_EVT_TEST_2);
    }
    // event 处理
    if (events & DEMO_TASK_TMOS_EVT_TEST_3)
    {
        PRINT("Run TEST_EVENT_3 in led_tesk\r\n");
        // 申请消息内存空间
        test_message = (tmos_event_hdr_t *)tmos_msg_allocate(sizeof(tmos_event_hdr_t));
        if (test_message)
        {
            test_message->event = MSG_EVENT;
            test_message->status = 0x55;
            // 发送消息至 ledMsg_task_id 任务
            tmos_msg_send(ledMsg_task_id, (uint8_t *)test_message);
        }

        return (events ^ DEMO_TASK_TMOS_EVT_TEST_3);
    }

    return 0;
}
static uint16_t ledMsg_task_process_event(uint8_t task_id, uint16_t events)
{
    uint8_t *msgPtr;
    // 处理系统消息事件,该事件由tmos_msg_send发送消息时产生
    if (events & SYS_EVENT_MSG)
    {
        msgPtr = tmos_msg_receive(task_id);
        if (msgPtr)
        {
            // 消息处理
            demo_task_process_TMOSMsg((tmos_event_hdr_t *)msgPtr);
            // 释放消息空间
            tmos_msg_deallocate(msgPtr);
        }
        return events ^ SYS_EVENT_MSG;
    }
    // event 处理
    if (events & TEST_MSG_EVENT_1)
    {
        uint16_t data_size = 10; // 示例数据大小
        tmos_var_msg_t *msg = (tmos_var_msg_t *)tmos_msg_allocate(sizeof(tmos_var_msg_t) + data_size);
        if (msg)
        {
            msg->hdr.event = MSG_TEST_EVENT;
            msg->hdr.status = 0x77;
            // 填充数据（在data数组后）
            memcpy(msg->data, test_data, data_size);
            tmos_msg_send(led_task_id, (uint8_t *)msg);
        }
        return (events ^ TEST_MSG_EVENT_1); // 异或的方式清除该事件运行标志，并返回未运行的事件标志
    }
    if (events & TEST_MSG_EVENT_2)
    {
        PRINT("成功收到 led_task 的消息，并触发 ledMsg_task 的事件2。\r\n");
        return (events ^ TEST_MSG_EVENT_2); // 异或的方式清除该事件运行标志，并返回未运行的事件标志
    }
    return 0;
}
