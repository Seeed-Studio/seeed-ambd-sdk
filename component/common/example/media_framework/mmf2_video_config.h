#ifndef MMF2_VIDEO_CONFIG_H
#define MMF2_VIDEO_CONFIG_H

#include "platform_opts.h"
#include "video_common_api.h"

#define VIDEO_240P			1
#define VIDEO_480P			2
#define VIDEO_720P			3
#define VIDEO_1080P			4

#define V3_JPEG_OFF			0
#define V3_JPEG_STREAMING	1
#define V3_JPEG_SNAPSHOT	2

#define SNAPSHOT_TFTP_TYPE //The default setting is TFTP transfer. If undef the marco it will choose the sd transfer        

// ---------- Settings ----------

//------------------------------------------------------------------------------
// V1
//------------------------------------------------------------------------------
#define V1_FPS						30 //Default is recording
#define V1_RESOLUTION				VIDEO_720P
#define V1_HW_SLOT					1
#define V1_SW_SLOT					3
// H264
#define V1_BITRATE					2*1024*1024
#define V1_MEM_BUF_SEC				10
#define V1_H264_RCMODE				RC_MODE_H264CBR
//#define V1_H264_QUEUE_LEN			(V1_FPS*(V2_MEM_BUF_SEC))
#define V1_H264_QUEUE_LEN			70
// JPEG
#define ENABLE_V1_SNAPSHOT			1 // snapshot during h264 streaming
#if ENABLE_V1_SNAPSHOT
#define V1_SNAPSHOT_LEVEL			1 // JPEG quality level
#endif

//------------------------------------------------------------------------------
// V2
//------------------------------------------------------------------------------
#define V2_FPS						15 //Default is streaming
#define V2_RESOLUTION				VIDEO_1080P
#define V2_HW_SLOT					1
#define V2_SW_SLOT					2

// H264
#define V2_BITRATE					1*1024*1024
#if ISP_BOOT_MODE_ENABLE
#define V2_MEM_BUF_SEC				20
#else
#define V2_MEM_BUF_SEC				10
#endif
#define V2_H264_RCMODE				RC_MODE_H264CBR
//#define V2_H264_QUEUE_LEN			(V2_FPS*(V2_MEM_BUF_SEC))
#define V2_H264_QUEUE_LEN			40

//------------------------------------------------------------------------------
// V3
//------------------------------------------------------------------------------
#define ENABLE_V3_JPEG						V3_JPEG_OFF
#if ENABLE_V3_JPEG == V3_JPEG_STREAMING
#define ENABLE_V3_SNAPSHOT_WHEN_STREAMING	0
#endif
#define V3_FPS								10
#define V3_RESOLUTION						VIDEO_720P
#define V3_HW_SLOT							2
#define V3_SW_SLOT							3
// JPEG
#define V3_JPEG_LEVEL						1 // quality level

// ---------- End of Settings ----------

#if V1_RESOLUTION == VIDEO_240P
	#define V1_WIDTH	VIDEO_240P_WIDTH
	#define V1_HEIGHT	VIDEO_240P_HEIGHT
#elif V1_RESOLUTION == VIDEO_480P
	#define V1_WIDTH	VIDEO_480P_WIDTH
	#define V1_HEIGHT	VIDEO_480P_HEIGHT
#elif V1_RESOLUTION == VIDEO_720P
	#define V1_WIDTH	VIDEO_720P_WIDTH
	#define V1_HEIGHT	VIDEO_720P_HEIGHT
#elif V1_RESOLUTION == VIDEO_1080P
	#define V1_WIDTH	VIDEO_1080P_WIDTH
	#define V1_HEIGHT	VIDEO_1080P_HEIGHT
#endif

#if V2_RESOLUTION == VIDEO_240P
	#define V2_WIDTH	VIDEO_240P_WIDTH
	#define V2_HEIGHT	VIDEO_240P_HEIGHT
#elif V2_RESOLUTION == VIDEO_480P
	#define V2_WIDTH	VIDEO_480P_WIDTH
	#define V2_HEIGHT	VIDEO_480P_HEIGHT
#elif V2_RESOLUTION == VIDEO_720P
	#define V2_WIDTH	VIDEO_720P_WIDTH
	#define V2_HEIGHT	VIDEO_720P_HEIGHT
#elif V2_RESOLUTION == VIDEO_1080P
	#define V2_WIDTH	VIDEO_1080P_WIDTH
	#define V2_HEIGHT	VIDEO_1080P_HEIGHT
#endif

#if V3_RESOLUTION == VIDEO_240P
	#define V3_WIDTH	VIDEO_240P_WIDTH
	#define V3_HEIGHT	VIDEO_240P_HEIGHT
#elif V3_RESOLUTION == VIDEO_480P
	#define V3_WIDTH	VIDEO_480P_WIDTH
	#define V3_HEIGHT	VIDEO_480P_HEIGHT
#elif V3_RESOLUTION == VIDEO_720P
	#define V3_WIDTH	VIDEO_720P_WIDTH
	#define V3_HEIGHT	VIDEO_720P_HEIGHT
#elif V3_RESOLUTION == VIDEO_1080P
	#define V3_WIDTH	VIDEO_1080P_WIDTH
	#define V3_HEIGHT	VIDEO_1080P_HEIGHT
#endif

// ---------- Buffer Size Settings ----------

#define V1_BLOCK_SIZE					1024*10
#define V1_FRAME_SIZE					V1_WIDTH*V1_HEIGHT/2
//#define V1_BUFFER_SIZE					V1_FRAME_SIZE*6
#define V1_BUFFER_SIZE					V1_FRAME_SIZE + (V1_BITRATE/8*V1_MEM_BUF_SEC)

#define V2_BLOCK_SIZE						1024*10
#define V2_FRAME_SIZE						V2_WIDTH*V2_HEIGHT/2
//#define V2_BUFFER_SIZE						V2_FRAME_SIZE*3
#define V2_BUFFER_SIZE						V2_FRAME_SIZE + (V2_BITRATE/8*V2_MEM_BUF_SEC)

#define V3_BLOCK_SIZE						1024*10
#define V3_FRAME_SIZE						V3_WIDTH*V3_HEIGHT/2
#define V3_BUFFER_SIZE						V3_FRAME_SIZE*3 

// ---------- End of Buffer Size Settings ---------

#if (ENABLE_V1_SNAPSHOT) && (ENABLE_V3_JPEG)
#error "Don't set both ENABLE_V1_SNAPSHOT and ENABLE_V3_JPEG"
#elif (ENABLE_V1_SNAPSHOT) || (ENABLE_V3_JPEG == V3_JPEG_SNAPSHOT) || (ENABLE_V3_SNAPSHOT_WHEN_STREAMING)
	#define ENABLE_SNAPSHOT 1
#endif

#if ENABLE_V1_SNAPSHOT
#define SNAPSHOT_WIDTH		V1_WIDTH
#define SNAPSHOT_HEIGHT		V1_HEIGHT
#define SNAPSHOT_FPS		V1_FPS
#define SNAPSHOT_LEVEL		V1_SNAPSHOT_LEVEL
#define SNAPSHOT_BUFFER_SIZE	V1_WIDTH*V1_HEIGHT/2
#elif ENABLE_V3_SNAPSHOT_WHEN_STREAMING || (ENABLE_V3_JPEG == V3_JPEG_SNAPSHOT)
#define SNAPSHOT_WIDTH		V3_WIDTH
#define SNAPSHOT_HEIGHT		V3_HEIGHT
#define SNAPSHOT_FPS		V3_FPS
#define SNAPSHOT_LEVEL		V3_JPEG_LEVEL
#define SNAPSHOT_BUFFER_SIZE	V3_WIDTH*V3_HEIGHT/2
#endif

#endif /* MMF2_EXAMPLE_H */