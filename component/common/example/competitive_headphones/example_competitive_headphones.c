#include <platform_opts.h>
#if defined(CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES) && \
	CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include <lwip/sockets.h>
#include <lwip/raw.h>
#include <lwip/icmp.h>
#include <lwip/inet_chksum.h>
#include <platform/platform_stdlib.h>
#include "example_competitive_headphones.h"
#include <wifi/wifi_conf.h>
#include <wifi/wifi_util.h>
#include "wifi_structures.h"
#include "lwip_netconf.h"
#include "rl6548.h"
#include "freertos_service.h"
#include "gpio_api.h"
#include "resample/resample_library.h"
#include "headphone_rl6548_audio.h"
#include "headphone_burst.h"

static SP_OBJ sp_obj;
_sema packet_buffer_sema = NULL;
_sema packet_processed_sema = NULL;
static char udp_recv_buf[RECV_BUF_SIZE];

#ifdef HEADPHONE_MIC
static _sema audio_rx_sema = NULL;
static char* audio_rx_pbuf;
u8 mic_recv_pkt_buff[SP_DMA_PAGE_SIZE*RECV_BUF_NUM];
u8 mic_resample_buf[RECORD_RESAMPLE_SIZE];
_sema mic_rx_sema = NULL;
static xQueueHandle mic_pkt_queue = NULL;
#define MIC_PKT_QUEUE_LENGTH	1
#endif

static xSemaphoreHandle recv_qlen_lock;
uint32_t recv_queue_len_test = 0;
static xQueueHandle recv_pkt_queue = NULL;

u8 play_buf[SP_DMA_PAGE_SIZE];
u8 phone_test[256];
static int play_start = 0;
static int play_index = 0;
int first_play_packet = 0;
int first_receive_packet = 0;
u8 recv_packet[BURST_SIZE * RECV_SP_BUF_SIZE];
u8 recv_packet_index[4] = {0};
u32 recv_packet_num = 0;

void example_audio_dac_thread(void* param)
{
	recv_pkt_t recv_pkt;

	while(1){
		if(xSemaphoreTake(packet_processed_sema, 0xFFFFFFFF) != pdTRUE) continue;

		memcpy(play_buf, recv_packet, RECV_SP_BUF_SIZE * BURST_SIZE);

		while(sp_get_free_tx_page() == 0){
			vTaskDelay(1);
		}

		sp_write_tx_page((u8 *)play_buf, SP_DMA_PAGE_SIZE);
	}
exit:
	vTaskDelete(NULL);
}

void audio_play_init(void)
{
	sp_obj.sample_rate = SR_48K;
	sp_obj.word_len = WL_16;
	sp_obj.mono_stereo = CH_STEREO;
#ifdef HEADPHONE_MIC
	sp_obj.direction = APP_LINE_OUT | APP_DMIC_IN;
#else
	sp_obj.direction = APP_LINE_OUT;
#endif
	audio_codec_init((void*)(&sp_obj));

	if(xTaskCreate(example_audio_dac_thread, ((const char*)"example_audio_dac_thread"), 512, (void *)(&sp_obj), tskIDLE_PRIORITY + 3, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_audio_dac_thread) failed", __FUNCTION__);
}

_sema udp_burststop_sema = NULL;
u32 burst_dataseq;
u32 burst_burststart;
static void udp_burststop_thread(void *param)
{
	while(1){
		if(xSemaphoreTake(udp_burststop_sema, 0xFFFFFFFF) != pdTRUE) continue;
		udp_server_burststop(burst_dataseq, burst_burststart);
	}
	vTaskDelete(NULL);
}

static char udp_recv_buf_bk[RECV_SP_BUF_SIZE];
u32 burststart;
u32 burststart_processed =0;
int processed = 0;
int first_packet = 0;

extern int packet_compensation_timeout_start(int delay);
static void udp_server_for_headphones_thread(void *param)
{
	struct sockaddr_in	 ser_addr, client_addr;
	int	addrlen = sizeof(struct sockaddr_in);
	int server_fd;
	int n = 1;
	int recv_size = 0;
	recv_pkt_t recv_pkt;
#ifdef HEADPHONE_MIC
	recv_pkt_t mic_pkt;
#endif
	//wait for first packet to start
	static int play_start_num = 0;
	u32	dataseq;
	u32 last_burst_time = 0;
	u32 last_recv_pkt_num = 0;
	u32 recv_pkt_num = 0;
	int i;

#ifdef HEADPHONE_MIC
	u8 tos_value = 96;
	int mic_client_fd;
	struct sockaddr_in	 mic_client_addr;
#endif
	udp_server_timeout_init();

	if((server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
		printf("\n\r[ERROR] %s: Create socket failed", __func__);
		return;
	}

	//Set the reuse the addr when close socket
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *) &n, sizeof( n ) );

	//initialize structure dest
	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(HEADPHONES_SERVER_PORT);
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//	ser_addr.sin_addr.s_addr = inet_addr((char const*)"192.168.1.1"); 

	// binding the TCP socket to the TCP server address
	if( bind(server_fd, (struct sockaddr*)&ser_addr, sizeof(ser_addr)) < 0) {
		printf("\n\r[ERROR] %s: Bind socket failed",__func__);
		close(server_fd);
		return;
	}
	printf("\n\r%s: Bind socket successfully",__func__);
#ifdef HEADPHONE_MIC
//create socket for mic data send
	if((mic_client_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("\n\r[ERROR] %s: Create socket failed", __func__);
		return;
	}
	memset(&mic_client_addr, 0, sizeof(mic_client_addr));
	mic_client_addr.sin_family = AF_INET;
	mic_client_addr.sin_port = htons(HEADPHONES_CLIENT_PORT);
	mic_client_addr.sin_addr.s_addr = inet_addr((char const*)"192.168.1.1");  //should changed
	lwip_setsockopt(mic_client_fd,IPPROTO_IP,IP_TOS,&tos_value,sizeof(tos_value));
	
#endif

	burststart_processed = 0;

	while(1){
			recv_size = recvfrom(server_fd, udp_recv_buf, RECV_BUF_SIZE,0,(struct sockaddr *) &client_addr,
								(u32_t*)&addrlen);

			//Add wifi ps control
			//end of the wifi ps control
			if(recv_size == RECV_BUF_SIZE ){		
				memcpy(&dataseq, &udp_recv_buf[RECV_SP_BUF_SIZE], 4);
				memcpy(&burststart, &udp_recv_buf[RECV_SP_BUF_SIZE + 4], 4);
				memcpy(&first_packet, &udp_recv_buf[RECV_SP_BUF_SIZE + 8], 4);
				
				if(burststart != burststart_processed){
					if((dataseq >= 0) && (dataseq <=3)){
						memcpy(&recv_packet[dataseq * RECV_SP_BUF_SIZE], udp_recv_buf, RECV_SP_BUF_SIZE);	
					}
					recv_packet_index[dataseq] = 1;
					recv_packet_num++;

					if(recv_packet_num == 1){
						packet_compensation_timeout_start(PACKET_PROCESS_DELAY);
#ifdef HEADPHONE_MIC
									//Send mic packet to dongle
					
					if((sendto(mic_client_fd, mic_resample_buf, RECORD_SEND_PKT_SIZE,0,
										(struct sockaddr*)&mic_client_addr, sizeof(struct sockaddr_in))) < 0) {
						printf("error %d\n", lwip_getsocklasterr((long)mic_client_fd)); 
					}
#endif
					}
					//udp_server_burststop(dataseq, burststart);
						burst_dataseq = dataseq;
						burst_burststart = burststart;
						xSemaphoreGive(udp_burststop_sema);
				}
			}
	}
	
	if(recv_pkt.data!= NULL)
		free(recv_pkt.data);
	close(server_fd);
	vTaskDelete(NULL);
}

#ifdef HEADPHONE_MIC
RESAMPLE_STAT48TO16 state;
int resample_temp[SP_DMA_PAGE_SIZE/2 + 16];
static void mic_receive_thread(void* param)
{
	int ret;
	recv_pkt_t mic_pkt;
	static int count = 0;
	
	ResampleReset48khzTo16khz(&state);
	while(1){
		if(xSemaphoreTake(mic_rx_sema, 0xFFFFFFFF) != pdTRUE) continue;

			memset(mic_resample_buf, 0, RECORD_RESAMPLE_SIZE);
			Resample48khzTo16khz(&mic_recv_pkt_buff[count*SP_DMA_PAGE_SIZE], SP_DMA_PAGE_SIZE/2, 
									mic_resample_buf, &state, resample_temp);
			count++;
			count = count % 2;
	}
	
	vTaskDelete(NULL);
}
#endif

void example_competitive_headphones_init(void *param)
{
//TODO
#if 0
	headphone_pair();
#endif		

	//Create udp_server_thread
	//init should in one function
	rtw_init_sema(&packet_buffer_sema, 0);
	packet_processed_sema = xSemaphoreCreateCounting(0xffffffff, 0);	
	recv_pkt_queue = xQueueCreate(RECV_PKT_QUEUE_LENGTH, sizeof(recv_pkt_t));
#ifdef HEADPHONE_MIC
	mic_rx_sema = xSemaphoreCreateCounting(0xffffffff, 0);	
	mic_pkt_queue = xQueueCreate(MIC_PKT_QUEUE_LENGTH, sizeof(recv_pkt_t));
#endif
	extern int packet_compensation_server_timeout_init(void);
	packet_compensation_server_timeout_init();
	if(xTaskCreate(udp_server_for_headphones_thread, ((const char*)"udp_server_for_headphones_thread"), 
					1024, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(udp_server_for_headphones_thread) failed", __FUNCTION__);

#ifdef HEADPHONE_MIC
	if(xTaskCreate(mic_receive_thread, ((const char*)"mic_receive_thread"), 
					1024, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(mic_receive_thread) failed", __FUNCTION__);
#endif

	audio_play_init();

	rtw_init_sema(&udp_burststop_sema, 0);
	if(xTaskCreate(udp_burststop_thread, ((const char*)"udp_burststop_thread"), 
				1024, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
	printf("\n\r%s xTaskCreate(udp_burststop_thread) failed", __FUNCTION__);

	vTaskDelete(NULL);
}

void example_competitive_headphones()
{
	if(xTaskCreate(example_competitive_headphones_init, ((const char*)"headphones example init"), 
					1024, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(headphones example thread) failed", __FUNCTION__);

}
#endif

