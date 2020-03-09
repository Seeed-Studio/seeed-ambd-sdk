 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "mmf2_link.h"
#include "mmf2_siso.h"
#include "mmf2_miso.h"

#include "module_isp.h"
#include "module_h264.h"
#include "module_rtsp2.h"
#include "module_audio.h"
#include "module_g711.h"
#include "module_rtp.h"
#include "module_array.h"

#include "sample_doorbell_pcmu.h"

#include "mmf2_video_config.h"

static mm_context_t* isp_v1_ctx				= NULL;
static mm_context_t* h264_v1_ctx			= NULL;
static mm_context_t* rtsp2_v1_ctx			= NULL;
static mm_context_t* audio_ctx				= NULL;
static mm_context_t* g711e_ctx				= NULL;
static mm_context_t* g711d_ctx				= NULL;
static mm_context_t* rtp_ctx				= NULL;
static mm_context_t* array_ctx				= NULL;

static mm_siso_t* siso_audio_g711e			= NULL;
static mm_siso_t* siso_isp_h264_v1			= NULL;
static mm_miso_t* miso_h264_g711_rtsp		= NULL;
static mm_miso_t* miso_rtp_array_g711d		= NULL;
static mm_siso_t* siso_g711d_audio			= NULL;

static isp_params_t isp_v1_params = {
	.width    = V1_WIDTH, 
	.height   = V1_HEIGHT,
	.fps      = V1_FPS,
	.slot_num = V1_HW_SLOT,
	.buff_num = V1_SW_SLOT,
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

static audio_params_t audio_params = {
	.sample_rate = ASR_8KHZ,
	.word_length = WL_16BIT,
	.mic_gain    = MIC_40DB,
	.channel     = 1,
	.enable_aec  = 0
};

static g711_params_t g711e_params = {
	.codec_id = AV_CODEC_ID_PCMU,
	.buf_len = 2048,
	.mode     = G711_ENCODE
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

static rtsp2_params_t rtsp2_a_pcmu_params = {
	.type = AVMEDIA_TYPE_AUDIO,
	.u = {
		.a = {
			.codec_id   = AV_CODEC_ID_PCMU,
			.channel    = 1,
			.samplerate = 8000
		}
	}
};

static rtp_params_t rtp_g711d_params = {
	.valid_pt = 0xFFFFFFFF,
	.port = 16384,
	.frame_size = 1500,
	.cache_depth = 6
};

static g711_params_t g711d_params = {
	.codec_id = AV_CODEC_ID_PCMU,
	.buf_len = 2048,
	.mode     = G711_DECODE
};

static array_params_t doorbell_pcmu_array_params = {
	.type = AVMEDIA_TYPE_AUDIO,
	.codec_id = AV_CODEC_ID_PCMU,
	.mode = ARRAY_MODE_ONCE,
	.u = {
		.a = {
			.channel    = 1,
			.samplerate = 8000,
			.frame_size = 160,
		}
	}
};

void doorbell_ring(void)
{
	int state = 0;
	mm_module_ctrl(array_ctx, CMD_ARRAY_GET_STATE, (int)&state);
	if (state) {
		printf("doorbell is ringing\n\r");
	}
	else {
		printf("start doorbell_ring\n\r");
		miso_resume(miso_rtp_array_g711d);
		miso_pause(miso_rtp_array_g711d,MM_INPUT0);	// pause audio from rtp
		mm_module_ctrl(array_ctx, CMD_ARRAY_STREAMING, 1);	// doorbell ring
		
		do {	// wait until doorbell_ring done
			vTaskDelay(100);
			mm_module_ctrl(array_ctx, CMD_ARRAY_GET_STATE, (int)&state);
		} while (state==1);
		
		miso_resume(miso_rtp_array_g711d);
		miso_pause(miso_rtp_array_g711d,MM_INPUT1);	// pause array
		printf("doorbell_ring done!\n\r");
	}
}

void mmf2_example_h264_2way_audio_pcmu_doorbell_init(void)
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
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
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
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
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
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
	}
	
	g711e_ctx = mm_module_open(&g711_module);
	if(g711e_ctx){
		mm_module_ctrl(g711e_ctx, CMD_G711_SET_PARAMS, (int)&g711e_params);
		mm_module_ctrl(g711e_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(g711e_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(g711e_ctx, CMD_G711_APPLY, 0);
	}else{
		rt_printf("G711 open fail\n\r");
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
	}
	
	//--------------RTSP---------------
	rtsp2_v1_ctx = mm_module_open(&rtsp2_module);
	if(rtsp2_v1_ctx){
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_v1_params);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_APPLY, 0);
		
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SELECT_STREAM, 1);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_a_pcmu_params);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_APPLY, 0);
		
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_STREAMMING, ON);
		//mm_module_ctrl(rtsp2_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		//mm_module_ctrl(rtsp2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
	}else{
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
	}
	
	//--------------Link---------------------------
	siso_isp_h264_v1 = siso_create();
	if(siso_isp_h264_v1){
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_INPUT, (uint32_t)isp_v1_ctx, 0);
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_OUTPUT, (uint32_t)h264_v1_ctx, 0);
		siso_start(siso_isp_h264_v1);
	}else{
		rt_printf("siso_isp_h264_v1 open fail\n\r");
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
	}
	
	rt_printf("siso_isp_h264_v1 started\n\r");
	
	siso_audio_g711e = siso_create();
	if(siso_audio_g711e){
		siso_ctrl(siso_audio_g711e, MMIC_CMD_ADD_INPUT, (uint32_t)audio_ctx, 0);
		siso_ctrl(siso_audio_g711e, MMIC_CMD_ADD_OUTPUT, (uint32_t)g711e_ctx, 0);
		siso_start(siso_audio_g711e);
	}else{
		rt_printf("siso_audio_g711e open fail\n\r");
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
	}
	
	rt_printf("siso_audio_g711e started\n\r");
	
	miso_h264_g711_rtsp = miso_create();
	if(miso_h264_g711_rtsp){
		miso_ctrl(miso_h264_g711_rtsp, MMIC_CMD_ADD_INPUT0, (uint32_t)h264_v1_ctx, 0);
		miso_ctrl(miso_h264_g711_rtsp, MMIC_CMD_ADD_INPUT1, (uint32_t)g711e_ctx, 0);
		miso_ctrl(miso_h264_g711_rtsp, MMIC_CMD_ADD_OUTPUT, (uint32_t)rtsp2_v1_ctx, 0);
		miso_start(miso_h264_g711_rtsp);
	}else{
		rt_printf("miso_h264_g711_rtsp fail\n\r");
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
	}
	rt_printf("miso_h264_g711_rtsp started\n\r");

	// RTP audio
	rtp_ctx = mm_module_open(&rtp_module);
	if(rtp_ctx){
		mm_module_ctrl(rtp_ctx, CMD_RTP_SET_PARAMS, (int)&rtp_g711d_params);
		mm_module_ctrl(rtp_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(rtp_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(rtp_ctx, CMD_RTP_APPLY, 0);
		mm_module_ctrl(rtp_ctx, CMD_RTP_STREAMING, 1);	// streamming on
	}else{
		rt_printf("RTP open fail\n\r");
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
	}
	
	// Audio array input (doorbell)
	array_t array;
	array.data_addr = (uint32_t) doorbell_pcmu_sample;
	array.data_len = (uint32_t) doorbell_pcmu_sample_size;
	array_ctx = mm_module_open(&array_module);
	if(array_ctx){
		mm_module_ctrl(array_ctx, CMD_ARRAY_SET_PARAMS, (int)&doorbell_pcmu_array_params);
		mm_module_ctrl(array_ctx, CMD_ARRAY_SET_ARRAY, (int)&array);
		mm_module_ctrl(array_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(array_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(array_ctx, CMD_ARRAY_APPLY, 0);
		//mm_module_ctrl(array_ctx, CMD_ARRAY_STREAMING, 1);	// streamming on
	}else{
		rt_printf("ARRAY open fail\n\r");
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
	}
	
	// G711D
	g711d_ctx = mm_module_open(&g711_module);
	if(g711d_ctx){
		mm_module_ctrl(g711d_ctx, CMD_G711_SET_PARAMS, (int)&g711d_params);
		mm_module_ctrl(g711d_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(g711d_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(g711d_ctx, CMD_G711_APPLY, 0);
	}else{
		rt_printf("G711 open fail\n\r");
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
	}
	
	miso_rtp_array_g711d = miso_create();
	if(miso_rtp_array_g711d){
		miso_ctrl(miso_rtp_array_g711d, MMIC_CMD_ADD_INPUT0, (uint32_t)rtp_ctx, 0);
		miso_ctrl(miso_rtp_array_g711d, MMIC_CMD_ADD_INPUT1, (uint32_t)array_ctx, 0);
		miso_ctrl(miso_rtp_array_g711d, MMIC_CMD_ADD_OUTPUT, (uint32_t)g711d_ctx, 0);
		miso_start(miso_rtp_array_g711d);
	}else{
		rt_printf("miso_rtp_array_g711d open fail\n\r");
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
	}
	
	rt_printf("miso_rtp_array_g711d started\n\r");
	
	siso_g711d_audio = siso_create();
	if(siso_g711d_audio){
		siso_ctrl(siso_g711d_audio, MMIC_CMD_ADD_INPUT, (uint32_t)g711d_ctx, 0);
		siso_ctrl(siso_g711d_audio, MMIC_CMD_ADD_OUTPUT, (uint32_t)audio_ctx, 0);
		siso_start(siso_g711d_audio);
	}else{
		rt_printf("siso_g711d_audio open fail\n\r");
		goto mmf2_example_h264_two_way_audio_pcmu_fail;
	}
	
	rt_printf("siso_g711d_audio started\n\r");
	
	//printf("Delay 10s and then ring the doorbell\n\r");
	//vTaskDelay(10000);
	//doorbell_ring();
	
	snapshot_setting(h264_v1_ctx);
	
	return;
mmf2_example_h264_two_way_audio_pcmu_fail:
	
	return;
}