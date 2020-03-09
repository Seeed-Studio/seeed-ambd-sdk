/******************************************************************************
  *
  * This module is a confidential and proprietary property of RealTek and
  * possession or use of this module requires written permission of RealTek.
  *
  * Copyright(c) 2016, Realtek Semiconductor Corporation. All rights reserved. 
  *
******************************************************************************/
#include <platform_opts.h>
#if defined(CONFIG_EXAMPLE_SW_PTA) && CONFIG_EXAMPLE_SW_PTA
#include "device.h"
#include "sw_pta_api.h"
#include <wifi/wifi_conf.h>

#define SW_PTA_PIN_REQ	_PA_26
#define SW_PTA_PIN_PRI	_PB_1
#define SW_PTA_PIN_GNT	_PA_27
#define GRANT	0
#define NO_GRANT	1

#define LOOP_TEST	0//use 2 internal gpio to simulate external device's request and prority pin
#define TEST_CMD	0//may be used during test period

#if LOOP_TEST
	#define SW_PTA_TEST_REQ		_PB_23
	#define SW_PTA_TEST_PRI		_PB_22
#endif

#if TEST_CMD
#include "log_service.h"
	u8 sw_pta_state = 0;
#endif

IMAGE2_RAM_TEXT_SECTION
void sw_pta_req_handler (uint32_t id, u32 event)
{
	static u8 backup_internal_info1 = 0;
	static u8 backup_internal_info2 = 0;
	u32 high_pri = 0;
	rtw_coex_state_bt_t bt_state = 0;
	rtw_coex_state_wifi_t wifi_state = 0;
	static u8 interval_dev_stopped = 0;
	u8 dev_flag = 0;

	if((!wifi_is_up(RTW_STA_INTERFACE)) || sw_pta_wifi_is_ips()){
		GPIO_WriteBit(SW_PTA_PIN_GNT, GRANT);//assert grant to zigbee
		interval_dev_stopped = 0;
		return;
	}
	if ((event & 0x0000ffff) == HAL_IRQ_RISE){//external request on
		high_pri = GPIO_ReadDataBit(SW_PTA_PIN_PRI);
		wifi_coex_get_internal_dev_state(&wifi_state, &bt_state);
		sw_pta_get_internal_dev_flag(&dev_flag);
		if (!interval_dev_stopped &&
			//add adjudicate logic here,
			//for example: assert grant when zigbee pri high or wifi/bt flag both 0 
			(high_pri 
			|| (!(dev_flag&DEV_FLAG_WIFI_RX_BEACON) && !(dev_flag&DEV_FLAG_BT_ACTIVE)))
			&& (wifi_state != COEX_STATE_WIFI_STA_SCAN && wifi_state != COEX_STATE_WIFI_STA_CONNECTING)
			){
				if(sw_pta_internal_dev_stop(&backup_internal_info1, &backup_internal_info2)){
					interval_dev_stopped = 1;
					GPIO_WriteBit(SW_PTA_PIN_GNT, GRANT);
				}
		}
	} else if((event & 0x0000ffff) == HAL_IRQ_FALL){////external request off
		if (interval_dev_stopped){
			sw_pta_internal_dev_resume(&backup_internal_info1, &backup_internal_info2);
			interval_dev_stopped = 0;
			GPIO_WriteBit(SW_PTA_PIN_GNT, NO_GRANT);
		}
#if TEST_CMD
		if(sw_pta_state == 0){
			GPIO_INTConfig(SW_PTA_PIN_REQ, DISABLE);
			GPIO_WriteBit(SW_PTA_PIN_GNT, NO_GRANT);
		}
#endif
	}	
}

void sw_pta_port_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct_GNT;
	GPIO_InitTypeDef GPIO_InitStruct_REQ;
	GPIO_InitTypeDef GPIO_InitStruct_PRI;
	u32 port_num; 
	u32 val32;

	// Init PRI pin
	GPIO_InitStruct_GNT.GPIO_Pin = SW_PTA_PIN_PRI;
	GPIO_InitStruct_GNT.GPIO_Mode = GPIO_Mode_IN;
	GPIO_Init(&GPIO_InitStruct_GNT);
	
	// Init GNT pin
	if (SW_PTA_PIN_GNT == _PA_27){//pa27 is default used for swd
		val32 = HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_SWD_PMUX_EN);
		val32 &= (~BIT_LSYS_SWD_PMUX_EN);
		HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_SWD_PMUX_EN, val32);
		val32 = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SWD_PMUX_EN);	
		val32 &= (~BIT_LSYS_SWD_PMUX_EN);	
		HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SWD_PMUX_EN, val32);
	}
	
	GPIO_InitStruct_GNT.GPIO_Pin = SW_PTA_PIN_GNT;
	GPIO_InitStruct_GNT.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(&GPIO_InitStruct_GNT);
	GPIO_WriteBit(SW_PTA_PIN_GNT, NO_GRANT);

	// Init REQ pin
	GPIO_INTConfig(SW_PTA_PIN_REQ, DISABLE);
	port_num = PORT_NUM(SW_PTA_PIN_REQ);
	
	GPIO_InitStruct_REQ.GPIO_Pin = SW_PTA_PIN_REQ;
	GPIO_InitStruct_REQ.GPIO_Mode = GPIO_Mode_INT;
	GPIO_InitStruct_REQ.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStruct_REQ.GPIO_ITTrigger = GPIO_INT_Trigger_BOTHEDGE;
	GPIO_InitStruct_REQ.GPIO_ITDebounce = GPIO_INT_DEBOUNCE_DISABLE;

	if (port_num == GPIO_PORT_A) {
		InterruptRegister((IRQ_FUN)GPIO_INTHandler, GPIOA_IRQ, (u32)GPIOA_BASE, 10);		
		InterruptEn(GPIOA_IRQ, 10);
	} else if (port_num == GPIO_PORT_B) {
		InterruptRegister((IRQ_FUN)GPIO_INTHandler, GPIOB_IRQ, (u32)GPIOB_BASE, 10);		
		InterruptEn(GPIOB_IRQ, 10);
	}
	
	GPIO_Init(&GPIO_InitStruct_REQ);
	GPIO_UserRegIrq(SW_PTA_PIN_REQ, (void *)sw_pta_req_handler, &GPIO_InitStruct_REQ);
	GPIO_INTConfig(SW_PTA_PIN_REQ, ENABLE);
#if TEST_CMD
	sw_pta_state = 1;
#endif
	
}

#if LOOP_TEST
void sw_pta_loop_test_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct_Test;

	GPIO_InitStruct_Test.GPIO_Pin = SW_PTA_TEST_PRI;
	GPIO_InitStruct_Test.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(&GPIO_InitStruct_Test);
	GPIO_WriteBit(SW_PTA_TEST_PRI, 0);

	GPIO_InitStruct_Test.GPIO_Pin = SW_PTA_TEST_REQ;
	GPIO_InitStruct_Test.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_Init(&GPIO_InitStruct_Test);
	GPIO_WriteBit(SW_PTA_TEST_REQ, 0);
}

void sw_pta_loop_test(void)
{
	GPIO_WriteBit(SW_PTA_TEST_PRI, 1);
	GPIO_WriteBit(SW_PTA_TEST_REQ, 1);
	DelayMs(2);
	GPIO_WriteBit(SW_PTA_TEST_PRI, 0);
	GPIO_WriteBit(SW_PTA_TEST_REQ, 0);
}
#endif

#if TEST_CMD
void	sw_pta_enable(u8 enable)
{
	if(enable){
		sw_pta_state = 1;
		GPIO_INTConfig(SW_PTA_PIN_REQ, ENABLE);
	} else{
		sw_pta_state = 0;
	}
}

u8 sw_pta_get_state(void)
{
	printf("sw pta state: %s",sw_pta_state? "enable" : "disable");
	return sw_pta_state;
}

void fATPe(void *arg)
{
	u8 enable = 0;

	if(!arg){
		printf("[ATPe]Usage: \n\r");
		printf("             ATPe=0   disable sw pta\n\r");
		printf("             ATPe=1   enable sw pta\n\r");
		return;
	}
	enable = (unsigned char) atoi((const char *)arg);
	printf("[ATPe]:Set SW PTA [%s]\n\r", enable? "enable" : "disable");
	sw_pta_enable(enable);

	return;
}

void fATPg(void *arg)
{
	sw_pta_get_state();
	return;
}

#if LOOP_TEST
void fATPt(void *arg)
{
	sw_pta_loop_test();
	printf("sw pta loop test\n");
	return;
}
#endif

extern int rltk_set_tx_power_percentage(rtw_tx_pwr_percentage_t power_percentage_idx);
void fATPp(void *arg)
{
	if(!arg){
		printf("[ATPp]Usage: set percentage of tx power\n\r");
		printf("             0   100%%\n\r");
		printf("             1   75%%\n\r");
		printf("             2   50%%\n\r");
		printf("             3   25%%\n\r");
		printf("             4   12.5%%\n\r");
		return;
	}
	u8 percentage = (unsigned char) atoi((const char *)arg);
	rltk_set_tx_power_percentage(percentage);
}
log_item_t at_sw_pta_items[ ] = {
	{"ATPe", fATPe,{NULL,NULL}},
	{"ATP?", fATPg,{NULL,NULL}},
	{"ATPt", fATPt,{NULL,NULL}},
	{"ATPp", fATPp,{NULL,NULL}},
};
#endif

//call this function after wifi init done, such as at the last of init_thread()
void example_sw_pta_init(void)
{
	sw_pta_port_init();
	
#if TEST_CMD	
	log_service_add_table(at_sw_pta_items, sizeof(at_sw_pta_items)/sizeof(at_sw_pta_items[0]));
#endif
#if LOOP_TEST
	sw_pta_loop_test_init();
#endif
	vTaskDelete(NULL);
}

void example_sw_pta(void)
{    
    if(xTaskCreate(example_sw_pta_init, ((const char*)"example_sw_pta_init"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
        printf("\n\r%s xTaskCreate(example_sw_pta_init) failed", __FUNCTION__);
}
#endif
