/*****************************************************************************
 * @file aros_rf.h
 * @author ZH
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2025, Angran Inc. All rights reserved.
 *
 * Angran: alive and robust
 * **************************************************************************/

#ifndef __AROS_RF___
#define __AROS_RF___

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

#if !defined(ARF_WIN) && (defined(WIN32)||defined(__WIN32__)||defined(_WIN32))
#define ARF_WIN
#endif

#if !defined(ARF_RISCV) && (defined(__riscv) || defined(__riscv__))
#define ARF_RISCV
#endif

#if defined(ARF_RISCV)
#define ARF_TC_us		625		// TMOS clock 625us
#elif defined(ARF_WIN)
#define ARF_TC_us		1000	// WIN clock 1000us
#else
#define ARF_TC_us		100		// ARM clock 100us
#endif

#define ARF_MsgN		32
#define arf_RSSI(b)		b[ARF_MsgN]
#define arf_MsgLk(b)	b[ARF_MsgN + 1]
#define ARF_MsgTC		(ARF_MsgN + 2)
#define ARF_MsgBufN		(ARF_MsgN + 4)

#define ARF_TimerN		3

#define ARF_LED_RX		1
#define ARF_LED_TX		2
#define ARF_LED_SEND	4

#define arf_u16(v)		((uint16_t)((v) & 0xFFFF))
#define arf_u16toi(b)	((uint16_t)((*(b)<<8) + *(b+1)))
#define arf_u16toa(b,v)	(*(b)=((v)>>8)&255, *(b+1)=(v)&255)
#define arf_u32toi(b)	\
	((uint32_t)((*(b)<<24) + (*(b+1)<<16) + (*(b+2)<<8) + *(b+3)))
#define arf_u32toa(b,v)	\
	(*(b)=(v)>>24, *(b+1)=((v)>>16)&255, *(b+2)=((v)>>8)&255, *(b+3)=(v)&255)

typedef uint16_t (*arf_task_fc)(uint8_t, uint16_t);
typedef int 	 (*arf_newdat_fc)(uint8_t *);
typedef void 	 (*arf_proc_fc)(void);

extern void		arf_SysInit( void );
extern void		arf_MainLoop( void );
extern void		arf_Init(void);
extern void		arf_set_timer(int tmr, int t_us, arf_proc_fc tmrf);
extern void		arf_set_procfunc(arf_proc_fc procf);
extern void		arf_set_newdatfunc(arf_newdat_fc newdatf);
extern uint8_t	arf_register(arf_task_fc taskf);
extern void		arf_event(uint8_t task_id, uint16_t evt);
extern void		arf_event_at(uint8_t task_id, uint16_t evt, int tc);
extern void		arf_proc(void);
extern void		arf_RxStart(void);
extern uint8_t*	arf_isRxFinish(void);
extern int		arf_TxSend(uint8_t buf[], int len);
extern int		arf_TxTrySend(uint8_t buf[], int len);
extern int		arf_isTxBusy(void);
extern uint16_t	arf_get_tc16(void);
extern uint32_t	arf_get_tc(void);
extern void		arf_delayms(int tm_ms);
extern void		arf_led_on(int tm_ms);
extern void		arf_led_mod(int mod);
extern int		arf_led_id(int id);

#ifdef __cplusplus
}
#endif

#endif /* __AROS_RF___ */
/**
 * @}
 */
