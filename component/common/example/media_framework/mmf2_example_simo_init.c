 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "mmf2_link.h"
#include "mmf2_siso.h"
#include "mmf2_simo.h"

#include "module_isp.h"
#include "module_h264.h"
#include "module_rtsp2.h"

#include "mmf2_video_config.h"
#include "sensor.h"

static mm_context_t* isp_v2_ctx				= NULL;
static mm_context_t* h264_v2_ctx			= NULL;
static mm_context_t* rtsp2_v2_ctx			= NULL;
static mm_context_t* rtsp2_v3_ctx			= NULL;

static mm_siso_t* siso_isp_h264_v2			= NULL;
static mm_simo_t* simo_h264_rtsp_v2_v3		= NULL;

static isp_params_t isp_v2_params = {
	.width    = V2_WIDTH, 
	.height   = V2_HEIGHT,
	.fps      = V2_FPS,
	.slot_num = V2_HW_SLOT,
	.buff_num = V2_SW_SLOT,
	.format   = ISP_FORMAT_YUV420_SEMIPLANAR
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

void mmf2_example_simo_init(void)
{
	isp_v2_ctx = mm_module_open(&isp_module);
	if(isp_v2_ctx){
		mm_module_ctrl(isp_v2_ctx, CMD_ISP_SET_PARAMS, (int)&isp_v2_params);
		mm_module_ctrl(isp_v2_ctx, MM_CMD_SET_QUEUE_LEN, V2_SW_SLOT);
		mm_module_ctrl(isp_v2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(isp_v2_ctx, CMD_ISP_APPLY, 0);	// start channel 0
	}else{
		rt_printf("ISP open fail\n\r");
		goto mmf2_exmaple_simo_fail;
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
		goto mmf2_exmaple_simo_fail;
	}
	
	rtsp2_v2_ctx = mm_module_open(&rtsp2_module);
	if(rtsp2_v2_ctx){
		mm_module_ctrl(rtsp2_v2_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_v2_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_v2_params);
		mm_module_ctrl(rtsp2_v2_ctx, CMD_RTSP2_SET_APPLY, 0);
		mm_module_ctrl(rtsp2_v2_ctx, CMD_RTSP2_SET_STREAMMING, ON);
		//mm_module_ctrl(rtsp2_v2_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		//mm_module_ctrl(rtsp2_v2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
	}else{
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_exmaple_simo_fail;
	}
	
	rtsp2_v3_ctx = mm_module_open(&rtsp2_module);
	if(rtsp2_v3_ctx){
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_v2_params);
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SET_APPLY, 0);
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SET_STREAMMING, ON);
		//mm_module_ctrl(rtsp2_v2_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		//mm_module_ctrl(rtsp2_v2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
	}else{
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_exmaple_simo_fail;
	}	
	
	siso_isp_h264_v2 = siso_create();
	if(siso_isp_h264_v2){
		siso_ctrl(siso_isp_h264_v2, MMIC_CMD_ADD_INPUT, (uint32_t)isp_v2_ctx, 0);
		siso_ctrl(siso_isp_h264_v2, MMIC_CMD_ADD_OUTPUT, (uint32_t)h264_v2_ctx, 0);
		siso_start(siso_isp_h264_v2);
	}else{
		rt_printf("siso3 open fail\n\r");
		goto mmf2_exmaple_simo_fail;
	}	
	
	simo_h264_rtsp_v2_v3 = simo_create();
	if(simo_h264_rtsp_v2_v3){
		simo_ctrl(simo_h264_rtsp_v2_v3, MMIC_CMD_ADD_INPUT, (uint32_t)h264_v2_ctx, 0);
		simo_ctrl(simo_h264_rtsp_v2_v3, MMIC_CMD_ADD_OUTPUT0, (uint32_t)rtsp2_v2_ctx, 0);
		simo_ctrl(simo_h264_rtsp_v2_v3, MMIC_CMD_ADD_OUTPUT1, (uint32_t)rtsp2_v3_ctx, 0);
		simo_start(simo_h264_rtsp_v2_v3);
	}else{
		rt_printf("simo open fail\n\r");
		goto mmf2_exmaple_simo_fail;
	}	
	return;
mmf2_exmaple_simo_fail:
	
	return;
}