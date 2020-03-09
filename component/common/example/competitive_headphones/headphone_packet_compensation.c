
#include <platform_opts.h>
#if defined(CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES) && \
	CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES
#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include "freertos_service.h"
#include "example_competitive_headphones.h"
#include "gpio_api.h"

TimerHandle_t		packet_compensation_timer;
extern u8 recv_packet[BURST_SIZE * RECV_SP_BUF_SIZE];
extern u8 recv_packet_index[4];
extern u8 recv_packet_num;
extern u32 burststart_processed;
extern _sema packet_processed_sema ;
extern u32 burststart;
extern u32 burststart_processed;
extern int processed;

void packet_compensation_timeout_handle(void)
{
 	int i = 0;
	int temp = 0;
	int last_recv_num;

	for(i = 0; i < BURST_SIZE; i++){
		if(recv_packet_index[i] != 0){
			temp++;
			last_recv_num = i;
		}
	}

	if(temp != BURST_SIZE){
		for(i = 0; i < BURST_SIZE; i++){
			if(recv_packet_index[i] == 0){
				memcpy(&recv_packet[i * RECV_SP_BUF_SIZE], &recv_packet[last_recv_num * RECV_SP_BUF_SIZE], RECV_SP_BUF_SIZE);
			}
		}
	}

	for(i=0; i< 4; i++)
		recv_packet_index[i] = 0;
	burststart_processed = burststart;
	recv_packet_num = 0;
	processed = 0;
	xSemaphoreGive(packet_processed_sema);
}

int packet_compensation_timeout_start(int delay)
{
	int rs = FALSE;

	do {
		if (xTimerChangePeriod(packet_compensation_timer, delay / portTICK_PERIOD_MS, 500) == pdFALSE) {
			printf("Change the period failed");
			break;
		}

		// Start the timer.  No block time is specified, and even if one was
		// it would be ignored because the scheduler has not yet been
		// started.
		if (xTimerStart(packet_compensation_timer, 500) != pdPASS) {
			// The timer could not be set into the Active state.
			printf("Failed to start timer");
			break;
		}

		rs = TRUE;
	} while (0);

	return rs;
}

int packet_compensation_server_timeout_init(void)
{
	packet_compensation_timer = xTimerCreate("packet_compensation__timer",   	// Just a text name, not used by the kernel.
		(0x7fffffff),		// The timer period in ticks.
		packet_compensation_timer,	// The timers will auto-reload themselves when they expire.
		NULL,  			// Assign each timer a unique id equal to its array index.
		packet_compensation_timeout_handle	// Each timer calls the same callback when it expires.
		);
	if (packet_compensation_timer == NULL) {
		printf("Timer Create Failed!!!");
	}
}

int packet_compensation_server_timeout_deinit(void)
{
	xTimerStop(packet_compensation_timer, 5000);
	xTimerDelete(packet_compensation_timer, 5000);
}
#endif
