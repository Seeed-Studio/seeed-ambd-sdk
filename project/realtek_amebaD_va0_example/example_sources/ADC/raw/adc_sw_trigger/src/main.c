/*
 *  Routines to access hardware
 *
 *  Copyright (c) 2013 Realtek Semiconductor Corp.
 *
 *  This module is a confidential and proprietary property of RealTek and
 *  possession or use of this module requires written permission of RealTek.
 */

#include "device.h"
#include "analogin_api.h"
#include <sys_api.h>

/*
 * OFFSET:   10 times of sample data at 0.000v, 10*value(0.000v)
 * GAIN_DIV: 10 times of value(1.000v)-value(0.000v) or value(2.000v)-value(1.000v) or value(3.000v)-value(2.000v)
 */
/* Normal channel*/
u16 OFFSET;
u16 GAIN_DIV;
#define AD2MV(ad,offset,gain) ((10 * ad - offset) * 1000 / gain)

VOID 
main (
    VOID
)
{
	u8 EfuseBuf[2];
	u8 IC_Type;
	u32 index, Temp;
	u32 addressOffset = 0x1D0;
	u32 addressGain = 0x1D2;
	u32 IC_Type_Address = 0x1F0;

	u16 value, data, ch_idx;
	int vol;

	EFUSE_PMAP_READ8(0, IC_Type_Address, &IC_Type, L25EOUTVOLTAGE);
	IC_Type &= (BIT(1) | BIT(0));
//	DBG_8195A("IC_Type = %x\n", IC_Type);

	if ( IC_Type == 0x2) {
		/*Write reg 0x4801C404[7:6] with 0'b 00*/
		Temp = HAL_READ32(CTC_REG_BASE, 0x404);
		Temp &= ~(BIT(6) | BIT(7));
		HAL_WRITE32(CTC_REG_BASE, 0x404, Temp);

		/*Write reg 0x4801C400[10:8] with 0'b 101*/
		Temp = HAL_READ32(CTC_REG_BASE, 0x400);
		Temp &= ~(BIT(9));
		Temp |= (BIT(8) | BIT(10));
		HAL_WRITE32(CTC_REG_BASE, 0x400, Temp);
	} else {
		u32 Temp;
		/*Write reg 0x4801C404[7:6]  with 0'b 00*/
		Temp = HAL_READ32(CTC_REG_BASE, 0x404);
		Temp &= ~(BIT(6) | BIT(7));
		HAL_WRITE32(CTC_REG_BASE, 0x404, Temp);

		/*Write reg 0x4801C400[10:8] with 0'b 100*/
		Temp = HAL_READ32(CTC_REG_BASE, 0x400);
		Temp &= ~(BIT(8) | BIT(9));
		Temp |= BIT(10);
		HAL_WRITE32(CTC_REG_BASE, 0x400, Temp);
	}

//	Temp = HAL_READ32(CTC_REG_BASE, 0x404);
//	DBG_8195A("REG1X_LPAD = %x\n", Temp);
//	Temp = HAL_READ32(CTC_REG_BASE, 0x400);
//	DBG_8195A("REG0X_LPAD = %x\n", Temp);


	for (index = 0; index< 2; index++) {
		EFUSERead8(0, addressOffset+index, EfuseBuf + index, L25EOUTVOLTAGE);
	}
	OFFSET = EfuseBuf[1]<<8|EfuseBuf[0];

	for (index = 0; index< 2; index++) {
		EFUSERead8(0, addressGain+index, EfuseBuf + index, L25EOUTVOLTAGE);
	}
	GAIN_DIV = EfuseBuf[1]<<8|EfuseBuf[0];

	if (OFFSET == 0xFFFF) {
		OFFSET = 0x9B0;
		DBG_8195A("Default OFFSET!\n");
	}

	if (GAIN_DIV == 0xFFFF) {
		GAIN_DIV = 0x2F12;
		DBG_8195A("Default GAIN_DIV!\n");
	}

	DBG_8195A("OFFSET = %x\n", OFFSET);
	DBG_8195A("GAIN_DIV = %x\n", GAIN_DIV);

	ADC_InitTypeDef ADC_InitStruct; 
	
	/* Initialize ADC to Software-trigger mode */
	ADC_StructInit(&ADC_InitStruct);
	ADC_InitStruct.ADC_OpMode = ADC_SW_TRI_MODE;
	ADC_InitStruct.ADC_CvlistLen = 1; 	/* ADC_CvlistLen should be channel number -1*/
	ADC_InitStruct.ADC_Cvlist[0] = ADC_CH1;
	ADC_InitStruct.ADC_Cvlist[1] = ADC_CH3;
	ADC_InitStruct.ADC_ChIDEn = ENABLE; /* MSB 4bit is channel index*/
	ADC_Init(&ADC_InitStruct);

	ADC_Cmd(ENABLE);

	while (1) {
		/* SW trigger to switch channel and sample */
		ADC_SWTrigCmd(ENABLE);
		while(ADC_Readable()== 0);
		ADC_SWTrigCmd(DISABLE);
		
		value = ADC_Read();

		ch_idx = (value & BIT_MASK_DAT_CHID) >> 12;
		data = value & BIT_MASK_DAT_GLOBAL;
		vol = AD2MV(data, OFFSET, GAIN_DIV);

		DBG_8195A("AD%d:%x = %d mv\n", ch_idx, data, vol);

		DelayMs(500);
	}

}
