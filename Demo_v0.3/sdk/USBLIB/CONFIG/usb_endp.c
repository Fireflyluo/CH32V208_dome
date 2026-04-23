/********************************** (C) COPYRIGHT *******************************
 * File Name          : usb_endp.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2021/08/08
 * Description        : Endpoint routines
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "hw_config.h"
#include "usb_cdc.h"
#include "usb_desc.h"
#include "usb_istr.h"
#include "usb_lib.h"
#include "usb_mem.h"
#include "usb_prop.h"
#include "usb_pwr.h"

uint8_t USBD_Endp3_Busy;
uint16_t USB_Rx_Cnt = 0;
static uint8_t ep2_rx_buf[DEF_USBD_MAX_PACK_SIZE];

/*********************************************************************
 * @fn      EP1_IN_Callback
 *
 * @brief   Endpoint 1 IN.
 *
 * @return  none
 */
void EP1_IN_Callback(void)
{
}

/*********************************************************************
 * @fn      EP2_OUT_Callback
 *
 * @brief   Endpoint 2 OUT.
 *
 * @return  none
 */
void EP2_OUT_Callback(void)
{
    uint16_t len = GetEPRxCount(EP2_OUT & 0x7F);
    if (len > DEF_USBD_MAX_PACK_SIZE)
    {
        len = DEF_USBD_MAX_PACK_SIZE;
    }

    PMAToUserBufferCopy(ep2_rx_buf, GetEPRxAddr(EP2_OUT & 0x7F), len);
    CDC_StoreReceivedData(ep2_rx_buf, len);
    SetEPRxValid(ENDP2);
}

/*********************************************************************
 * @fn      EP3_IN_Callback
 *
 * @brief   Endpoint 3 IN.
 *
 * @return  none
 */
void EP3_IN_Callback(void)
{
    USBD_Endp3_Busy = 0;
}

/*********************************************************************
 * @fn      USBD_ENDPx_DataUp
 *
 * @brief   USBD ENDPx DataUp Function
 *
 * @param   endp - endpoint num.
 *          pbuf - A pointer points to data.
 *          len - data length to transmit.
 *
 * @return  data up status.
 */
uint8_t USBD_ENDPx_DataUp(uint8_t endp, uint8_t *pbuf, uint16_t len)
{
    if (endp != ENDP3)
    {
        return USB_ERROR;
    }

    if (USBD_Endp3_Busy)
    {
        return USB_ERROR;
    }

    USB_SIL_Write(EP3_IN, pbuf, len);
    USBD_Endp3_Busy = 1;
    SetEPTxStatus(ENDP3, EP_TX_VALID);
    return USB_SUCCESS;
}

