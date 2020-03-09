 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "mmf2_link.h"
#include "mmf2_siso.h"
#include "avcodec.h"

#include "module_audio.h"
#include "module_g711.h"

static mm_context_t* audio_ctx            = NULL;
static mm_context_t* g711e_ctx      	  = NULL;
static mm_context_t* g711d_ctx       	  = NULL;
static mm_siso_t* siso_audio_g711e          = NULL;
static mm_siso_t* siso_g711_e2d			  = NULL;
static mm_siso_t* siso_g711d_audio     	  = NULL;

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

static g711_params_t g711e_params = {
	.codec_id = AV_CODEC_ID_PCMU,
	.buf_len = 2048,
	.mode     = G711_ENCODE
};

static g711_params_t g711d_params = {
	.codec_id = AV_CODEC_ID_PCMU,
	.buf_len = 2048,
	.mode     = G711_DECODE
};

void mmf2_example_g711loop_init(void)
{
	audio_ctx = mm_module_open(&audio_module);
	if(audio_ctx){
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
		mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
	}else{
		rt_printf("audio open fail\n\r");
		goto mmf2_exmaple_g711loop_fail;
	}

	g711e_ctx = mm_module_open(&g711_module);
	if(g711e_ctx){
		mm_module_ctrl(g711e_ctx, CMD_G711_SET_PARAMS, (int)&g711e_params);
		mm_module_ctrl(g711e_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(g711e_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(g711e_ctx, CMD_G711_APPLY, 0);
	}else{
		rt_printf("G711 open fail\n\r");
		goto mmf2_exmaple_g711loop_fail;
	}	
	
	
	g711d_ctx = mm_module_open(&g711_module);
	if(g711d_ctx){
		mm_module_ctrl(g711d_ctx, CMD_G711_SET_PARAMS, (int)&g711d_params);
		mm_module_ctrl(g711d_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(g711d_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(g711d_ctx, CMD_G711_APPLY, 0);
	}else{
		rt_printf("G711 open fail\n\r");
		goto mmf2_exmaple_g711loop_fail;
	}
	
	
	siso_audio_g711e = siso_create();
	if(siso_audio_g711e){
		siso_ctrl(siso_audio_g711e, MMIC_CMD_ADD_INPUT, (uint32_t)audio_ctx, 0);
		siso_ctrl(siso_audio_g711e, MMIC_CMD_ADD_OUTPUT, (uint32_t)g711e_ctx, 0);
		siso_start(siso_audio_g711e);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_exmaple_g711loop_fail;
	}
	
	
	siso_g711_e2d = siso_create();
	if(siso_g711_e2d){
		siso_ctrl(siso_g711_e2d, MMIC_CMD_ADD_INPUT, (uint32_t)g711e_ctx, 0);
		siso_ctrl(siso_g711_e2d, MMIC_CMD_ADD_OUTPUT, (uint32_t)g711d_ctx, 0);
		siso_start(siso_g711_e2d);
	}else{
		rt_printf("siso2 open fail\n\r");
		goto mmf2_exmaple_g711loop_fail;
	}
	
	siso_g711d_audio = siso_create();
	if(siso_g711d_audio){
		siso_ctrl(siso_g711d_audio, MMIC_CMD_ADD_INPUT, (uint32_t)g711d_ctx, 0);
		siso_ctrl(siso_g711d_audio, MMIC_CMD_ADD_OUTPUT, (uint32_t)audio_ctx, 0);
		siso_start(siso_g711d_audio);
	}else{
		rt_printf("siso3 open fail\n\r");
		goto mmf2_exmaple_g711loop_fail;
	}
	
	rt_printf("siso1 started\n\r");


	return;
mmf2_exmaple_g711loop_fail:
	
	return;
}