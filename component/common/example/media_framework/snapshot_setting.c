 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "mmf2_video_config.h"
#include "platform_stdlib.h"

#if ENABLE_SNAPSHOT
#include "module_jpeg.h"
#include "module_h264.h"
#include "jpeg_snapshot.h"
#include "snapshot_tftp_handler.h"

u8* snapshot_buffer = NULL;
static void* ctx = NULL;
#endif

void snapshot_setting(void* p)
{
#if ENABLE_SNAPSHOT

#if ENABLE_V1_SNAPSHOT
	ctx = (h264_ctx_t*)p;
#elif ENABLE_V3_SNAPSHOT_WHEN_STREAMING
	ctx = (jpeg_ctx_t*)p;
#endif

	snapshot_buffer = malloc(SNAPSHOT_BUFFER_SIZE);
	if(snapshot_buffer == NULL){
		printf("malloc snapshot_buffer fail\r\n");
		while(1);
	}
#if ENABLE_V1_SNAPSHOT
	mm_module_ctrl(ctx,CMD_SNAPSHOT_ENCODE_CB, (int)jpeg_snapshot_encode_cb);
#elif ENABLE_V3_SNAPSHOT_WHEN_STREAMING
	mm_module_ctrl(ctx, CMD_SNAPSHOT_CB, (int)jpeg_snapshot_cb);
#endif

#if ENABLE_V3_SNAPSHOT_WHEN_STREAMING
	void* jpeg_encoder = NULL;
	mm_module_ctrl(ctx, CMD_JPEG_GET_ENCODER, (int)&jpeg_encoder);
	jpeg_snapshot_initial_with_instance(jpeg_encoder, (u32)snapshot_buffer, SNAPSHOT_BUFFER_SIZE,1);
#else
	jpeg_snapshot_initial(SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, SNAPSHOT_FPS, SNAPSHOT_LEVEL, (u32)snapshot_buffer, SNAPSHOT_BUFFER_SIZE);
#endif

#if ENABLE_V3_JPEG == V3_JPEG_SNAPSHOT
	jpeg_snapshot_isp_config(2);
	printf("Video 3 JPEG_LEVEL %d, ISP_STREAMID 2 (SNAPSHOT MODE)\n\r",V3_JPEG_LEVEL);
#endif
#ifdef SNAPSHOT_TFTP_TYPE
	jpeg_snapshot_create_tftp_thread();
#else
	jpeg_snapshot_create_sd_thread();
#endif
#endif
}