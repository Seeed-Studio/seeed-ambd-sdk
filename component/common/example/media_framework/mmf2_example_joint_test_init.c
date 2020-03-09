 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "mmf2_link.h"
#include "mmf2_siso.h"
#include "mmf2_mimo.h"

#include "module_isp.h"
#include "module_h264.h"
#include "module_rtsp2.h"
#include "module_audio.h"
#include "module_aac.h"
#include "module_aad.h"
#include "module_rtp.h"

#include "mmf2_video_config.h"
#include "sensor.h"

#define RTSP4 0

static mm_context_t* isp_v1_ctx				= NULL;
static mm_context_t* isp_v2_ctx				= NULL;
static mm_context_t* h264_v1_ctx			= NULL;
static mm_context_t* h264_v2_ctx			= NULL;
static mm_context_t* rtsp2_v1_ctx			= NULL;
static mm_context_t* rtsp2_v2_ctx			= NULL;
static mm_context_t* rtsp2_v3_ctx			= NULL;
static mm_context_t* rtsp2_v4_ctx			= NULL;
static mm_context_t* audio_ctx				= NULL;
static mm_context_t* aac_ctx				= NULL;
static mm_context_t* rtp_ctx				= NULL;
static mm_context_t* aad_ctx				= NULL;

static mm_siso_t* siso_audio_aac			= NULL;
static mm_siso_t* siso_isp_h264_v1			= NULL;
static mm_siso_t* siso_isp_h264_v2			= NULL;
static mm_mimo_t* mimo_2v_1a_rtsp			= NULL;
static mm_siso_t* siso_rtp_aad				= NULL;
static mm_siso_t* siso_aad_audio			= NULL;

static isp_params_t isp_v1_params = {
	.width    = V1_WIDTH, 
	.height   = V1_HEIGHT,
	.fps      = V1_FPS,
	.slot_num = V1_HW_SLOT,
	.buff_num = V1_SW_SLOT,
	.format   = ISP_FORMAT_YUV420_SEMIPLANAR
};

static isp_params_t isp_v2_params = {
	.width    = V2_WIDTH, 
	.height   = V2_HEIGHT,
	.fps      = V2_FPS,
	.slot_num = V2_HW_SLOT,
	.buff_num = V2_SW_SLOT,
	.format   = ISP_FORMAT_YUV420_SEMIPLANAR
};

static h264_params_t h264_v1_params = {
	.width          = V1_WIDTH,
	.height         = V1_HEIGHT,
	.bps            = V1_BITRATE,
	.fps            = V1_FPS,
	.gop            = V1_FPS,
	.rc_mode        = V1_H264_RCMODE,
	.mem_total_size = V1_BUFFER_SIZE,
	.mem_block_size = V1_BLOCK_SIZE,
	.mem_frame_size = V1_FRAME_SIZE
};

static h264_params_t h264_v2_params = {
	.width          = V2_WIDTH,
	.height         = V2_HEIGHT,
	.bps            = V2_BITRATE,
	.fps            = V2_FPS,
	.gop            = V2_FPS,
	.rc_mode        = V2_H264_RCMODE,
	.mem_total_size = V2_BUFFER_SIZE,
	.mem_block_size = V2_BLOCK_SIZE,
	.mem_frame_size = V2_FRAME_SIZE
};

static audio_params_t audio_params = {
	.sample_rate = ASR_8KHZ,
	.word_length = WL_16BIT,
	.mic_gain    = MIC_40DB,
	.channel     = 1,
	.enable_aec  = 0
};

static aac_params_t aac_params = {
	.sample_rate = 8000,
	.channel = 1,
	.bit_length = FAAC_INPUT_16BIT,
	.mpeg_version = MPEG4,
	.mem_total_size = 10*1024,
	.mem_block_size = 128,
	.mem_frame_size = 1024
};

static rtsp2_params_t rtsp2_v1_params = {
	.type = AVMEDIA_TYPE_VIDEO,
	.u = {
		.v = {
			.codec_id = AV_CODEC_ID_H264,
			.fps      = V1_FPS,
			.bps      = V1_BITRATE
		}
	}
};

static rtsp2_params_t rtsp2_v2_params = {
	.type = AVMEDIA_TYPE_VIDEO,
	.u = {
		.v = {
			.codec_id = AV_CODEC_ID_H264,
			.fps      = V2_FPS,
			.bps      = V2_BITRATE
		}
	}
};

static rtsp2_params_t rtsp2_a_params = {
	.type = AVMEDIA_TYPE_AUDIO,
	.u = {
		.a = {
			.codec_id   = AV_CODEC_ID_MP4A_LATM,
			.channel    = 1,
			.samplerate = 8000
		}
	}
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

void mmf2_example_joint_test_init(void)
{
	// ------ Channel 1--------------
	isp_v1_ctx = mm_module_open(&isp_module);
	if(isp_v1_ctx){
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_SET_PARAMS, (int)&isp_v1_params);
		mm_module_ctrl(isp_v1_ctx, MM_CMD_SET_QUEUE_LEN, V1_SW_SLOT);
		mm_module_ctrl(isp_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_APPLY, 0);	// start channel 0
	}else{
		rt_printf("ISP open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}

	h264_v1_ctx = mm_module_open(&h264_module);
	if(h264_v1_ctx){
		mm_module_ctrl(h264_v1_ctx, CMD_H264_SET_PARAMS, (int)&h264_v1_params);
		mm_module_ctrl(h264_v1_ctx, MM_CMD_SET_QUEUE_LEN, V1_H264_QUEUE_LEN);
		mm_module_ctrl(h264_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(h264_v1_ctx, CMD_H264_INIT_MEM_POOL, 0);
		mm_module_ctrl(h264_v1_ctx, CMD_H264_APPLY, 0);
	}else{
		rt_printf("H264 open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}
	

	
	// ------ Channel 2--------------
	isp_v2_ctx = mm_module_open(&isp_module);
	if(isp_v2_ctx){
		mm_module_ctrl(isp_v2_ctx, CMD_ISP_SET_PARAMS, (int)&isp_v2_params);
		mm_module_ctrl(isp_v2_ctx, MM_CMD_SET_QUEUE_LEN, V2_SW_SLOT);
		mm_module_ctrl(isp_v2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(isp_v2_ctx, CMD_ISP_APPLY, 1);	// start channel 1
	}else{
		rt_printf("ISP open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}

	h264_v2_ctx = mm_module_open(&h264_module);
	if(h264_v2_ctx){
		mm_module_ctrl(h264_v2_ctx, CMD_H264_SET_PARAMS, (int)&h264_v2_params);
		mm_module_ctrl(h264_v2_ctx, MM_CMD_SET_QUEUE_LEN, V2_H264_QUEUE_LEN);
		mm_module_ctrl(h264_v2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(h264_v2_ctx, CMD_H264_INIT_MEM_POOL, 0);
		mm_module_ctrl(h264_v2_ctx, CMD_H264_APPLY, 0);
	}else{
		rt_printf("H264 open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}
	
	//--------------Audio --------------
	audio_ctx = mm_module_open(&audio_module);
	if(audio_ctx){
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
		mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
	}else{
		rt_printf("audio open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}

	aac_ctx = mm_module_open(&aac_module);
	if(aac_ctx){
		mm_module_ctrl(aac_ctx, CMD_AAC_SET_PARAMS, (int)&aac_params);
		mm_module_ctrl(aac_ctx, MM_CMD_SET_QUEUE_LEN, 16);
		mm_module_ctrl(aac_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(aac_ctx, CMD_AAC_INIT_MEM_POOL, 0);
		mm_module_ctrl(aac_ctx, CMD_AAC_APPLY, 0);
	}else{
		rt_printf("AAC open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}
	

	//--------------RTSP---------------
	rtsp2_v1_ctx = mm_module_open(&rtsp2_module);
	if(rtsp2_v1_ctx){
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_v1_params);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_APPLY, 0);

		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SELECT_STREAM, 1);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_a_params);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_APPLY, 0);
		
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_STREAMMING, ON);
		//mm_module_ctrl(rtsp2_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		//mm_module_ctrl(rtsp2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
	}else{
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}
	

	rtsp2_v2_ctx = mm_module_open(&rtsp2_module);
	if(rtsp2_v2_ctx){
		mm_module_ctrl(rtsp2_v2_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_v2_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_v2_params);
		mm_module_ctrl(rtsp2_v2_ctx, CMD_RTSP2_SET_APPLY, 0);
		
		mm_module_ctrl(rtsp2_v2_ctx, CMD_RTSP2_SELECT_STREAM, 1);
		mm_module_ctrl(rtsp2_v2_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_a_params);
		mm_module_ctrl(rtsp2_v2_ctx, CMD_RTSP2_SET_APPLY, 0);
		
		mm_module_ctrl(rtsp2_v2_ctx, CMD_RTSP2_SET_STREAMMING, ON);
		//mm_module_ctrl(rtsp2_v2_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		//mm_module_ctrl(rtsp2_v2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
	}else{
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}

#if RTSP4==1
	rtsp2_v3_ctx = mm_module_open(&rtsp2_module);
	if(rtsp2_v3_ctx){
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_v1_params);
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SET_APPLY, 0);

		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SELECT_STREAM, 1);
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_a_params);
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SET_APPLY, 0);
		
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SET_STREAMMING, ON);
		//mm_module_ctrl(rtsp2_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		//mm_module_ctrl(rtsp2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
	}else{
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}
	

	rtsp2_v4_ctx = mm_module_open(&rtsp2_module);
	if(rtsp2_v4_ctx){
		mm_module_ctrl(rtsp2_v4_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_v4_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_v2_params);
		mm_module_ctrl(rtsp2_v4_ctx, CMD_RTSP2_SET_APPLY, 0);
		
		mm_module_ctrl(rtsp2_v4_ctx, CMD_RTSP2_SELECT_STREAM, 1);
		mm_module_ctrl(rtsp2_v4_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_a_params);
		mm_module_ctrl(rtsp2_v4_ctx, CMD_RTSP2_SET_APPLY, 0);
		
		mm_module_ctrl(rtsp2_v4_ctx, CMD_RTSP2_SET_STREAMMING, ON);
		//mm_module_ctrl(rtsp2_v2_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		//mm_module_ctrl(rtsp2_v2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
	}else{
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}	
#endif
	//--------------Link---------------------------
	siso_isp_h264_v1 = siso_create();
	if(siso_isp_h264_v1){
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_INPUT, (uint32_t)isp_v1_ctx, 0);
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_OUTPUT, (uint32_t)h264_v1_ctx, 0);
		siso_start(siso_isp_h264_v1);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}

	siso_isp_h264_v2 = siso_create();
	if(siso_isp_h264_v2){
		siso_ctrl(siso_isp_h264_v2, MMIC_CMD_ADD_INPUT, (uint32_t)isp_v2_ctx, 0);
		siso_ctrl(siso_isp_h264_v2, MMIC_CMD_ADD_OUTPUT, (uint32_t)h264_v2_ctx, 0);
		siso_start(siso_isp_h264_v2);
	}else{
		rt_printf("siso2 open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}	
	
	siso_audio_aac = siso_create();
	if(siso_audio_aac){
		siso_ctrl(siso_audio_aac, MMIC_CMD_ADD_INPUT, (uint32_t)audio_ctx, 0);
		siso_ctrl(siso_audio_aac, MMIC_CMD_ADD_OUTPUT, (uint32_t)aac_ctx, 0);
		siso_start(siso_audio_aac);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}
	
	
	rt_printf("siso started\n\r");
	
	mimo_2v_1a_rtsp = mimo_create();
	if(mimo_2v_1a_rtsp){
		mimo_ctrl(mimo_2v_1a_rtsp, MMIC_CMD_ADD_INPUT0, (uint32_t)h264_v1_ctx, 0);
		mimo_ctrl(mimo_2v_1a_rtsp, MMIC_CMD_ADD_INPUT1, (uint32_t)h264_v2_ctx, 0);
		mimo_ctrl(mimo_2v_1a_rtsp, MMIC_CMD_ADD_INPUT2, (uint32_t)aac_ctx, 0);
		mimo_ctrl(mimo_2v_1a_rtsp, MMIC_CMD_ADD_OUTPUT0, (uint32_t)rtsp2_v1_ctx, MMIC_DEP_INPUT0|MMIC_DEP_INPUT2);
		mimo_ctrl(mimo_2v_1a_rtsp, MMIC_CMD_ADD_OUTPUT1, (uint32_t)rtsp2_v2_ctx, MMIC_DEP_INPUT1|MMIC_DEP_INPUT2);
#if RTSP4==1
		mimo_ctrl(mimo_2v_1a_rtsp, MMIC_CMD_ADD_OUTPUT2, (uint32_t)rtsp2_v3_ctx, MMIC_DEP_INPUT0|MMIC_DEP_INPUT2);
		mimo_ctrl(mimo_2v_1a_rtsp, MMIC_CMD_ADD_OUTPUT3, (uint32_t)rtsp2_v4_ctx, MMIC_DEP_INPUT1|MMIC_DEP_INPUT2);
#endif
		mimo_start(mimo_2v_1a_rtsp);
	}else{
		rt_printf("mimo open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}
	rt_printf("mimo started\n\r");

	// RTP audio

	rtp_ctx = mm_module_open(&rtp_module);
	if(rtp_ctx){
		mm_module_ctrl(rtp_ctx, CMD_RTP_SET_PARAMS, (int)&rtp_aad_params);
		mm_module_ctrl(rtp_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(rtp_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(rtp_ctx, CMD_RTP_APPLY, 0);
		mm_module_ctrl(rtp_ctx, CMD_RTP_STREAMING, 1);	// streamming on
	}else{
		rt_printf("RTP open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}

	aad_ctx = mm_module_open(&aad_module);
	if(aad_ctx){
		mm_module_ctrl(aad_ctx, CMD_AAD_SET_PARAMS, (int)&aad_rtp_params);
		mm_module_ctrl(aad_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(aad_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(aad_ctx, CMD_AAD_APPLY, 0);
	}else{
		rt_printf("AAD open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}	
	
	siso_rtp_aad = siso_create();
	if(siso_rtp_aad){
		siso_ctrl(siso_rtp_aad, MMIC_CMD_ADD_INPUT, (uint32_t)rtp_ctx, 0);
		siso_ctrl(siso_rtp_aad, MMIC_CMD_ADD_OUTPUT, (uint32_t)aad_ctx, 0);
		siso_start(siso_rtp_aad);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}
	
	rt_printf("siso3 started\n\r");

	siso_aad_audio = siso_create();
	if(siso_aad_audio){
		siso_ctrl(siso_aad_audio, MMIC_CMD_ADD_INPUT, (uint32_t)aad_ctx, 0);
		siso_ctrl(siso_aad_audio, MMIC_CMD_ADD_OUTPUT, (uint32_t)audio_ctx, 0);
		siso_start(siso_aad_audio);
	}else{
		rt_printf("siso2 open fail\n\r");
		goto mmf2_exmaple_joint_test_fail;
	}	
	
	snapshot_setting(h264_v1_ctx);
	
	return;
mmf2_exmaple_joint_test_fail:
	
	return;

}