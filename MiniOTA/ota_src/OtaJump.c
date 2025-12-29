/*******************************************************************************
  * @file           : OtaJump.c
  * @brief          : Xmodem Table-Driven Refactored
  ******************************************************************************/

#include "stdint.h"
#include "stm32f10x.h"

#include "OtaInterface.h"

typedef void (*pFunction)(void);

void JumpToApp(void)
{
    uint32_t app_sp;
    uint32_t app_reset;
    pFunction app_entry;

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    /* 设置中断向量表到 App 区 */
    SCB->VTOR = OTA_APP_START_ADDRESS;

    app_sp    = *(uint32_t *)OTA_APP_START_ADDRESS;
    app_reset = *(uint32_t *)(OTA_APP_START_ADDRESS + 4);

    __set_MSP(app_sp);

    app_entry = (pFunction)app_reset;
    app_entry();
}

