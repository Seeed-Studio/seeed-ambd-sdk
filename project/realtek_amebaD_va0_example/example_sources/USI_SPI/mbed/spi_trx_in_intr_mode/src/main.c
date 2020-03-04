/*
 *  Routines to access hardware
 *
 *  Copyright (c) 2014 Realtek Semiconductor Corp.
 *  Copyright (c) 2020 Seeed Technology Co.,Ltd.
 *
 *  This module is a confidential and proprietary property of RealTek and
 *  possession or use of this module requires written permission of RealTek.
 */

#include "device.h"
#include "diag.h"
#include "main.h"
#include "usi_api.h"
#include "usi_ex_api.h"
#include "wait_api.h"

#define DataFrameSize	8
#define dfs_mask		0xFF
#define Mode			0
#define SCLK_FREQ		100000
#define TEST_BUF_SIZE	256

/*USI_SPI pin location:

		PA_25  (MOSI)
		PA_26  (MISO)
		PA_30  (SCLK)
		PA_28  (CS)
*/

#define USI_SPI_MOSI	PA_25
#define USI_SPI_MISO	PA_26
#define USI_SPI_SCLK	PA_30
#define USI_SPI_CS	PA_28

SRAM_NOCACHE_DATA_SECTION u16 SlaveTxBuf[TEST_BUF_SIZE];
SRAM_NOCACHE_DATA_SECTION u16 SlaveRxBuf[TEST_BUF_SIZE];

volatile int SlaveTxDone;
volatile int SlaveRxDone;

void Slave_tr_done_callback(void *pdata, SpiIrq event)
{
	(void) pdata;

	switch(event){
		case SpiRxIrq:
			SlaveRxDone = 1;
			break;
		case SpiTxIrq:
			SlaveTxDone = 1;
			break;
		default:
			DBG_8195A("unknown interrput evnent!\n");
	}
}

BOOL SsiDataCompare(u16 *pSrc, u16 *pDst, int Length)
{
	int Index;
	u8 *PSrc_8 = (u8*)pSrc;
	u8 *PDst_8 = (u8*)pDst;
	u8 res = _TRUE;
	
	if(DataFrameSize > 8) {
		for (Index = 0; Index < Length; ++Index){
			if ((pSrc[Index] & dfs_mask) != pDst[Index]) {
				DBG_8195A("%x:   %X ---- %X\n",Index, pSrc[Index] & dfs_mask, pDst[Index]);
				res = _FALSE;
			}
		}
	} else {
		for (Index = 0; Index < Length; ++Index){
			if((PSrc_8[Index] & dfs_mask) != PDst_8[Index]) {
				DBG_8195A("%x:   %X ---- %X\n",Index, PSrc_8[Index] & dfs_mask, PDst_8[Index]);
				res = _FALSE;
			}
		}
	}
	return res;
}

void SsiPrint(u16 *pSrc, int Length)
{
	int Index;
	u8 *PSrc_8 = (u8*)pSrc;

	for (Index = 0;Index < Length; Index++){
		if(DataFrameSize > 8)
			DBG_8195A("%X: %X\n",Index, pSrc[Index] & dfs_mask);
		else
			DBG_8195A("%X: %X\n",Index, PSrc_8[Index] & dfs_mask);
	}
} 

/**
  * @brief  Main program.
  * @param  None 
  * @retval None
  */
spi_t spi_slave;

int main(void)
{

	/* USI0_DEV is as Slave */
	spi_slave.spi_idx = MBED_USI0_SPI;
	uspi_init(&spi_slave, USI_SPI_MOSI, USI_SPI_MISO, USI_SPI_SCLK, USI_SPI_CS);
	uspi_format(&spi_slave, DataFrameSize, Mode, 1);

	int i=0;
	int result1 = 1;
	u32 trans_bytes = (DataFrameSize > 8) ? (TEST_BUF_SIZE * 2) : TEST_BUF_SIZE;

	LOG_MASK(LEVEL_ERROR, -1UL);

	int k;

	for (k = 0; k < 2; k++) {

		_memset(SlaveTxBuf, 0, TEST_BUF_SIZE);
		_memset(SlaveRxBuf, 0, TEST_BUF_SIZE);
	
		for (i = 0; i < TEST_BUF_SIZE;i++) {
			if (DataFrameSize > 8)
				*((u16*)SlaveTxBuf + i) = i;
			else
				*((u8*)SlaveTxBuf + i) = i;
		}


	/**
	* Slave read/write
	*/
	DBG_8195A("------USI_SPI Slave read/write-------\n");
	
		uspi_irq_hook(&spi_slave, (spi_irq_handler)Slave_tr_done_callback, (uint32_t)&spi_slave);
		
		SlaveRxDone = 0;

		uspi_slave_read_stream(&spi_slave, (char*)SlaveRxBuf, trans_bytes);
		uspi_slave_write_stream(&spi_slave, (char*)SlaveTxBuf, trans_bytes);
		

		i=0;
		while (SlaveRxDone == 0) {
			wait_ms(100);
			i++;
			if (i>150) {
				DBG_8195A("SPI Slave TRx Wait Timeout\r\n");
				break;
			}
		}
		
		// SsiPrint(SlaveRxBuf, TEST_BUF_SIZE);
		result1 = SsiDataCompare(SlaveTxBuf, SlaveRxBuf, TEST_BUF_SIZE);

	/**
	* Slave write
	*/
	DBG_8195A("-----------Slave write------------\n");

		SlaveTxDone = 0;

		uspi_slave_write_stream(&spi_slave, (char*)SlaveTxBuf, trans_bytes);

		i=0;
		while(SlaveTxDone == 0) {
			wait_ms(100);
			i++;
			if (i>150) {
				DBG_8195A("SPI Slave Tx Wait Timeout\r\n");
				break;
			}
		}
		uspi_flush_rx_fifo(&spi_slave);


		DBG_8195A("\r\nResult is %s\r\n", result1? "success" : "fail");
	}
	
	uspi_free(&spi_slave);

	DBG_8195A("USI SPI MBED Demo finished.\n");

	for(;;);
	return 0;
}

