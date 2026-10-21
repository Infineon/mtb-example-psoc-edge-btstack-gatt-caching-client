/*******************************************************************************
* File Name        : app_bt_gatt_handler.c
*
* Description      : This file contains the task that handles GATT events.
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

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "inttypes.h"
#include <FreeRTOS.h>
#include <task.h>
#include "wiced_memory.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "app_bt_bonding.h"
#include "cycfg_gap.h"
#include "app_bt_utils.h"
#include "app_bt_event_handler.h"
#include "app_bt_gatt_handler.h"
#include "app_hw_device.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define RESET_VAL                                       (0U)
#define RESPONSE_DATA_LENGTH                            (20U)
#define SERVICE_CHANGE_ARR_LEN                          (2U)   
/* Macros to calculate handles from service handle in local GATT DB */
#define CCCD_ATT_OFFSET                                 (3)
#define DEVICE_NAME_ATT_OFFSET                          (7)
#define SC_CCCD_ATT_HANDLE                              (0x0009)

/* UUID of custom service */
#define __UUID_SERVICE_CUSTOM_SERVICE                   0x38, 0x28, 0x2E, 0x5F, 0xA5, 0x1E, 0xC7, 0xA4, 0xC2, 0x46, \
                                                        0x47, 0x74, 0xB6, 0xC7, 0x81, 0x2F
#define __UUID_CHARACTERISTIC_CUSTOM_SERVICE_NOTIFY     0xBD, 0xED, 0x5E, 0xC7, 0x28, 0x3A, 0x86, 0xAD, 0xAF, 0x44, \
                                                        0x45, 0x9F, 0x3D, 0x62, 0xEA, 0x29
#define __UUID_CHARACTERISTIC_CUSTOM_SERVICE_BLINK      0x8E, 0xB1, 0x5D, 0xFA, 0x97, 0x33, 0x5A, 0xA1, 0x61, 0x41, \
                                                        0xA7, 0x13, 0x50, 0x36, 0xAD, 0xD2
#define UUID_CHAR_HASH                                  (0x2B2A)
#define UUID_CHAR_CLIENT_SUPPORTED_FEATURES             (0x2B29)
#define UUID_CHAR_SERVICE_CHANGED                       (0x2A05)
#define GATT_HANDLE_START                               (0x0001)
#define GATT_HANDLE_END                                 (0xFFFF)
#define VALUE_0                                         (0U)
#define CSF_CHAR_VALUE_LENGTH                           (1U)
#define CSF_FEATURE_ROBUST_CACHING_ENABLE               (0x01)
#define CCCD_INDEX_0                                    (0U)
#define CCCD_INDEX_1                                    (1U)

/*******************************************************************************
* Global Variables 
*******************************************************************************/
wiced_bt_db_hash_t curr_hash;

/* Index for managing connected devices or services */
uint8_t device_index = RESET_VAL;

/* Pointers to discovery data for the two versions of the custom service */
cs_discovery_data_t *cs_discovery_data;
cs_discovery_data_t *new_cs_discovery_data;
cs_discovery_data_t hash_data;

/* GATT Client Supported Features value (written to peer server) */
uint8_t client_supported_features;

/* Buffer to hold Service Changed characteristic value (for indication) */
uint8_t service_changed[SERVICE_CHANGE_ARR_LEN];

/* Buffer to store Database Hash value read from the peer GATT server */
wiced_bt_db_hash_t database_hash;

/* Pointer to currently active peer's GATT handle structure (Service Changed, DB Hash, etc.) */
handle_gatt_database_t *curr_handle_gatt = NULL; 

/* UUID of custom service 1 */
uint8_t uuid_service_custom_service[MAX_UUID_SIZE] = {__UUID_SERVICE_CUSTOM_SERVICE};
uint8_t uuid_char_custom_service_notify[MAX_UUID_SIZE] = {__UUID_CHARACTERISTIC_CUSTOM_SERVICE_NOTIFY};
uint8_t uuid_char_custom_service_blink[MAX_UUID_SIZE] = {__UUID_CHARACTERISTIC_CUSTOM_SERVICE_BLINK};

/* UUID of new custom service 2 */
uint8_t uuid_service_new_custom_service[MAX_UUID_SIZE] = {__UUID_SERVICE_NEW_CUSTOM_SERVICE};
uint8_t uuid_char_new_custom_service_blink[MAX_UUID_SIZE] = {__UUID_CHARACTERISTIC_NEW_CUSTOM_SERVICE_BLINK};

/* 16-bit UUIDs for standard GATT characteristics */
uint16_t uuid_char_hash = UUID_CHAR_HASH;
uint16_t uuid_char_client_supported_features = UUID_CHAR_CLIENT_SUPPORTED_FEATURES;
uint16_t uuid_char_service_changed  = UUID_CHAR_SERVICE_CHANGED;

/* Variable that stores connection ID of peripheral against service handle in local GATT DB
   The application is tested for 4 (MAX_PERIPHERALS) peripheral connections */
static local_gatt_db_info_t local_gatt_db_info[MAX_PERIPHERALS];

/* Variables sent to BTSTACK during read request and write request */
static uint8_t read_resp_data[RESPONSE_DATA_LENGTH];

/* Flag to indicate if Database Hash was read immediately after service discovery */
bool hash_read_after_discovery = false;

/* Flag to track whether pairing/bonding was initiated due to insufficient authentication */
bool pairing_started = false;

bool flag;

/* Parameters used when initiating GATT service/characteristic discovery */
wiced_bt_gatt_discovery_param_t service_discovery_setup =
{
    .s_handle = GATT_HANDLE_START,
    .e_handle = GATT_HANDLE_END,
};
/* Track the number of peripheral connections */
uint8_t num_peripherals;

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: app_bt_clear_discovered_handles
********************************************************************************
* Summary:
*  Clears all discovered GATT service and characteristic handles.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void app_bt_clear_discovered_handles(void)
{
    memset(cs_discovery_data, RESET_VAL, sizeof(cs_discovery_data_t));
    memset(new_cs_discovery_data, RESET_VAL, sizeof(cs_discovery_data_t));
}

/*******************************************************************************
* Function Name: app_bt_gatt_event_callback
********************************************************************************
* Summary:
*  This function handles GATT events from the BT stack.
*
* Parameters:
*  wiced_bt_gatt_evt_t event                : LE GATT event code of one
*                                             byte length
*  wiced_bt_gatt_event_data_t *p_event_data : Pointer to LE GATT event
*                                             structures
*
* Return:
*  wiced_bt_gatt_status_t                   : See possible status
*                                             codes in wiced_bt_gatt_status_e
*                                             in wiced_bt_gatt.h
*
*******************************************************************************/
wiced_bt_gatt_status_t app_bt_gatt_callback(wiced_bt_gatt_evt_t event,
                            wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_SUCCESS;

    /* Call the appropriate callback function based on the GATT event type,
     * and pass the relevant event parameters to the callback function */
    switch (event)
    {
        case GATT_CONNECTION_STATUS_EVT:
            gatt_status = app_bt_gatt_conn_status_cb( &p_event_data->connection_status );

            break;
        
        case GATT_OPERATION_CPLT_EVT:
            if(p_event_data->operation_complete.status == WICED_BT_GATT_DATABASE_OUT_OF_SYNC)
            {
                printf("\nDatabase Out of Sync Error received. Do Service discovery again...\r\n");

                app_bt_clear_discovered_handles();
                cs_discovery_data->service_found = false;
                gatt_status = wiced_bt_gatt_client_send_discover(p_event_data->operation_complete.conn_id,
                                                                GATT_DISCOVER_SERVICES_ALL,
                                                                &service_discovery_setup);
                if(WICED_BT_GATT_SUCCESS != gatt_status)
                {
                    printf("GATT Discovery request failed. Error code: %d, "
                            "Conn id: %d\n", gatt_status, p_event_data->operation_complete.conn_id);
                }
                else
                {
                    printf("\nService Discovery Started\n\n");
                }
            }
            gatt_status = app_bt_gatt_operation_cplt_handler(&p_event_data->operation_complete);

            break;

        case GATT_DISCOVERY_RESULT_EVT:
            gatt_status = app_bt_discovery_result_handler(&p_event_data->discovery_result);

            break;

        case GATT_DISCOVERY_CPLT_EVT:
            gatt_status = app_bt_service_discovery_handler(&p_event_data->discovery_complete);
            if(cs_discovery_data->service_found)
            {
                /* If the custom service is found, read all the readable attributes
                   and store in local GATT DB */
                app_bt_read_peer_attributes(p_event_data->discovery_complete.conn_id);
            }

            break;

        default:
            gatt_status = WICED_BT_GATT_ERROR;

            break;
    }

    return gatt_status;
}

/*******************************************************************************
* Function Name: app_bt_gatt_conn_status_cb
********************************************************************************
* Summary:
*  This callback function handles connection status changes.
*
* Parameters:
*  wiced_bt_gatt_connection_status_t *p_conn_status : Pointer to data that
*                                                     has connection details
*
* Return:
*  wiced_bt_gatt_status_t   : See possible status codes in
*                             wiced_bt_gatt_status_e in wiced_bt_gatt.h
*
*******************************************************************************/
wiced_bt_gatt_status_t app_bt_gatt_conn_status_cb
                       (wiced_bt_gatt_connection_status_t *p_conn_status)
{
    wiced_bt_gatt_status_t result;

    if (p_conn_status->connected)
    {
        result= app_bt_gatt_connection_up(p_conn_status);
    }
    else
    {
        result= app_bt_gatt_connection_down(p_conn_status);
    }

    return result;
}

/*******************************************************************************
* Function Name: app_bt_gatt_connection_up
********************************************************************************
* Summary:
*  This function is invoked when connection is established
*
* Parameters:
*  wiced_bt_gatt_connection_status_t *p_status : Pointer to data that has
*                                                connection details
*
* Return:
*  wiced_bt_gatt_status_t : See possible status codes in
*                           wiced_bt_gatt_status_e in wiced_bt_gatt.h
*
*******************************************************************************/
wiced_bt_gatt_status_t
app_bt_gatt_connection_up( wiced_bt_gatt_connection_status_t *p_status )
{
    uint8_t peer_index;
    wiced_bt_gatt_status_t gatt_status =  WICED_BT_GATT_SUCCESS;

    printf("Peer device connected: ");
    print_bd_address(p_status->bd_addr);
    /* Store the latest connection ID for easy access */
    current_conn_id = p_status->conn_id;

    /* Store and print the connection information of the peer in a local variable
       This data will be used in GATT and Security operations following the connection */
    app_bt_store_peer_info(p_status);
  
    /* Check if the connected device is Peripheral */
    /* if are_we_central flag is set we are in central role for that connection*/
    peer_index = app_bt_get_peer_index(p_status->conn_id);
    if(MAX_NUM_CONNECTIONS <= peer_index )
    {
        printf("This app only supports upto 4 connections (including Central device)\n");
        return WICED_BT_GATT_SUCCESS;
    }

    /*Disconnect from the newly connected device if maximum peripheral limit reached*/
    if(MAX_PERIPHERALS == num_peripherals)
    {
        printf("Reached maximum peripheral limit, disconnecting from recently connected server! \r\n");
        wiced_bt_gatt_disconnect(current_conn_id);

        return WICED_BT_GATT_SUCCESS;
    }

    /* Increment the number of peripherals connected count */
    num_peripherals++;

    app_bt_update_gatt_db(ATT_DEVICE_NAME, p_status->conn_id, RESET_VAL);

    if ( curr_handle_gatt->database_hash_handle )
    {
        hash_read_after_discovery = false;
        wiced_bt_gatt_client_send_read_handle(p_status->conn_id,
                        curr_handle_gatt->database_hash_handle, VALUE_0,
                        database_hash, sizeof(database_hash),
                        GATT_AUTH_REQ_NONE);   
    }
    else
    {
        printf("Hash key handle is not found. Doing service discovery...\n");

        app_bt_clear_discovered_handles();

        cs_discovery_data->service_found = false;
        gatt_status = wiced_bt_gatt_client_send_discover(p_status->conn_id,
                                            GATT_DISCOVER_SERVICES_ALL,
                                                    &service_discovery_setup);
        if(WICED_BT_GATT_SUCCESS != gatt_status)
        {
            printf("GATT Discovery request failed. Error code: %d, "
                    "Conn id: %d\n", gatt_status, p_status->conn_id);
        }
        else
        {
            printf("\nService Discovery Started\n\n");
        }
    }

    return WICED_BT_GATT_SUCCESS;
}

/*******************************************************************************
* Function Name: app_bt_gatt_connection_down
********************************************************************************
* Summary:
*  This function is invoked when connection is disconnected
*
* Parameters:
*  wiced_bt_gatt_connection_status_t *p_status : Pointer to data that has
*                                                connection details
*
* Return:
*  wiced_bt_gatt_status_t  : See possible status codes in wiced_bt_gatt_status_e
*                            in wiced_bt_gatt.h
*
*******************************************************************************/
wiced_bt_gatt_status_t
app_bt_gatt_connection_down(wiced_bt_gatt_connection_status_t *p_status)
{
    uint8_t peer_index;
    printf("Peer device disconnected: ");
    print_bd_address(p_status->bd_addr);

    printf("conn_id:%d reason:%s\n", p_status->conn_id,
           get_bt_gatt_disconn_reason_name(p_status->reason));

    peer_index = app_bt_get_peer_index(p_status->conn_id);

    if(MAX_NUM_CONNECTIONS <= peer_index )
    {
        printf("This app only supports upto 4 connections (including Central device)\n");
        return WICED_BT_GATT_SUCCESS;
    }

    /* Clear the disconnected peer info */
    memset(&(peer_info[peer_index].conn_id), RESET_VAL, sizeof(hello_client_peer_info_t));

    /* Reset the stored connection IDs and flag */
    current_conn_id = RESET_VAL;
    cs_discovery_data->service_found = false;

    return WICED_BT_GATT_SUCCESS;
}

/*******************************************************************************
* Function Name: app_bt_discovery_result_handler
********************************************************************************
* Summary:
*  Processes individual GATT discovery results (services, characteristics,
*  descriptors) and populates discovery data structures for Custom Service 1,
*  New Custom Service 2, and standard GATT characteristics (Database Hash,
*  Service Changed, etc.).
*
* Parameters:
*  discovery_result - Pointer to the discovery result from the BLE stack
*
* Return:
*  wiced_bt_gatt_status_t - Always returns WICED_BT_GATT_SUCCESS
*
*******************************************************************************/

wiced_bt_gatt_status_t
app_bt_discovery_result_handler(wiced_bt_gatt_discovery_result_t *discovery_result)
{
    wiced_bt_gatt_status_t gatt_status =  WICED_BT_GATT_SUCCESS;
    wiced_bt_gatt_discovery_type_t discovery_type;
    discovery_type = discovery_result->discovery_type;

    switch (discovery_type)
    {
        case GATT_DISCOVER_SERVICES_ALL:
            if(!memcmp(uuid_service_custom_service, discovery_result->discovery_data.group_value.service_type.uu.uuid128
                , MAX_UUID_SIZE))
            {
                cs_discovery_data->start_handle = discovery_result->discovery_data.group_value.s_handle;
                cs_discovery_data->end_handle = discovery_result->discovery_data.group_value.e_handle;
                printf("Custom Service 1 Found, Start Handle = 0x%x, End Handle = 0x%x \n",
                        cs_discovery_data->start_handle,
                        cs_discovery_data->end_handle);
            }
            if(!memcmp(uuid_service_new_custom_service, discovery_result->discovery_data.group_value.service_type.uu
                .uuid128, MAX_UUID_SIZE))
            {
                new_cs_discovery_data->start_handle = discovery_result->discovery_data.group_value.s_handle;
                new_cs_discovery_data->end_handle = discovery_result->discovery_data.group_value.e_handle;
                printf("New Custom Service 2 Found, Start Handle = 0x%x, End Handle = 0x%x \n",
                        new_cs_discovery_data->start_handle,
                        new_cs_discovery_data->end_handle);
            }

            break;

        case GATT_DISCOVER_CHARACTERISTICS:
            if(!memcmp(uuid_char_custom_service_notify, discovery_result->discovery_data.characteristic_declaration
                .char_uuid.uu.uuid128, MAX_UUID_SIZE))
            {
                cs_discovery_data->char_notify_handle = discovery_result->discovery_data.characteristic_declaration
                                                                        .handle;
                cs_discovery_data->char_val_notify_handle = discovery_result->discovery_data.characteristic_declaration
                                                                            .val_handle;
            }
            if(!memcmp(uuid_char_custom_service_blink, discovery_result->discovery_data.characteristic_declaration
                .char_uuid.uu.uuid128, MAX_UUID_SIZE))
            {
                cs_discovery_data->char_blink_handle = discovery_result->discovery_data.characteristic_declaration
                                                                        .handle;
                cs_discovery_data->char_val_blink_handle = discovery_result->discovery_data.characteristic_declaration
                                                                            .val_handle;
            }

            if(!memcmp(uuid_char_new_custom_service_blink, discovery_result->discovery_data.characteristic_declaration
                .char_uuid.uu.uuid128, MAX_UUID_SIZE))
            {
                new_cs_discovery_data->char_blink_handle = discovery_result->discovery_data.characteristic_declaration
                                                                            .handle;
                new_cs_discovery_data->char_val_blink_handle = discovery_result->discovery_data
                                                                            .characteristic_declaration.val_handle;
                new_cs_discovery_data->service_found = true;
            }
            if(uuid_char_hash ==  discovery_result->discovery_data.characteristic_declaration.char_uuid.uu.uuid16)
            {
                printf("Discovery : database hash characteristic found\n\r");
                curr_handle_gatt->database_hash_handle =  discovery_result->discovery_data.characteristic_declaration
                                                                            .val_handle;
            }

            if(uuid_char_client_supported_features ==  discovery_result->discovery_data.characteristic_declaration
                .char_uuid.uu.uuid16)
            {
                printf("Discovery : client supported features characteristic found\n\r");
                curr_handle_gatt->client_supported_features_handle =  discovery_result->discovery_data
                                                                                .characteristic_declaration.val_handle;
            }
            if(uuid_char_service_changed ==  discovery_result->discovery_data.characteristic_declaration
                .char_uuid.uu.uuid16)
            {
                printf("Discovery : service changed characteristic found\n\r");
                curr_handle_gatt->service_changed_handle =  discovery_result->discovery_data.characteristic_declaration
                                                                            .val_handle;
            }

            break;

        case GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS:
            if(__UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION ==
               discovery_result->discovery_data.char_descr_info.type.uu.uuid16)
            {
                cs_discovery_data->cccd_handle = discovery_result->discovery_data.char_descr_info.handle;
                cs_discovery_data->service_found = true;
            }

            break;

        default:

            break;
        }

    return gatt_status;
}

/*******************************************************************************
* Function Name: app_bt_service_discovery_handler
********************************************************************************
* Summary:
*  Handles GATT discovery completion events and sequentially triggers the next
*  discovery phase: primary services to characteristics to characteristic descriptors.
*  Prints completion when full service discovery is finished.
*
* Parameters:
*  discovery_complete - Pointer to discovery complete event data
*
* Return:
*  wiced_bt_gatt_status_t - Status of the triggered next discovery operation
*
*******************************************************************************/
wiced_bt_gatt_status_t
app_bt_service_discovery_handler(wiced_bt_gatt_discovery_complete_t *discovery_complete)
{
    wiced_bt_gatt_status_t gatt_status =  WICED_BT_GATT_SUCCESS;

    wiced_bt_gatt_discovery_type_t discovery_type;
    discovery_type = discovery_complete->discovery_type;

    switch (discovery_type)
    {
        case GATT_DISCOVER_SERVICES_ALL:
            gatt_status = wiced_bt_gatt_client_send_discover(discovery_complete->conn_id,
                                                GATT_DISCOVER_CHARACTERISTICS,
                                                &service_discovery_setup);
            if(WICED_BT_GATT_SUCCESS != gatt_status)
            {
                printf("GATT characteristics discovery failed-1! Error code = %d\n", gatt_status);
            }

            break;

        case GATT_DISCOVER_CHARACTERISTICS:
            gatt_status = wiced_bt_gatt_client_send_discover(discovery_complete->conn_id,
                                               GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS,
                                               &service_discovery_setup);

            if(WICED_BT_GATT_SUCCESS != gatt_status)
            {
                printf("GATT CCCD discovery failed! Error code = %d\n", gatt_status);
            }

            break;
        
        case GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS:
            printf("\nService Discovery Completed\n\n");

            break;

        default:

            break;
    }

    return gatt_status;
}



/*******************************************************************************
* Function Name: compare_hex_array
********************************************************************************
* Summary:
*  Compares two byte arrays for equality over the specified length. Returns
*  false if either pointer is NULL or any byte differs.
*
* Parameters:
*  arr1 - First byte array to compare
*  arr2 - Second byte array to compare
*  len  - Number of bytes to compare
*
* Return:
*  bool - true if arrays are identical, false otherwise
*
*******************************************************************************/
bool compare_hex_array(const uint8_t *arr1, const uint8_t *arr2, uint16_t len)
{
    if (arr1 == NULL || arr2 == NULL)
    {
        return false;
    }

    for (uint16_t i = RESET_VAL; i < len; i++)
    {
        if (arr1[i] != arr2[i])
        {
            return false;
        }
    }

    return true;
}

/*******************************************************************************
* Function Name: app_bt_gatt_operation_cplt_handler
********************************************************************************
* Summary:
*  Handles GATT operation completion events (notifications, indications,
*  read responses). Processes Service Changed indications, database hash
*  validation, read responses for custom characteristics, and manages pairing/
*  encryption when required.
*
* Parameters:
*  p_operation_cplt - Pointer to GATT operation complete data
*
* Return:
*  wiced_bt_gatt_status_t - Always returns WICED_BT_GATT_SUCCESS
*
*******************************************************************************/
wiced_bt_gatt_status_t
app_bt_gatt_operation_cplt_handler(wiced_bt_gatt_operation_complete_t *p_operation_cplt)
{
    wiced_bt_gatt_status_t status;
    uint8_t peer_index;
    uint16_t handle;
    wiced_result_t result;
    wiced_bt_gatt_status_t gatt_status;
    uint16_t length = VALUE_0;
    switch(p_operation_cplt->op)
    {
        /* Notification/Indication received from GATT Server */
        case GATTC_OPTYPE_NOTIFICATION:
        case GATTC_OPTYPE_INDICATION:
            /* If the GATT Server has sent an indication, send a confirmation packet to GATT Server */
            if(p_operation_cplt->op == GATTC_OPTYPE_INDICATION)
            {
                status = wiced_bt_gatt_client_send_indication_confirm(p_operation_cplt->conn_id,
                        p_operation_cplt->response_data.att_value.handle);
                printf("Indication confirmation send status = %d\n", status);
            }
            if(HDLC_GATT_SERVICE_CHANGED_VALUE == p_operation_cplt->response_data.att_value.handle)
            {
                app_bt_clear_discovered_handles();
                cs_discovery_data->service_found = false;
                gatt_status = wiced_bt_gatt_client_send_discover(p_operation_cplt->conn_id,
                                                    GATT_DISCOVER_SERVICES_ALL,
                                                            &service_discovery_setup);
                if(WICED_BT_GATT_SUCCESS != gatt_status)
                {
                    printf("GATT Discovery request failed. Error code: %d, "
                            "Conn id: %d\n", gatt_status,p_operation_cplt->conn_id);
                }
                else
                {
                    printf("\nService Discovery Started\n\n");
                }
            }
            /* This notification or Indication needs to be sent to Central.
               Get the handle of service in which the value from GATT server
               needs to be stored using the associated connection ID of peer GATT Server */
            handle = app_bt_get_service_handle_from_conn_id(p_operation_cplt->conn_id);
            length = p_operation_cplt->response_data.att_value.len;

            p_operation_cplt->response_data.att_value.p_data[length] = '\0';
        
            break;

        case GATTC_OPTYPE_READ_BY_TYPE:
            printf("\r\n Read by type request to get handle..\r\n");

            printf("Handle = 0x%X\r\n", p_operation_cplt->response_data.handle);

            break;

        /* Read response received from GATT Server */
        case GATTC_OPTYPE_READ_HANDLE:
            /* Get the handle of service in which the read response value from GATT server
               needs to be stored using the associated connection ID of peer GATT Server */
            handle = app_bt_get_service_handle_from_conn_id(p_operation_cplt->conn_id);

            /* Get the index of peer data stored to set flags to track completion of read */
            peer_index = app_bt_get_peer_index(p_operation_cplt->conn_id);
            if(MAX_NUM_CONNECTIONS <= peer_index )
            {
                printf("This app only supports upto 4 connections (including Central device)\n");

                return WICED_BT_GATT_SUCCESS;
            }
                        
            /* Check the handle from which the read response is received. There are three readable attributes: 
                Notify, CCCD and Blink characteristic */
            if((p_operation_cplt->status == WICED_BT_GATT_SUCCESS) && (p_operation_cplt->response_data.handle == 
                                                                        cs_discovery_data->char_val_notify_handle))
            {
                hash_read_after_discovery = true;

                gatt_status = wiced_bt_gatt_client_send_read_handle(p_operation_cplt->conn_id,
                                curr_handle_gatt->database_hash_handle, VALUE_0,
                                database_hash, sizeof(database_hash),
                                GATT_AUTH_REQ_NONE);
                
                if(WICED_BT_GATT_SUCCESS != gatt_status)
                {
                    printf("GATT read by handle request failed. Error code: %d, "
                            "Conn id: %d\n", gatt_status, p_operation_cplt->conn_id);
                }

            }
            else if((p_operation_cplt->status == WICED_BT_GATT_SUCCESS) && (p_operation_cplt->response_data.handle == 
                                                                        cs_discovery_data->char_val_blink_handle))
            {
                printf("Received Custom Service 1 data characteristic value :");
                
                printf("%d \n\r", p_operation_cplt->response_data.att_value.p_data[VALUE_0]);
                
            }
            else if((p_operation_cplt->status == WICED_BT_GATT_SUCCESS) && (p_operation_cplt->response_data.handle == 
                                                                        cs_discovery_data->cccd_handle))
            {
                printf("\nRead response received for cccd\n");
                for(uint8_t i = RESET_VAL; i < app_gatt_db_ext_attr_tbl_size; i++)
                {
                    if(app_gatt_db_ext_attr_tbl[i].handle == (handle + CCCD_ATT_OFFSET))
                    {
                        memcpy(app_gatt_db_ext_attr_tbl[i].p_data, p_operation_cplt->response_data.att_value.p_data, 
                                app_gatt_db_ext_attr_tbl[i].max_len);

                        break;
                    }
                }

                /* Read request on Blink characteristic is successful, Set the flag */
                peer_info[peer_index].is_read_cccd_cplt = TRUE;

            }
            /* Check if the GATT Server sent the error code indicating authentication is required before accessing 
               the attribute Start the pairing procedure or encrypt the link based on bond info for the peer */
            else if(((p_operation_cplt->status == WICED_BT_GATT_INSUF_ENCRYPTION) || (p_operation_cplt->status == 
                                                                                WICED_BT_GATT_INSUF_AUTHENTICATION))
                && (p_operation_cplt->response_data.handle == cs_discovery_data->char_val_notify_handle))
            {
                /* Reset the flag for read request to notify characteristic, our device has to send read request once 
                    encryption is complete */
                peer_info[peer_index].is_read_notify_char_cplt = FALSE;
                printf("Response received from server for read request: ");
                if(WICED_BT_GATT_INSUF_ENCRYPTION == p_operation_cplt->status)
                {
                    printf("WICED_BT_GATT_INSUF_ENCRYPTION\n");
                }
                else if(WICED_BT_GATT_INSUF_AUTHENTICATION == p_operation_cplt->status)
                {
                    printf("WICED_BT_GATT_INSUF_AUTHENTICATION\n");
                }
                /* Check if the device is present in NVRAM, if yes, we already have the keys to encrypt the connection, 
                    if no, we need to initiate pairing */
                if(BOND_INDEX_MAX == app_bt_find_device_in_nvm(peer_info->remote_addr))
                {
                    pairing_started = true;
                    printf("Peer device was not bonded previously, initiate pairing and bonding\n ");
                    result = wiced_bt_dev_sec_bond(peer_info[peer_index].remote_addr,
                                                    peer_info[peer_index].addr_type,
                                                    peer_info[peer_index].transport,
                                                    VALUE_0, NULL);
                    if(WICED_BT_PENDING == result)
                    {
                        printf("Bonding initiated successfully \n");
                    }
                    else if(WICED_BT_SUCCESS == result)
                    {
                        printf("Peer device already paired \n");
                    }
                    else
                    {
                        printf("wiced_bt_dev_sec_bond failed, error code = %d\n", result);
                    }
                }
                else
                {
                    printf("Peer device is already bonded, encrypt the link\n");
                    wiced_bt_ble_sec_action_type_t sec_type = BTM_BLE_SEC_ENCRYPT;
                    result = wiced_bt_dev_set_encryption(peer_info[peer_index].remote_addr,
                                                         peer_info[peer_index].transport,
                                                         (void*)&sec_type);
                    if(WICED_BT_BUSY == result || WICED_BT_PENDING ==result)
                    {
                        printf("Encryption in progress, Check BTM_ENCRYPTION_STATUS_EVT for status!\n");
                    }
                    else if(WICED_BT_SUCCESS == result)
                    {
                        printf("Link already encrypted\n");
                    }
                    else if(WICED_BT_WRONG_MODE == result)
                    {
                        printf("Peer device is not connected\n");
                    }
                    else
                    {
                        printf("wiced_bt_dev_set_encryption failed, error code - %d\n", result);
                    }
                }
            }
            else if((p_operation_cplt->status == WICED_BT_GATT_SUCCESS) && (p_operation_cplt->response_data.handle == 
                                                                        new_cs_discovery_data->char_val_blink_handle ))
            {
                printf("Received New Custom Service 2 data characteristic value : ");
                for (int i = RESET_VAL; i < p_operation_cplt->response_data.att_value.len; i++)
                {
                    printf("%c", p_operation_cplt->response_data.att_value.p_data[i]);
                }
                printf("\n");

                 break;
            }
            else if((p_operation_cplt->status == WICED_BT_GATT_SUCCESS) && (p_operation_cplt->response_data.handle == 
                                                                        curr_handle_gatt->database_hash_handle))
            {
                if( BT_DB_HASH_LEN != p_operation_cplt->response_data.att_value.len)
                {
                    printf("\n Invalid database hash length. \n\r"); 

                    break;
                }

                printf("Received database hash : ");
                /* print the hash value */
                for (int i = RESET_VAL; i < p_operation_cplt->response_data.att_value.len; i++) 
                {
                    printf("%02x ", p_operation_cplt->response_data.att_value.p_data[i]);
                }
                printf("\n");

                uint8_t index = app_bt_get_peer_index(p_operation_cplt->conn_id);
                if ( MAX_NUM_CONNECTIONS <= index )
                {
                    printf("Invalid peer index (%d). Ignoring database hash.\n", index);
                    break;
                }

                if ( (!hash_read_after_discovery) && ( ! compare_hex_array(peer_info[index].database_hash,
                    p_operation_cplt->response_data.att_value.p_data , BT_DB_HASH_LEN) ) )
                {
                    printf("HASH key value mismatched\n\r");

                    app_bt_clear_discovered_handles();
                    cs_discovery_data->service_found = false;
                    gatt_status = wiced_bt_gatt_client_send_discover(p_operation_cplt->conn_id,
                                                        GATT_DISCOVER_SERVICES_ALL,
                                                                &service_discovery_setup);
                    if(WICED_BT_GATT_SUCCESS != gatt_status)
                    {
                        printf("GATT Discovery request failed. Error code: %d, "
                                "Conn id: %d\n", gatt_status, p_operation_cplt->conn_id);
                    }
                    else
                    {
                        printf("\nService Discovery Started\n\n");
                    }
                }
                else
                {
                    if(!hash_read_after_discovery)
                    {
                        printf("HASH key matched. Skipping Service Discovery.\n\r");
                    }
                    wiced_bt_gatt_write_hdr_t app_write_csf = {RESET_VAL};
                    app_write_csf.auth_req = GATT_AUTH_REQ_NONE;
                    app_write_csf.handle = curr_handle_gatt->client_supported_features_handle;
                    app_write_csf.len = CSF_CHAR_VALUE_LENGTH;
                    app_write_csf.offset = VALUE_0;

                    uint8_t data[] = {CSF_FEATURE_ROBUST_CACHING_ENABLE};

                    gatt_status = wiced_bt_gatt_client_send_write(p_operation_cplt->conn_id,
                                GATT_REQ_WRITE, &app_write_csf,
                                data, NULL);

                    if(WICED_BT_GATT_SUCCESS != gatt_status)
                    {
                        printf("GATT write request failed. Error code: %d, "
                                "Conn id: %d\n", gatt_status, p_operation_cplt->conn_id);
                    }

                }
                if ( MAX_NUM_CONNECTIONS > index )
                {
                    memcpy(peer_info[index].database_hash,p_operation_cplt->response_data.att_value.p_data ,
                            p_operation_cplt->response_data.att_value.len);
                }
                
                app_bt_update_peer_info();
            }

            break;   

    }
    
    return WICED_BT_GATT_SUCCESS;
}

/*******************************************************************************
* Function Name: app_bt_write_peer_cccd
********************************************************************************
* Summary:
*  Sends a GATT write request to the peer's CCCD to enable/disable notifications
*  or indications for the GATT Service Changed characteristic.
*
* Parameters:
*  cccd_val - 2-byte CCCD value (little-endian)
*
* Return:
*  void
*
*******************************************************************************/
void app_bt_write_peer_cccd(uint8_t* cccd_val)
{
    wiced_bt_gatt_write_hdr_t app_write_cccd = {RESET_VAL};
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    uint8_t notif_val[CCCD_LEN];

    notif_val[CCCD_INDEX_0] = cccd_val[CCCD_INDEX_0];
    notif_val[CCCD_INDEX_1] = cccd_val[CCCD_INDEX_1];
    app_write_cccd.auth_req = GATT_AUTH_REQ_NONE;
    app_write_cccd.handle = SC_CCCD_ATT_HANDLE;
    app_write_cccd.len = CCCD_LEN;
    app_write_cccd.offset = RESET_VAL;
    status = wiced_bt_gatt_client_send_write(current_conn_id, GATT_REQ_WRITE, &app_write_cccd, notif_val, NULL);
    if(WICED_BT_GATT_SUCCESS != status)
    {
        printf("GATT Write request to CCCD failed, error code = %x \n", status);
    }
    else
    {
        printf("Write request to CCCD is successful\n");
    }
}

/*******************************************************************************
* Function Name: app_bt_store_peer_info
********************************************************************************
* Summary:
*  Stores or updates peer connection information (conn_id, address, role, etc.)
*  in the global peer_info array. Reuses existing slot if conn_id exists,
*  otherwise finds the first empty slot.
*
* Parameters:
*  peer_conn_info - Pointer to connection status structure from the stack
*
* Return:
*  void
*
*******************************************************************************/
void app_bt_store_peer_info(wiced_bt_gatt_connection_status_t* peer_conn_info)
{
    uint8_t index = app_bt_get_peer_index(peer_conn_info->conn_id);

    if(MAX_NUM_CONNECTIONS == index)
    {
        for (index = RESET_VAL; index < MAX_NUM_CONNECTIONS; index++)
        {
            if (peer_info[index].conn_id == VALUE_0)
            {
                peer_info[index].handle_gatt.service_changed_handle = RESET_VAL;
                peer_info[index].handle_gatt.client_supported_features_handle = RESET_VAL;
                peer_info[index].handle_gatt.database_hash_handle = RESET_VAL;
                break;
            }
        }

        /* If no free slot available, simply return */
        if (index == MAX_NUM_CONNECTIONS)
        {
            return;
        }
    }

    /* Store/Update peer info in the determined index */
    peer_info[index].conn_id    = peer_conn_info->conn_id;
    peer_info[index].link_role  = peer_conn_info->link_role;
    peer_info[index].transport  = peer_conn_info->transport;
    peer_info[index].connected  = peer_conn_info->connected;
    peer_info[index].addr_type  = peer_conn_info->addr_type;

    memcpy(peer_info[index].remote_addr, peer_conn_info->bd_addr, BD_ADDR_LEN);

    curr_handle_gatt = &peer_info[index].handle_gatt;
    cs_discovery_data = &peer_info[index].cs_discovery_data;
    new_cs_discovery_data = &peer_info[index].new_cs_discovery_data;

    return;
}

/*******************************************************************************
* Function Name: app_bt_get_peer_index
********************************************************************************
* Summary:
*  Searches the peer_info array and returns the index corresponding to the
*  given connection ID. Returns MAX_NUM_CONNECTIONS if not found.
*
* Parameters:
*  conn_id - GATT connection identifier
*
* Return:
*  uint8_t - Index in peer_info[] or MAX_NUM_CONNECTIONS if not found
*
*******************************************************************************/
uint8_t app_bt_get_peer_index(uint16_t conn_id)
{
    uint8_t index;
    for (index = RESET_VAL; index < MAX_NUM_CONNECTIONS; index++)
    {
        if(conn_id == peer_info[index].conn_id)
        {
            return index;
        }
    }
    return MAX_NUM_CONNECTIONS;
}

/*******************************************************************************
* Function Name: app_bt_get_peer_index
********************************************************************************
* Summary:
*  Read all the characteristics of peer.
*
* Parameters:
*  uint16_t conn_id: connection ID of the peer to send write req
*
* Return:
*  void
*
*******************************************************************************/
void app_bt_read_peer_attributes(uint16_t conn_id)
{
    uint8_t peer_index;
    peer_index = app_bt_get_peer_index(conn_id);

    if(MAX_NUM_CONNECTIONS <= peer_index )
    {
        printf("This app only supports upto 4 connections (including Central device)\n");
        return;
    }
    /* Send read request to Notify Value Attribute */
    wiced_bt_gatt_client_send_read_handle(conn_id, cs_discovery_data->char_val_notify_handle, VALUE_0, read_resp_data, 
                                                    sizeof(wiced_bt_gatt_operation_complete_t), GATT_AUTH_REQ_NONE);

}

/*******************************************************************************
* Function Name: app_bt_update_gatt_db
********************************************************************************
* Summary:
*  Updates the local GATT database when a Central writes to dynamic attributes
*  (e.g. Device Name). Currently handles ATT_DEVICE_NAME by assigning a free
*  service instance and populating the peripheral's advertised name.
*
* Parameters:
*  att_val_id - Identifier of the attribute being updated (ATT_DEVICE_NAME, etc.)
*  conn_id    - Connection ID of the peripheral
*  handle     - Handle in local GATT DB that was written
*
* Return:
*  void
*
*******************************************************************************/
void app_bt_update_gatt_db( uint8_t att_val_id, uint16_t conn_id, uint16_t handle)
{
    switch(att_val_id)
    {
        case ATT_DEVICE_NAME:
        {
            uint8_t att_handle;
            /* Search for a free custom service. So that, the new connection can be allocated to the free service */
            for(uint8_t index = RESET_VAL; index < MAX_PERIPHERALS; index++)
            {
                if(TRUE == local_gatt_db_info[index].is_service_free)
                {
                    /* Update the flag in local gatt db info to indicate that the service will be occupied by current connection */
                    local_gatt_db_info[index].is_service_free = FALSE;
                    att_handle = local_gatt_db_info[index].service_handle + DEVICE_NAME_ATT_OFFSET;
                    local_gatt_db_info[index].conn_id = conn_id;
                    /* Populate the GATT DB with peer device name, so that a user can identify the service associated with peripheral */
                    for(uint8_t i = RESET_VAL; i < app_gatt_db_ext_attr_tbl_size; i++)
                    {
                        if(app_gatt_db_ext_attr_tbl[i].handle == att_handle)
                        {
                            app_gatt_db_ext_attr_tbl[i].p_data = adv_device_name;

                            break;
                        }
                    }
                    break;
                }
            }
        }
        break;
    }
}

/*******************************************************************************
* Function Name: app_bt_get_service_handle_from_conn_id
********************************************************************************
* Summary:
*  Looks up and returns the base service handle in the local GATT database
*  that corresponds to the given peripheral connection ID.
*
* Parameters:
*  conn_id - GATT connection ID of the peripheral device
*
* Return:
*  uint16_t - Service start handle, or 0 if not found
*
*******************************************************************************/
uint16_t app_bt_get_service_handle_from_conn_id(uint16_t conn_id)
{
    for(uint8_t i = RESET_VAL; i < MAX_PERIPHERALS; i++)
    {
        if(local_gatt_db_info[i].conn_id == conn_id)
        {
            return local_gatt_db_info[i].service_handle;
        }
    }
    return 0;
}

/* END OF FILE [] */
