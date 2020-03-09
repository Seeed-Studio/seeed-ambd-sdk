#include <platform_opts.h>
#if defined(CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES) && \
	CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES
#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include "freertos_service.h"
#include "example_competitive_headphones.h"

static u32 udp_server_stop = 0;
static xSemaphoreHandle udp_server_rfoff_sema = NULL;
static u32 rf_status = 1;
static u32 burststart_g = 0;
static u32 burstperiod_g = 0;

TimerHandle_t		recv_timeout_timer;

static void udp_server_timeout_handler(void)
{
	portBASE_TYPE taskWoken = pdFALSE;

#ifdef RFOFF_USER_TIMER
	if (rf_status == 1) {
		//u32 current_tsf = HAL_READ32(WIFI_REG_BASE, 0x560);
		u32 current_tsf = wifi_get_tsf_low(0);
		int offtime = burstperiod_g - (current_tsf - burststart_g) / 1000; //us->ms

		if (offtime > MIN_RFOFF_TIME) {
			wifi_rf_contrl(OFF);
			rf_status = 0;
			udp_server_timeout_start(offtime);
		}
	} else {
		wifi_rf_contrl(ON);
		rf_status = 1;
	}
#else
	xSemaphoreGiveFromISR(udp_server_rfoff_sema, &taskWoken);
	portEND_SWITCHING_ISR(taskWoken);
#endif
}

int udp_server_timeout_start(size_t delay)
{
	int rs = FALSE;

	do {
		if (xTimerChangePeriod(recv_timeout_timer, delay / portTICK_PERIOD_MS, 500) == pdFALSE) {
			DBG_8195A("Change the period failed");
			break;
		}

		// Start the timer.  No block time is specified, and even if one was
		// it would be ignored because the scheduler has not yet been
		// started.
		if (xTimerStart(recv_timeout_timer, 500) != pdPASS) {
			// The timer could not be set into the Active state.
			DBG_8195A("Failed to start timer");
			break;
		}

		rs = TRUE;
	} while (0);

	return rs;
}

void udp_server_rfoff_thread(void)
{
	u32 current_tsf = 0;
	int offtime = 0; //us->ms

    do{
        xSemaphoreTake(udp_server_rfoff_sema, 0xFFFFFFFF);

		if (udp_server_stop) {
			break;
		}
		
		//current_tsf = HAL_READ32(WIFI_REG_BASE, 0x560);
		current_tsf = wifi_get_tsf_low(0);
		offtime = burstperiod_g - (current_tsf - burststart_g) / 1000; //us->ms

		if (offtime > MIN_RFOFF_TIME) {
			wifi_rf_contrl(OFF);
			vTaskDelay(offtime);
			wifi_rf_contrl(ON);
		}

    }while(1);    
}

int udp_server_timeout_init(void)
{
	recv_timeout_timer = xTimerCreate("recv_timeout_timer",   	// Just a text name, not used by the kernel.
		(0x7fffffff),		// The timer period in ticks.
		recv_timeout_timer,	// The timers will auto-reload themselves when they expire.
		NULL,  			// Assign each timer a unique id equal to its array index.
		udp_server_timeout_handler	// Each timer calls the same callback when it expires.
		);
	
	if (recv_timeout_timer == NULL) {
		DBG_8195A("Timer Create Failed!!!");
		//assert_param(0);
	}

	udp_server_stop = 0;
	vSemaphoreCreateBinary(udp_server_rfoff_sema);
	xSemaphoreTake(udp_server_rfoff_sema, 1/portTICK_RATE_MS);

#ifndef RFOFF_USER_TIMER
	if (pdTRUE != xTaskCreate(udp_server_rfoff_thread, (const char * const)"udp_server_rfoff", 64, 
		NULL, tskIDLE_PRIORITY + 6, NULL))
	{
		DBG_8195A("Create udp_server_rfoff Err!!\n");
	}
#endif
}

int udp_server_timeout_deinit(void)
{
	xTimerStop(recv_timeout_timer, 5000);
	xTimerDelete(recv_timeout_timer, 5000);

	udp_server_stop = 1;
	xSemaphoreGive(udp_server_rfoff_sema);
	vSemaphoreDelete(udp_server_rfoff_sema);
}

void udp_server_burststop(u32 dataseq, u32 burststart)
{
	u32 burstsize = 0;
	int offtime = 0;
	u32 current_time = 0;
	u32 burstperiod = 0;
	u32 lastpktcheck = 0;
	
	burstsize = BURST_SIZE;
	burstperiod = BURST_PERIOD;

	/* burst stop */
	lastpktcheck = burstsize - dataseq;

	/* last 1 packet recvd */
	if (lastpktcheck == 1) {
		current_time = wifi_get_tsf_low(0);
		if(current_time < burststart) {
			//DBG_8195A("timer_sync ing no tdma\n");
			return -1;
		}
		offtime = burstperiod - (current_time - burststart) / 1000; //us->ms
		//DBG_8195A("R:%d (%d)\n", offtime, client_hdr.burstindex);
		if (offtime > MIN_RFOFF_TIME) {			
			xTimerStop(recv_timeout_timer, 5000);
			wifi_rf_contrl(OFF);
			vTaskDelay(offtime);
			wifi_rf_contrl(ON);
		}
	}

	/* last 2 packet, if last packet lost */
	if ((lastpktcheck == 2) || (lastpktcheck == 3)) {
		burststart_g = burststart;
		burstperiod_g = burstperiod;
		udp_server_timeout_start(5);
	}
	
	return 0;

}
#endif

