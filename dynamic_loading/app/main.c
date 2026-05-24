/**
 * @file    main.c
 * @brief   TMOS application entry.
 */

#include "CONFIG.h"
#include "HAL.h"
#include "ad_hoc_task.h"
#include "board.h"
#include "ch32v20x.h"
#include "display_task.h"
#include "module_manager_selftest.h"
#include "sensor_task.h"
#include "serial_upload_task.h"
#include "tmos_task.h"

#include <stdint.h>
#include "debug.h"

/* BLE memory pool (4-byte aligned) */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

int main(void)
{
    board_init();

    /* 串口 DMA 吞吐测速（阻塞式，完成后进入主循环） */
    Debug_UART_SpeedTest(10u);

    WCHBLE_Init();
    HAL_Init();

    /* 启动早期先跑一轮单槽位模块管理自检，再进入正常业务。 */
    module_manager_selftest_run();

    sensor_task_init();
    display_task_init();
    serial_upload_task_init();
    ad_hoc_task_init();

    while (1)
    {
        TMOS_SystemProcess();
    }
}
