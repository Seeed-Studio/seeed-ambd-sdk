 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "mmf2_link.h"
#include "mmf2_siso.h"

#include "module_audio.h"
#include "module_aad.h"
#include "module_rtp.h"

static mm_context_t* rtp_ctx		= NULL;
static mm_context_t* aad_ctx		= NULL;
static mm_context_t* audio_ctx		= NULL;

static mm_siso_t* siso_rtp_aad		= NULL;
static mm_siso_t* siso_aad_audio	= NULL;

static audio_params_t audio_params = {
#if defined(CONFIG_PLATFORM_8721D)
	.sample_rate = SR_8K,
	.word_length = WL_16,
	.mono_stereo = CH_MONO,
	// .direction = APP_AMIC_IN|APP_LINE_OUT,
	.direction = APP_LINE_IN|APP_LINE_OUT,
#else
	.sample_rate = ASR_8KHZ,
	.word_length = WL_16BIT,
	.mic_gain    = MIC_40DB,
	.channel     = 1,
#endif
	.mix_mode = 0,
	.enable_aec  = 0
};

static aad_params_t aad_rtp_params = {
	.sample_rate = 8000,
	.channel = 1,
	.type = TYPE_RTP_RAW
};

static rtp_params_t rtp_aad_params = {
	.valid_pt = 0xFFFFFFFF,
	.port = 16384,
	.frame_size = 1500,
	.cache_depth = 6
};

void mmf2_example_rtp_aad_init(void)
{
	rtp_ctx = mm_module_open(&rtp_module);
	if(rtp_ctx){
		mm_module_ctrl(rtp_ctx, CMD_RTP_SET_PARAMS, (int)&rtp_aad_params);
		mm_module_ctrl(rtp_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(rtp_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(rtp_ctx, CMD_RTP_APPLY, 0);
		mm_module_ctrl(rtp_ctx, CMD_RTP_STREAMING, 1);	// streamming on
	}else{
		rt_printf("RTP open fail\n\r");
		goto mmf2_exmaple_rtp_aad_fail;
	}

	aad_ctx = mm_module_open(&aad_module);
	if(aad_ctx){
		mm_module_ctrl(aad_ctx, CMD_AAD_SET_PARAMS, (int)&aad_rtp_params);
		mm_module_ctrl(aad_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(aad_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(aad_ctx, CMD_AAD_APPLY, 0);
	}else{
		rt_printf("AAD open fail\n\r");
		goto mmf2_exmaple_rtp_aad_fail;
	}
	
	audio_ctx = mm_module_open(&audio_module);
	if(audio_ctx){
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
		//mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		//mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
	}else{
		rt_printf("audio open fail\n\r");
		goto mmf2_exmaple_rtp_aad_fail;
	}
	
	siso_rtp_aad = siso_create();
	if(siso_rtp_aad){
		siso_ctrl(siso_rtp_aad, MMIC_CMD_ADD_INPUT, (uint32_t)rtp_ctx, 0);
		siso_ctrl(siso_rtp_aad, MMIC_CMD_ADD_OUTPUT, (uint32_t)aad_ctx, 0);
		siso_start(siso_rtp_aad);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_exmaple_rtp_aad_fail;
	}
	
	rt_printf("siso1 started\n\r");

	siso_rtp_aad = siso_create();
	if(siso_rtp_aad){
		siso_ctrl(siso_rtp_aad, MMIC_CMD_ADD_INPUT, (uint32_t)aad_ctx, 0);
		siso_ctrl(siso_rtp_aad, MMIC_CMD_ADD_OUTPUT, (uint32_t)audio_ctx, 0);
		siso_start(siso_rtp_aad);
	}else{
		rt_printf("siso2 open fail\n\r");
		goto mmf2_exmaple_rtp_aad_fail;
	}
	
	rt_printf("siso2 started\n\r");

	return;
mmf2_exmaple_rtp_aad_fail:
	
	return;

}