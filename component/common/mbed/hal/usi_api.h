/** mbed Microcontroller Library
  ******************************************************************************
  * @file    uspi_api.h
  * @author 
  * @version V1.0.0
  * @brief   This file provides following mbed SPI API
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2006-2013 ARM Limited
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  ****************************************************************************** 
  */
#ifndef MBED_USI_API_H
#define MBED_USI_API_H

#include "device.h"
#include "spi_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup spi USPI
 *  @ingroup    hal
 *  @brief      uspi functions
 *  @{
 */


#if (defined (CONFIG_PLATFORM_8721D) && (CONFIG_PLATFORM_8721D == 1))
///@name AmebaZ and AmebaD 
///@{
typedef enum {
	MBED_USI0_SPI = 0xF0,   /*!< means USI0_SPI */
} MBED_USI_IDX;
///@}
#endif //CONFIG_PLATFORM_8721D

///@name Ameba Common
///@{

/**
  * @brief  Initializes the SPI device, include clock/function/interrupt/SPI registers.
  * @param  obj: spi object define in application software.
  * @param  mosi: MOSI PinName according to pinmux spec.
  * @param  miso: MISO PinName according to pinmux spec.
  * @param  sclk: SCLK PinName according to pinmux spec.
  * @param  ssel: CS PinName according to pinmux spec.
  * @retval none  
  * @note must set obj->spi_index to MBED_USI0_SPI before using uspi_init
  */
void uspi_init(spi_t *obj, PinName mosi, PinName miso, PinName sclk, PinName ssel);

/**
  * @brief  Deinitializes the SPI device, include interrupt/DMA/DISABLE SPI.
  * @param  obj: spi object define in application software.
  * @retval none  
  */
void uspi_free(spi_t *obj);

/**
  * @brief  Set SPI format,include DFS/Phase/Polarity.
  * @param  obj: spi object define in application software.
  * @param  bits: data frame size, 4-16 supported.
  * @param  mode: this parameter can be one of the following values:
  *		@arg 0 : [Polarity,Phase]=[0,0]
  *		@arg 1 : [Polarity,Phase]=[0,1]
  *		@arg 2 : [Polarity,Phase]=[1,0]
  *		@arg 3 : [Polarity,Phase]=[1,1]
  * @param  slave: this parameter can be one of the following values:
  *		@arg 0 : indicates role-master 
  *		@arg 1 : indicates role-slave
  * @retval none  
  */
void uspi_format(spi_t *obj, int bits, int mode, int slave);

/**
  * @brief  Set SPI baudrate.
  * @param  obj: spi master object define in application software.
  * @param  hz: baudrate for SPI bus
  * @retval none  
  * @note "hz" should be less or equal to half of the SPI IpClk
  */
void uspi_frequency(spi_t *obj, int hz);

/**
  * @brief  Master send one frame use SPI.
  * @param  obj: spi master object define in application software.
  * @param  value: the data to transmit.
  * @retval : data received from slave
  */
int  uspi_master_write(spi_t *obj, int value);

/**
  * @brief  Get slave readable && busy state.
  * @param  obj: spi slave object define in application software.
  * @retval : slave Readable && Busy State
  */
int  uspi_slave_receive(spi_t *obj);

/**
  * @brief  Slave receive one frame use SPI.
  * @param  obj: spi slave object define in application software.
  * @retval : data received from master
  */
int  uspi_slave_read(spi_t *obj);

/**
  * @brief  Slave send one frame use SPI.
  * @param  obj: spi slave object define in application software.
  * @param  value: the data to transmit.
  * @retval none
  */
void uspi_slave_write(spi_t *obj, int value);

/**
  * @brief  Get SPI busy state.
  * @param  obj: spi object define in application software.
  * @retval : current busy state
  */
int  uspi_busy(spi_t *obj);

/**
  * @brief  SPI device to flush rx fifo.
  * @param  obj: spi  object define in application software.
  * @retval  none
  */
void uspi_flush_rx_fifo(spi_t *obj);

/**
  * @brief  Open SPI device clock.
  * @param  obj: spi object define in application software.
  * @retval none
  */
void uspi_enable(spi_t *obj);

/**
  * @brief  Close SPI device clock.
  * @param  obj: spi object define in application software.
  * @retval none
  */
void uspi_disable(spi_t *obj);
///@}

/*\@}*/

#ifdef __cplusplus
}
#endif

#endif//MBED_USI_API_H

