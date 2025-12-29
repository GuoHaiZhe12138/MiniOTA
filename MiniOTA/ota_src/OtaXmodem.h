/*******************************************************************************
  * @file           : OtaTrans.h
  * @brief          : Xmodem Table-Driven Refactored
  ******************************************************************************/

#ifndef __OTAXMODEM_H
#define __OTAXMODEM_H

#include <stdint.h>
#include "OtaInterface.h"

/* ---------------- XMODEM Control ---------------- */
#define XM_SOH   0x01
#define XM_STX   0x02
#define XM_EOT   0x04
#define XM_ACK   0x06
#define XM_NAK   0x15

/* ---------------- 状态定义 ---------------- */
typedef enum {
    XM_WAIT_START = 0,
    XM_WAIT_BLK,
    XM_WAIT_BLK_INV,
    XM_WAIT_DATA,
    XM_WAIT_CRC1,
    XM_WAIT_CRC2,
    XM_STATE_MAX
} xm_state_t;


/* ---------------- Xmodem Handle ---------------- */
typedef struct {
    xm_state_t state;
    uint8_t    blk;
    uint8_t    blk_inv;
    uint8_t    expected_blk;
    uint16_t   data_len;
    uint16_t   data_cnt;
    uint16_t   crc_recv;
    uint16_t   crc_calc;
    uint8_t    data_buf[1024];
} xmodem_t;

/* 定义状态处理函数指针类型 */
typedef void (*xm_state_fn_t)(uint8_t);

/* ---------------- API ---------------- */
void OTA_XmodemInit(void);
void OTA_XmodemRevByte(uint8_t ch);
uint8_t OTA_XmodemRevCompFlag(void);
xmodem_t* OTA_GetXmodemHandle(void);

#endif
