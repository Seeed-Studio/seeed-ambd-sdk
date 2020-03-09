 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "mmf2_link.h"
#include "mmf2_siso.h"

#include "module_audio.h"

static mm_context_t* audio_ctx = NULL;
static mm_siso_t* siso_audio_loop       = NULL;

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

void mmf2_example_audioloop_init(void)
{
	audio_ctx = mm_module_open(&audio_module);
	if(audio_ctx){
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
		mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
	}else{
		rt_printf("audio open fail\n\r");
		goto mmf2_exmaple_audioloop_fail;
	}

	
	siso_audio_loop = siso_create();
	if(siso_audio_loop){
		siso_ctrl(siso_audio_loop, MMIC_CMD_ADD_INPUT, (uint32_t)audio_ctx, 0);
		siso_ctrl(siso_audio_loop, MMIC_CMD_ADD_OUTPUT, (uint32_t)audio_ctx, 0);
		siso_start(siso_audio_loop);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_exmaple_audioloop_fail;
	}
	
	rt_printf("siso1 started\n\r");


	return;
mmf2_exmaple_audioloop_fail:
	
	return;
}