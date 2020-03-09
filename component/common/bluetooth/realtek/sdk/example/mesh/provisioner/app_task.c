/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_task.c
   * @brief     Routines to create App task and handle events & messages
   * @author    jane
   * @date      2017-06-02
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <os_msg.h>
#include <os_task.h>
#include <gap.h>
#include <gap_le.h>
#include <gap_msg.h>
#include <trace.h>
#include <app_task.h>
#include <app_msg.h>
#if defined(CONFIG_PLATFORM_8721D)
#include "ameba_soc.h"
#endif

#include "mesh_api.h"
#include "provisioner_app.h"
#include "data_uart.h"
#include "user_cmd_parse.h"
#include "provisioner_cmd.h"
#include "platform_opts.h"

/*============================================================================*
 *                              Macros
 *============================================================================*/
#define EVENT_MESH                    0x80
#define APP_TASK_PRIORITY             1         //!< Task priorities
#define APP_TASK_STACK_SIZE           256 * 10  //!< Task stack size
#define MAX_NUMBER_OF_GAP_MESSAGE     0x20      //!< GAP message queue size
#define MAX_NUMBER_OF_IO_MESSAGE      0x40      //!< IO message queue size
#define MAX_NUMBER_OF_EVENT_MESSAGE   (MAX_NUMBER_OF_GAP_MESSAGE + MAX_NUMBER_OF_IO_MESSAGE + MESH_INNER_MSG_NUM) //!< Event message queue size

#if defined(CONFIG_PLATFORM_8721D)
#define UART_TX    _PA_18
#define UART_RX    _PA_19
#elif defined(CONFIG_PLATFORM_8710C)
#include "serial_api.h"
#define UART_TX    PA_14
#define UART_RX    PA_13
#endif

/*============================================================================*
 *                              Variables
 *============================================================================*/
void *app_task_handle;   //!< APP Task handle
void *evt_queue_handle;  //!< Event queue handle
void *io_queue_handle;   //!< IO queue handle

/*============================================================================*
 *                              Functions
 *============================================================================*/
void app_main_task(void *p_param);

void app_send_uart_msg(uint8_t data)
{
    uint8_t event = EVENT_IO_TO_APP;
    T_IO_MSG msg;
    msg.type = IO_MSG_TYPE_UART;
    msg.subtype = data;
    if (os_msg_send(io_queue_handle, &msg, 0) == false)
    {
    }
    else if (os_msg_send(evt_queue_handle, &event, 0) == false)
    {
    }
}

/**
 * @brief  Initialize App task
 * @return void
 */
void app_task_init()
{
    os_task_create(&app_task_handle, "app", app_main_task, 0, APP_TASK_STACK_SIZE,
                   APP_TASK_PRIORITY);
}

/**
 * @brief        App task to handle events & messages
 * @param[in]    p_param    Parameters sending to the task
 * @return       void
 */
void app_main_task(void *p_param)
{
    uint8_t event;

    os_msg_queue_create(&io_queue_handle, MAX_NUMBER_OF_IO_MESSAGE, sizeof(T_IO_MSG));
    os_msg_queue_create(&evt_queue_handle, MAX_NUMBER_OF_EVENT_MESSAGE, sizeof(uint8_t));
    gap_start_bt_stack(evt_queue_handle, io_queue_handle, MAX_NUMBER_OF_GAP_MESSAGE);

    mesh_start(EVENT_MESH, EVENT_IO_TO_APP, evt_queue_handle, io_queue_handle);

    data_uart_init(UART_TX, UART_RX, app_send_uart_msg);
    user_cmd_init("MeshProvisioner");

    while (true)
    {
        if (os_msg_recv(evt_queue_handle, &event, 0xFFFFFFFF) == true)
        {
            if (event == EVENT_IO_TO_APP)
            {
                T_IO_MSG io_msg;
                if (os_msg_recv(io_queue_handle, &io_msg, 0) == true)
                {
                    app_handle_io_msg(io_msg);
                }
            }
#if defined(CONFIG_EXAMPLE_BT_MESH_DEMO) && CONFIG_EXAMPLE_BT_MESH_DEMO
            else if (event == EVENT_USER_HTTP_SERVER)
            {
                T_IO_MSG io_msg;
                if (os_msg_recv(io_queue_handle, &io_msg, 0) == true)
                {
                    bt_mesh_demo_io_msg_handler(io_msg);
                }
            }
#endif
            else if (event == EVENT_MESH)
            {
                mesh_inner_msg_handle(event);
            }
            else
            {
                gap_handle_msg(event);
            }
        }
    }
}

