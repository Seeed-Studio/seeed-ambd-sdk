#include <platform_opts.h>

#if defined(CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES_DONGLE) && \
	CONFIG_EXAMPLE_COMPETITIVE_HEADPHONES_DONGLE
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include <lwip/sockets.h>
#include <lwip/raw.h>
#include <lwip/icmp.h>
#include <lwip/inet_chksum.h>
#include <platform/platform_stdlib.h>
#include "wifi_structures.h"
#include "i2s_api.h"
#include "freertos_service.h"
#include "example_competitive_headphones_dongle.h"
#ifdef MIC_RESAMPLE
#include "resample_library.h"
#endif

#if CONFIG_LWIP_LAYER
extern struct netif xnetif[NET_IF_NUM]; 
#endif

xSemaphoreHandle i2s_rx_sema = NULL;

_sema recv_sema = NULL;

u8 recv_buf[I2S_DMA_PAGE_SIZE*RECV_BUF_NUM];

static int count = 0;
gpio_t i2s_intr;
gpio_t test_gpio1;
gpio_t test_gpio2;
gpio_t test_gpio3;


int first_packet = 0;

char send_buf[SEND_PKT_SIZE];
char headphone_ip_addr[20];

static u32 audio_start_first = 0;

//Set bit chann mode
void usb_audio_plug_process(uint8_t audio_path, uint8_t bit_res, uint8_t sf,
                                            uint8_t chann_mode)
{
	audio_start_first = 1;
}

void usb_audio_unplug_process(uint8_t audio_path)
{

}

static void udp_client_for_dongle_thread(void *param)
{
	static int out_idx = 0;
	int i;
	int num = 0;
	u8 tos_value = 96;
	int i2s_packet_num;
	int last_err;
	int sockfd;
	int ret;
	u32 dataseq;
	u32 burststart;
	struct sockaddr_in ser_addr;

	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(DONGLE_CLINET_PORT);
	ser_addr.sin_addr.s_addr = inet_addr((char const*)headphone_ip_addr);  //should changed

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		printf("\n\r[ERROR] %s: Create socket failed", __func__);
		return;
	}

	lwip_setsockopt(sockfd,IPPROTO_IP,IP_TOS,&tos_value,sizeof(tos_value));

	printf("connect to the server addr: 0x%x, port: %d\n\r", ser_addr.sin_addr.s_addr, ser_addr.sin_port);

	while(1){	
		if(xSemaphoreTake(recv_sema, portMAX_DELAY) != pdTRUE) continue;
		
		if(count >= out_idx){
			i2s_packet_num = count - out_idx; 
		}else{
			i2s_packet_num = count + RECV_BUF_NUM - out_idx;
		}

		if(i2s_packet_num >= RECV_PAGE_NUM){
			dataseq = 0;
			burststart= wifi_get_tsf_low(0);

			for(i=0; i<RECV_PAGE_NUM; i++){
				memcpy(send_buf, &recv_buf[I2S_DMA_PAGE_SIZE * out_idx], I2S_DMA_PAGE_SIZE);
				
				memcpy(&send_buf[I2S_DMA_PAGE_SIZE], &dataseq, 4);
				memcpy(&send_buf[I2S_DMA_PAGE_SIZE + 4], &burststart, 4);
				memcpy(&send_buf[I2S_DMA_PAGE_SIZE + 8], &audio_start_first, 4);
				
				//printf(" %d\n\r", burststart, dataseq);
				//printf("s\n\r");
				if(send_buf[8] != 0){
					gpio_write(&test_gpio3, 1);
					gpio_write(&test_gpio3, 0);
				}
				gpio_write(&test_gpio1, 1);
				gpio_write(&test_gpio1, 0);
				if((ret = sendto(sockfd, send_buf, SEND_PKT_SIZE,0,
									(struct sockaddr*)&ser_addr, sizeof(struct sockaddr_in))) < 0) {
					last_err = lwip_getsocklasterr((long)sockfd);	
					printf("last_err = %d\n\r", last_err);
				}


				out_idx++;
				out_idx = out_idx % RECV_BUF_NUM;
				dataseq++;
			}

			if(audio_start_first == 1)
				audio_start_first = 0;
		}
	}

	close(sockfd);
	vTaskDelete(NULL);
}

static u8 udp_dongle_recv_buf[DONGLE_RECV_BUF_SIZE];
#ifdef RECORD_SUPPORT
#ifdef MIC_RESAMPLE
u8 mic_recv_pkt_buff[SP_DMA_PAGE_SIZE*RECV_BUF_NUM];
#else
u8 mic_recv_pkt_buff[DONGLE_RECV_BUF_SIZE*RECV_BUF_NUM];
#endif
static void udp_server_for_dongle_thread(void *param)
{
	int sockfd;
	int *ptx_buf;
	int recv_size = 0;
	struct sockaddr_in	 ser_addr, client_addr;
	int	addrlen = sizeof(struct sockaddr_in);
#ifdef MIC_RESAMPLE
	RESAMPLE_STAT16TO48 state;
#endif
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		printf("\n\r[ERROR] %s: Create socket failed", __func__);
		return;
	}
	memset(&ser_addr, 0, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(DONGLE_SERVER_PORT);
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //should changed

		// binding the TCP socket to the TCP server address
	if( bind(sockfd, (struct sockaddr*)&ser_addr, sizeof(ser_addr)) < 0) {
		printf("\n\r[ERROR] %s: Bind socket failed",__func__);
		close(sockfd);
		return;
	}

	static int count_in = 0;

#ifdef MIC_RESAMPLE
	ResampleReset16khzTo48khz(&state);
#endif
	u8 voice_resample[I2S_DMA_PAGE_SIZE];
	int temp[DONGLE_RECV_BUF_SIZE/BURST_SIZE + 16];

	memset(voice_resample, 0, I2S_DMA_PAGE_SIZE);
	
	memset(temp, 0, sizeof(temp));
	
	while(1){	
		#if 1
		recv_size = recvfrom(sockfd, udp_dongle_recv_buf, DONGLE_RECV_BUF_SIZE,0,
							(struct sockaddr *) &client_addr,(u32_t*)&addrlen);

		#if 1
#ifdef MIC_RESAMPLE
		
			memset(voice_resample, 0, I2S_DMA_PAGE_SIZE);
			Resample16khzTo48khz(&udp_dongle_recv_buf[i*DONGLE_RECV_BUF_SIZE/BURST_SIZE], 
								DONGLE_RECV_BUF_SIZE/BURST_SIZE/2, voice_resample, &state, temp);
			
			_memcpy((void*)&mic_recv_pkt_buff[SP_DMA_PAGE_SIZE*count_in], voice_resample, I2S_DMA_PAGE_SIZE);
#else
			_memcpy((void*)&mic_recv_pkt_buff[DONGLE_RECV_BUF_SIZE*count_in], udp_dongle_recv_buf, DONGLE_RECV_BUF_SIZE);

#endif
		count_in++;
		count_in = count_in%RECV_BUF_NUM;
		#endif
		#endif
		
	}

	close(sockfd);
	vTaskDelete(NULL);

}
#endif

extern unsigned int iso_out_count;
extern unsigned int iso_in_count;
extern int usb_audio_init(int N_bh, int Size_bh, int type);
bool usb_ep_out_packet_process(u8* p_data, u16 len)
{
	int copy_size = 0; // BUF_SIZE;
	static int copy_pos = 0;
	static int test_i = 0;
	int j = 0;
	if((copy_pos + len) <= (I2S_DMA_PAGE_SIZE * RECV_BUF_NUM)){
		copy_size = len;
		memcpy(&recv_buf[copy_pos], p_data, copy_size);
		copy_pos = copy_pos + len;
	}else{
		copy_size = (I2S_DMA_PAGE_SIZE * RECV_BUF_NUM) - copy_pos;
		memcpy(&recv_buf[copy_pos], p_data, copy_size);
		memcpy(&recv_buf[0], (p_data+(copy_size)), (len-copy_size));
		copy_pos = len - copy_size;
	}
	if(copy_pos == I2S_DMA_PAGE_SIZE * RECV_BUF_NUM)
		copy_pos = 0;

	if(count != (copy_pos / I2S_DMA_PAGE_SIZE)){
		count = copy_pos / I2S_DMA_PAGE_SIZE;
		if(*p_data != 0){
			gpio_write(&test_gpio2, 1);
			gpio_write(&test_gpio2, 0);
		}

		xSemaphoreGive(recv_sema);
	}
}

#ifdef RECORD_SUPPORT
#ifdef MIC_RESAMPLE
#define BUF_SIZE SP_DMA_PAGE_SIZE
#else
#define BUF_SIZE	DONGLE_RECV_BUF_SIZE	
#endif
bool usb_ep_in_packet_process(u8* p_data, u16 len)
{
	int copy_size;
	static int copy_pos = 0;
	if((copy_pos + len) <= (BUF_SIZE * RECV_BUF_NUM)){
		copy_size = len;
		memcpy(p_data, &mic_recv_pkt_buff[copy_pos], len);
		copy_pos = (copy_pos + len) % (BUF_SIZE * RECV_BUF_NUM);
	}else{
		copy_size = (BUF_SIZE*4) - copy_pos;
		memcpy(p_data, &mic_recv_pkt_buff[copy_pos], copy_size);
		memcpy((p_data+(copy_size)), &mic_recv_pkt_buff[0], (len-copy_size));
		copy_pos = len - copy_size;
	}
}
#endif



 int headphone_inited = 0;
void example_headphones_dongle_thread(void *param)
{
//TODO
#if 0
	int timeout = 20;
	while(timeout > 0) {
		if(rltk_wlan_running(0)) {
			break;
		}

		vTaskDelay(1000 / portTICK_RATE_MS);
		timeout --;
	}

//	audio_data_recv();
	softap_for_dongle();
#endif
	//Set softap Wifi Retry time to 0x4
	u8 RetryLimit = 0x4;
	u16 val16 = (RetryLimit << 8) | (RetryLimit << 0);
	HAL_WRITE16(WIFI_REG_BASE, 0x042A, val16);

   // gpio_init(&i2s_intr, PA_22);
   // gpio_dir(&i2s_intr, PIN_OUTPUT);
   // gpio_mode(&i2s_intr, PullNone);

    gpio_init(&test_gpio1, PA_22);
    gpio_dir(&test_gpio1, PIN_OUTPUT);
    gpio_mode(&test_gpio1, PullNone);

    gpio_init(&test_gpio2, PA_10);
    gpio_dir(&test_gpio2, PIN_OUTPUT);
    gpio_mode(&test_gpio2, PullNone);

	gpio_init(&test_gpio3, PA_11);
    gpio_dir(&test_gpio3, PIN_OUTPUT);
    gpio_mode(&test_gpio3, PullNone);

	headphone_inited = 1;

	i2s_rx_sema = xSemaphoreCreateCounting(0xffffffff, 0);	

	rtw_init_sema(&recv_sema, 0);

	if(xTaskCreate(udp_client_for_dongle_thread, ((const char*)"dongle client"), 
					(256+1024), NULL, tskIDLE_PRIORITY + 2 + PRIORITIE_OFFSET, NULL) != pdPASS){
		printf("udp_client_thread_for_dongle thread create fail\n\r");
	}

	usb_if_set_au_put_audio_data_callback(usb_ep_out_packet_process);
#ifdef RECORD_SUPPORT
	usb_if_set_au_pull_audio_data_callback(usb_ep_in_packet_process);
#endif
	usb_if_set_app_usb_audio_plug_callback(usb_audio_plug_process);
	usb_if_set_app_usb_audio_unplug_callback(usb_audio_unplug_process);

	_usb_init();

	
#ifdef RECORD_SUPPORT
	if(xTaskCreate(udp_server_for_dongle_thread, ((const char*)"dongle server"), 
					(256 + 1024), NULL, tskIDLE_PRIORITY + 2 + PRIORITIE_OFFSET, NULL) != pdPASS){
		printf("udp_server_thread_for_dongle thread create fail\n\r");
	}
#endif

//	audio_data_recv_init(NULL);
	vTaskDelete(NULL);
}

void example_competitive_headphones_dongle()
{
	if(xTaskCreate(example_headphones_dongle_thread, ((const char*)"headphones example thread"), 
					1024, NULL, tskIDLE_PRIORITY + 1 + PRIORITIE_OFFSET, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(headphones example thread) failed", __FUNCTION__);
}
#endif

