/**
 ******************************************************************************
 * @file    main.c
 * @brief   MiniOTA BootLoader 示例入口（STM32F411CEU6）
 *          初始化时钟、串口与 IAP 按键后调用 OTA_Run()，返回则进入空循环
 ******************************************************************************
 */
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "OtaInterface.h"

static void UART1_Init(void);
static void UART2_Init(void);
static void PB11_Input_Init(void);

int main(void)
{
    UART1_Init();
    UART2_Init();
    PB11_Input_Init();

    OTA_Run();

    for (;;)
    {
    }
}

static void PB11_Input_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_11;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
}

static void UART1_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct = {0};
    USART_InitTypeDef USART_InitStruct = {0};
    NVIC_InitTypeDef NVIC_InitStruct = {0};

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    USART_InitStruct.USART_BaudRate            = 9600;
    USART_InitStruct.USART_WordLength         = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits           = USART_StopBits_1;
    USART_InitStruct.USART_Parity             = USART_Parity_No;
    USART_InitStruct.USART_Mode               = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &USART_InitStruct);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_InitStruct.NVIC_IRQChannel                   = USART1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority       = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd               = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    USART_Cmd(USART1, ENABLE);
}

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint8_t ch = (uint8_t)USART_ReceiveData(USART1);
        OTA_ReceiveTask(ch);
    }
}

static void UART2_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct = {0};
    USART_InitTypeDef USART_InitStruct = {0};

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    USART_InitStruct.USART_BaudRate            = 115200;
    USART_InitStruct.USART_WordLength         = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits           = USART_StopBits_1;
    USART_InitStruct.USART_Parity             = USART_Parity_No;
    USART_InitStruct.USART_Mode               = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART2, &USART_InitStruct);

    USART_Cmd(USART2, ENABLE);
}
