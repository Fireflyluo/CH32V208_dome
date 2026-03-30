/**
 * @file    i2c_request_task.c
 * @brief   I2C请求任务实现文件
 * @details 本文件实现了基于TMOS的I2C请求处理任务，主要功能包括：
 *          - 提供异步I2C请求提交接口
 *          - 处理I2C总线仲裁器的周期性调度
 *          - 支持任务间消息传递机制处理I2C请求
 *          - 调用同步I2C总线API执行实际的I2C操作
 *          
 *          任务特性：
 *          - 使用消息队列接收来自其他任务的I2C请求
 *          - 周期性（1个系统tick）调用I2C总线仲裁器处理函数
 *          - 支持请求完成回调通知
 *          - 提供错误码返回机制
 *          
 *          事件定义：
 *          - I2C_TASK_EVT_PUMP: I2C总线泵送事件，用于驱动仲裁器
 *          - I2C_TASK_MSG_SUBMIT_REQ: 消息类型，表示I2C请求提交
 *
 * @author  WCH (南京沁恒微电子股份有限公司)
 * @version V1.0.0
 * @date    2022/06/16
 */

#include "i2c_request_task.h"

#include <errno.h>

#define I2C_TASK_EVT_PUMP            (0x0001u << 0)
#define I2C_TASK_PUMP_PERIOD_TICK    1u
#define I2C_TASK_MSG_SUBMIT_REQ      0xA1u

/**
 * @brief  I2C请求提交消息结构体
 * @details 封装了I2C请求、完成回调函数和用户上下文的完整消息结构
 */
typedef struct
{
    tmos_event_hdr_t hdr;           /**< 消息头 */
    i2c_bus_request_t req;          /**< I2C总线请求结构体 */
    i2c_request_done_cb_t done_cb;  /**< 请求完成回调函数指针 */
    void *user_ctx;                 /**< 用户上下文指针 */
} i2c_submit_msg_t;

static tmosTaskID s_i2c_task_id = INVALID_TASK_ID;

static tmosEvents i2c_request_task_process_event(tmosTaskID task_id, tmosEvents events);
static void i2c_request_task_process_msg(i2c_submit_msg_t *msg);

/**
 * @brief  I2C请求任务初始化函数
 * @details 注册TMOS I2C请求任务并启动周期性泵送事件。
 *          如果任务已存在或注册失败，则直接返回。
 */
void i2c_request_task_init(void)
{
    if (s_i2c_task_id != INVALID_TASK_ID)
    {
        return;
    }

    s_i2c_task_id = TMOS_ProcessEventRegister(i2c_request_task_process_event);
    if (s_i2c_task_id == INVALID_TASK_ID)
    {
        return;
    }

    (void)tmos_start_reload_task(s_i2c_task_id, I2C_TASK_EVT_PUMP, I2C_TASK_PUMP_PERIOD_TICK);
}

/**
 * @brief  获取I2C请求任务ID
 * @return I2C请求任务的TMOS任务ID
 */
tmosTaskID i2c_request_task_id_get(void)
{
    return s_i2c_task_id;
}

/**
 * @brief  提交I2C请求到任务队列
 * @details 将I2C请求封装为消息并发送到I2C请求任务的消息队列中。
 *          支持指定完成回调函数和用户上下文。
 * 
 * @param[in] req 指向I2C总线请求结构体的指针
 * @param[in] done_cb 请求完成时的回调函数指针（可为NULL）
 * @param[in] user_ctx 用户上下文指针（传递给回调函数）
 * @return 0表示成功，负数表示错误码（-EINVAL, -EPIPE, -ENOMEM, -EIO）
 */
int i2c_request_task_submit(const i2c_bus_request_t *req, i2c_request_done_cb_t done_cb, void *user_ctx)
{
    i2c_submit_msg_t *msg;

    if (req == NULL)
    {
        return -EINVAL;
    }
    if (s_i2c_task_id == INVALID_TASK_ID)
    {
        return -EPIPE;
    }

    msg = (i2c_submit_msg_t *)tmos_msg_allocate(sizeof(i2c_submit_msg_t));
    if (msg == NULL)
    {
        return -ENOMEM;
    }

    tmos_memset(msg, 0, sizeof(*msg));
    msg->hdr.event = I2C_TASK_MSG_SUBMIT_REQ;
    msg->req = *req;
    msg->done_cb = done_cb;
    msg->user_ctx = user_ctx;

    if (tmos_msg_send(s_i2c_task_id, (uint8_t *)msg) != SUCCESS)
    {
        (void)tmos_msg_deallocate((uint8_t *)msg);
        return -EIO;
    }

    return 0;
}

/**
 * @brief  处理I2C请求消息
 * @details 执行实际的I2C同步操作并调用完成回调函数。
 * 
 * @param[in] msg 指向I2C提交消息结构体的指针
 */
static void i2c_request_task_process_msg(i2c_submit_msg_t *msg)
{
    int status;

    if (msg == NULL)
    {
        return;
    }

    if (msg->hdr.event != I2C_TASK_MSG_SUBMIT_REQ)
    {
        return;
    }

    status = i2c_bus_submit_sync(&msg->req);
    if (msg->done_cb != NULL)
    {
        msg->done_cb(&msg->req, status, msg->user_ctx);
    }
}

/**
 * @brief  I2C请求任务事件处理函数
 * @details 处理I2C请求任务的各类事件：
 *          - SYS_EVENT_MSG: 处理消息队列中的I2C请求消息
 *          - I2C_TASK_EVT_PUMP: 调用I2C总线仲裁器处理函数
 * 
 * @param[in] task_id 当前任务ID
 * @param[in] events 待处理的事件位图
 * @return 未处理的事件
 */
static tmosEvents i2c_request_task_process_event(tmosTaskID task_id, tmosEvents events)
{
    uint8_t *msg_ptr;

    if (events & SYS_EVENT_MSG)
    {
        while ((msg_ptr = tmos_msg_receive(task_id)) != NULL)
        {
            i2c_request_task_process_msg((i2c_submit_msg_t *)msg_ptr);
            (void)tmos_msg_deallocate(msg_ptr);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & I2C_TASK_EVT_PUMP)
    {
        i2c_bus_arbiter_process(I2C_NUM_1);
        return (events ^ I2C_TASK_EVT_PUMP);
    }

    return 0;
}