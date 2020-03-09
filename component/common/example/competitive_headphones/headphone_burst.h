#if CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES
#ifndef __HEADPHONE_BURST_H__
#define __HEADPHONE_BURST_H__

#include "ameba_soc.h"

extern int udp_server_timeout_init(void);
extern void udp_server_burststop(u32 dataseq, u32 burststart);
#endif
#endif

