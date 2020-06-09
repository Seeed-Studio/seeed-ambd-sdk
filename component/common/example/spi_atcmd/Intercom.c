#include "Intercom.h"

#include <osdep_service.h>
#include <device.h>
#include "rtl8721d_usi_ssi.h"
#include "DebugDout.h"

#define PIN_DIR_RX			(_PA_13)
#define PIN_EXIST_TX_DATA	(_PA_12)

#define USI_SPI_MOSI		(PA_25)
#define USI_SPI_MISO		(PA_26)
#define USI_SPI_SCLK		(PA_30)
#define USI_SPI_CS			(PA_28)

typedef struct
{
	USI_TypeDef* usi_dev;

	void* RxData;
	void* TxData;
	u32  RxLength;
	u32  TxLength;

	GDMA_InitTypeDef USISsiTxGdmaInitStruct;
	GDMA_InitTypeDef USISsiRxGdmaInitStruct;

	u32   Role;
}
USISSI_OBJ, *P_USISSI_OBJ;

static USISSI_OBJ USISsiObj;
static _sema _SemaRxDone;
static _sema _SemaTxDone;

static void USISsiFlushRxFifo(P_USISSI_OBJ pHalSsiAdaptor)
{
	while (USI_SSI_Readable(pHalSsiAdaptor->usi_dev))
	{
		u32 rx_fifo_level = USI_SSI_GetRxCount(pHalSsiAdaptor->usi_dev);
		for (u32 i = 0; i < rx_fifo_level; ++i)
		{
			USI_SSI_ReadData(pHalSsiAdaptor->usi_dev);
		}
	}
}

static void USISsiDmaRxIrqHandle(P_USISSI_OBJ pUSISsiObj)
{
	u32 Length = pUSISsiObj->RxLength;
	u32* pRxData = pUSISsiObj->RxData;
	PGDMA_InitTypeDef GDMA_InitStruct;

	GDMA_InitStruct = &pUSISsiObj->USISsiRxGdmaInitStruct;

	/* Clear Pending ISR */
	GDMA_ClearINT(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);
	GDMA_Cmd(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, DISABLE);

	DCache_Invalidate((u32)pRxData, Length);

	USI_SSI_SetDmaEnable(pUSISsiObj->usi_dev, DISABLE, USI_RX_DMA_ENABLE);

	/*  RX complete callback */
	rtw_up_sema(&_SemaRxDone);

	GDMA_ChnlFree(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);

	DebugDout0(0);
}

static void USISsiSlaveReadStreamDma(P_USISSI_OBJ pUSISsiObj, u8* rx_buffer, u32 length)
{
	DebugDout0(1);

	pUSISsiObj->RxLength = length;
	pUSISsiObj->RxData = (void*)rx_buffer;

	USI_SSI_RXGDMA_Init(0, &pUSISsiObj->USISsiRxGdmaInitStruct, pUSISsiObj, (IRQ_FUN)USISsiDmaRxIrqHandle, rx_buffer, length);
	USI_SSI_SetDmaEnable(pUSISsiObj->usi_dev, ENABLE, USI_RX_DMA_ENABLE);
}

static void USISsiDmaTxIrqHandle(P_USISSI_OBJ pUSISsiObj)
{
	PGDMA_InitTypeDef GDMA_InitStruct;

	GDMA_InitStruct = &pUSISsiObj->USISsiTxGdmaInitStruct;

	/* Clear Pending ISR */
	GDMA_ClearINT(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);
	GDMA_Cmd(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, DISABLE);

	/*  TX complete callback */
	rtw_up_sema(&_SemaTxDone);

	USI_SSI_SetDmaEnable(pUSISsiObj->usi_dev, DISABLE, USI_TX_DMA_ENABLE);

	GDMA_ChnlFree(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);
}

static void USISsiSlaveWriteStreamDma(P_USISSI_OBJ pUSISsiObj, const u8* tx_buffer, u32 length)
{
	pUSISsiObj->TxLength = length;
	pUSISsiObj->TxData = (void*)tx_buffer;

	USI_SSI_TXGDMA_Init(0, &pUSISsiObj->USISsiTxGdmaInitStruct, pUSISsiObj, (IRQ_FUN)USISsiDmaTxIrqHandle, (u8*)tx_buffer, length);
	USI_SSI_SetDmaEnable(pUSISsiObj->usi_dev, ENABLE, USI_TX_DMA_ENABLE);
}

void IntercomInit(void)
{
	rtw_init_sema(&_SemaRxDone, 1);
	rtw_down_sema(&_SemaRxDone);
	rtw_init_sema(&_SemaTxDone, 1);
	rtw_down_sema(&_SemaTxDone);

	GPIO_InitTypeDef gpioDef;
	memset(&gpioDef, 0, sizeof(gpioDef));
	gpioDef.GPIO_Pin = PIN_DIR_RX;
	gpioDef.GPIO_Mode = GPIO_Mode_OUT;
	gpioDef.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(&gpioDef);
	IntercomDirRx(true);

	memset(&gpioDef, 0, sizeof(gpioDef));
	gpioDef.GPIO_Pin = PIN_EXIST_TX_DATA;
	gpioDef.GPIO_Mode = GPIO_Mode_OUT;
	gpioDef.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(&gpioDef);
	IntercomExistTxData(false);

	RCC_PeriphClockCmd(APBPeriph_USI_REG, APBPeriph_USI_CLOCK, ENABLE);
	Pinmux_Config(USI_SPI_MOSI, PINMUX_FUNCTION_SPIS);
	Pinmux_Config(USI_SPI_MISO, PINMUX_FUNCTION_SPIS);
	Pinmux_Config(USI_SPI_CS  , PINMUX_FUNCTION_SPIS);
	Pinmux_Config(USI_SPI_SCLK, PINMUX_FUNCTION_SPIS);

	PAD_PullCtrl(USI_SPI_CS  , GPIO_PuPd_UP);
	PAD_PullCtrl(USI_SPI_SCLK, GPIO_PuPd_DOWN);

	USI_SSI_InitTypeDef USI_SSI_InitStruct;
	USI_SSI_StructInit(&USI_SSI_InitStruct);
	USI_SSI_InitStruct.USI_SPI_Role = USI_SPI_SLAVE;
	USI_SSI_InitStruct.USI_SPI_SclkPhase = USI_SPI_SCPH_TOGGLES_IN_MIDDLE;
	USI_SSI_InitStruct.USI_SPI_SclkPolarity = USI_SPI_SCPOL_INACTIVE_IS_LOW;
	USI_SSI_InitStruct.USI_SPI_DataFrameSize = 8 - 1;
	USI_SSI_Init(USI0_DEV, &USI_SSI_InitStruct);

	USISsiObj.usi_dev = USI0_DEV;
}

void IntercomDirRx(bool on)
{
	GPIO_WriteBit(PIN_DIR_RX, on ? GPIO_PIN_HIGH : GPIO_PIN_LOW);
}

void IntercomExistTxData(bool on)
{
	GPIO_WriteBit(PIN_EXIST_TX_DATA, on ? GPIO_PIN_HIGH : GPIO_PIN_LOW);
}

int IntercomRx(u8* buf, u16 len)
{
	USISsiSlaveReadStreamDma(&USISsiObj, buf, len);

	DebugDout1(1);
	rtw_down_sema(&_SemaRxDone);
	DebugDout1(0);

	return len;
}

int IntercomTx(const u8* buf, u16 len)
{
	static u8* readBuf = NULL;
	static u16 readLen = 0;

	if (len > readLen)
	{
		if (readBuf != NULL) free(readBuf);

		readLen = len;
		readBuf = malloc(sizeof(u8) * readLen);
	}

	USISsiSlaveWriteStreamDma(&USISsiObj, buf, len);
	USISsiSlaveReadStreamDma(&USISsiObj, readBuf, len);

	IntercomDirRx(false);

	DebugDout1(1);
	rtw_down_sema(&_SemaTxDone);
	rtw_down_sema(&_SemaRxDone);
	DebugDout1(0);

	IntercomDirRx(true);

	return len;
}
