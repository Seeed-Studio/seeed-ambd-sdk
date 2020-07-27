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

#define min(a,b)			((a)<(b) ? (a):(b))

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

static u32 USISsiInterruptHandle(void* Adaptor)
{
	P_USISSI_OBJ usi_ssi_adapter = (P_USISSI_OBJ)Adaptor;
	u32 InterruptStatus = USI_SSI_GetIsr(usi_ssi_adapter->usi_dev);

	USI_SSI_SetIsrClean(usi_ssi_adapter->usi_dev, InterruptStatus);

	if (InterruptStatus & USI_RXFIFO_ALMOST_FULL_INTS) {
		DebugDout1(true);

		u32 TransLen = USI_SSI_ReceiveData(usi_ssi_adapter->usi_dev, usi_ssi_adapter->RxData, usi_ssi_adapter->RxLength);
		usi_ssi_adapter->RxLength -= TransLen;
		if (usi_ssi_adapter->RxData != NULL) usi_ssi_adapter->RxData = (void*)(((u8*)usi_ssi_adapter->RxData) + TransLen);

		if (usi_ssi_adapter->RxLength >= 1) {
			USI_SSI_SetRxFifoLevel(USI0_DEV, min(usi_ssi_adapter->RxLength - 1, USI_SPI_RX_FIFO_DEPTH / 2));
		}
		else {
			USI_SSI_INTConfig(usi_ssi_adapter->usi_dev, (USI_RXFIFO_ALMOST_FULL_INTR_EN | USI_RXFIFO_OVERFLOW_INTR_EN | USI_RXFIFO_UNDERFLOW_INTR_EN), DISABLE);
			rtw_up_sema(&_SemaRxDone);
		}

		DebugDout1(false);
	}

	if (InterruptStatus & USI_TXFIFO_ALMOST_EMTY_INTS) {
		u32 TransLen = USI_SSI_SendData(usi_ssi_adapter->usi_dev, usi_ssi_adapter->TxData, usi_ssi_adapter->TxLength, usi_ssi_adapter->Role);
		usi_ssi_adapter->TxLength -= TransLen;
		if (usi_ssi_adapter->TxData != NULL) usi_ssi_adapter->TxData = (void*)(((u8*)usi_ssi_adapter->TxData) + TransLen);

		if (usi_ssi_adapter->TxLength == 0) {
			USI_SSI_INTConfig(usi_ssi_adapter->usi_dev, (USI_TXFIFO_OVERFLOW_INTR_EN | USI_TXFIFO_ALMOST_EMTY_INTR_EN), DISABLE);
			rtw_up_sema(&_SemaTxDone);
		}
	}

	return 0;
}

static void USISsiSlaveReadStream(P_USISSI_OBJ pUSISsiObj, char* rx_buffer, u32 length)
{
	while (USI_SSI_Busy(pUSISsiObj->usi_dev));

	pUSISsiObj->RxLength = length;
	pUSISsiObj->RxData = rx_buffer;

	u32 TransLen = USI_SSI_ReceiveData(pUSISsiObj->usi_dev, pUSISsiObj->RxData, pUSISsiObj->RxLength);
	pUSISsiObj->RxLength -= TransLen;
	if (pUSISsiObj->RxData != NULL) pUSISsiObj->RxData = (void*)(((u8*)pUSISsiObj->RxData) + TransLen);

	if (pUSISsiObj->RxLength >= 1) {
		USI_SSI_SetRxFifoLevel(USI0_DEV, min(pUSISsiObj->RxLength - 1, USI_SPI_RX_FIFO_DEPTH / 2));
		USI_SSI_INTConfig(pUSISsiObj->usi_dev, (USI_RXFIFO_ALMOST_FULL_INTR_EN | USI_RXFIFO_OVERFLOW_INTR_EN | USI_RXFIFO_UNDERFLOW_INTR_EN), ENABLE);
	}
	else {
		rtw_up_sema(&_SemaRxDone);
	}
}

static void USISsiSlaveWriteStream(P_USISSI_OBJ pUSISsiObj, char* tx_buffer, u32 length)
{
	while (USI_SSI_Busy(pUSISsiObj->usi_dev));

	pUSISsiObj->TxLength = length;
	pUSISsiObj->TxData = tx_buffer;

	u32 TransLen = USI_SSI_SendData(pUSISsiObj->usi_dev, pUSISsiObj->TxData, pUSISsiObj->TxLength, pUSISsiObj->Role);
	pUSISsiObj->TxLength -= TransLen;
	if (pUSISsiObj->TxData != NULL) pUSISsiObj->TxData = (void*)(((u8*)pUSISsiObj->TxData) + TransLen);

	if (pUSISsiObj->TxLength >= 1) {
		USI_SSI_INTConfig(pUSISsiObj->usi_dev, (USI_TXFIFO_OVERFLOW_INTR_EN | USI_TXFIFO_ALMOST_EMTY_INTR_EN), ENABLE);
	}
	else {
		rtw_up_sema(&_SemaTxDone);
	}
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

	PAD_PullCtrl(USI_SPI_MOSI, GPIO_PuPd_NOPULL);
	PAD_PullCtrl(USI_SPI_MISO, GPIO_PuPd_NOPULL);
	PAD_PullCtrl(USI_SPI_CS  , GPIO_PuPd_UP);
	PAD_PullCtrl(USI_SPI_SCLK, GPIO_PuPd_NOPULL);

	USI_SSI_InitTypeDef USI_SSI_InitStruct;
	USI_SSI_StructInit(&USI_SSI_InitStruct);
	USI_SSI_InitStruct.USI_SPI_Role = USI_SPI_SLAVE;
	USI_SSI_InitStruct.USI_SPI_SclkPhase = USI_SPI_SCPH_TOGGLES_IN_MIDDLE;
	USI_SSI_InitStruct.USI_SPI_SclkPolarity = USI_SPI_SCPOL_INACTIVE_IS_LOW;
	USI_SSI_InitStruct.USI_SPI_DataFrameSize = 8 - 1;
	USI_SSI_Init(USI0_DEV, &USI_SSI_InitStruct);
	USI_SSI_SetTxFifoLevel(USI0_DEV, USI_SPI_TX_FIFO_DEPTH / 2);

	USISsiObj.usi_dev = USI0_DEV;
	InterruptRegister((IRQ_FUN)USISsiInterruptHandle, USI_IRQ, (u32)&USISsiObj, 10);
	InterruptEn(USI_IRQ, 10);

	DebugDoutInit();
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
	DebugDout0(true);

	USISsiSlaveReadStream(&USISsiObj, buf, len);

	rtw_down_sema(&_SemaRxDone);

	DebugDout0(false);

	return len;
}

int IntercomTx(const u8* buf, u16 len)
{
	USISsiSlaveWriteStream(&USISsiObj, buf, len);
	USISsiSlaveReadStream(&USISsiObj, NULL, len);

	IntercomDirRx(false);

	rtw_down_sema(&_SemaTxDone);
	rtw_down_sema(&_SemaRxDone);

	IntercomDirRx(true);

	return len;
}
