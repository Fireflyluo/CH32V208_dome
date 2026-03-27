/**
 * @file    main.c
 * @brief   TMOS application entry.
 */

#include "CONFIG.h"
#include "HAL.h"
#include "board.h"
#include "ch32v20x.h"
#include "display_task.h"
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

    sensor_task_init();
    display_task_init();
    serial_upload_task_init();

    while (1)
    {
        TMOS_SystemProcess();
    }
}
