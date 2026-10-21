/*******************************************************************************
* File Name        : app_bt_event_handler.h
*
* Description      : This file is the public interface of app_bt_event_handler.c
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
#ifndef SOURCE_APP_BT_APP_BT_EVENT_HANDLER_H_
#define SOURCE_APP_BT_APP_BT_EVENT_HANDLER_H_

/*******************************************************************************
* Macros
*******************************************************************************/
/* Service New Custom Service */
#define HDLS_NEW_CUSTOM_SERVICE                         (0x0014)
/* Characteristic Blink */
#define HDLC_NEW_CUSTOM_SERVICE_BLINK                   (0x0015)
#define HDLC_NEW_CUSTOM_SERVICE_BLINK_VALUE             (0x0016)
#define MAX_LEN_NEW_CUSTOM_SERVICE_BLINK                (0x0001)
#define MAX_NUM_CONNECTIONS                             (4U)
#define MAX_PERIPHERALS                                 (4U)
#define ADV_DEVICE_NAME_SIZE                            (15U)
#define __UUID_SERVICE_NEW_CUSTOM_SERVICE               0x20, 0x3A, 0x02, 0x4C, 0xF2, 0xD0, 0x4D, 0xB8, 0x3F, 0x47, \
                                                        0x63, 0x09, 0x31, 0xED, 0xF6, 0xFB
#define __UUID_CHARACTERISTIC_NEW_CUSTOM_SERVICE_BLINK  0x0A, 0x38, 0x3F, 0x8F, 0x01, 0xC1, 0xCA, 0x9E, 0xD8, 0x48, \
                                                        0xBA, 0x59, 0x2A, 0x8E, 0x23, 0x41

/********************************************************************************
* Data Structures and Types
********************************************************************************/

/* Structure to store discovered GATT service and characteristic handles
 * for a custom service during client-side discovery */
typedef struct
{
    uint16_t start_handle;              /* Start handle of the discovered service */
    uint16_t end_handle;                /* End handle of the discovered service */
    uint16_t char_notify_handle;        /* Handle of the Notify characteristic */
    uint16_t char_val_notify_handle;    /* Value handle of the Notify characteristic */
    uint16_t char_blink_handle;         /* Handle of the Blink/Data characteristic */
    uint16_t char_val_blink_handle;     /* Value handle of the Blink/Data characteristic */
    uint16_t cccd_handle;               /* Handle of the Client Characteristic Configuration Descriptor */
    bool     service_found;             /* Flag indicating whether the service was successfully discovered */
} cs_discovery_data_t;

/* Structure holding handles of standard GATT and Generic Attribute service
 * characteristics relevant to database change management */
typedef struct
{
    uint16_t service_changed_handle;            /* Handle of GATT Service Changed characteristic */
    uint16_t client_supported_features_handle;  /* Handle of GATT Client Supported Features characteristic */
    uint16_t database_hash_handle;              /* Handle of GATT Database Hash characteristic */
} handle_gatt_database_t;

/* Comprehensive structure storing all information about a connected peer
 * device when this device acts as a GATT client (central role) */
typedef struct
{
    uint16_t                     conn_id;                  /* connection ID of the peer */
    wiced_bt_device_address_t    remote_addr;              /* remote peer device address */
    uint16_t                     peer_mtu;                 /* peer MTU */
    wiced_bt_dev_role_t          link_role;                /* Device role */
    wiced_bt_transport_t         transport;                /* BR/EDR oR LE transport */
    wiced_bool_t                 connected;                /* TRUE - connected, FALSE - not connected */
    wiced_bt_ble_address_type_t  addr_type;                /* Peer device BDA type */
    bool                         is_read_notify_char_cplt; /* Flags to check if read response */
    bool                         is_read_cccd_cplt;        /* is received before initiating */
    bool                         is_read_blink_char_cplt;  /* a new read request */
    bool                         flag_indication_sent;     /* to store the state of indication confirmation */
    handle_gatt_database_t       handle_gatt;              /* Handles of GATT service characteristics */
    cs_discovery_data_t          cs_discovery_data;        /* Discovery info for legacy Custom Service */
    cs_discovery_data_t          new_cs_discovery_data;    /* Discovery info for new/updated Custom Service */
    wiced_bt_db_hash_t           database_hash;            /* Cached GATT Database Hash received from the peer */

} hello_client_peer_info_t;

/*******************************************************************************
* Global Variables
*******************************************************************************/
extern wiced_bt_db_hash_t database_hash;
extern uint16_t  connection_id;
extern uint8_t bondindex;
extern uint8_t adv_device_name[ADV_DEVICE_NAME_SIZE];
extern hello_client_peer_info_t peer_info[MAX_NUM_CONNECTIONS];
extern uint16_t current_conn_id;
extern const uint8_t  gatt_database_2[];
extern const uint16_t gatt_database_2_len;
extern cs_discovery_data_t *new_cs_discovery_data;
extern cs_discovery_data_t *cs_discovery_data;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
wiced_result_t app_bt_management_callback(wiced_bt_management_evt_t event,
                                        wiced_bt_management_evt_data_t *p_event_data);
void app_bt_application_init(void);
void app_scan_result_handler(wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data);

#endif /* SOURCE_APP_BT_APP_BT_EVENT_HANDLER_H_ */

/* [] END OF FILE */