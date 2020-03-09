/*
 * audio_out_win.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "a52dec/include/config.h"

#ifdef LIBAO_SPORT

#include <stdio.h>
#include <stdlib.h>
//#include <fcntl.h>
//#include <windows.h>
//#include <mmsystem.h>
#include <inttypes.h>

#include "a52.h"
#include "audio_out.h"
#include "audio_out_internal.h"

#include "audio_out_sport.h"
#include <platform/platform_stdlib.h>
#include "rl6548.h"

#if CONFIG_EXAMPLE_AUDIO_AC3
static void sport_close (ao_instance_t * _instance);

typedef struct sport_instance_s {
    ao_instance_t ao;
    int16_t tmp_buffer[256*2];
    int audio_cnt;
    int sample_rate;
    int set_params;
    int flags;
}sport_instance_t;

static SP_InitTypeDef SP_InitStruct;
static SP_GDMA_STRUCT SPGdmaStruct;
static SP_OBJ sp_obj;
static SP_TX_INFO sp_tx_info;
//use static pages to transfer data
#if 0
static u8 sp_tx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];
static u8 sp_zero_buf[SP_ZERO_BUF_SIZE];
//malloc memory for pages to transfer data
#else
static u8 *sp_tx_buf;
static u8 *sp_zero_buf;
#endif
static SP_OBJ sp_obj;
//define a struct to save audio file information
typedef struct _file_info {
	u32 sampling_freq;
	u32 word_len;
	u32 sr_value;
	u8 *wav;
}file_info;
extern file_info audio_files[];
extern u8 file_index;

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
	ptx_block->tx_gdma_own = 1;
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
}

static void sp_init_tx_variables(void)
{
	int i;

	sp_tx_buf = malloc(SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM);
	sp_zero_buf = malloc(SP_ZERO_BUF_SIZE);
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

static int sp_rate_mapping(int sample_rate)
{
	int ret_val;
	switch(sample_rate){
		case 48000:
			ret_val = SR_48K;
			break;
		case 96000:
			ret_val = SR_96K;
			break;
		case 32000:
			ret_val = SR_32K;
			break;
		case 16000:
			ret_val = SR_16K;
			break;
		case 8000:
			ret_val = SR_8K;
			break;
		case 44100:
			ret_val = SR_44P1K;
			break;
		case 88200:
			ret_val = SR_88P2K;
			break;
		default:
			ret_val = 0xFF;
			break;
	}	
	return ret_val;
}

static int sport_setup (ao_instance_t * _instance, int sample_rate, int * flags,
		      sample_t * level, sample_t * bias)
{
	
	u32 tx_addr;
	u32 tx_length;
	pSP_OBJ	psp_obj = &sp_obj;
	u32 div;
	
	//DBG_8195A("setup\n");

#if 1
    sport_instance_t * instance = (sport_instance_t *) _instance;

    if ((instance->set_params == 0) && (instance->sample_rate != sample_rate))
	return 1;
    //instance->current_buffer = 0;

    *flags = instance->flags;
    *level = 1;
    *bias = 384;
#endif
	if (instance->sample_rate !=sample_rate){
	    instance->sample_rate = sample_rate;
		psp_obj->sample_rate = sp_rate_mapping(instance->sample_rate);

		//DBG_8195A("rate %d\n", instance->sample_rate);
		
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
			default:
				DBG_8195A("sample rate not supported!!\n");
				break;
		}
		PLL_Div(div);
		AUDIO_SI_WriteReg(0x17, (psp_obj->sample_rate << 4) | psp_obj->sample_rate);

	}

    return 0;
}

static int sport_play (ao_instance_t * _instance, int flags, sample_t * _samples)
{

    sport_instance_t * instance = (sport_instance_t *) _instance;
    float * samples = _samples;
    static int time1;
    int time2;
	int deltaCycle;
    static int totalCycle = 0;
    static int sec = 0;
    int CpuClk = CPU_ClkGet(FALSE) / 1000000;

    //float2s16_2(_samples, &instance->audio_buffer[instance->audio_cnt*2]);
#if 0    
    float2s16_2(samples, instance->tmp_buffer);
    memcpy(&instance->audio_buffer[instance->audio_cnt*2], instance->tmp_buffer, 256*2*sizeof(int16_t));
    instance->audio_cnt += 256;
    if ((instance->audio_cnt+256) >= 48000*2){
        fprintf(stderr, "audio buffer overflow\n");
        while(1){};
        //return 1;
        //instance->audio_cnt = 0;
    }
    else{
        //fprintf(stderr, "audio_cnt = %x\n", instance->audio_cnt);
    }
#else
    float2s16_1(samples, instance->tmp_buffer);

#if !MCPS_EVAL
	while(1){		
		if(sp_get_free_tx_page()){
			sp_write_tx_page((u8 *)instance->tmp_buffer, SP_DMA_PAGE_SIZE);
			break;
		}
		else{
			vTaskDelay(1);
		}		
	}	
#else
	/*
	if (instance->audio_cnt == 0){
		time1 = RTIM_GetCount(TIM0);
	}
	
	instance->audio_cnt += 256;
	if (instance->audio_cnt > (48000+256)){
		instance->audio_cnt = 0;
		time2 = RTIM_GetCount(TIM0);
		deltaCycle = (time2-time1)*32*CpuClk/1000;
		totalCycle += deltaCycle;
		sec++;
		
		DBG_8195A("cur MCPS: %d, avg MCPS: %d\n", deltaCycle, totalCycle/sec);
		
	}*/
#endif

#if 0
    memcpy(&instance->audio_buffer[instance->audio_cnt], instance->tmp_buffer, 256*sizeof(int16_t));
    instance->audio_cnt += 256;
    if ((instance->audio_cnt+256) > 48000){
		dbg = 2;
        //fprintf(stderr, "audio buffer overflow\n");
        //while(1){};
        //return 1;
        instance->audio_cnt = 0;
    }
    else{
        //fprintf(stderr, "audio_cnt = %x\n", instance->audio_cnt);
    }
#endif

#endif
    return 0;
}

static void sport_close (ao_instance_t * _instance)
{
	AUDIO_SP_TdmaCmd(AUDIO_SPORT_DEV, DISABLE);
	AUDIO_SP_TxStart(AUDIO_SPORT_DEV, DISABLE);
	GDMA_ClearINT(SPGdmaStruct.SpTxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpTxGdmaInitStruct.GDMA_ChNum);
	GDMA_Cmd(SPGdmaStruct.SpTxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpTxGdmaInitStruct.GDMA_ChNum, DISABLE);
	GDMA_ChnlFree(SPGdmaStruct.SpTxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpTxGdmaInitStruct.GDMA_ChNum);
	free(sp_tx_buf);
	free(sp_zero_buf);
	free(_instance);
		return;
#if 0	
    win_instance_t * instance = (win_instance_t *) _instance;
    int i;

    waveOutReset (instance->h_waveout);

    for (i = 0; i < NUMBUF; i++)
	waveOutUnprepareHeader (instance->h_waveout,
				&instance->waveheader[i],
				sizeof(WAVEHDR));
    waveOutClose (instance->h_waveout);
#endif
	
}

static ao_instance_t * sport_open (int flags)
{
    sport_instance_t * instance;
	pSP_OBJ	psp_obj = &sp_obj;
	u32 tx_addr;
	u32 tx_length;
    instance = malloc (sizeof (sport_instance_t));
    if (instance == NULL)
	return NULL;

    instance->ao.setup = sport_setup;
    instance->ao.play = sport_play;
    instance->ao.close = sport_close;

#if 1
    instance -> sample_rate = audio_files[file_index].sampling_freq;
    instance->set_params = 1;
    instance->flags = flags;
    instance->audio_cnt = 0;
#endif


	psp_obj->direction = APP_LINE_OUT;
	psp_obj->mono_stereo = CH_MONO;
	psp_obj->sample_rate = sp_rate_mapping(instance->sample_rate);
	psp_obj -> word_len = audio_files[file_index].word_len;
	printf("instance->sample_rate %d\n", instance->sample_rate);
	printf("psp_obj sample_rate %d\n", psp_obj->sample_rate);
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

#if MCPS_EVAL
    RTIM_Cmd(TIM0, ENABLE);
#endif

    return (ao_instance_t *) instance;
}

ao_instance_t * ao_sport_open (void)
{
    //return mem_open (A52_STEREO);
    return sport_open(A52_MONO);
}

#endif

#endif
