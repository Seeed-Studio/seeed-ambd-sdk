/**
*****************************************************************************************
*     Copyright(c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
* @file     tp_control.c
* @brief    Source file for ping control model.
* @details  Data types and external functions declaration.
* @author   bill
* @date     2018-3-7
* @version  v1.0
* *************************************************************************************
*/

/* Add Includes here */
#include <string.h>
#include "trace.h"
#include "tp.h"

mesh_model_info_t tp_control;

struct tp_tx_ctx_t
{
    uint32_t tid;
    uint32_t count;
    uint16_t dst;
    uint8_t ttl;
    uint16_t app_key_index;
    uint32_t begin_time;
    uint32_t end_time;
} tp_tx_ctx;

static mesh_msg_send_cause_t tp_control_send(uint16_t dst, uint8_t ttl, uint8_t *pmsg,
                                             uint16_t msg_len, uint16_t app_key_index)
{
    mesh_msg_t mesh_msg;
    mesh_msg.pmodel_info = &tp_control;
    access_cfg(&mesh_msg);
    mesh_msg.pbuffer = pmsg;
    mesh_msg.msg_len = msg_len;
    mesh_msg.dst = dst;
    mesh_msg.ttl = ttl;
    mesh_msg.app_key_index = app_key_index;
    return access_send(&mesh_msg);
}

mesh_msg_send_cause_t tp_seg(uint16_t dst, uint8_t ttl, uint16_t app_key_index)
{
    tp_seg_t *pmsg = (tp_seg_t *)plt_malloc(sizeof(tp_seg_t), RAM_TYPE_DATA_OFF);
    if (pmsg == NULL)
    {
        return MESH_MSG_SEND_CAUSE_NO_MEMORY;
    }
    pmsg->tid = tp_tx_ctx.tid;
    ACCESS_OPCODE_BYTE(pmsg->opcode, MESH_MSG_TP_SEG);
    memset(pmsg->padding, 0, sizeof(pmsg->padding));
    mesh_msg_send_cause_t ret = tp_control_send(dst, ttl, (uint8_t *)pmsg, sizeof(tp_seg_t),
                                                app_key_index);
    plt_free(pmsg, RAM_TYPE_DATA_OFF);
    return ret;
}

mesh_msg_send_cause_t tp_seg_start(uint16_t dst, uint8_t ttl, uint16_t app_key_index,
                                   uint32_t count)
{
    if (MESH_IS_UNASSIGNED_ADDR(dst) || count == 0)
    {
        /* quit to send segmented message */
        tp_tx_ctx.count = tp_tx_ctx.tid;
        return MESH_MSG_SEND_CAUSE_SUCCESS;
    }
    tp_tx_ctx.tid = 0;
    tp_tx_ctx.count = count;
    tp_tx_ctx.dst = dst;
    tp_tx_ctx.ttl = ttl;
    tp_tx_ctx.app_key_index = app_key_index;
    tp_tx_ctx.begin_time = plt_time_read_ms();
    return tp_seg(tp_tx_ctx.dst, tp_tx_ctx.ttl, tp_tx_ctx.app_key_index);
}

bool tp_control_receive(mesh_msg_p pmesh_msg)
{
    bool ret = TRUE;
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;
    switch (pmesh_msg->access_opcode)
    {
    case MESH_MSG_TP_SEG:
        if (pmesh_msg->msg_len == sizeof(tp_seg_t))
        {
            tp_seg_t *pmsg = (tp_seg_t *)(pbuffer);
            data_uart_debug("tp rx src=0x%04x dst=0x%04x tid=%d\r\n", pmesh_msg->src, pmesh_msg->dst,
                            pmsg->tid);
        }
        break;
    default:
        ret = FALSE;
        break;
    }

    return ret;
}

void tp_send_cb(mesh_model_info_p pmodel_info, mesh_msg_send_stat_t stat, uint32_t access_opcode)
{
    if (stat == MESH_MSG_SEND_STAT_ACKED)
    {
        tp_tx_ctx.tid++;
    }
    else
    {
        data_uart_debug("tp tx fail reason=%d\r\n", stat);
    }
    if (tp_tx_ctx.tid < tp_tx_ctx.count)
    {
        tp_seg(tp_tx_ctx.dst, tp_tx_ctx.ttl, tp_tx_ctx.app_key_index);
    }
    else
    {
        tp_tx_ctx.end_time = plt_time_read_ms();
        data_uart_debug("tp tx elapsed time=%dms tp=%dbyte/s\r\n",
                        tp_tx_ctx.end_time - tp_tx_ctx.begin_time,
                        ACCESS_PAYLOAD_MAX_SIZE * tp_tx_ctx.tid * 1000 / (tp_tx_ctx.end_time - tp_tx_ctx.begin_time));
    }
}

void tp_control_reg(void)
{
    tp_control.model_id = MESH_MODEL_TP_CONTROL;
    tp_control.model_receive = tp_control_receive;
    tp_control.model_send_cb = tp_send_cb;
    mesh_model_reg(0, &tp_control);
}

