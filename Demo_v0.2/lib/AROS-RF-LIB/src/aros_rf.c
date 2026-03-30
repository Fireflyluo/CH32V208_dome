/****************************** (C) COPYRIGHT *******************************
* File Name          : aros_rf.c	//	RF_PHY.c & RF_main.c
* Author             : ZH
* Version            : V1.0
* Date               : 2025/11/12
* Description        : 
*****************************************************************************/

/****************************************************************************/
#include "aros_rf.h"

#ifndef TX_DelayTC
#define TX_DelayTC		(2000 / ARF_TC_us)
#endif
#ifndef TX_WaitTC
#define TX_WaitTC		(5000 / ARF_TC_us)
#endif

#ifndef ARF_LOG_HOTPATH
#define ARF_LOG_HOTPATH 0
#endif

#define Max_Task		1
#define Max_Event		16

#define Ndat			16
#define Ndat_next(i)	(((i)+1) & (Ndat - 1))

#define EVT_RX_START	1
#define EVT_TX_START	2
#define EVT_LEDOFF		4
#define EVT_DELAY		8
#define EVT_TIMER1		16
#define EVT_TIMER2		32
#define EVT_TIMER3		64

static uint8_t taskID = 0, led_mod = ARF_LED_RX | ARF_LED_SEND;
static struct { uint8_t len, delaystat, buf[ARF_MsgN]; } txdat = { 0 };
static struct { uint8_t irecv, iget, buf[Ndat][ARF_MsgBufN]; } rxdat = { 0 };

arf_newdat_fc		arf_newdatfunc = NULL;
static arf_proc_fc 	proc_func = NULL;
static arf_proc_fc	timer_func[ARF_TimerN] = { NULL, NULL, NULL };
static int			timer_TC[ARF_TimerN] = { 0, 0, 0 };

// **********************************************************************
#ifdef ARF_RISCV

#include "CONFIG.h"
#include "HAL.h"

//#define LLE_MODE_ORIGINAL_RX	(0x80)
	//如果配置LLEMODE时加上此宏，则接收第一字节为原始数据（原来为RSSI）

#define tc_sys()					TMOS_GetSystemClock()
#define event_set(task_id, evt)		tmos_set_event(task_id, evt)
#define event_at(task_id, evt, tc)	tmos_start_task(task_id, evt, tc)

#ifdef ID_CH32V208
#define LED_off()	    GPIO_WriteBit(GPIOC, GPIO_Pin_9, Bit_SET)
#define LED_on()	    GPIO_WriteBit(GPIOC, GPIO_Pin_9, Bit_RESET)
#else
#define LED_off()	    GPIOA_SetBits(GPIO_Pin_9)
#define LED_on()	    GPIOA_ResetBits(GPIO_Pin_9)
#endif

#ifndef ARF_USE_EXTERNAL_MEM_BUF
__attribute__((aligned(4))) u32 MEM_BUF[BLE_MEMHEAP_SIZE/4];
#endif
__attribute__((section(".highcode")))

#else //----------------- !ARF_RISCV -------------------------

#ifdef ARF_WIN
#include <sys/timeb.h>
#else
extern uint32_t sys_cnt_100us;
#endif	// ARF_WIN

static uint16_t arf_taskfc0(uint8_t task_id, uint16_t event) { return 0; }

typedef struct { arf_task_fc func; uint32_t tcs[Max_Event]; } arf_task_t;

static struct {	uint32_t tc, top; arf_task_t tasks[Max_Task]; } arf = {0,0};

#define tc_sys()					( arf.tc )
#define event_set(task_id, evt)		arf_event(task_id, evt)
#define event_at(task_id, evt, tc)	arf_event_at(task_id, evt, tc)

#define LED_off()
#define LED_on()
//#define	RF_Shut()
//#define	RF_Rx(buf, len, rxType, txType)
//#define	RF_Tx(buf, len, txPower, txLLEMode)

#endif	// ----------------------- ARF_RISCV ------------------------

// -------------------------- arf_MainLoop --------------------------------
void arf_MainLoop() { while(1) { arf_proc(); } }

// --------------------------- arf_SysInit --------------------------------
void arf_SysInit( void ) {
#ifdef ARF_RISCV
#ifdef ID_CH32V208
  SystemCoreClockUpdate();
  Delay_Init();
#ifdef DEBUG
  USART_Printf_Init( 115200 );
#endif
  PRINT("%s\n", VER_LIB);
  WCHBLE_Init();
  HAL_Init();
  RF_RoleInit();
  PRINT("start.\n");
#else	// CH32V208
#if (defined (DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
  PWR_DCDCCfg( ENABLE );
#endif
  SetSysClock( CLK_SOURCE_PLL_60MHz );
#if (defined (HAL_SLEEP)) && (HAL_SLEEP == TRUE)
  GPIOA_ModeCfg( GPIO_Pin_All, GPIO_ModeIN_PU );
  GPIOB_ModeCfg( GPIO_Pin_All, GPIO_ModeIN_PU );
#endif
#ifdef DEBUG
  GPIOA_SetBits( bTXD1 );
  GPIOA_ModeCfg( bTXD1, GPIO_ModeOut_PP_5mA );
	UART1_DefInit( );
#endif  
  PRINT("start.\n");
  {
    PRINT("%s\n",VER_LIB);
  }
  CH57X_BLEInit( );
  HAL_Init(  );
  RF_RoleInit( );
#endif	// CH32V208
#endif	// ARF_RISCV
}

/*********************************************************************
 * @fn      arf_2G4_cbfc0
 *
 * @brief   RF 状态回调
 *	注意：不可在此函数中直接调用RF接收或者发送API，需要使用事件的方式调用
 *
 * @param   sta     - 状态类型
 * @param   crc     - crc校验结果
 * @param   rxBuf   - 数据buf指针
 *
 * @return  none
 */
static void arf_2G4_cbfc0(uint8_t sta, uint8_t crc, uint8_t *rxBuf) {
#ifdef ARF_RISCV
    switch(sta) {
    case TX_MODE_TX_FINISH:
		if((led_mod & ARF_LED_TX) != 0) { arf_led_on(5); }
		txdat.len = 0;
		tmos_set_event( taskID , EVT_RX_START );
		break;
    case TX_MODE_TX_FAIL:
		txdat.len = ARF_MsgN+2;
		tmos_set_event( taskID , EVT_RX_START );
		break;
    case TX_MODE_RX_DATA:
    case TX_MODE_RX_TIMEOUT: // Timeout is about 200us
        break;

    case RX_MODE_RX_DATA:
        if (crc == 0) {	//  && rxBuf[1] == ARF_MsgN
            int i = (int)rxdat.irecv;
			uint8_t *rx = rxdat.buf[i];
			memcpy(rx, rxBuf+2, ARF_MsgN); arf_RSSI(rx) = rxBuf[0];
			rxdat.irecv = (uint8_t)Ndat_next(i);
			uint16_t tc = arf_u16(arf_get_tc16()-TX_DelayTC-1);
			arf_u16toa(rx + ARF_MsgTC, tc);
			if((led_mod & ARF_LED_RX) != 0) { arf_led_on(5); }
		}
        /* fall through */
    case RX_MODE_TX_FINISH:
    case RX_MODE_TX_FAIL:
		tmos_set_event( taskID , EVT_RX_START );
        break;
    }
    #if ARF_LOG_HOTPATH
    PRINT("STA: %x\n", sta);
#endif
#endif
}

//************************** ARF_ProcessEvent ***************************
static uint16_t arf_ProcessEvent(uint8_t task_id, uint16_t events) {
#ifdef ARF_RISCV
    if(events & SYS_EVENT_MSG)    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(task_id)) != NULL) {
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    if(events & EVT_RX_START)    {
        uint8_t state;
        RF_Shut();
        state = RF_Rx(txdat.buf, 0, 0xFF, 0xFF);
#if ARF_LOG_HOTPATH
        PRINT("RX mode.state = %x\n", state);
#else
        (void)state;
#endif
        return events ^ EVT_RX_START;
    }
    if(events & EVT_TX_START)    {
		RF_Shut();
		RF_Tx(txdat.buf, txdat.len, 0xFF, 0xFF);
        return events ^ EVT_TX_START;
    }
#endif
    if(events & EVT_LEDOFF)	{ LED_off(); return events ^ EVT_LEDOFF; }
    if(events & EVT_DELAY)	{ txdat.delaystat = 0; return events ^ EVT_DELAY; }
    if(events & EVT_TIMER1)	{
        if(timer_func[0] != NULL) (*timer_func[0])();
		if(timer_TC[0] > 0) event_at(task_id, EVT_TIMER1, timer_TC[0]);
        return events ^ EVT_TIMER1;
    }
    if(events & EVT_TIMER2)	{
        if(timer_func[1] != NULL) (*timer_func[1])();
		if(timer_TC[1] > 0) event_at(task_id, EVT_TIMER2, timer_TC[1]);
        return events ^ EVT_TIMER2;
    }
    if(events & EVT_TIMER3)	{
        if(timer_func[2] != NULL) (*timer_func[2])();
		if(timer_TC[2] > 0) event_at(task_id, EVT_TIMER3, timer_TC[2]);
        return events ^ EVT_TIMER3;
    }
    return 0;
}

//****************************** ARF_Init *********************************
void arf_Init(void) {
#if defined(ARF_RISCV)
    uint8_t    state;
    rfConfig_t rfConfig;

    tmos_memset(&rfConfig, 0, sizeof(rfConfig_t));
    taskID = TMOS_ProcessEventRegister(arf_ProcessEvent);
    rfConfig.accessAddress = 0x71764129;
	// 禁止使用0x55555555以及0xAAAAAAAA(建议不超过24次位反转,且不超过连续的6个0或1)
    rfConfig.CRCInit = 0x555555;
    rfConfig.Channel = 8;
    rfConfig.Frequency = 2480000;
    rfConfig.LLEMode = LLE_MODE_BASIC | LLE_MODE_EX_CHANNEL;
		// 使能 LLE_MODE_EX_CHANNEL 表示 选择 rfConfig.Frequency 作为通信频点
    rfConfig.rfStatusCB = arf_2G4_cbfc0;
    //rfConfig.RxMaxlen = ARF_MsgN;
    state = RF_Config(&rfConfig);
    PRINT("rf 2.4g init: %x\n", state);
#else
    taskID = arf_register(arf_ProcessEvent);
#endif
}

void arf_set_timer(int tmr, int t_us, arf_proc_fc tmrf) {
	int tc = t_us / ARF_TC_us;	// t_us>0 cycle timer, t_us<0 one-time timer
	if(tmr < 0 || tmr >= ARF_TimerN) tmr = ARF_TimerN - 1;
	timer_func[tmr] = tmrf; timer_TC[tmr] = tc;
	if(tc != 0) event_at(taskID, EVT_TIMER1 << tmr, tc > 0? tc: -tc);
}

void arf_set_procfunc(arf_proc_fc procf) { proc_func = procf; }
void arf_set_newdatfunc(arf_newdat_fc newdatf) { arf_newdatfunc = newdatf; }

//*************************** arf_register  *******************************
uint8_t arf_register(arf_task_fc taskf) {
#ifdef ARF_RISCV
	return (int)TMOS_ProcessEventRegister(taskf);
#else
	arf_task_t *tskp; int tsk, evt;

	tsk = arf.top;
	if(tsk >= Max_Task) return -1;
	arf.top = tsk + 1;
	tskp = arf.tasks + tsk;
	tskp->func = (taskf != NULL)? taskf: arf_taskfc0;
	for(evt = 0; evt < Max_Event; evt++) { tskp->tcs[evt] = 0; }
	arf_proc();
	return (uint8_t)tsk;
#endif
}

//****************************** arf_event  *******************************
void arf_event(uint8_t task_id, uint16_t evt) {
#ifdef ARF_RISCV
	tmos_set_event(task_id, evt);
#else
	(*arf.tasks[task_id].func)(task_id, evt);
#endif
}

void arf_event_at(uint8_t task_id, uint16_t evt, int tc) {
#ifdef ARF_RISCV
	tmos_start_task(task_id, evt, tc);
#else
	for(int i = 0; i < Max_Event; i++) { 
		if(evt == (1 << i)) { arf.tasks[task_id].tcs[i] = tc; break; }
	}
#endif
}

//******************************** arf_proc  *******************************
void arf_proc(void) {
	if(proc_func != NULL) (*proc_func)();
#ifdef ARF_RISCV
	TMOS_SystemProcess();
#else
	uint32_t tc, dtc; arf_task_t *tskp; int tsk, evt;
	
#ifdef ARF_WIN
	struct timeb ftm;
	ftime(&ftm);
	tc = (uint32_t)ftm.time * 1000 + (uint32_t)ftm.millitm;
#else
	tc = sys_cnt_100us;
#endif	// ARF_WIN

	if(tc == arf.tc) return;
	dtc = (tc - arf.tc) & 0xFFFFFF;	arf.tc = tc;
	for(tsk = 0, tskp=arf.tasks; tsk < arf.top; tsk++, tskp++) {
		for(evt = 0; evt < Max_Event; evt++) {
			if((tc = tskp->tcs[evt]) > 0) {
				if(tc > dtc) { tskp->tcs[evt] = tc - dtc; }
				else { tskp->tcs[evt] = 0; arf_event(tsk, (1<<evt)); }
	}	}	}
#endif	// ARF_RISCV
}

// ---------------------------- Rx-Tx --------------------------------------

void arf_RxStart(void) { event_set( taskID , EVT_RX_START ); }

uint8_t* arf_isRxFinish(void) {
	int i = (int)rxdat.iget;
    if(i == (int)rxdat.irecv)  {
#ifndef ARF_RISCV
        arf_proc();
#endif
        return NULL;
    }
	
	uint8_t *rxbuf = rxdat.buf[i];
	rxdat.iget = Ndat_next(i);
	return rxbuf;
}

static int arf_TxStart(uint8_t buf[], int len) {
	if(len <= 0 || len > (int)sizeof(txdat.buf)) { return -1; }
	if(txdat.len > 0 && txdat.len <= ARF_MsgN)  {
#ifndef ARF_RISCV
        arf_proc();
#endif
        return 0;
    }
	txdat.len = (uint8_t)len;
	memcpy(txdat.buf, buf, sizeof(txdat.buf));
#if TX_DelayTC > 0
    event_at(taskID, EVT_TX_START, TX_DelayTC);
#else
	event_set( taskID , EVT_TX_START );
#endif
	return len;
}

static void arf_WaitTxFinish(void) {
	uint16_t tc0 = arf_u16(tc_sys());
    while(txdat.len > 0 && txdat.len <= ARF_MsgN)  {
		arf_proc();
		uint16_t dtc = arf_u16(tc_sys() - tc0);
		if(dtc >= TX_WaitTC) { txdat.len = ARF_MsgN+3; break; }
}	}
	
int arf_TxSend(uint8_t buf[], int len) {
	if(len <= 0 || len > (int)sizeof(txdat.buf)) { len = ARF_MsgN; }
	while(arf_TxStart(buf, len) == 0) ;
	arf_WaitTxFinish();
	if((led_mod & ARF_LED_SEND) != 0) { arf_led_on(40); }
	return len;
}

int arf_TxTrySend(uint8_t buf[], int len) {
	if(len <= 0 || len > (int)sizeof(txdat.buf)) { len = ARF_MsgN; }
	return arf_TxStart(buf, len);
}

int arf_isTxBusy(void) {
	return (txdat.len > 0 && txdat.len <= ARF_MsgN)? 1: 0;
}

//**************************** get_TC delayms ******************************
uint16_t arf_get_tc16(void)  { return arf_u16(tc_sys()); }
uint32_t arf_get_tc(void)	 { return (uint32_t)tc_sys(); }

void arf_delayms(int tm_ms) {
	txdat.delaystat = 1;
    event_at(taskID, EVT_DELAY, tm_ms*1000/ARF_TC_us);
    while(txdat.delaystat != 0)    {   arf_proc();    }
}

//********************************* LED ***********************************
void arf_led_on(int tm_ms) {
	LED_on();
	event_at(taskID, EVT_LEDOFF, tm_ms*1000/ARF_TC_us);
}

void arf_led_mod(int mod) { led_mod = mod; }

int arf_led_id(int id) {
//	const uint8_t msc_v[10] = { 1,1,1,1,1,0,16,8,4,2 };
//	const uint8_t msc_n[10] = { 1,2,3,4,5,5, 5,4,3,2 };
	
	arf_delayms(600);
	if(id < 0) { id = (int)led_mod; }
	int n = 8;
	while(--n > 0) { if((id & (1 << n)) != 0) { break; } }
	for(; n >= 0; n--) {
		LED_on();
		arf_delayms((id & (1 << n)) == 0? 5: 200);
		LED_off();
		arf_delayms(300);
	}
	arf_delayms(600);
	return id;
}

/******************************** endfile @ main ******************************/











