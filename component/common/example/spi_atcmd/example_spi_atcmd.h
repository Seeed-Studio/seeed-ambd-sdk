/******************************************************************************
 *
 * Copyright(c) 2007 - 2015 Realtek Corporation. All rights reserved.
 *
 *
 ******************************************************************************/

#ifndef __EXAMPLE_SPI_ATCMD_H__
#define __EXAMPLE_SPI_ATCMD_H__

#if CONFIG_EXAMPLE_SPI_ATCMD

#ifdef CONFIG_RTL8721D
// amebad hs usi spi pin
#define USI_SPI_MOSI PA_25
#define USI_SPI_MISO PA_26
#define USI_SPI_SCLK PA_30
#define USI_SPI_CS   PA_28
#define GPIO_HRDY    PA_12
#define GPIO_SYNC    PA_13
#define IS_USI_SPI 1
#else
#define SPI0_MOSI  PC_2
#define SPI0_MISO  PC_3
#define SPI0_SCLK  PC_1
#define SPI0_CS    PC_0
#define GPIO_HRDY  PA_1
#define GPIO_SYNC  PB_3
#endif

#define GPIO_CS    PA_3

void example_spi_atcmd(void);

#endif // #if CONFIG_EXAMPLE_SPI_ATCMD

#endif // #ifndef __EXAMPLE_SPI_ATCMD_H__
