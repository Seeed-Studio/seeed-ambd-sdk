/******************************************************************************
 *
 * Copyright(c) 2007 - 2015 Realtek Corporation. All rights reserved.
 * Copyright(c) 2019 - 2020 Seeed Technology.    All rights reserved.
 *
 ******************************************************************************/

#include <platform_opts.h>

#if CONFIG_EXAMPLE_SPI_ATCMD

#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include "semphr.h"
#include "device.h"
#include "osdep_service.h"
#include "device_lock.h"

#include "spi_atcmd/example_spi_atcmd.h"

#include "at_cmd/log_service.h"
#include "at_cmd/atcmd_wifi.h"
#include "at_cmd/atcmd_lwip.h"

#include "flash_api.h"
#include "gpio_api.h"
#include "spi_api.h"
#include "spi_ex_api.h"
#include "usi_api.h"
#include "usi_ex_api.h"

#include "gpio_irq_api.h"
#include "gpio_irq_ex_api.h"
#include "wifi_conf.h"

#include "rtl8721d_usi_ssi.h"
#include "lwip/pbuf.h"

#include "Intercom.h"

/**** LOG SERVICE ****/
#define LOG_TX_BUFFER_SIZE	(2048)

static uint8_t spi_rx_buf[LOG_SERVICE_BUFLEN];
static struct
{
	uint32_t header;
	u8 buffer[LOG_TX_BUFFER_SIZE];
} spi_tx_buf;

static atcmd_pbuf_t at_pbufs[2];
#define ATPB_W (&at_pbufs[0])
#define ATPB_R (&at_pbufs[1])

// SPI transfer tags, also used by the master SPI device
enum
{
	SPT_TAG_PRE	= 0x55, /* Master initiate a TRANSFER */
	SPT_TAG_WR	= 0x80, /* Master WRITE  to Slave */
	SPT_TAG_RD	= 0x00, /* Master READ from Slave */
	SPT_TAG_ACK	= 0xBE, /* Slave  Acknowledgement */
	SPT_TAG_DMY	= 0xFF, /* dummy */

	SPT_ERR_OK	= 0x00,
};

////////////////////////////////////////////////////////////////////////////////
//

void spi_at_send_buf(uint8_t* buf, uint32_t size)
{
	int old_mask = 0;
	struct pbuf* pb;

	if (size >= UINT16_MAX || !(pb = pbuf_alloc(PBUF_RAW, size, PBUF_RAM))) {
		printf("L%d at tx overflow size=%d\n", __LINE__, (int)size);
		return;
	}

	pbuf_take(pb, buf, size);

	// should protect the pbuf list
	if (__get_IPSR()) {
		old_mask = taskENTER_CRITICAL_FROM_ISR();
	}
	else {
		taskENTER_CRITICAL();
	}

	if (ATPB_W->pb == NULL) {
		ATPB_W->pb = pb;
		IntercomExistTxData(true);
	}
	else {
		pbuf_cat(ATPB_W->pb, pb);
	}

	if (__get_IPSR()) {
		taskEXIT_CRITICAL_FROM_ISR(old_mask);
	}
	else {
		taskEXIT_CRITICAL();
	}
}

static void atcmd_check_special_case(char* buf)
{
	int i;

	if (strncmp(buf, "ATPT", 4) == 0) {
		for (i = 0; i < (int)strlen(buf); i++) {
			if (buf[i] == ':') {
				buf[i] = '\0';
				break;
			}
		}
	}
	else {
		/* Remove tail \r or \n */
		for (i = strlen(buf) - 1;
			i >= 0 && (buf[i] == '\r' || buf[i] == '\n');
			i--
			) {
			buf[i] = '\0';
		}
	}
}

static int spi_rx_char(int c)
{
	static uint8_t cmd_buf[ATSTRING_LEN];
	static int idx = 0;

	/* not empty line or \r\n */
	if (idx == 0) {
		if (c == '\r' || c == '\n') return 0;
	}

	/* process all \r, \n, \r\n */
	if (c == '\n') c = '\r';
	cmd_buf[idx] = c;
	if (idx < (int)(sizeof cmd_buf - 1)) {
		idx++;
	}
	else {
		printf("L%d at rx overflow\n", __LINE__);
	}

	if (idx > 1 && c == '\r') {
		cmd_buf[idx] = 0;
		strcpy(log_buf, (char*)cmd_buf);

		// Debug only
		// printf("$$$ =%s=\n", cmd_buf);

		atcmd_check_special_case(log_buf);
		rtw_up_sema(&log_rx_interrupt_sema);

		idx = 0;
	}

	return 0;
}

static int spi_rx_mux(const uint8_t* buf, int size)
{
	int n;

	if ((n = at_get_data_counter())) {
		if (n > size) {
			n = size;
		}
		at_net_store_data(buf, n);
	}

	// left bytes are commands
	for (int i = n; i < size; i++) {
		spi_rx_char(spi_rx_buf[i]);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//

static void spi_atcmd_initial(void)
{
	wifi_disable_powersave();

	IntercomInit();
}

static void spi_trx_thread(void* param)
{
	(void)param;

	union
	{
		uint8_t  v8[4];
		uint16_t v16[2];
		uint32_t v32;
	} u;

	for (;;)
	{
		/* wait SPT_TAG_PRE */
		// -------------------------------------------------------------------------------- RX
		IntercomRx(&u.v8[0], 1);
		if (u.v8[0] != SPT_TAG_PRE)
		{
			if (u.v8[0] != SPT_TAG_DMY) {
				printf("*R%02X\n", u.v8[0]);
			}
			continue;
		}

		/* wait SPT_TAG_WR/SPT_TAG_RD */
		// -------------------------------------------------------------------------------- RX
		IntercomRx(&u.v8[1], 3);
		if (u.v8[1] != SPT_TAG_RD && u.v8[1] != SPT_TAG_WR)
		{
			printf("#R%02X\n", u.v8[0]);
			continue;
		}
		const uint8_t cmd = u.v8[1];
		uint16_t len = ntohs(u.v16[1]);

		if (cmd == SPT_TAG_WR)	/* The master write to this slave */
		{
			if (len > sizeof(spi_rx_buf)) len = sizeof(spi_rx_buf);

			u.v8[0] = SPT_TAG_ACK;
			u.v8[1] = SPT_ERR_OK;
			u.v16[1] = htons(len);
			// -------------------------------------------------------------------------------- TX
			IntercomTx(u.v8, 4);

			if (len)
			{
				// -------------------------------------------------------------------------------- RX
				IntercomRx(spi_rx_buf, len);
				spi_rx_mux(spi_rx_buf, len);
			}
		}
		else if (cmd == SPT_TAG_RD)	/* The master read from this slave */
		{
			// Move the pbuf list from Writing Slot
			// to Reading Slot.
			if (ATPB_R->pb == NULL)
			{
				if (ATPB_W->pb != NULL)
				{
					taskENTER_CRITICAL();
					ATPB_R->pb = ATPB_W->pb;
					ATPB_W->pb = NULL;
					taskEXIT_CRITICAL();

					ATPB_R->iter = 0;
				}
			}

			// Preparing data & length to send.
			if (ATPB_R->pb == NULL)                            len = 0;
			else if (len > ATPB_R->pb->tot_len - ATPB_R->iter) len = ATPB_R->pb->tot_len - ATPB_R->iter;

			if (len > sizeof(spi_tx_buf.buffer)) len = sizeof(spi_tx_buf.buffer);

			if (len) pbuf_copy_partial(ATPB_R->pb, spi_tx_buf.buffer, len, ATPB_R->iter);

			u.v8[0] = SPT_TAG_ACK;
			u.v8[1] = SPT_ERR_OK;
			u.v16[1] = htons(len);

			if (!len)
			{
				// -------------------------------------------------------------------------------- TX
				IntercomTx(u.v8, 4);
			}
			else
			{
				spi_tx_buf.header = u.v32;
				// -------------------------------------------------------------------------------- TX
				IntercomTx((u8*)&spi_tx_buf, sizeof(spi_tx_buf.header) + len);

				ATPB_R->iter += len;
				if (ATPB_R->iter >= ATPB_R->pb->tot_len)
				{
					/* Free the pbuf not required anymore */
					pbuf_free(ATPB_R->pb);
					ATPB_R->pb = NULL;

					taskENTER_CRITICAL();
					if (ATPB_W->pb == NULL) IntercomExistTxData(false);
					taskEXIT_CRITICAL();
				}
			}
		}
	}

	vTaskDelete(NULL);
}

static int spi_atcmd_module_init(void)
{
	p_wlan_init_done_callback = NULL;
	atcmd_wifi_restore_from_flash();
	atcmd_lwip_restore_from_flash();
	rtw_msleep_os(20);

	spi_atcmd_initial();
	at_set_debug_mask(-1UL);

	at_printf("\r\nready\r\n");	// esp-at compatible

	if (xTaskCreate(spi_trx_thread, "spi_trx_thread", 4096, NULL, tskIDLE_PRIORITY + 6, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(spi_trx_thread) failed", __FUNCTION__);

	return 0;
}

void example_spi_atcmd(void)
{
	at_prt_lock_init();
	p_wlan_init_done_callback = spi_atcmd_module_init;
}

////////////////////////////////////////////////////////////////////////////////

#endif // CONFIG_EXAMPLE_SPI_ATCMD
