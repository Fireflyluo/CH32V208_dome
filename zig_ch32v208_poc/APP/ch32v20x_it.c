// Minimal interrupt handlers for this mixed Zig/C firmware.
//
// Notes:
// - `WCHBLE_Init()` enables BB/LLE IRQs. LLE_IRQHandler is provided by
//   `sdk/LIB/ble_task_scheduler.S`, BB_IRQHandler must be provided by us.

#include "CONFIG.h"
#include "ch32v20x.h"

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void BB_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    NVIC_SystemReset();
    while (1)
    {
    }
}

void BB_IRQHandler(void)
{
    BB_IRQLibHandler();
}

