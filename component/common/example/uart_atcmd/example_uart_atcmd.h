#ifndef __EXAMPLE_UART_ATCMD_H__
#define __EXAMPLE_UART_ATCMD_H__

/******************************************************************************
 *
 * Copyright(c) 2007 - 2015 Realtek Corporation. All rights reserved.
 *
 *
 ******************************************************************************/
#if CONFIG_EXAMPLE_UART_ATCMD
#include "FreeRTOS.h"
#include "semphr.h"
#define AMEBA1		1
#define AMEBAZ		2
#define AMEBAD		3
#define AMEBAZ2		4

/*UART Pinmux*/
#define CONFIG_AMEBA AMEBAD
#if (CONFIG_AMEBA == AMEBA1 )
#define UART_TX			PA_4
#define UART_RX			PA_0
#define UART_RTS			PA_2
#define UART_CTS			PA_1
#elif (CONFIG_AMEBA == AMEBAZ )
#define UART_TX			PA_23
#define UART_RX			PA_18
#define UART_RTS			PA_22
#define UART_CTS			PA_19
#elif (CONFIG_AMEBA == AMEBAD )
#define UART_TX			PA_18
#define UART_RX			PA_19
#define UART_RTS			PA_16
#define UART_CTS			PA_17
#elif (CONFIG_AMEBA == AMEBAZ2)
#define UART_TX			PA_14
#define UART_RX			PA_13
#define UART_RTS			NC
#define UART_CTS			NC
#endif

#define ATCMD_RX_GPIO_WAKEUP 0
#define KEY_NL			0xa // '\n'
#define KEY_ENTER		0xd // '\r'
#define KEY_BS			0x8
#define KEY_ESC			0x1B
#define KEY_LBRKT		0x5B

void uart_at_send_string(char *str);
void uart_at_send_buf(u8 *buf, u32 len);
void example_uart_atcmd(void);

#endif //#if CONFIG_EXAMPLE_UART_ATCMD
#endif //#ifndef __EXAMPLE_UART_ATCMD_H__
