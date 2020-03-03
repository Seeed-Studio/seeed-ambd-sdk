/** mbed Microcontroller Library
  ******************************************************************************
  * @file    uspi_ex_api.h
  * @author 
  * @version V1.0.0
  * @brief   This file provides following mbed SPI API
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2015, Realtek Semiconductor Corp.
  * Copyright (c) 2020, Seeed Technology Co.,Ltd.
  * All rights reserved.
  *
  * This module is a confidential and proprietary property of RealTek and
  * possession or use of this module requires written permission of RealTek.
  ****************************************************************************** 
  */
#ifndef MBED_USI_EXT_API_H
#define MBED_USI_EXT_API_H

#include "device.h"
#include "spi_ex_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup usi_ex USI_EX
 *  @ingroup    hal
 *  @brief      usi extended functions
 *  @{
 */

///@name Ameba Common
///@{
/**
  * @brief  Set SPI interrupt handler if needed.
  * @param  obj: spi object define in application software.
  * @param  handler: interrupt callback function
  * @param  id: interrupt callback parameter
  * @retval none  
  */
void uspi_irq_hook(spi_t *obj, spi_irq_handler handler, uint32_t id);

/**
  * @brief  Set SPI interrupt bus tx done handler if needed.
  * @param  obj: spi object define in application software.
  * @param  handler: interrupt bus tx done callback function
  * @param  id: interrupt callback parameter
  * @retval none  
  */
void uspi_bus_tx_done_irq_hook(spi_t *obj, spi_irq_handler handler, uint32_t id);

/**
  * @brief  Slave device to flush tx fifo.
  * @param  obj: spi slave object define in application software.
  * @note : It will discard all data in both tx fifo and rx fifo
  */
void uspi_slave_flush_fifo(spi_t * obj);

/**
  * @brief  slave recv target length data use interrupt mode.
  * @param  obj: spi slave object define in application software.
  * @param  rx_buffer: buffer to save data read from SPI FIFO.
  * @param  length: number of data bytes to be read.
  * @retval  : stream init status
  */
int32_t uspi_slave_read_stream(spi_t *obj, char *rx_buffer, uint32_t length);

/**
  * @brief  slave send target length data use interrupt mode.
  * @param  obj: spi slave object define in application software.
  * @param  tx_buffer: buffer to be written to Tx FIFO.
  * @param  length: number of data bytes to be send.
  * @retval  : stream init status
  */
int32_t uspi_slave_write_stream(spi_t *obj, char *tx_buffer, uint32_t length);

/**
  * @brief  master recv target length data use interrupt mode.
  * @param  obj: spi master object define in application software.
  * @param  rx_buffer: buffer to save data read from SPI FIFO.
  * @param  length: number of data bytes to be read.
  * @retval  : stream init status
  */
int32_t uspi_master_read_stream(spi_t *obj, char *rx_buffer, uint32_t length);

/**
  * @brief  master send target length data use interrupt mode.
  * @param  obj: spi master object define in application software.
  * @param  tx_buffer: buffer to be written to Tx FIFO.
  * @param  length: number of data bytes to be send.
  * @retval  : stream init status
  */
int32_t uspi_master_write_stream(spi_t *obj, char *tx_buffer, uint32_t length);

/**
  * @brief  master send & recv target length data use interrupt mode.
  * @param  obj: spi master object define in application software.
  * @param  tx_buffer: buffer to be written to Tx FIFO.
  * @param  rx_buffer: buffer to save data read from SPI FIFO.
  * @param  length: number of data bytes to be send & recv.
  * @retval  : stream init status
  */
int32_t uspi_master_write_read_stream(spi_t *obj, char *tx_buffer, char *rx_buffer, uint32_t length);

/**
  * @brief  slave recv target length data use interrupt mode and timeout mechanism.
  * @param  obj: spi slave object define in application software.
  * @param  rx_buffer: buffer to save data read from SPI FIFO.
  * @param  length: number of data bytes to be read.
  * @param  timeout_ms: timeout waiting time.
  * @retval  : number of bytes read already
  */
int32_t uspi_slave_read_stream_timeout(spi_t *obj, char *rx_buffer, uint32_t length, uint32_t timeout_ms);

/**
  * @brief  slave recv target length data use interrupt mode and stop if the spi bus is idle.
  * @param  obj: spi slave object define in application software.
  * @param  rx_buffer: buffer to save data read from SPI FIFO.
  * @param  length: number of data bytes to be read.
  * @retval  : number of bytes read already
  */
int32_t uspi_slave_read_stream_terminate(spi_t *obj, char *rx_buffer, uint32_t length);

//#ifdef CONFIG_GDMA_EN  
/**
  * @brief  slave recv target length data use DMA mode.
  * @param  obj: spi slave object define in application software.
  * @param  rx_buffer: buffer to save data read from SPI FIFO.
  * @param  length: number of data bytes to be read.
  * @retval  : stream init status
  */  
int32_t uspi_slave_read_stream_dma(spi_t *obj, char *rx_buffer, uint32_t length);

/**
  * @brief  slave send target length data use DMA mode.
  * @param  obj: spi slave object define in application software.
  * @param  tx_buffer: buffer to be written to Tx FIFO.
  * @param  length: number of data bytes to be send.
  * @retval  : stream init status
  */
int32_t uspi_slave_write_stream_dma(spi_t *obj, char *tx_buffer, uint32_t length);

/**
  * @brief  master send & recv target length data use DMA mode.
  * @param  obj: spi master object define in application software.
  * @param  tx_buffer: buffer to be written to Tx FIFO.
  * @param  rx_buffer: buffer to save data read from SPI FIFO.
  * @param  length: number of data bytes to be send & recv.
  * @retval  : stream init status
  */
int32_t uspi_master_write_read_stream_dma(spi_t * obj, char * tx_buffer, char * rx_buffer, uint32_t length);

/**
  * @brief  master recv target length data use DMA mode.
  * @param  obj: spi master object define in application software.
  * @param  rx_buffer: buffer to save data read from SPI FIFO.
  * @param  length: number of data bytes to be read.
  * @retval  : stream init status
  * @note : DMA or Interrupt mode can be used to TX dummy data
  */
int32_t uspi_master_read_stream_dma(spi_t *obj, char *rx_buffer, uint32_t length);

/**
  * @brief  master send target length data use DMA mode.
  * @param  obj: spi master object define in application software.
  * @param  tx_buffer: buffer to be written to Tx FIFO.
  * @param  length: number of data bytes to be send.
  * @retval  : stream init status
  */
int32_t uspi_master_write_stream_dma(spi_t *obj, char *tx_buffer, uint32_t length);

/**
  * @brief  slave recv target length data use DMA mode and timeout mechanism.
  * @param  obj: spi slave object define in application software.
  * @param  rx_buffer: buffer to save data read from SPI FIFO.
  * @param  length: number of data bytes to be read.
  * @param  timeout_ms: timeout waiting time.
  * @retval  : number of bytes read already
  */
int32_t uspi_slave_read_stream_dma_timeout(spi_t *obj, char *rx_buffer, uint32_t length, uint32_t timeout_ms);

/**
  * @brief  slave recv target length data use DMA mode and stop if the spi bus is idle.
  * @param  obj: spi slave object define in application software.
  * @param  rx_buffer: buffer to save data read from SPI FIFO.
  * @param  length: number of data bytes to be read.
  * @retval  : number of bytes read already
  */
int32_t uspi_slave_read_stream_dma_terminate(spi_t * obj, char * rx_buffer, uint32_t length);
//#endif

///@}

/*\@}*/

#ifdef __cplusplus
}
#endif


#endif//MBED_USI_EXT_API_H
