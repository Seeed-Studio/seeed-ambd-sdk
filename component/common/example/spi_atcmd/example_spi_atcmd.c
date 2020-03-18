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
#include "atcmd_wifi.h"
#include "atcmd_lwip.h"

#if defined(IS_USI_SPI) && IS_USI_SPI
	#define USE_USI_SPI_SLAVE  1
	#include "rtl8721d_usi_ssi.h"
	#include "lwip/pbuf.h"
#else
	#define USE_USI_SPI_SLAVE  0
#endif

/**** SPI FUNCTIONS ****/
static spi_t spi_obj;
gpio_t gpio_cs;

#define SPI_RX_BUFFER_SIZE ATSTRING_LEN/2
#define SPI_TX_BUFFER_SIZE ATSTRING_LEN/2
uint16_t spi_chunk_buffer[ATSTRING_LEN / 2];

static _sema rx_done_sema;
static _sema tx_done_sema;

/**** SLAVE HARDWARE READY ****/
gpio_t gpio_hrdy;

#define SPI_SLAVE_BUSY 0
/* On slave side code, we haven't data to send, should named IDLE not BUSY. */
#define SPI_SLAVE_IDLE  0
#define SPI_SLAVE_READY 1

volatile int spi_slave_status = SPI_SLAVE_BUSY;
_sema spi_check_hrdy_sema;

volatile int hrdy_pull_down_counter = 0;

/**** SLAVE SYNC ****/
gpio_irq_t gpio_sync;

#define SPI_STATE_MISO 0
#define SPI_STATE_MOSI 1
int spi_state = SPI_STATE_MISO;

/**** TASK THREAD ****/
_sema spi_check_trx_sema;

/**** LOG SERVICE ****/
#define LOG_TX_BUFFER_SIZE 1024
u8 log_tx_buffer[LOG_TX_BUFFER_SIZE];
volatile uint32_t log_tx_tail = 0;

/**** DATA FORMAT ****/
#define PREAMBLE_COMMAND     0x6000
#define PREAMBLE_DATA_READ   0x1000
#define PREAMBLE_DATA_WRITE  0x0000

#define COMMAND_DUMMY             0x0000
#define COMMAND_BEGIN             0x0304
#define COMMAND_END               0x0305
#define COMMAND_READ_BEGIN        0x0012
#define COMMAND_READ_RAW          0x0013
#define COMMAND_WRITE_BEGIN       0x0014
#define COMMAND_READ_WRITE_END    0x0015

void atcmd_update_partition_info(AT_PARTITION id, AT_PARTITION_OP ops, u8 * data, u16 len)
{
	flash_t flash;
	int size, offset, i;
	u32 read_data;

	switch (id) {
	case AT_PARTITION_SPI:
		size = SPI_CONF_DATA_SIZE;
		offset = SPI_CONF_DATA_OFFSET;
		break;
	case AT_PARTITION_WIFI:
		size = WIFI_CONF_DATA_SIZE;
		offset = WIFI_CONF_DATA_OFFSET;
		break;
	case AT_PARTITION_LWIP:
		size = LWIP_CONF_DATA_SIZE;
		offset = LWIP_CONF_DATA_OFFSET;
		break;
	case AT_PARTITION_ALL:
		size = 0x1000;
		offset = 0;
		break;
	default:
		printf("partition id is invalid!\r\n");
		return;
	}

	device_mutex_lock(RT_DEV_LOCK_FLASH);

	if (id == AT_PARTITION_ALL && ops == AT_PARTITION_ERASE) {
		flash_erase_sector(&flash, SPI_SETTING_SECTOR);
		goto exit;
	}

	if (ops == AT_PARTITION_READ) {
		flash_stream_read(&flash, SPI_SETTING_SECTOR + offset, len, data);
		goto exit;
	}
	//erase BACKUP_SECTOR
	flash_erase_sector(&flash, SPI_SETTING_BACKUP_SECTOR);

	if (ops == AT_PARTITION_WRITE) {
		// backup new data
		flash_stream_write(&flash, SPI_SETTING_BACKUP_SECTOR + offset, len, data);
	}
	//backup front data to backup sector
	for (i = 0; i < offset; i += sizeof(read_data)) {
		flash_read_word(&flash, SPI_SETTING_SECTOR + i, &read_data);
		flash_write_word(&flash, SPI_SETTING_BACKUP_SECTOR + i, read_data);
	}

	//backup rear data
	for (i = (offset + size); i < 0x1000; i += sizeof(read_data)) {
		flash_read_word(&flash, SPI_SETTING_SECTOR + i, &read_data);
		flash_write_word(&flash, SPI_SETTING_BACKUP_SECTOR + i, read_data);
	}

	//erase UART_SETTING_SECTOR
	flash_erase_sector(&flash, SPI_SETTING_SECTOR);

	//retore data to UART_SETTING_SECTOR from UART_SETTING_BACKUP_SECTOR
	for (i = 0; i < 0x1000; i += sizeof(read_data)) {
		flash_read_word(&flash, SPI_SETTING_BACKUP_SECTOR + i, &read_data);
		flash_write_word(&flash, SPI_SETTING_SECTOR + i, read_data);
	}

	//erase BACKUP_SECTOR
	flash_erase_sector(&flash, SPI_SETTING_BACKUP_SECTOR);

      exit:
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	return;
}

#if ! USE_USI_SPI_SLAVE
/* AT cmd V2 API */
void spi_at_send_buf(u8 * buf, u32 len)
{
	int i;

	if (!len || (!buf)) {
		return;
	}

	// debug_buf("F", (char*)buf, len);

	if (buf == log_tx_buffer) {
		log_tx_tail = len;
	} else {
		for (i = 0; i < (int) len; i++) {
			if (log_tx_tail == LOG_TX_BUFFER_SIZE) {
				printf("L%d at tx overflow\n", __LINE__);
				break;
			}
			log_tx_buffer[log_tx_tail++] = buf[i];
		}
	}

	if (__get_IPSR() != 0) {
		rtw_up_sema(&spi_check_trx_sema);
	} else {
		rtw_up_sema_from_isr(&spi_check_trx_sema);
	}
}

/* IRQ handler called when SPI TX/RX finish */
void master_trx_done_callback(void *pdata, SpiIrq event)
{
	(void) pdata;
	switch (event) {
	case SpiRxIrq:
		rtw_up_sema_from_isr(&rx_done_sema);
		//DBG_8195A("Master RX done!\n");
		break;
	case SpiTxIrq:
		//DBG_8195A("Master TX done!\n");
		break;
	default:
		DBG_8195A("unknown interrput evnent!\n");
	}
}

/* IRQ handler called when SPI TX finish */
static void master_tx_done_callback(uint32_t id)
{
	(void) id;
	rtw_up_sema_from_isr(&tx_done_sema);
}

/* IRQ handler as gpio hrdy hit rising edge */
void slave_hrdy_change_callback(uint32_t id)
{
	(void) id;
	gpio_irq_disable(&gpio_hrdy);

	if (spi_slave_status == SPI_SLAVE_BUSY) {
		// Transition from LOW to HIGH. Change to listen IRQ_LOW
		spi_slave_status = SPI_SLAVE_READY;
		hrdy_pull_down_counter++;

		gpio_irq_set(&gpio_hrdy, IRQ_LOW, 1);
		gpio_irq_enable(&gpio_hrdy);
	} else {
		// Transition from HIGH to LOW. Change to listen IRQ_HIGH
		spi_slave_status = SPI_SLAVE_BUSY;

		gpio_irq_set(&gpio_hrdy, IRQ_HIGH, 1);
		gpio_irq_enable(&gpio_hrdy);
	}

	rtw_up_sema_from_isr(&spi_check_hrdy_sema);
}

/* IRQ Handler as gpio sync state change */
void slave_sync_chagne_callback(uint32_t id)
{
	(void) id;
	gpio_irq_disable(&gpio_sync);

	if (spi_state == SPI_STATE_MISO) {
		// Transition from LOW to HIGH. Change to listen IRQ_LOW
		spi_state = SPI_STATE_MOSI;

		gpio_irq_set(&gpio_sync, IRQ_LOW, 1);
		gpio_irq_enable(&gpio_sync);
	} else {
		// Transition from HIGH to LOW. Change to listen IRQ_HIGH
		spi_state = SPI_STATE_MISO;

		gpio_irq_set(&gpio_sync, IRQ_HIGH, 1);
		gpio_irq_enable(&gpio_sync);
	}

	rtw_up_sema_from_isr(&spi_check_trx_sema);
}

static void spi_atcmd_initial(void)
{
	wifi_disable_powersave();

	// read settings
	SPI_LOG_CONF spiconf;
	spiconf.bits = 16;
	spiconf.frequency = 20000000;
	spiconf.mode = (SPI_SCLK_IDLE_LOW | SPI_SCLK_TOGGLE_MIDDLE);

	// init spi
	spi_init(&spi_obj, SPI0_MOSI, SPI0_MISO, SPI0_SCLK, SPI0_CS);
	spi_frequency(&spi_obj, spiconf.frequency);
	spi_format(&spi_obj, spiconf.bits, spiconf.mode, 0);

	spi_bus_tx_done_irq_hook(&spi_obj, (spi_irq_handler) master_tx_done_callback, (uint32_t) & spi_obj);
	spi_irq_hook(&spi_obj, (spi_irq_handler) master_trx_done_callback, (uint32_t) & spi_obj);

	// init simulated spi cs
	gpio_init(&gpio_cs, GPIO_CS);
	gpio_dir(&gpio_cs, PIN_OUTPUT);
	gpio_mode(&gpio_cs, PullNone);
	gpio_write(&gpio_cs, 1);

	// init gpio for check if spi slave hw ready
	gpio_init(&gpio_hrdy, GPIO_HRDY);
	gpio_dir(&gpio_hrdy, PIN_INPUT);
	gpio_mode(&gpio_hrdy, PullDown);

	gpio_irq_init(&gpio_hrdy, GPIO_HRDY, (gpio_irq_handler) slave_hrdy_change_callback, (uint32_t) & gpio_hrdy);
	gpio_irq_set(&gpio_hrdy, IRQ_HIGH, 1);
	gpio_irq_enable(&gpio_hrdy);

	// init gpio for check if spi slave want to send data
	gpio_irq_init(&gpio_sync, GPIO_SYNC, (gpio_irq_handler) slave_sync_chagne_callback, (uint32_t) & gpio_sync);
	gpio_irq_set(&gpio_sync, IRQ_HIGH, 1);
	gpio_irq_enable(&gpio_sync);

	// init semaphore for check hardware ready
	rtw_init_sema(&spi_check_hrdy_sema, 1);
	rtw_down_sema(&spi_check_hrdy_sema);

	// init semaphore that makes spi tx/rx thread to check something
	rtw_init_sema(&spi_check_trx_sema, 1);
	rtw_down_sema(&spi_check_trx_sema);

	// init semaphore for master tx
	rtw_init_sema(&tx_done_sema, 1);
	rtw_down_sema(&tx_done_sema);

	// init semaphore for master rx
	rtw_init_sema(&rx_done_sema, 1);
	rtw_down_sema(&rx_done_sema);
}

int32_t spi_master_send(spi_t * obj, char *tx_buffer, uint32_t length)
{
	hrdy_pull_down_counter = 0;
	spi_master_write_stream_dma(obj, tx_buffer, length);
	rtw_down_sema(&tx_done_sema);

	if (spi_slave_status == SPI_SLAVE_BUSY) {
		while (hrdy_pull_down_counter == 0);
		hrdy_pull_down_counter = 0;
	}
	return 0;
}

int32_t spi_master_recv(spi_t * obj, char *rx_buffer, uint32_t length)
{
	hrdy_pull_down_counter = 0;
	spi_flush_rx_fifo(obj);
	spi_master_read_stream_dma(obj, rx_buffer, length);
	rtw_down_sema(&rx_done_sema);
	rtw_down_sema(&tx_done_sema);

	if (spi_slave_status == SPI_SLAVE_BUSY) {
		while (hrdy_pull_down_counter == 0);
		hrdy_pull_down_counter = 0;
	}
	return 0;
}
#endif//!USE_USI_SPI_SLAVE

void atcmd_check_special_case(char *buf)
{
	int i;

	if (strncmp(buf, "ATPT", 4) == 0) {
		for (i = 0; i < (int) strlen(buf); i++) {
			if (buf[i] == ':') {
				buf[i] = '\0';
				break;
			}
		}
	} else {
		/* Remove tail \r or \n */
		for (i = strlen(buf) - 1;
		     i >= 0 && (buf[i] == '\r' || buf[i] == '\n');
		     i--
		) {
			buf[i] = '\0';
		}
	}
}

#if !USE_USI_SPI_SLAVE
static void spi_trx_thread(void *param)
{
	uint32_t rxlen, txlen;
	uint32_t recv_len, send_len;
	uint16_t dummy, L_address, H_address, L_size, H_size;

	(void) dummy;
	(void) param;

	int slave_ready = 0;
	do {
		slave_ready = gpio_read(&gpio_hrdy);
		rtw_msleep_os(1000);
	} while (slave_ready == 0);

	while (1) {
		rtw_down_sema(&spi_check_trx_sema);

		if (spi_state == SPI_STATE_MOSI) {
			if (log_tx_tail > 0) {
				/* Slave hw is ready, and Master has something to send. */

				// stage A, read target address
				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_COMMAND;
				spi_chunk_buffer[txlen++] = COMMAND_BEGIN;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				gpio_write(&gpio_cs, 0);
				spi_chunk_buffer[0] = PREAMBLE_DATA_READ;
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, 1 * 2);
				spi_master_recv(&spi_obj, (char *) spi_chunk_buffer, 1 * 2);
				dummy = spi_chunk_buffer[0];
				spi_master_recv(&spi_obj, (char *) spi_chunk_buffer, 1 * 2);
				L_address = spi_chunk_buffer[0];
				spi_master_recv(&spi_obj, (char *) spi_chunk_buffer, 1 * 2);
				H_address = spi_chunk_buffer[0];
				spi_master_recv(&spi_obj, (char *) spi_chunk_buffer, 1 * 2);
				L_size = spi_chunk_buffer[0];
				spi_master_recv(&spi_obj, (char *) spi_chunk_buffer, 1 * 2);
				H_size = spi_chunk_buffer[0];
				gpio_write(&gpio_cs, 1);

				// stage B, write data
				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_COMMAND;
				spi_chunk_buffer[txlen++] = COMMAND_WRITE_BEGIN;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				if (log_tx_tail % 2 != 0) {
					log_tx_buffer[log_tx_tail++] = 0;
				}
				send_len = log_tx_tail / 2;
				L_size = send_len & 0x0000FFFF;
				H_size = (send_len & 0xFFFF0000) >> 16;

				gpio_write(&gpio_cs, 0);
				txlen = 1;
				spi_chunk_buffer[0] = PREAMBLE_DATA_WRITE;
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				spi_chunk_buffer[0] = L_address;
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				spi_chunk_buffer[0] = H_address;
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				spi_chunk_buffer[0] = L_size;
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				spi_chunk_buffer[0] = H_size;
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				gpio_write(&gpio_cs, 0);
				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_DATA_WRITE;
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				txlen = log_tx_tail / 2;
				spi_master_send(&spi_obj, (char *) log_tx_buffer, txlen * 2);	// sending raw data
				gpio_write(&gpio_cs, 1);

				// stage C, write data end
				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_COMMAND;
				spi_chunk_buffer[txlen++] = COMMAND_READ_WRITE_END;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				// stage final
				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_COMMAND;
				spi_chunk_buffer[txlen++] = COMMAND_END;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				L_size = log_tx_tail & 0x0000FFFF;
				H_size = (log_tx_tail & 0xFFFF0000) >> 16;

				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_DATA_WRITE;
				spi_chunk_buffer[txlen++] = L_size;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_DATA_WRITE;
				spi_chunk_buffer[txlen++] = H_size;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				// finalize
				log_tx_tail = 0;
			}

		} else if (spi_state == SPI_STATE_MISO) {
			/* Slave hw is ready, and Slave want to send something. */
			do {
				// stage A, read target address
				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_COMMAND;
				spi_chunk_buffer[txlen++] = COMMAND_BEGIN;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_DATA_READ;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				spi_master_recv(&spi_obj, (char *) spi_chunk_buffer, 1 * 2);
				dummy = spi_chunk_buffer[0];
				spi_master_recv(&spi_obj, (char *) spi_chunk_buffer, 1 * 2);
				L_address = spi_chunk_buffer[0];
				spi_master_recv(&spi_obj, (char *) spi_chunk_buffer, 1 * 2);
				H_address = spi_chunk_buffer[0];
				spi_master_recv(&spi_obj, (char *) spi_chunk_buffer, 1 * 2);
				L_size = spi_chunk_buffer[0];
				spi_master_recv(&spi_obj, (char *) spi_chunk_buffer, 1 * 2);
				H_size = spi_chunk_buffer[0];
				gpio_write(&gpio_cs, 1);

				recv_len = ((H_size << 16) | L_size);

				if (recv_len == 0) {
					break;
				}
				// Stage B, confirm addr & len
				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_COMMAND;
				spi_chunk_buffer[txlen++] = COMMAND_READ_BEGIN;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				gpio_write(&gpio_cs, 0);
				txlen = 1;

				spi_chunk_buffer[0] = PREAMBLE_DATA_WRITE;
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				spi_chunk_buffer[0] = L_address;
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				spi_chunk_buffer[0] = H_address;
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				spi_chunk_buffer[0] = L_size;
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				spi_chunk_buffer[0] = H_size;
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				// Stage C, begin to read
				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_COMMAND;
				spi_chunk_buffer[txlen++] = COMMAND_READ_RAW;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_DATA_READ;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				spi_master_recv(&spi_obj, (char *) spi_chunk_buffer, 1 * 2);	// recv dummy
				rxlen = recv_len;
				spi_master_recv(&spi_obj, log_buf, rxlen * 2);
				log_buf[rxlen * 2] = '\0';
				gpio_write(&gpio_cs, 1);

				// Stage D, read end
				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_COMMAND;
				spi_chunk_buffer[txlen++] = COMMAND_READ_WRITE_END;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				// stage final
				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_COMMAND;
				spi_chunk_buffer[txlen++] = COMMAND_END;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				L_size = (recv_len) & 0x0000FFFF;
				H_size = ((recv_len) & 0xFFFF0000) >> 16;

				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_DATA_WRITE;
				spi_chunk_buffer[txlen++] = L_size;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				txlen = 0;
				spi_chunk_buffer[txlen++] = PREAMBLE_DATA_WRITE;
				spi_chunk_buffer[txlen++] = H_size;

				gpio_write(&gpio_cs, 0);
				spi_master_send(&spi_obj, (char *) spi_chunk_buffer, txlen * 2);
				gpio_write(&gpio_cs, 1);

				// finalize
				//printf("%s", log_buf);
				atcmd_check_special_case(log_buf);
				rtw_up_sema(&log_rx_interrupt_sema);
				taskYIELD();
			} while (0);
		}
	}

	vTaskDelete(NULL);
}
#else //!USE_USI_SPI_SLAVE




// SPI transfer tags, also used by the master SPI device
enum {
	SPT_TAG_PRE = 0x55, /* Master initiate a TRANSFER */
	SPT_TAG_ACK = 0xBE, /* Slave  Acknowledgement */
	SPT_TAG_WR  = 0x80, /* Master WRITE  to Slave */
	SPT_TAG_RD  = 0x00, /* Master READ from Slave */
	SPT_TAG_DMY = 0xFF, /* dummy */
};

enum {
	SPT_ERR_OK  = 0x00,
	SPT_ERR_DEC_SPC = 0x01,
};

typedef struct at_pbuf_s {
	struct pbuf* pb;
	int iter;
} at_pbuf_t;

#define ATPB_W (&at_pbufs[0])
#define ATPB_R (&at_pbufs[1])
static at_pbuf_t at_pbufs[2];

static uint8_t spi_rx_buf[LOG_SERVICE_BUFLEN];

/* Complete Flag of TRx */
volatile int SlaveTxDone;
volatile int SlaveRxDone;

/*
static SRAM_NOCACHE_DATA_SECTION u8 slave_tx_buf[LOG_TX_BUFFER_SIZE];
static SRAM_NOCACHE_DATA_SECTION u8 slave_rx_buf[LOG_TX_BUFFER_SIZE];
*/

/* sometimes we also need lowlevel api */
static USI_TypeDef * usi_dev;

static void slave_tr_done_callback(uint32_t id, SpiIrq event) {
	(void) id;

	switch(event){
	case SpiRxIrq:
		SlaveRxDone = 1;
		rtw_up_sema(&rx_done_sema);
		break;

	case SpiTxIrq:
		SlaveTxDone = 1;
		rtw_up_sema(&tx_done_sema);
		break;

	case SpiIdleIrq:
		DBG_8195A("SpiIdleIrq\n");
		break;

	default:
		DBG_8195A("unknown interrput evnent!\n");
	}
	return;
}

static void spi_atcmd_initial(void)
{
	wifi_disable_powersave();

	/* Schmitt input make input more stable */
	PAD_UpdateCtrl(USI_SPI_MOSI, PAD_BIT_SCHMITT_TRIGGER_EN, PAD_BIT_SCHMITT_TRIGGER_EN);
	PAD_UpdateCtrl(USI_SPI_CS,   PAD_BIT_SCHMITT_TRIGGER_EN, PAD_BIT_SCHMITT_TRIGGER_EN);

	// gpio_hrdy output high level if this/slave has data to send
	gpio_init(&gpio_hrdy, GPIO_HRDY);
	gpio_write(&gpio_hrdy, SPI_SLAVE_IDLE);
	gpio_mode(&gpio_hrdy, PullUp);
	gpio_dir(&gpio_hrdy, PIN_OUTPUT);

	// gpio_sync output low level if this/slave prepared well the TX FIFO
	gpio_init(&gpio_sync, GPIO_SYNC);
	gpio_write(&gpio_sync, SPI_STATE_MOSI);
	gpio_mode(&gpio_sync, PullUp);
	gpio_dir(&gpio_sync, PIN_OUTPUT);

	/* USI0_DEV is as Slave */
	usi_dev = USI0_DEV;
	spi_obj.spi_idx = MBED_USI0_SPI;
	uspi_init(&spi_obj, USI_SPI_MOSI, USI_SPI_MISO, USI_SPI_SCLK, USI_SPI_CS);
	uspi_format(&spi_obj, 8/*bits*/, 0/*mode*/, 1/*is_slave*/);
	uspi_irq_hook(&spi_obj, slave_tr_done_callback, (uint32_t)&spi_obj);

	// init semaphore that makes spi tx/rx thread to check something
	rtw_init_sema(&spi_check_trx_sema, 1);
	rtw_down_sema(&spi_check_trx_sema);

	// init semaphore for slave tx
	rtw_init_sema(&tx_done_sema, 1);
	rtw_down_sema(&tx_done_sema);

	// init semaphore for slave rx
	rtw_init_sema(&rx_done_sema, 1);
	rtw_down_sema(&rx_done_sema);

	return;
}

static int spi_get_fifo_intr_status(u32* status) {
	status[0]  = USI_SSI_GetRawIsr(usi_dev);
	status[0] |= USI_SSI_GetTxCount(usi_dev) << 16;
	status[0] |= USI_SSI_GetRxCount(usi_dev) << 24;
	return 0;
}

int uspi_slave_rx_wait(u8* buf, u16 len) {
	int r;
	u16 i;

	for (i = 0; i < len; ) {
		/* drain fifo data as possible */
		if (USI_SSI_Readable(usi_dev)) {
			buf[i++] = USI_SSI_ReadData(usi_dev);
			continue;
		}
	}
	if (i >= len) {
		return len;
	}

	/* interrupt reading */
	for (;;) {
		r = uspi_slave_read_stream(&spi_obj, (char*)&buf[i], len - i);
		if (r == HAL_OK) break;
		rtw_msleep_os(1);
	}
	rtw_down_sema(&rx_done_sema);

	return len;
}

int uspi_slave_wait_tx_end(int wait_us) {
	u32 us;

	wait_us /= 1000;

	/* Wait Tx FIFO empty */
	while (0 != (us = USI_SSI_GetTxCount(usi_dev))) {
		if (wait_us-- <= 0) {
			goto __ret;
		}
		rtw_msleep_os(1);
	}

	/* Clear RX FIFO */
	while (USI_SSI_Readable(usi_dev)) {
		USI_SSI_ReadData(usi_dev);
	}

	while (USI_SSI_Busy(usi_dev));

__ret:
	if (wait_us < 0) {
		printf("U%d", us);
		return -1;
	}
	return 0;
}



int uspi_slave_tx_wait(u8* buf, u16 len) {
	int r;

	USI_SSI_TRxPath_Cmd(usi_dev, USI_SPI_RX_ENABLE, DISABLE);

	/* interrupt writing */
	for (;;) {
		r = uspi_slave_write_stream(&spi_obj, (char*)buf, len);
		if (r == HAL_OK) break;
		rtw_msleep_os(1);
	}

	/* inform the master we have data ready to shift data out to MISO */
	gpio_write(&gpio_sync, SPI_STATE_MISO);

	rtw_down_sema(&tx_done_sema);
	uspi_slave_wait_tx_end(100000); /* max wait 100 milli second */

	USI_SSI_TRxPath_Cmd(usi_dev, USI_SPI_RX_ENABLE, ENABLE);

	/* inform the master we are ready for shifting data in from MOSI */
	gpio_write(&gpio_sync, SPI_STATE_MOSI);

	return len;
}


void spi_at_send_buf(uint8_t* buf, uint32_t size) {
	int old_mask = 0;
	struct pbuf* pb;

	if (size >= UINT16_MAX || !(pb = pbuf_alloc(PBUF_RAW, size, PBUF_RAM))) {
		printf("L%d at tx overflow size=%u\n", __LINE__, size);
		return;
	}

	pbuf_take(pb, buf, size);

	// should protect the pbuf list
	if (__get_IPSR()) {
		old_mask = taskENTER_CRITICAL_FROM_ISR();
	} else {
		taskENTER_CRITICAL();
	}

	if (ATPB_W->pb == NULL) {
		ATPB_W->pb = pb;
	} else {
		pbuf_cat(ATPB_W->pb, pb);
	}

	if (__get_IPSR()) {
		taskEXIT_CRITICAL_FROM_ISR(old_mask);
	} else {
		taskEXIT_CRITICAL();
	}
	return;
}

int spi_rx_char(int c) {
	static uint8_t cmd_buf[ATSTRING_LEN];
	static int idx = 0;

	/* not empty line or \r\n */
	if (idx == 0) {
		if (c == '\r' || c == '\n') return 0;
	}

	/* process all \r, \n, \r\n */
	if (c == '\n') c = '\r';
	cmd_buf[idx] = c;
	if (idx < sizeof cmd_buf - 1) {
		idx++;
	} else {
		printf("L%d at rx overflow\n", __LINE__);
	}

	if (idx > 1 && c == '\r') {
		cmd_buf[idx] = 0;
		strcpy(log_buf, cmd_buf);

		atcmd_check_special_case(log_buf);
		rtw_up_sema(&log_rx_interrupt_sema);

		idx = 0;
	}

	return 0;
}

static void spi_trx_thread(void *param)
{
	union {
		uint8_t v8[4];
		uint16_t v16[2];
		uint32_t v32;
	} u;
	uint8_t cmd;
	uint16_t len;
	u32 status[4];

	(void) param;

	for (;;) {
_repeat:
		/* wait SPT_TAG_PRE */
		uspi_slave_rx_wait(u.v8, 1);

		if (u.v8[0] != SPT_TAG_PRE) {
			if (u.v8[0] != SPT_TAG_DMY) {
				printf("*R%02X\n", u.v8[0]);
			}
			goto _repeat;
		}
		spi_get_fifo_intr_status(status + 0);

		/* wait SPT_TAG_WR/SPT_TAG_RD */
		uspi_slave_rx_wait(u.v8, 1);
		if (u.v8[0] != SPT_TAG_RD && u.v8[0] != SPT_TAG_WR) {
			printf("#R%02X\n", u.v8[0]);
			goto _repeat;
		}
		cmd = u.v8[0];

		/* recv len (2B) */
		uspi_slave_rx_wait(u.v8, 2);
		len = ntohs(u.v16[0]);

		spi_get_fifo_intr_status(status + 1);

		/* TODO, check len */
		if (cmd == SPT_TAG_WR) { /* The master write to this slave */
			if (len > sizeof spi_rx_buf) {
				len = sizeof spi_rx_buf;
			}
			u.v8[0] = SPT_TAG_ACK;
			u.v8[1] = SPT_ERR_OK;
			u.v16[1] = htons(len);
			uspi_slave_tx_wait(u.v8, 4);
			spi_get_fifo_intr_status(status + 2);

			if (len) {
				uspi_slave_rx_wait(spi_rx_buf, len);
			}
			spi_get_fifo_intr_status(status + 3);

			if (len) {
				// debug_buf("[", (char*)spi_rx_buf, len);
				// spi_rx_buf[len] = 0;
				for (int i = 0; i < len; i++) {
					spi_rx_char(spi_rx_buf[i]);
				}
			}

			DBG_PRINTF(MODULE_SPI, LEVEL_TRACE, "L%dI=%08X\n", __LINE__, status[0]);

		} else
		if (cmd == SPT_TAG_RD) { /* The master read from this slave */

			// Move the pbuf list from Writing Slot
			// to Reading Slot.
			if (ATPB_R->pb == NULL) {
				if (ATPB_W->pb != NULL) {
					taskENTER_CRITICAL();

					ATPB_R->pb = ATPB_W->pb;
					ATPB_W->pb = NULL;
					taskEXIT_CRITICAL();

					ATPB_R->iter = 0;
				}
			}

			// Preparing data & length to send.
			if (ATPB_R->pb == NULL) {
				len = 0;
			} else if (len > ATPB_R->pb->tot_len - ATPB_R->iter){
				len = ATPB_R->pb->tot_len - ATPB_R->iter;
			}
			if (len > sizeof log_tx_buffer) {
				len = sizeof log_tx_buffer;
			}

			if (len) {
				pbuf_copy_partial(ATPB_R->pb, log_tx_buffer, len, ATPB_R->iter);
			}

			u.v8[0] = SPT_TAG_ACK;
			u.v8[1] = SPT_ERR_OK;
			u.v16[1] = htons(len);
			uspi_slave_tx_wait(u.v8, 4);
			spi_get_fifo_intr_status(status + 2);

			if (len) {
				uspi_slave_tx_wait(log_tx_buffer, len);

				ATPB_R->iter += len;
				if (ATPB_R->iter >= ATPB_R->pb->tot_len) {
					/* Free the pbuf not required anymore */
					pbuf_free(ATPB_R->pb);
					ATPB_R->pb = NULL;
				}
			}
			spi_get_fifo_intr_status(status + 3);
			DBG_PRINTF(MODULE_SPI, LEVEL_TRACE, "L%dI=%08X\n", __LINE__, status[0]);

		}

		if (status[1] != status[2] || status[2] != status[3]) {
			DBG_PRINTF(MODULE_SPI, LEVEL_TRACE, "L%dI=%08X\n", __LINE__, status[1]);
			DBG_PRINTF(MODULE_SPI, LEVEL_TRACE, "L%dI=%08X\n", __LINE__, status[2]);
			DBG_PRINTF(MODULE_SPI, LEVEL_TRACE, "L%dI=%08X\n", __LINE__, status[3]);
		}

		// rtw_up_sema(&spi_check_trx_sema);
	}

	// taskYIELD();

	vTaskDelete(NULL);
}

#endif//!USE_USI_SPI_SLAVE

static void spi_atcmd_thread(void *param)
{
	(void) param;
	p_wlan_init_done_callback = NULL;
	atcmd_wifi_restore_from_flash();
	atcmd_lwip_restore_from_flash();
	rtw_msleep_os(20);
	spi_atcmd_initial();

	// the rx_buffer of atcmd is to receive and sending out to log_tx
	atcmd_lwip_set_rx_buffer(log_tx_buffer, sizeof(log_tx_buffer));

	at_set_debug_mask(-1UL);

	at_printf("\r\nready\r\n");	// esp-at compatible

	if (xTaskCreate(spi_trx_thread, ((const char *) "spi_trx_thread"), 4096, NULL, tskIDLE_PRIORITY + 6, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(spi_trx_thread) failed", __FUNCTION__);

	vTaskDelete(NULL);
}

int spi_atcmd_module_init(void)
{
	if (xTaskCreate(spi_atcmd_thread, ((const char *) "spi_atcmd_thread"), 1024, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(spi_atcmd_thread) failed", __FUNCTION__);
	return 0;
}

void example_spi_atcmd(void)
{
	p_wlan_init_done_callback = spi_atcmd_module_init;
	return;
}

#endif// #if CONFIG_EXAMPLE_SPI_ATCMD
