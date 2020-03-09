#if CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES
#ifndef __EXAMPLE_COMPETITIVE_HEADPHONES_H__
#define __EXAMPLE_COMPETITIVE_HEADPHONES_H__

#include "ameba_soc.h"

#define HEADPHONE_MIC	1
#define BURST_SIZE	4
#define BURST_PERIOD	12
#define MIN_RFOFF_TIME		2 //ms
#define RFOFF_USER_TIMER	1

#define SP_ZERO_BUF_SIZE	128
#define PS_USED	0

#define RECV_BUF_NUM	2
#define RECV_SP_BUF_SIZE	576
#define BURST_HEADER_SIZE	8
#define PLAY_PACKET_NUM		(BURST_SIZE)
#define SP_DMA_PAGE_SIZE	(RECV_SP_BUF_SIZE*PLAY_PACKET_NUM)// 2 ~ 4096
#define PACKET_BUFFER_SIZE	0
#define SEND_INTERFACE_HEADER	4
#define RECV_BUF_SIZE	(RECV_SP_BUF_SIZE + BURST_HEADER_SIZE + SEND_INTERFACE_HEADER)

#define SP_FULL_BUF_SIZE		128
#define RESAMPLE_RATIO			3
#define RECORD_RESAMPLE_SIZE	(SP_DMA_PAGE_SIZE/RESAMPLE_RATIO)
#define RECORD_SEND_PKT_SIZE	(SP_DMA_PAGE_SIZE/RESAMPLE_RATIO)

#define HEADPHONES_SERVER_PORT	5002
#define HEADPHONES_CLIENT_PORT	5001
#define RECV_PKT_QUEUE_LENGTH (64) //(100)
#define PACKET_PROCESS_DELAY	4

typedef struct _recv_pkt_t {
    uint8_t *data;
    uint32_t data_len;
} recv_pkt_t;
extern void example_competitive_headphones();
#endif
#endif

