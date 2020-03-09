/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     ali.h
  * @brief    Head file for ali models.
  * @details  Data types and external functions declaration.
  * @author   bill
  * @date     2018-5-17
  * @version  v1.0
  * *************************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef _ALI_H
#define _ALI_H

/* Add Includes here */
#include "mesh_api.h"

BEGIN_DECLS

/**
 * @addtogroup ALI
 * @{
 */

/**
 * @defgroup ALI_ACCESS_OPCODE Access Opcode
 * @brief Mesh message access opcode
 * @{
 */
#define MESH_MSG_ALI_GET                                0xC0A801
#define MESH_MSG_ALI_SET                                0xC1A801
#define MESH_MSG_ALI_SET_UNACK                          0xC2A801
#define MESH_MSG_ALI_STAT                               0xC3A801
#define MESH_MSG_ALI_OTA_GET                            0xC4A801
#define MESH_MSG_ALI_OTA_SET                            0xC5A801
#define MESH_MSG_ALI_OTA_SET_UNACK                      0xC6A801
#define MESH_MSG_ALI_OTA_STAT                           0xC7A801
/** @} */

/**
 * @defgroup ALI_MODEL_ID Model ID
 * @brief Mesh model id
 * @{
 */
#define MESH_MODEL_ALI_VENDOR                           0x000001A8
/** @} */

/**
 * @defgroup ALI_MESH_MSG Mesh Msg
 * @brief Mesh message types used by models
 * @{
 */
enum
{
    ALI_MSG_TYPE_NONE = 0xffff
} _SHORT_ENUM_;
typedef uint16_t ali_msg_type_t;

typedef struct
{
    uint8_t opcode[ACCESS_OPCODE_SIZE(MESH_MSG_ALI_GET)];
    uint8_t tid;
    ali_msg_type_t type;
} _PACKED_ ali_get_t;

typedef struct
{
    uint8_t opcode[ACCESS_OPCODE_SIZE(MESH_MSG_ALI_SET)];
    uint8_t tid;
    ali_msg_type_t type;
    uint16_t len;
    uint8_t value[1];
} _PACKED_ ali_set_t;

typedef struct
{
    uint8_t opcode[ACCESS_OPCODE_SIZE(MESH_MSG_ALI_STAT)];
    uint8_t tid;
    ali_msg_type_t type;
    uint16_t len;
    uint8_t stat[1];
} _PACKED_ ali_stat_t;

typedef struct
{
    uint8_t type: 4;
    uint8_t version: 4;
} _PACKED_ ali_ota_type_t;

enum
{
    ALI_OTA_CMD_VERSION_REQ = 0x0,
    ALI_OTA_CMD_VERSION_RSP = 0x1,
    ALI_OTA_CMD_UPDATE_REQ = 0x2,
    ALI_OTA_CMD_UPDATE_RSP = 0x3,
    ALI_OTA_CMD_FRAME_REQ = 0x4,
    ALI_OTA_CMD_FRAME_RSP = 0x5,
    ALI_OTA_CMD_CHECK_REQ = 0x6,
    ALI_OTA_CMD_CHECK_RSP = 0x6,
    ALI_OTA_CMD_RESULT = 0x8,
    ALI_OTA_CMD_DATA = 0x9
} _SHORT_ENUM_;
typedef uint8_t ali_ota_cmd_t;

typedef struct
{
    uint16_t fw_ver_x;
    uint16_t fw_ver_y;
    uint16_t fw_ver_z;
    uint8_t fw_len[3];
    uint16_t crc;
} _PACKED_ ali_ota_update_req_t;

typedef struct
{
    uint16_t fw_ver_x;
    uint16_t fw_ver_y;
    uint16_t fw_ver_z;
} _PACKED_ ali_ota_update_rsp_t;

typedef struct
{
    uint16_t frame_num;
    uint16_t data_len;
    uint8_t data[1];
} _PACKED_ ali_ota_frame_req_t;

typedef struct
{
    uint16_t frame_num;
    uint16_t data_len;
} _PACKED_ ali_ota_frame_rsp_t;

typedef struct
{
    uint8_t opcode[ACCESS_OPCODE_SIZE(MESH_MSG_ALI_OTA_GET)];
    ali_ota_type_t type;
    ali_ota_cmd_t cmd;
} _PACKED_ ali_ota_get_t;

typedef struct
{
    uint8_t opcode[ACCESS_OPCODE_SIZE(MESH_MSG_ALI_OTA_SET)];
    ali_ota_type_t type;
    ali_ota_cmd_t cmd;
    union
    {
        ali_ota_update_req_t upd_req;
        ali_ota_frame_req_t frame_req;
    };
} _PACKED_ ali_ota_set_t;

typedef struct
{
    uint8_t opcode[ACCESS_OPCODE_SIZE(MESH_MSG_ALI_OTA_STAT)];
    ali_ota_type_t type;
    ali_ota_cmd_t cmd;
    union
    {
        ali_ota_update_rsp_t upd_rsp;
        ali_ota_frame_rsp_t frame_rsp;
    };
} _PACKED_ ali_ota_stat_t;
/** @} */

/**
 * @defgroup ALI_SERVER_API Server API
 * @brief Functions declaration
 * @{
 */

/** @} */
/** @} */

END_DECLS

#endif /* _ALI_H */
