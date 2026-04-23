#ifndef BSP_BOARD_H
#define BSP_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

// Basic board bring-up for CH32V208:
// - clock/update + delay init
// - debug UART printf init
// - LED GPIO init (PC9 by default)
void board_init(void);

void board_led_write(int on);
void board_led_toggle(void);

#ifdef __cplusplus
}
#endif

#endif // BSP_BOARD_H

