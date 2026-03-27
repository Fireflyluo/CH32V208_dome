/**
 * @file    main.c
 * @brief   TMOS application entry.
 */

#include "CONFIG.h"
#include "HAL.h"
#include "board.h"
#include "ch32v20x.h"
#include "display_task.h"
#include "protocol_task.h"
#include "sensor_task.h"
#include "serial_upload_task.h"

#include <stdint.h>

/* BLE memory pool (4-byte aligned) */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

int main(void)
{
    board_init();

    WCHBLE_Init();
    HAL_Init();

    sensor_task_init();//传感器任务
    display_task_init();//显示任务
    serial_upload_task_init();//串口上传任务
    protocol_task_init();//协议处理任务

    while (1)
    {
        TMOS_SystemProcess();
    }
}
