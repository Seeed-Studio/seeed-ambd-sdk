#include "example_audio_ekho.h"
#include <platform/platform_stdlib.h>
#include "platform_opts.h"
#include "section_config.h"
#include "rl6548.h"
//------------------------------------- ---CONFIG Parameters-----------------------------------------------//
                                
#ifdef __GNUC__

#endif

#ifdef CONFIG_EXAMPLE_EKHO
static SP_InitTypeDef SP_InitStruct;
static SP_GDMA_STRUCT SPGdmaStruct;
static SP_OBJ sp_obj;
static SP_TX_INFO sp_tx_info;

static u8 sp_tx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];
static u8 sp_zero_buf[SP_ZERO_BUF_SIZE];
static char* table[11] = {"零", "一", "二", "三", "四", "五", "六", "七", "八", "九", "点"};
u32 start_heap;

static u8 *sp_get_free_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_usr_cnt]);
	
	if (ptx_block->tx_gdma_own)
		return NULL;
	else{
		return (u8*)ptx_block->tx_addr;
	}	
}

static void sp_write_tx_page(u8 *src, u32 length)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_usr_cnt]);
	
	memcpy((void*)ptx_block->tx_addr, src, length);
	ptx_block-> tx_gdma_own = 1;
	ptx_block -> tx_length = length;
	sp_tx_info.tx_usr_cnt++;
	if (sp_tx_info.tx_usr_cnt == SP_DMA_PAGE_NUM){
		sp_tx_info.tx_usr_cnt = 0;
	}
}

static void sp_release_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);
	
	if (sp_tx_info.tx_empty_flag){
	}
	else{
		ptx_block->tx_gdma_own = 0;
		sp_tx_info.tx_gdma_cnt++;
		if (sp_tx_info.tx_gdma_cnt == SP_DMA_PAGE_NUM){
			sp_tx_info.tx_gdma_cnt = 0;
		}
	}
}

static u8 *sp_get_ready_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);
	
	if (ptx_block->tx_gdma_own){
		sp_tx_info.tx_empty_flag = 0;
		return (u8*)ptx_block->tx_addr;
	}
	else{
		sp_tx_info.tx_empty_flag = 1;
		return (u8*)sp_tx_info.tx_zero_block.tx_addr;	//for audio buffer empty case
	}
}

static u32 sp_get_ready_tx_length(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);

	if (sp_tx_info.tx_empty_flag){
		return sp_tx_info.tx_zero_block.tx_length;
	}
	else{
		return ptx_block->tx_length;
	}
}

static void sp_tx_complete(void *Data)
{
	SP_GDMA_STRUCT *gs = (SP_GDMA_STRUCT *) Data;
	PGDMA_InitTypeDef GDMA_InitStruct;
	u32 tx_addr;
	u32 tx_length;
	
	GDMA_InitStruct = &(gs->SpTxGdmaInitStruct);

	/* Clear Pending ISR */
	GDMA_ClearINT(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);
	
	sp_release_tx_page();
	tx_addr = (u32)sp_get_ready_tx_page();
	tx_length = sp_get_ready_tx_length();
	GDMA_SetSrcAddr(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, tx_addr);
	GDMA_SetBlkSize(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, tx_length>>2);
	
	GDMA_Cmd(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, ENABLE);
}

static void sp_rx_complete(void *data, char* pbuf)
{
	//
}

static void sp_init_hal(pSP_OBJ psp_obj)
{
	u32 div;
	
	PLL_I2S_Set(ENABLE);		//enable 98.304MHz PLL. needed if fs=8k/16k/32k/48k/96k
	PLL_PCM_Set(ENABLE);		//enable 45.1584MHz PLL. needed if fs=44.1k/8.2k
	PLL_Sel(PLL_I2S);
	RCC_PeriphClockCmd(APBPeriph_AUDIOC, APBPeriph_AUDIOC_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_SPORT, APBPeriph_SPORT_CLOCK, ENABLE);	

	//set clock divider to gen clock sample_rate*256 from 98.304M.
	switch(psp_obj->sample_rate){
		case SR_48K:
			div = 8;
			break;
		case SR_96K:
			div = 4;
			break;
		case SR_32K:
			div = 12;
			break;
		case SR_16K:
			div = 24;
			break;
		case SR_8K:
			div = 48;
			break;
		case SR_44P1K:
       	 	div = 4;
    		PLL_Sel(PLL_PCM);
    		break;	
		default:
			DBG_8195A("sample rate not supported!!\n");
			break;
	}
	PLL_Div(div);

	/*codec init*/
	CODEC_Init(psp_obj->sample_rate, psp_obj->word_len, psp_obj->mono_stereo, psp_obj->direction);
	CODEC_SetVolume(0xaf, 0xaf);
}

static void sp_init_tx_variables(void)
{
	int i;

	for(i=0; i<SP_ZERO_BUF_SIZE; i++){
		sp_zero_buf[i] = 0;
	}
	sp_tx_info.tx_zero_block.tx_addr = (u32)sp_zero_buf;
	sp_tx_info.tx_zero_block.tx_length = (u32)SP_ZERO_BUF_SIZE;
	
	sp_tx_info.tx_gdma_cnt = 0;
	sp_tx_info.tx_usr_cnt = 0;
	sp_tx_info.tx_empty_flag = 0;
	
	for(i=0; i<SP_DMA_PAGE_NUM; i++){
		sp_tx_info.tx_block[i].tx_gdma_own = 0;
		sp_tx_info.tx_block[i].tx_addr = (u32)(sp_tx_buf+i*SP_DMA_PAGE_SIZE);
		sp_tx_info.tx_block[i].tx_length = SP_DMA_PAGE_SIZE;
	}
}

//convert a string which contains arabic numerals to a string which only contains chinese character
static void text_convert(const char *p_original_text, char** p_text){
	int all_bytes = 0;
	for(int i = 0; p_original_text[i] != 0; i++){
		char temp1 = p_original_text[i] - '0';
		char temp2 = p_original_text[i] - '.';
		if((temp1 >= 0) && (temp1 < 10)){
			all_bytes += 3;
		} else if(temp2 == 0){
			all_bytes += 3;
		}	
		else{
			all_bytes++;
		}
	}
	
	*p_text = (char*)malloc(sizeof(char)*(all_bytes + 1));
	int index = 0;
	for(int i = 0; p_original_text[i]!=0; i++){
		char temp1 = p_original_text[i] - '0';
		char temp2 = p_original_text[i] - '.';
		if((temp1 >= 0) && (temp1 < 10)){
			(*p_text)[index++] = table[p_original_text[i] - '0'][0];
			(*p_text)[index++] = table[p_original_text[i] - '0'][1];
			(*p_text)[index++] = table[p_original_text[i] - '0'][2];
		} else if(temp2 == 0){
			(*p_text)[index++] = table[10][0];
			(*p_text)[index++] = table[10][1];
			(*p_text)[index++] = table[10][2];
		}
		else{
			(*p_text)[index++] = p_original_text[i];
		}
		
	}
	
	(*p_text)[index] = '\0';
	
}

void audio_play_ekho(void){

	int drv_num = 0;
	int frame_size = 0;
	u32 read_length = 0;

	char	logical_drv[4]; //root diretor
	char abs_path[32]; //Path to input file
	unsigned long  bytes_left;
	unsigned long file_size;
	volatile u32 tim1 = 0;
	volatile u32 tim2 = 0;
	int *ptx_buf;
	
	const char *p_original_text;
	char *p_text;
	
	int sync_word[4] = {0, 0, 0, 0};
	int suspect_area[10] = {0}; 
	int count = 0;
	int bit_rate = 0;
	int sample_rate = 0;
	int frame_size_cal = 0;
	int new_offset = 0;
	int head_count = 0;
	int flag = 0;
	unsigned short code;
	int pcm_len;
	int dma_page_size;
	int audio_buf_offset = 0;
	char* audio_buf;
	
    p_original_text = "支付宝收款12.3元付款108.56元到账1000.479元购买商品18件";
	text_convert(p_original_text, &p_text);

	audio_buf = malloc(32000);
		
	printf("enter ekho_demo\n");
	tts_init();

	for (int i = 0;p_text[i]!=0; i=i+3){
		if (((p_text[i]&0xf0) != 0xe0) || ((p_text[i+1]&0xc0) != 0x80) || ((p_text[i+2]&0xc0) != 0x80)){
			printf("seems not chinese character!\n");
			continue;
		}
		
		code = ((p_text[i]&0x0f)<<12)|((p_text[i+1]&0x3f)<<6)|(p_text[i+2]&0x3f);	//utf8->unicode
		code &= 0xffff;


		pcm_len = tts_lookup(code, audio_buf);
		audio_buf_offset = 0;
		
		while(pcm_len){
			
			if(pcm_len >= SP_DMA_PAGE_SIZE){
				dma_page_size = SP_DMA_PAGE_SIZE;
			}
			else{
				dma_page_size = pcm_len;
			}
			pcm_len -= dma_page_size;
			
		RETRY:
			if(sp_get_free_tx_page()){
				sp_write_tx_page(audio_buf + audio_buf_offset, dma_page_size);
				audio_buf_offset += dma_page_size;
			}
			else{
				vTaskDelay(1);
				goto RETRY;
			}
		}
		
	}
	
	free(audio_buf);
	free(p_text);
	tts_deinit();
}

void example_audio_ekho_thread(void* param)
{
	u32 tx_addr;
	u32 tx_length;
	pSP_OBJ psp_obj = (pSP_OBJ)param;

	
	printf("Audio codec ekho demo begin......\n");
	
	sp_init_hal(psp_obj);
	
	sp_init_tx_variables();

	/*configure Sport according to the parameters*/
	AUDIO_SP_StructInit(&SP_InitStruct);
	SP_InitStruct.SP_MonoStereo= psp_obj->mono_stereo;
	SP_InitStruct.SP_WordLen = psp_obj->word_len;

	AUDIO_SP_Init(AUDIO_SPORT_DEV, &SP_InitStruct);
	
	AUDIO_SP_TdmaCmd(AUDIO_SPORT_DEV, ENABLE);
	AUDIO_SP_TxStart(AUDIO_SPORT_DEV, ENABLE);

	tx_addr = (u32)sp_get_ready_tx_page();
	tx_length = sp_get_ready_tx_length();
	AUDIO_SP_TXGDMA_Init(0, &SPGdmaStruct.SpTxGdmaInitStruct, &SPGdmaStruct, (IRQ_FUN)sp_tx_complete, (u8*)tx_addr, tx_length);
	for(int i = 0; i < 1000; i++){
		printf("index is %d\n", i);
		audio_play_ekho();
	}
		
exit:
	vTaskDelete(NULL);
}

void example_audio_ekho(void)
{
	
	sp_obj.mono_stereo = CH_MONO;
	sp_obj.sample_rate = SR_16K;
	sp_obj.word_len = WL_16;
	sp_obj.direction = APP_LINE_OUT;
	start_heap = xPortGetFreeHeapSize();
	if(xTaskCreate(example_audio_ekho_thread, ((const char*)"example_audio_ekho_thread"), 20000, (void *)(&sp_obj), tskIDLE_PRIORITY + 1, NULL) != pdPASS)
	printf("\n\r%s xTaskCreate(example_audio_ekho_thread) failed", __FUNCTION__);
}

static void vApplicationMallocFailedHook(void)
{

	
}
#endif