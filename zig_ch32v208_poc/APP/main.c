// C entry point: do low-level init here, then transfer control to Zig app.
//
// - C: startup/BSP/SDK + `main()`
// - Zig: app layer (`zig_app_main()`), runs TMOS loop and user logic

#include "CONFIG.h"
#include "HAL.h"
#include "board.h"

// BLE/TMOS library memory pool (4-byte aligned)
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

// Exported by `zig/app.zig`
void zig_app_main(void) __attribute__((noreturn));

int main(void)
{
    board_init();

    // TMOS implementation lives in WCH BLE library; init it to enable TMOS.
    WCHBLE_Init();
    HAL_Init();

    // Hand off to Zig application layer.
    zig_app_main();
}
