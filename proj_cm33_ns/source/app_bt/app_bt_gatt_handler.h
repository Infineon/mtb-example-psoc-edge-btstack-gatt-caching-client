/*******************************************************************************
* File Name        : app_bt_gatt_handler.h
*
* Description      : This file is the public interface of app_bt_gatt_handler.c
*
* Related Document : See README.md
*
********************************************************************************
* (c) 2025-2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/
#ifndef SOURCE_APP_BT_APP_BT_GATT_HANDLER_H_
#define SOURCE_APP_BT_APP_BT_GATT_HANDLER_H_

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "wiced_bt_gatt.h"
#include "cycfg_gatt_db.h"

/*******************************************************************************
*        Macros
*******************************************************************************/
#define ATT_CCCD                       (1U)
#define ATT_BLINK                      (2U)
#define ATT_DEVICE_NAME                (3U)
#define ATT_CLIENT_SUPPORTED_FEATURES  (4U)
#define NOTIFY                         (0x01)
#define INDICATE                       (0x02)
#define CCCD_LEN                       (2U)

/*******************************************************************************
* Data Structures and Types
*******************************************************************************/
typedef struct
{
    uint16_t   conn_id;          /* Connection ID of the peer */
    uint16_t   service_handle;  /* The service handle associated to the connection
                                   This helps to identify the specific GATT server */
    bool       is_service_free;
}local_gatt_db_info_t;

/*******************************************************************************
* Global Variables
*******************************************************************************/
extern uint8_t uuid_service_custom_service[MAX_UUID_SIZE];
extern bool pairing_started;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
wiced_bt_gatt_status_t
app_bt_gatt_operation_cplt_handler(wiced_bt_gatt_operation_complete_t *p_operation_cplt);
void app_bt_write_peer_cccd(uint8_t* cccd_val);
void app_bt_read_peer_attributes(uint16_t conn_id);
wiced_bt_gatt_status_t
app_bt_discovery_result_handler(wiced_bt_gatt_discovery_result_t *discovery_result);
wiced_bt_gatt_status_t
app_bt_service_discovery_handler(wiced_bt_gatt_discovery_complete_t *discovery_complete);
void app_bt_store_peer_info(wiced_bt_gatt_connection_status_t* peer_conn_info);
uint8_t app_bt_get_peer_index(uint16_t conn_id);
void app_bt_update_gatt_db(uint8_t att_val_id, uint16_t conn_id, uint16_t handle);
uint16_t app_bt_get_service_handle_from_conn_id(uint16_t conn_id);
wiced_bt_gatt_status_t
app_bt_gatt_callback(wiced_bt_gatt_evt_t event,
                     wiced_bt_gatt_event_data_t *p_data);
wiced_bt_gatt_status_t
app_bt_gatt_req_handler(wiced_bt_gatt_attribute_request_t *p_attr_req,
        uint16_t *p_error_handle);
wiced_bt_gatt_status_t
app_bt_gatt_conn_status_cb(wiced_bt_gatt_connection_status_t *p_conn_status);
wiced_bt_gatt_status_t
app_bt_gatt_req_write_handler(uint16_t conn_id, wiced_bt_gatt_opcode_t opcode,
        wiced_bt_gatt_write_req_t *p_write_req, uint16_t len_req,
        uint16_t *p_error_handle);
wiced_bt_gatt_status_t
app_bt_gatt_connection_up(wiced_bt_gatt_connection_status_t *p_status);
wiced_bt_gatt_status_t
app_bt_gatt_connection_down(wiced_bt_gatt_connection_status_t *p_status);

#endif /* SOURCE_APP_BT_APP_BT_GATT_HANDLER_H_ */

/* [] END OF FILE */