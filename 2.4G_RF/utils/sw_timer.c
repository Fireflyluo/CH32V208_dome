#include "sw_timer.h"
#include "ch32v20x.h"
#include <stddef.h>
#include <string.h>

/** @brief 时间轮的槽位数量，决定了最大定时范围 */
#define SW_TIMER_WHEEL_SIZE 256U

/** @brief 时间轮主数组，每个槽位存储一个定时器链表 */
static sw_timer_t *g_wheel[SW_TIMER_WHEEL_SIZE];

/** @brief 到期定时器链表，存储所有已到期但未处理的定时器 */
static sw_timer_t *g_expired_list;

/** @brief 当前时间轮指针位置 */
static uint16_t g_current_slot;

/** @brief 系统tick的时间间隔(毫秒) */
static uint32_t g_tick_ms;

/**
 * @brief 将毫秒时间转换为tick数量
 * 
 * @param ms 输入的毫秒时间
 * @return uint32_t 对应的tick数量
 * 
 * @note 特殊处理: 
 *       - 如果ms为0，返回0表示立即触发
 *       - 否则向上取整，确保不会提前触发
 */
static uint32_t ms_to_ticks(uint32_t ms)
{
    /* 特殊情况：0毫秒表示立即触发 */
    if (ms == 0U)
    {
        return 0U;
    }
    
    /* 计算需要的tick数，向上取整 */
    uint32_t ticks = ms / g_tick_ms;
    if ((ms % g_tick_ms) != 0U)
    {
        ticks++;
    }
    return (ticks == 0U) ? 1U : ticks;
}

/**
 * @brief 将定时器插入时间轮
 * 
 * @param timer 要插入的定时器
 * @param ticks 延迟的tick数量
 * 
 * @note 在中断禁用状态下调用，确保线程安全
 */
static void wheel_insert(sw_timer_t *timer, uint32_t ticks)
{
    /* 计算在时间轮中的偏移量和所需轮数 */
    uint32_t slot_offset = ticks % SW_TIMER_WHEEL_SIZE;
    timer->rounds = (uint16_t)(ticks / SW_TIMER_WHEEL_SIZE);
    timer->slot = (uint16_t)((g_current_slot + slot_offset) % SW_TIMER_WHEEL_SIZE);
    
    /* 插入到对应槽位的链表头部 */
    timer->next = g_wheel[timer->slot];
    g_wheel[timer->slot] = timer;
    timer->active = 1U;
}

/**
 * @brief 初始化时间轮系统
 * 
 * @param tick_ms 系统tick间隔(毫秒)
 * 
 * @note 如果tick_ms为0，则默认设置为1ms
 */
void sw_timer_wheel_init(uint32_t tick_ms)
{
    /* 确保tick_ms至少为1ms */
    if (tick_ms == 0U)
    {
        tick_ms = 1U;
    }

    __disable_irq();
    memset(g_wheel, 0, sizeof(g_wheel));
    g_expired_list = NULL;
    g_current_slot = 0U;
    g_tick_ms = tick_ms;
    __enable_irq();
}

/**
 * @brief 停止指定的定时器
 * 
 * @param timer 要停止的定时器指针
 * 
 * @note 在中断禁用状态下操作，确保线程安全
 */
void sw_timer_stop(sw_timer_t *timer)
{
    sw_timer_t **head;
    sw_timer_t *prev = NULL;
    sw_timer_t *node;

    /* 参数检查：空指针或未激活的定时器直接返回 */
    if (timer == NULL || timer->active == 0U)
    {
        return;
    }

    __disable_irq();
    head = &g_wheel[timer->slot];
    node = *head;
    while (node != NULL)
    {
        if (node == timer)
        {
            /* 从链表中移除定时器 */
            if (prev == NULL)
            {
                *head = node->next;
            }
            else
            {
                prev->next = node->next;
            }
            timer->next = NULL;
            timer->active = 0U;
            break;
        }
        prev = node;
        node = node->next;
    }
    __enable_irq();
}

/**
 * @brief 启动软件定时器
 * 
 * @param timer 定时器结构体指针
 * @param delay_ms 延迟时间(毫秒)，0表示立即触发
 * @param period_ms 周期时间(毫秒)，0表示单次定时器
 * @param cb 回调函数
 * @param arg 回调函数参数
 * 
 * @return int 错误码
 */
int sw_timer_start(sw_timer_t *timer, uint32_t delay_ms, uint32_t period_ms, sw_timer_cb_t cb, void *arg)
{
    uint32_t delay_ticks;

    /* 参数有效性检查 */
    if (timer == NULL || cb == NULL)
    {
        return -1;
    }

    /* 检查是否已初始化 */
    if (g_tick_ms == 0U)
    {
        return -2;
    }

    /* 转换时间单位 */
    delay_ticks = ms_to_ticks(delay_ms);
    timer->period_ticks = (period_ms == 0U) ? 0U : ms_to_ticks(period_ms);
    timer->periodic = (timer->period_ticks != 0U) ? 1U : 0U;
    timer->cb = cb;
    timer->arg = arg;

    /* 如果定时器已在运行，先停止 */
    sw_timer_stop(timer);

    __disable_irq();
    /* 特殊处理：延迟为0的定时器立即加入到期列表 */
    if (delay_ticks == 0U)
    {
        timer->next = g_expired_list;
        g_expired_list = timer;
        timer->active = 1U;
    }
    else
    {
        wheel_insert(timer, delay_ticks);
    }
    __enable_irq();

    return 0;
}

/**
 * @brief 定时器tick中断服务函数
 * 
 * @note 此函数应在系统定时器中断中定期调用
 *       负责推进时间轮并检查到期定时器
 */
void sw_timer_tick_isr(void)
{
    sw_timer_t *node;
    sw_timer_t *prev = NULL;
    sw_timer_t *next;

    /* 推进时间轮指针 */
    g_current_slot = (uint16_t)((g_current_slot + 1U) % SW_TIMER_WHEEL_SIZE);
    node = g_wheel[g_current_slot];

    /* 检查当前槽位的所有定时器 */
    while (node != NULL)
    {
        next = node->next;
        if (node->rounds > 0U)
        {
            /* 还未到期，减少剩余轮数 */
            node->rounds--;
            prev = node;
        }
        else
        {
            /* 定时器到期，移除并加入到期列表 */
            if (prev == NULL)
            {
                g_wheel[g_current_slot] = next;
            }
            else
            {
                prev->next = next;
            }
            node->next = g_expired_list;
            g_expired_list = node;
        }
        node = next;
    }
}

/**
 * @brief 处理所有到期的定时器
 * 
 * @note 此函数应在主循环中调用，执行回调函数
 *       对于周期性定时器会自动重新调度
 */
void sw_timer_process(void)
{
    sw_timer_t *local_list;
    sw_timer_t *node;

    /* 原子操作：获取到期列表并清空全局列表 */
    __disable_irq();
    local_list = g_expired_list;
    g_expired_list = NULL;
    __enable_irq();

    /* 处理所有到期的定时器 */
    while (local_list != NULL)
    {
        node = local_list;
        local_list = local_list->next;
        node->next = NULL;
        node->active = 0U;

        /* 执行回调函数 */
        if (node->cb != NULL)
        {
            node->cb(node->arg);
        }

        /* 如果是周期性定时器，重新插入时间轮 */
        if (node->periodic != 0U)
        {
            __disable_irq();
            wheel_insert(node, node->period_ticks);
            __enable_irq();
        }
    }
}