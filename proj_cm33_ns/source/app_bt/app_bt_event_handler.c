/*******************************************************************************
* File Name        : app_bt_event_handler.c
*
* Description      : This file contains the bluetooth event handler that 
*                    processes the bluetooth events from host stack.
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
#include "app_bt_bonding.h"
#include "cycfg_gap.h"
#include "app_bt_utils.h"
#include "app_hw_device.h"
#include "app_bt_event_handler.h"
#include "app_bt_gatt_handler.h"
#include "app_bt_utils.h"
#include "wiced_bt_stack.h"
#include "wiced_bt_dev.h"
#include "wiced_bt_ble.h"
#include "retarget_io_init.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* Macros to store the local BDA*/
#define LOCAL_BDA_1                             (0xC0U)
#define LOCAL_BDA_2                             (0xA0U)
#define LOCAL_BDA_3                             (0x50U)
#define LOCAL_BDA_4                             (0x11U)
#define LOCAL_BDA_5                             (0x44U)
#define LOCAL_BDA_6                             (0xBBU)
#define DEVICE_NAME_LEN                         (15U)  
#define INIT_VAL_0                              (0U)
#define PEER_INDEX_FIRST                        (0u)
#define PAIRING_IO_CAPABILITIES_MAX_KEY_SIZE    (0x10U)

/*******************************************************************************
* Global Variables 
*******************************************************************************/
/* Variable to hold connection information about the peer device */
hello_client_peer_info_t peer_info[MAX_NUM_CONNECTIONS];

/* Variable to store name from advertisement data */
uint8_t adv_device_name[DEVICE_NAME_LEN];

/* Variable to store current connection ID */
uint16_t current_conn_id;

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: app_bt_management_callback
********************************************************************************
* Summary:
*  This is a Bluetooth stack event handler function to receive
*  management events from the LE stack and process as per the application.
*
* Parameters:
*  wiced_bt_management_evt_t event : LE event code of one byte length
*  wiced_bt_management_evt_data_t *p_event_data: Pointer to LE management
*                                                event structures
*
* Return:
*  wiced_result_t : Error code from WICED_RESULT_LIST or BT_RESULT_LIST
*
*******************************************************************************/
wiced_result_t app_bt_management_callback(wiced_bt_management_evt_t event,
        wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t result = WICED_BT_SUCCESS;
    cy_rslt_t rslt;
    wiced_bt_dev_encryption_status_t *p_status;
    wiced_bt_dev_ble_pairing_info_t *p_info;
    wiced_bt_device_address_t local_bda = {LOCAL_BDA_1, LOCAL_BDA_2,LOCAL_BDA_3,
                                           LOCAL_BDA_4, LOCAL_BDA_5,LOCAL_BDA_6};
    uint8_t peer_index = INIT_VAL_0;
    uint8_t  bondindex = INIT_VAL_0;

    switch (event)
    {
        case BTM_ENABLED_EVT:
            /* Bluetooth Controller and Host Stack Enabled */
            if (WICED_BT_SUCCESS == p_event_data->enabled.status)
            {
                wiced_bt_set_local_bdaddr(local_bda, BLE_ADDR_PUBLIC);
                wiced_bt_dev_read_local_addr(local_bda);
                printf("Local Bluetooth Address: ");
                print_bd_address(local_bda);

                /* Perform application-specific initialization */
                app_bt_application_init();
                peer_index = app_bt_get_peer_index(current_conn_id);
                printf("Connected peer index = %d\n", peer_index);

                if(CY_RSLT_SUCCESS == app_bt_restore_peer_info())
                {
                    printf("successfully restore the db handle info from DB\n");
                      
                }
                print_peer_info(PEER_INDEX_FIRST);
            }
            else
            {
                printf("Bluetooth enable failed \n");
            }

            break;

        case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:
            p_event_data->pairing_io_capabilities_ble_request.local_io_cap  =  \
            BTM_IO_CAPABILITIES_NONE;
            p_event_data->pairing_io_capabilities_ble_request.oob_data      =  \
            BTM_OOB_NONE;
            p_event_data->pairing_io_capabilities_ble_request.auth_req      =  \
            BTM_LE_AUTH_REQ_SC_BOND;
            p_event_data->pairing_io_capabilities_ble_request.max_key_size  =  \
            PAIRING_IO_CAPABILITIES_MAX_KEY_SIZE;
            p_event_data->pairing_io_capabilities_ble_request.init_keys     =  \
            BTM_LE_KEY_PENC|BTM_LE_KEY_PID|BTM_LE_KEY_PCSRK|BTM_LE_KEY_LENC;
            p_event_data->pairing_io_capabilities_ble_request.resp_keys     =  \
            BTM_LE_KEY_PENC|BTM_LE_KEY_PID|BTM_LE_KEY_PCSRK|BTM_LE_KEY_LENC;

            break;

        case BTM_PAIRING_COMPLETE_EVT:
            p_info = &p_event_data->pairing_complete.pairing_complete_info.ble;
            printf("Pairing Complete Reason: %s \n",
                    get_bt_smp_status_name((wiced_bt_smp_status_t)
                    p_info->reason));

            /* Update Number of bonded devices and next free slot in slot data*/
            rslt = app_bt_update_slot_data();
            if(CY_RSLT_TYPE_ERROR == rslt)
            {
                printf("Error updating number of bonded device!\n");
            }
            pairing_started = false;
            app_bt_read_peer_attributes(current_conn_id);

            break;

        case BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT:
            /* save device keys to NVM */
            rslt = app_bt_save_device_link_keys(&
                    (p_event_data->paired_device_link_keys_update));

            if (CY_RSLT_SUCCESS == rslt)
            {
                printf("Successfully Bonded to ");
                print_bd_address(p_event_data->
                        paired_device_link_keys_update.bd_addr);
            }
            else
            {
                printf("Failed to bond! \n");
            }

            break;

        case  BTM_PAIRED_DEVICE_LINK_KEYS_REQUEST_EVT:
            /* Paired Device Link Keys Request */
            printf("Paired Device Link keys Request Event for device ");
            print_bd_address((uint8_t *)(p_event_data->
                    paired_device_link_keys_request.bd_addr));

            /* Need to search to see if the BD_ADDR
             * is in NVM. If not, return WICED_BT_ERROR
             * and the stack will generate keys and will then call
             * BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT so that they
             * can be stored.
             */

            /* Assume the device won't be found.
             * If it is, set this back to WICED_BT_SUCCESS */
            result = WICED_BT_ERROR;
            bondindex = app_bt_find_device_in_nvm(p_event_data->paired_device_link_keys_request.bd_addr);

            if(BOND_INDEX_MAX > bondindex)
            {
                /* Copy the keys to where the stack wants it */
                memcpy(&(p_event_data->paired_device_link_keys_request),
                       &bond_info.link_keys[bondindex],
                       sizeof(wiced_bt_device_link_keys_t));
                result = WICED_BT_SUCCESS;
            }
            else
            {
                printf("Device Link Keys not found in the database! \n");
            }

            break;

        case BTM_LOCAL_IDENTITY_KEYS_UPDATE_EVT:
            /* Update of local privacy keys - save to NVM */
            rslt = app_bt_save_local_identity_key(p_event_data->
                    local_identity_keys_update);
            if (CY_RSLT_SUCCESS != rslt)
            {
                result = WICED_BT_ERROR;
            }

            break;

        case  BTM_LOCAL_IDENTITY_KEYS_REQUEST_EVT:
            /* Read Local Identity Resolution Keys if present in NVM*/
            rslt = app_bt_read_local_identity_keys();

            if(CY_RSLT_SUCCESS == rslt)
            {
                memcpy(&(p_event_data->local_identity_keys_request),
                       &(identity_keys), sizeof(wiced_bt_local_identity_keys_t));
                print_array(&identity_keys, sizeof(wiced_bt_local_identity_keys_t));
                result = WICED_BT_SUCCESS;
            }
            else
            {
               result = WICED_BT_ERROR;
            }

             break;

        case BTM_ENCRYPTION_STATUS_EVT:
            peer_index = app_bt_get_peer_index(current_conn_id);
            if(MAX_NUM_CONNECTIONS <= peer_index)
            {
                printf("This app only supports upto 4 connections (including Central device)\n");
                return WICED_BT_SUCCESS;
            }
            p_status = &p_event_data->encryption_status;
            printf("Encryption status changed, Encryption Status: %d , BDA:", p_status->result);
            print_bd_address(p_status->bd_addr);
            if(CY_RSLT_SUCCESS != p_status->result)
            {
                int index = app_bt_find_device_in_nvm(p_status->bd_addr);

                if ( MAX_NUM_CONNECTIONS > index ) 
                {
                    app_bt_delete_device_info(index);
                }
                else
                {
                    printf("Device not found in NVM, skip delete\n");
                }
            }
            if( !pairing_started )
            {
                app_bt_read_peer_attributes(current_conn_id);
            }

            break;

        case BTM_SECURITY_REQUEST_EVT:
            wiced_bt_ble_security_grant(p_event_data->security_request.bd_addr,
                                        WICED_BT_SUCCESS);

            break;

        case BTM_BLE_CONNECTION_PARAM_UPDATE:
            printf("Connection parameter update status:%d, "
                   "Connection Interval: %d, "
                   "Connection Latency: %d, "
                   "Connection Timeout: %d\n",
                    p_event_data->ble_connection_param_update.status,
                    p_event_data->ble_connection_param_update.conn_interval,
                    p_event_data->ble_connection_param_update.conn_latency,
                    p_event_data->ble_connection_param_update.supervision_timeout);

            break;

        default:
            printf("Unhandled Bluetooth Management Event: 0x%x\n",
                    event);

            break;
    }

    return result;
}

/*******************************************************************************
* Function Name: app_bt_application_init
********************************************************************************
* Summary:
*  This function handles application level initialization tasks and is
*  called from the BT management callback once the LE stack enabled event
*  (BTM_ENABLED_EVT) is triggered. This function is executed in the
*  BTM_ENABLED_EVT management callback.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void app_bt_application_init(void)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;

    user_btn_interrupt_config();

    if(CY_RSLT_SUCCESS == app_bt_restore_bond_data())
    {
        printf("Keys found in NVM, add them to Addr Res DB\n");

        /* Load previous paired keys for address resolution */
        app_bt_add_devices_to_address_resolution_db();
    }

    /* Allow peer to pair */
    wiced_bt_set_pairable_mode(WICED_TRUE, FALSE);

    /* Register with BT stack to receive GATT callback */
    gatt_status = wiced_bt_gatt_register(app_bt_gatt_callback);
    printf("GATT event Handler registration status: %s \n",
            get_bt_gatt_status_name(gatt_status));

    printf("GATT database initialization status: %s \n",
           get_bt_gatt_status_name(gatt_status));
}

/************************************************************************************************
* Function Name: app_scan_result_handler
************************************************************************************************
* Summary:
*  BLE scan callback that filters advertisement packets. On detecting a device advertising 
*  the target 128-bit custom service UUID, stops scanning, extracts the device name, and 
*  initiates GATT connection.
*
* Parameters:
*  p_scan_result : Pointer to scan result (BD address, RSSI, etc.)
*  p_adv_data    : Pointer to raw advertisement data
*
* Return:
*  void
*
************************************************************************************************/
void app_scan_result_handler(wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data)
{
    wiced_result_t status = WICED_BT_SUCCESS;
    uint8_t length = INIT_VAL_0;
    uint8_t *p_data = NULL;

    if (p_scan_result)
    {
        /* Search for complete UUID in incoming ADV packet */
        p_data = wiced_bt_ble_check_advertising_data(p_adv_data,
                                                     BTM_BLE_ADVERT_TYPE_128SRV_COMPLETE,
                                                     &length);
        if (NULL != p_data)
        {
            /* Check if the peer device's UUID is for the custom service */
            if ((length == MAX_UUID_SIZE) &&
                ( !memcmp(p_data, (uint8_t *)uuid_service_custom_service, MAX_UUID_SIZE) ) )
            {
                printf("Scan completed\n Found peer device with BDA:\n");
                print_bd_address(p_scan_result->remote_bd_addr);

                /* Device found. Stop scan. */
                if (WICED_BT_SUCCESS != (status = wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_NONE, true,
                                                app_scan_result_handler)))
                {
                    printf("Scan off status %d\n", status);
                }

                /* After finding the peripheral, store the name from ADV packet.
                   The ADV name will be stored in Device name characteristic in
                   local GATT DB so that, the Central connected to us can identify
                   the Peripheral connected and perform GATT operations like
                   read/write to desired peripheral*/
                p_data = wiced_bt_ble_check_advertising_data(p_adv_data,
                                                BTM_BLE_ADVERT_TYPE_NAME_COMPLETE,
                                                &length);
                memcpy(adv_device_name, p_data, length);
                adv_device_name[length] = '\0';

                /* Initiate the connection */
                if (WICED_TRUE != wiced_bt_gatt_le_connect(p_scan_result->remote_bd_addr,
                                             p_scan_result->ble_addr_type,
                                             BLE_CONN_MODE_HIGH_DUTY,
                                             WICED_TRUE))
                {
                    printf("wiced_bt_gatt_connect failed\n");
                }
                else
                {
                    printf("GATT connection request sent to BDA:");
                    print_bd_address(p_scan_result->remote_bd_addr);
                }
            }
            else
            {
                /* Skip - This is not the device we are looking for. */
                return;
            }
        }
    }
}

/* END OF FILE [] */