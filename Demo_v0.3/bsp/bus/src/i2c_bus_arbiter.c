#include "i2c_bus_arbiter.h"
#include "board.h"

#include <string.h>

/**
 ******************************************************************************
 * @file    i2c_bus_arbiter.c
 * @brief   I2C总线仲裁器实现
 ******************************************************************************
 * @details 本文件实现了I2C总线仲裁器的核心功能，采用基于优先级队列的设计，
 *          支持多任务环境下的I2C总线访问管理。
 *          
 *          核心设计：
 *          - 使用三个优先级队列（高、正常、低）管理请求
 *          - 基于轮询的处理机制，避免中断嵌套复杂性
 *          - 完善的超时检测和总线恢复机制
 *          - 详细的统计信息收集，便于性能分析
 *          
 *          工作流程：
 *          1. 应用层调用i2c_bus_submit_sync()提交请求
 *          2. 请求被加入对应优先级的队列中
 *          3. 主循环定期调用i2c_bus_arbiter_process()处理队列
 *          4. 仲裁器从高优先级到低优先级依次处理请求
 *          5. 监控请求执行状态，处理完成或超时事件
 *
 ******************************************************************************
 */
#include "i2c_bus_arbiter.h"
#include "board.h"

#include <errno.h>
#include <string.h>

// I2C仲裁队列深度
#define I2C_ARB_QUEUE_DEPTH        8U
// I2C仲裁默认超时时间（毫秒）
#define I2C_ARB_DEFAULT_TIMEOUT_MS 50U

/**
 * @brief I2C总线请求节点结构体
 * @details 封装单个I2C请求及其元数据，用于队列存储。
 */
typedef struct
{
    uint32_t id;                  ///< 请求ID（用于同步等待）
    uint32_t enqueue_tick;        ///< 入队时间戳（用于统计等待时间）
    i2c_bus_request_t req;        ///< I2C总线请求
} i2c_bus_req_node_t;

/**
 * @brief I2C总线优先级队列结构体
 * @details 实现环形缓冲区形式的优先级队列。
 */
typedef struct
{
    i2c_bus_req_node_t buf[I2C_ARB_QUEUE_DEPTH];  ///< 请求缓冲区
    uint8_t head;                                 ///< 队列头指针
    uint8_t tail;                                 ///< 队列尾指针
    uint8_t count;                                ///< 队列中元素数量
} i2c_bus_prio_queue_t;

/**
 * @brief I2C总线上下文结构体
 * @details 管理单个I2C总线的所有状态信息，包括队列、当前请求、统计信息等。
 */
typedef struct
{
    i2c_bus_prio_queue_t queues[I2C_BUS_PRIO_MAX];  ///< 优先级队列数组
    uint8_t total_count;                            ///< 总请求数量

    uint8_t running;                                ///< 是否正在运行
    i2c_bus_req_node_t current;                     ///< 当前处理的请求
    uint32_t current_start_tick;                    ///< 当前请求开始时间戳

    uint32_t next_id;                               ///< 下一个请求ID
    uint32_t last_done_id;                          ///< 上次完成的请求ID
    int last_done_status;                           ///< 上次完成状态
    uint32_t total_wait_ms;                         ///< 总等待时间（毫秒）
    uint32_t total_exec_ms;                         ///< 总执行时间（毫秒）
    uint32_t total_done_cnt;                        ///< 总完成请求数

    i2c_bus_stats_t stats;                          ///< 统计信息
} i2c_bus_ctx_t;

// I2C总线上下文静态数组
static i2c_bus_ctx_t s_bus_ctx[I2C_NUM_MAX];

/**
 * @brief 规范化I2C优先级
 * @details 将用户提供的优先级值规范化为有效的枚举值，防止越界访问。
 * @param prio 原始优先级
 * @return 规范化后的优先级
 */
static uint8_t i2c_bus_norm_prio(uint8_t prio)
{
    if (prio >= I2C_BUS_PRIO_MAX)
    {
        return (uint8_t)I2C_BUS_PRIO_NORMAL;
    }
    return prio;
}

// 声明取消排队请求的内部函数
static int i2c_bus_cancel_queued_if(i2c_num_t bus, uint8_t match_owner, uint8_t key);

/**
 * @brief 将I2C返回值转换为错误码
 * @details 将底层I2C驱动的返回值映射为标准POSIX错误码，便于上层处理。
 * @param ret I2C返回值
 * @return 对应的错误码
 */
static int i2c_ret_to_errno(int ret)
{
    if (ret == I2C_OK)
    {
        return 0;
    }
    if (ret == I2C_ERR_TIMEOUT)
    {
        return -ETIMEDOUT;
    }
    if (ret == I2C_ERR_BUSY)
    {
        return -EBUSY;
    }
    if (ret == I2C_ERR_INVALID_PARAM)
    {
        return -EINVAL;
    }
    return -EIO;
}

/**
 * @brief 开始处理I2C请求
 * @details 根据请求类型和模式提示，调用底层I2C驱动执行相应的操作。
 *          支持轮询、中断、DMA三种传输模式，根据请求中的mode_hint字段选择。
 * @param req I2C总线请求指针
 * @return I2C操作结果（I2C_OK表示成功）
 */
static int i2c_bus_start_request(const i2c_bus_request_t *req)
{
    uint32_t timeout = I2C_TIMEOUT_DEFAULT;
    int ret = I2C_ERR_INVALID_PARAM;

    // 如果指定了超时时间，则使用指定的超时时间
    if (req->timeout_ms != 0U)
    {
        timeout = req->timeout_ms;
    }

    // 根据请求类型和模式提示执行相应的I2C操作
    switch (req->type)
    {
    case I2C_BUS_REQ_WRITE:
        if (req->mode_hint == I2C_MODE_DMA)
            ret = bsp_i2c_write_dma(req->bus, req->dev_addr, req->wbuf, req->len);
        else if (req->mode_hint == I2C_MODE_IT)
            ret = bsp_i2c_write_it(req->bus, req->dev_addr, req->wbuf, req->len);
        else
            ret = bsp_i2c_write_polling(req->bus, req->dev_addr, req->wbuf, req->len, timeout);
        break;

    case I2C_BUS_REQ_READ:
        if (req->mode_hint == I2C_MODE_DMA)
            ret = bsp_i2c_read_dma(req->bus, req->dev_addr, req->rbuf, req->len);
        else if (req->mode_hint == I2C_MODE_IT)
            ret = bsp_i2c_read_it(req->bus, req->dev_addr, req->rbuf, req->len);
        else
            ret = bsp_i2c_read_polling(req->bus, req->dev_addr, req->rbuf, req->len, timeout);
        break;

    case I2C_BUS_REQ_WRITE_REG:
        if (req->mode_hint == I2C_MODE_DMA)
            ret = bsp_i2c_write_register_dma(req->bus, req->dev_addr, req->reg, req->wbuf, req->len);
        else if (req->mode_hint == I2C_MODE_IT)
            ret = bsp_i2c_write_register_it(req->bus, req->dev_addr, req->reg, req->wbuf, req->len);
        else
            ret = bsp_i2c_write_register_polling(req->bus, req->dev_addr, req->reg, req->wbuf, req->len, timeout);
        break;

    case I2C_BUS_REQ_READ_REG:
        if (req->mode_hint == I2C_MODE_DMA)
            ret = bsp_i2c_read_register_dma(req->bus, req->dev_addr, req->reg, req->rbuf, req->len);
        else if (req->mode_hint == I2C_MODE_IT)
            ret = bsp_i2c_read_register_it(req->bus, req->dev_addr, req->reg, req->rbuf, req->len);
        else
            ret = bsp_i2c_read_register_polling(req->bus, req->dev_addr, req->reg, req->rbuf, req->len, timeout);
        break;

    default:
        ret = I2C_ERR_INVALID_PARAM;
        break;
    }

    return ret;
}

/**
 * @brief 向优先级队列中添加请求节点
 * @details 将请求节点添加到指定优先级队列的尾部，如果队列已满则返回错误。
 * @param q 优先级队列指针
 * @param node 请求节点指针
 * @return 成功返回0，失败返回-1
 */
static int prio_queue_push(i2c_bus_prio_queue_t *q, const i2c_bus_req_node_t *node)
{
    // 检查队列是否已满
    if (q->count >= I2C_ARB_QUEUE_DEPTH)
    {
        return -1;
    }
    q->buf[q->tail] = *node;
    q->tail = (uint8_t)((q->tail + 1U) % I2C_ARB_QUEUE_DEPTH);
    q->count++;
    return 0;
}

/**
 * @brief 从优先级队列中取出请求节点
 * @details 从指定优先级队列的头部取出请求节点，如果队列为空则返回错误。
 * @param q 优先级队列指针
 * @param node 请求节点指针（输出参数）
 * @return 成功返回0，失败返回-1
 */
static int prio_queue_pop(i2c_bus_prio_queue_t *q, i2c_bus_req_node_t *node)
{
    // 检查队列是否为空
    if (q->count == 0U)
    {
        return -1;
    }
    *node = q->buf[q->head];
    q->head = (uint8_t)((q->head + 1U) % I2C_ARB_QUEUE_DEPTH);
    q->count--;
    return 0;
}

/**
 * @brief 从仲裁器中取出下一个请求
 * @details 按照优先级顺序（高->正常->低）从队列中取出下一个待处理的请求。
 * @param ctx I2C总线上下文指针
 * @param node 请求节点指针（输出参数）
 * @return 成功返回0，失败返回-1
 */
static int arbiter_pop_next(i2c_bus_ctx_t *ctx, i2c_bus_req_node_t *node)
{
    uint8_t p;
    // 按优先级顺序查找（高优先级先处理）
    for (p = (uint8_t)I2C_BUS_PRIO_HIGH; p < (uint8_t)I2C_BUS_PRIO_MAX; p++)
    {
        if (prio_queue_pop(&ctx->queues[p], node) == 0)
        {
            ctx->total_count--;
            return 0;
        }
    }
    return -1;
}

/**
 * @brief 标记当前请求已完成
 * @details 更新统计信息，标记当前请求为完成状态，并记录执行时间和结果。
 * @param ctx I2C总线上下文指针
 * @param status 完成状态
 */
static void i2c_bus_mark_done(i2c_bus_ctx_t *ctx, int status)
{
    uint32_t exec_ms = HAL_GetTick() - ctx->current_start_tick;

    // 更新完成状态信息
    ctx->last_done_id = ctx->current.id;
    ctx->last_done_status = status;
    ctx->running = 0U;
    ctx->stats.running = 0U;
    ctx->stats.last_status = status;
    ctx->total_done_cnt++;
    ctx->total_exec_ms += exec_ms;

    // 更新最大执行时间
    if (exec_ms > ctx->stats.max_exec_ms)
    {
        ctx->stats.max_exec_ms = exec_ms;
    }
    // 更新成功/失败计数
    if (status == 0)
    {
        ctx->stats.done_ok++;
    }
    else
    {
        ctx->stats.done_err++;
    }
}

/**
 * @brief 初始化I2C总线仲裁器
 * @details 初始化指定I2C总线的仲裁器上下文，清空所有队列和统计信息，
 *          为后续的请求处理做好准备。
 * @param bus I2C总线编号
 */
void i2c_bus_arbiter_init(i2c_num_t bus)
{
    if (bus >= I2C_NUM_MAX)
    {
        return;
    }
    // 清空上下文并初始化下一个ID
    memset(&s_bus_ctx[bus], 0, sizeof(s_bus_ctx[bus]));
    s_bus_ctx[bus].next_id = 1U;
}

/**
 * @brief 处理I2C总线仲裁器
 * @details I2C总线仲裁器的主处理函数，需要在系统主循环或定时器中断中定期调用。
 *          执行以下主要任务：
 *          1. 如果当前没有运行的请求，从队列中取出下一个请求开始执行
 *          2. 如果当前有运行的请求，监控其执行状态和超时情况
 *          3. 处理请求完成、超时或错误事件
 * @param bus I2C总线编号
 */
void i2c_bus_arbiter_process(i2c_num_t bus)
{
    i2c_bus_ctx_t *ctx;
    int ret;
    uint32_t timeout_ms;

    if (bus >= I2C_NUM_MAX)
    {
        return;
    }
    ctx = &s_bus_ctx[bus];

    // 如果没有正在运行的请求
    if (!ctx->running)
    {
        // 如果队列为空，直接返回
        if (ctx->total_count == 0U)
        {
            return;
        }

        // 从队列中取出下一个请求
        if (arbiter_pop_next(ctx, &ctx->current) != 0)
        {
            return;
        }

        // 更新统计信息
        ctx->stats.queue_depth_curr = ctx->total_count;
        ctx->running = 1U;
        ctx->stats.running = 1U;
        ctx->current_start_tick = HAL_GetTick();
        ctx->stats.started++;
        {
            uint32_t wait_ms = ctx->current_start_tick - ctx->current.enqueue_tick;
            ctx->total_wait_ms += wait_ms;
            if (wait_ms > ctx->stats.max_wait_ms)
            {
                ctx->stats.max_wait_ms = wait_ms;
            }
        }

        // 开始处理请求
        ret = i2c_bus_start_request(&ctx->current.req);
        if (ret != I2C_OK)
        {
            // 如果启动失败，标记为完成并返回错误
            i2c_bus_mark_done(ctx, i2c_ret_to_errno(ret));
            return;
        }

        // 如果是轮询模式，立即标记为完成
        if (ctx->current.req.mode_hint == I2C_MODE_POLLING)
        {
            i2c_bus_mark_done(ctx, 0);
            return;
        }
    }

    // 检查是否超时
    timeout_ms = (ctx->current.req.timeout_ms == 0U) ? I2C_ARB_DEFAULT_TIMEOUT_MS : ctx->current.req.timeout_ms;
    if ((HAL_GetTick() - ctx->current_start_tick) >= timeout_ms)
    {
        // 超时处理：恢复I2C总线并标记为超时错误
        bsp_i2c_recover(bus);
        ctx->stats.recover_cnt++;
        ctx->stats.timeout_cnt++;
        i2c_bus_mark_done(ctx, -ETIMEDOUT);
        return;
    }

    // 检查I2C状态是否为空闲
    if ((bsp_i2c_get_state(bus) == I2C_STATE_IDLE) && (!bsp_i2c_is_busy(bus)))
    {
        ret = (int)bsp_i2c_get_error(bus);
        i2c_bus_mark_done(ctx, i2c_ret_to_errno(ret));
    }
}

/**
 * @brief 同步提交I2C总线请求
 * @details 同步方式提交I2C请求，内部将请求加入队列后阻塞等待直到完成。
 *          适用于需要确保操作完成后再继续执行的场景。
 * @param req I2C总线请求指针
 * @return 成功返回0，失败返回负错误码
 */
int i2c_bus_submit_sync(const i2c_bus_request_t *req)
{
    i2c_bus_ctx_t *ctx;
    i2c_bus_req_node_t node;
    i2c_bus_request_t req_copy;
    uint32_t id;
    uint32_t start;
    uint32_t total_timeout;
    uint8_t prio;

    // 参数验证
    if (req == NULL || req->bus >= I2C_NUM_MAX || req->len == 0U)
    {
        if (req != NULL && req->bus < I2C_NUM_MAX)
        {
            s_bus_ctx[req->bus].stats.submit_invalid++;
        }
        return -EINVAL;
    }

    ctx = &s_bus_ctx[req->bus];
    // 初始化下一个ID（如果需要）
    if (ctx->next_id == 0U)
    {
        ctx->next_id = 1U;
    }
    // 检查队列是否已满
    if (ctx->total_count >= I2C_ARB_QUEUE_DEPTH)
    {
        ctx->stats.submit_queue_full++;
        return -EBUSY;
    }

    // 复制请求并规范化优先级
    req_copy = *req;
    prio = i2c_bus_norm_prio(req_copy.prio);
    req_copy.prio = prio;

    // 创建请求节点
    id = ctx->next_id++;
    node.id = id;
    node.enqueue_tick = HAL_GetTick();
    node.req = req_copy;
    if (prio_queue_push(&ctx->queues[prio], &node) != 0)
    {
        ctx->stats.submit_queue_full++;
        return -EBUSY;
    }

    // 更新统计信息
    ctx->stats.submit_ok++;
    ctx->total_count++;
    ctx->stats.queue_depth_curr = ctx->total_count;
    if (ctx->stats.queue_depth_curr > ctx->stats.queue_depth_peak)
    {
        ctx->stats.queue_depth_peak = ctx->stats.queue_depth_curr;
    }

    // 等待请求完成
    start = HAL_GetTick();
    total_timeout = (req->timeout_ms == 0U) ? (I2C_ARB_DEFAULT_TIMEOUT_MS * 4U) : (req->timeout_ms * 4U);

    while (1)
    {
        i2c_bus_arbiter_process(req->bus);

        // 检查是否是当前请求已完成
        if (ctx->last_done_id == id)
        {
            return ctx->last_done_status;
        }
        // 检查是否超时
        if ((HAL_GetTick() - start) >= total_timeout)
        {
            bsp_i2c_recover(req->bus);
            ctx->stats.recover_cnt++;
            ctx->stats.timeout_cnt++;
            return -ETIMEDOUT;
        }
    }
}

/**
 * @brief 根据设备地址取消排队中的请求
 * @details 取消指定设备地址的所有排队请求，通常在设备异常或需要重置时使用。
 * @param bus I2C总线编号
 * @param dev_addr 设备地址
 * @return 取消的请求数量
 */
int i2c_bus_cancel_by_dev(i2c_num_t bus, uint8_t dev_addr)
{
    return i2c_bus_cancel_queued_if(bus, 0U, dev_addr);
}

/**
 * @brief 根据条件取消排队中的请求（内部函数）
 * @details 内部实现函数，根据match_owner参数决定是按设备地址还是所有者ID进行匹配取消。
 * @param bus I2C总线编号
 * @param match_owner 是否匹配所有者ID（0=匹配设备地址，非0=匹配所有者ID）
 * @param key 匹配键值（设备地址或所有者ID）
 * @return 取消的请求数量
 */
static int i2c_bus_cancel_queued_if(i2c_num_t bus, uint8_t match_owner, uint8_t key)
{
    i2c_bus_ctx_t *ctx;
    i2c_bus_req_node_t tmp[I2C_ARB_QUEUE_DEPTH];
    uint8_t p;
    int canceled = 0;

    if (bus >= I2C_NUM_MAX)
    {
        return -EINVAL;
    }
    ctx = &s_bus_ctx[bus];

    // 遍历所有优先级队列
    for (p = (uint8_t)I2C_BUS_PRIO_HIGH; p < (uint8_t)I2C_BUS_PRIO_MAX; p++)
    {
        i2c_bus_prio_queue_t *q = &ctx->queues[p];
        uint8_t i;
        uint8_t keep = 0;
        // 遍历队列中的每个请求
        for (i = 0; i < q->count; i++)
        {
            uint8_t idx = (uint8_t)((q->head + i) % I2C_ARB_QUEUE_DEPTH);
            uint8_t hit = 0U;

            // 根据match_owner参数决定匹配条件
            if (match_owner != 0U)
            {
                hit = (q->buf[idx].req.owner_id == key) ? 1U : 0U;
            }
            else
            {
                hit = (q->buf[idx].req.dev_addr == key) ? 1U : 0U;
            }

            if (hit != 0U)
            {
                canceled++;
            }
            else
            {
                tmp[keep++] = q->buf[idx];
            }
        }
        // 重建队列，移除已取消的请求
        q->head = 0U;
        q->tail = keep;
        q->count = keep;
        for (i = 0; i < keep; i++)
        {
            q->buf[i] = tmp[i];
        }
    }

    // 更新总请求数量和统计信息
    if (canceled > 0)
    {
        if (ctx->total_count >= (uint8_t)canceled)
        {
            ctx->total_count = (uint8_t)(ctx->total_count - (uint8_t)canceled);
        }
        else
        {
            ctx->total_count = 0U;
        }
        ctx->stats.queue_depth_curr = ctx->total_count;
    }
    return canceled;
}

/**
 * @brief 根据所有者ID取消排队中的请求
 * @details 取消指定所有者ID的所有排队请求，通常在模块卸载或任务结束时使用。
 * @param bus I2C总线编号
 * @param owner_id 所有者ID
 * @return 取消的请求数量
 */
int i2c_bus_cancel_by_owner(i2c_num_t bus, uint8_t owner_id)
{
    return i2c_bus_cancel_queued_if(bus, 1U, owner_id);
}

/**
 * @brief 获取I2C总线统计信息
 * @details 获取指定I2C总线的详细统计信息，包括成功率、平均等待时间、最大执行时间等。
 *          统计信息可用于性能分析和系统调试。
 * @param bus I2C总线编号
 * @param stats 统计信息结构体指针
 */
void i2c_bus_get_stats(i2c_num_t bus, i2c_bus_stats_t *stats)
{
    if (bus >= I2C_NUM_MAX || stats == NULL)
    {
        return;
    }

    // 复制统计信息
    *stats = s_bus_ctx[bus].stats;
    // 计算平均等待时间和平均执行时间
    if (s_bus_ctx[bus].total_done_cnt > 0U)
    {
        stats->avg_wait_ms = s_bus_ctx[bus].total_wait_ms / s_bus_ctx[bus].total_done_cnt;
        stats->avg_exec_ms = s_bus_ctx[bus].total_exec_ms / s_bus_ctx[bus].total_done_cnt;
    }
    else
    {
        stats->avg_wait_ms = 0U;
        stats->avg_exec_ms = 0U;
    }
    stats->queue_depth_curr = s_bus_ctx[bus].total_count;
    stats->running = s_bus_ctx[bus].running;
}

/**
 * @brief 重置I2C总线统计信息
 * @details 清空指定I2C总线的所有统计信息，重新开始收集数据。
 *          通常在系统初始化或需要重新开始性能分析时调用。
 * @param bus I2C总线编号
 */
void i2c_bus_reset_stats(i2c_num_t bus)
{
    if (bus >= I2C_NUM_MAX)
    {
        return;
    }

    // 清空统计信息
    memset(&s_bus_ctx[bus].stats, 0, sizeof(s_bus_ctx[bus].stats));
    s_bus_ctx[bus].total_wait_ms = 0U;
    s_bus_ctx[bus].total_exec_ms = 0U;
    s_bus_ctx[bus].total_done_cnt = 0U;
    s_bus_ctx[bus].stats.queue_depth_curr = s_bus_ctx[bus].total_count;
    s_bus_ctx[bus].stats.running = s_bus_ctx[bus].running;
}
