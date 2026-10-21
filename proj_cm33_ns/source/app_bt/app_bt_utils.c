/*******************************************************************************
* File Name        : app_bt_utils.c
*
* Description      : This file consists of the utility functions that will help
*                    debugging and developing the applications easier with much
*                    more meaningful information.
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
#include "app_bt_utils.h"
#include "wiced_bt_dev.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define MOD_VALUE  (16U)
#define RESET_VAL  (0U)
#define CASE_DEFAULT_STR  \
        default: printf( "%s: defalut case\n", __FUNCTION__ ); break;

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: print_bd_address()
********************************************************************************
* Summary:
*  This is the utility function that prints the address of the
*          Bluetooth device
*
* Parameters:
*  wiced_bt_device_address_t bdadr: Bluetooth address
*
* Return:
*  void
*
*******************************************************************************/
void print_bd_address(wiced_bt_device_address_t bdadr)
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X \n",bdadr[0],bdadr[1],bdadr[2],
            bdadr[3],bdadr[4],bdadr[5]);
}

/*******************************************************************************
* Function Name: void print_array( void* to_print, uint16_t len )
********************************************************************************
* Summary:
*  This is a utility function that prints the specified number of
*  values from memory
*
* Parameters:
*  void* to_print  : Pointer to the location to print
*  uint16_t        : Number of bytes to print
*
* Return: 
*  void
*
*******************************************************************************/
void print_array(void * to_print, uint16_t len)
{
    uint16_t counter;

    for( counter = RESET_VAL; counter<len;counter++ )
    {
       if( RESET_VAL == counter % MOD_VALUE )
       {
           printf( "\n" );
       }

       printf( "%02X ", *(((uint8_t *)(to_print)) + counter) );
    }

    printf( "\n" );
}

/*******************************************************************************
* Function Name: get_bt_gatt_disconn_reason_name
********************************************************************************
* Summary:
*  The function converts the wiced_bt_gatt_disconn_reason_t enum value
*  to its corresponding string literal. This will help the programmer
*  to debug easily with log traces without navigating through the
*  source code.
*
* Parameters:
*  wiced_bt_gatt_disconn_reason_t reason: GATT Disconnection reason
*
* Return:
*  wiced_bt_gatt_disconn_reason_t
*
*******************************************************************************/
const char *get_bt_gatt_disconn_reason_name(wiced_bt_gatt_disconn_reason_t reason)
{
    switch ((int)reason)
    {
        CASE_RETURN_STR(GATT_CONN_UNKNOWN)
        CASE_RETURN_STR(GATT_CONN_L2C_FAILURE)
        CASE_RETURN_STR(GATT_CONN_TIMEOUT)
        CASE_RETURN_STR(GATT_CONN_TERMINATE_PEER_USER)
        CASE_RETURN_STR(GATT_CONN_TERMINATE_LOCAL_HOST)
        CASE_RETURN_STR(GATT_CONN_FAIL_ESTABLISH)
        CASE_RETURN_STR(GATT_CONN_LMP_TIMEOUT)
        CASE_RETURN_STR(GATT_CONN_CANCEL)
        CASE_DEFAULT_STR
    }

    return "UNKNOWN_REASON";
}

/*******************************************************************************
* Function Name: get_bt_gatt_status_name
********************************************************************************
* Summary:
*  The function converts the wiced_bt_gatt_status_t enum value
*  to its corresponding string literal. This will help the programmer to debug
*  easily with log traces without navigating through the source code.
*
* Parameters:
*  wiced_bt_gatt_status_t status: GATT status
*
* Return:
*  wiced_bt_gatt_status_t
*
*******************************************************************************/
const char *get_bt_gatt_status_name(wiced_bt_gatt_status_t status)
{
    switch ((int)status)
    {
        CASE_RETURN_STR(WICED_BT_GATT_SUCCESS || WICED_BT_GATT_ENCRYPTED_MITM)
        CASE_RETURN_STR(WICED_BT_GATT_INVALID_HANDLE)
        CASE_RETURN_STR(WICED_BT_GATT_READ_NOT_PERMIT)
        CASE_RETURN_STR(WICED_BT_GATT_WRITE_NOT_PERMIT)
        CASE_RETURN_STR(WICED_BT_GATT_INVALID_PDU)
        CASE_RETURN_STR(WICED_BT_GATT_INSUF_AUTHENTICATION)
        CASE_RETURN_STR(WICED_BT_GATT_REQ_NOT_SUPPORTED)
        CASE_RETURN_STR(WICED_BT_GATT_INVALID_OFFSET)
        CASE_RETURN_STR(WICED_BT_GATT_INSUF_AUTHORIZATION)
        CASE_RETURN_STR(WICED_BT_GATT_PREPARE_Q_FULL)
        CASE_RETURN_STR(WICED_BT_GATT_ATTRIBUTE_NOT_FOUND)
        CASE_RETURN_STR(WICED_BT_GATT_NOT_LONG)
        CASE_RETURN_STR(WICED_BT_GATT_INSUF_KEY_SIZE)
        CASE_RETURN_STR(WICED_BT_GATT_INVALID_ATTR_LEN)
        CASE_RETURN_STR(WICED_BT_GATT_ERR_UNLIKELY)
        CASE_RETURN_STR(WICED_BT_GATT_INSUF_ENCRYPTION)
        CASE_RETURN_STR(WICED_BT_GATT_UNSUPPORT_GRP_TYPE)
        CASE_RETURN_STR(WICED_BT_GATT_INSUF_RESOURCE)
        CASE_RETURN_STR(WICED_BT_GATT_ILLEGAL_PARAMETER)
        CASE_RETURN_STR(WICED_BT_GATT_NO_RESOURCES)
        CASE_RETURN_STR(WICED_BT_GATT_INTERNAL_ERROR)
        CASE_RETURN_STR(WICED_BT_GATT_WRONG_STATE)
        CASE_RETURN_STR(WICED_BT_GATT_DB_FULL)
        CASE_RETURN_STR(WICED_BT_GATT_BUSY)
        CASE_RETURN_STR(WICED_BT_GATT_ERROR)
        CASE_RETURN_STR(WICED_BT_GATT_CMD_STARTED)
        CASE_RETURN_STR(WICED_BT_GATT_PENDING)
        CASE_RETURN_STR(WICED_BT_GATT_AUTH_FAIL)
        CASE_RETURN_STR(WICED_BT_GATT_MORE)
        CASE_RETURN_STR(WICED_BT_GATT_INVALID_CFG)
        CASE_RETURN_STR(WICED_BT_GATT_SERVICE_STARTED)
        CASE_RETURN_STR(WICED_BT_GATT_ENCRYPTED_NO_MITM)
        CASE_RETURN_STR(WICED_BT_GATT_NOT_ENCRYPTED)
        CASE_RETURN_STR(WICED_BT_GATT_CONGESTED)
        CASE_RETURN_STR(WICED_BT_GATT_WRITE_REQ_REJECTED)
        CASE_RETURN_STR(WICED_BT_GATT_CCC_CFG_ERR)
        CASE_RETURN_STR(WICED_BT_GATT_PRC_IN_PROGRESS)
        CASE_RETURN_STR(WICED_BT_GATT_OUT_OF_RANGE)
        CASE_DEFAULT_STR
    }

    return "UNKNOWN_STATUS";
}

/*******************************************************************************
* Function Name: get_bt_smp_status_name
********************************************************************************
* Summary:
*  The function converts the wiced_bt_smp_status_t enum value to its
*  corresponding string literal. This will help the programmer to debug easily
*  with log traces without navigating through the source code.
*
* Parameters:
*  wiced_bt_smp_status_t status: GATT status
*
* Return:
*  wiced_bt_smp_status_t
*
*******************************************************************************/
const char *get_bt_smp_status_name(wiced_bt_smp_status_t status)
{
    switch ((int)status)
    {
        CASE_RETURN_STR(SMP_SUCCESS)                /* Success */
        CASE_RETURN_STR(SMP_PASSKEY_ENTRY_FAIL)     /* Passkey entry failed */
        CASE_RETURN_STR(SMP_OOB_FAIL)               /* OOB failed */
        CASE_RETURN_STR(SMP_PAIR_AUTH_FAIL)         /* Authentication failed */
        CASE_RETURN_STR(SMP_CONFIRM_VALUE_ERR)      /* Value confirmation
                                                     * failed 
                                                     */
        CASE_RETURN_STR(SMP_PAIR_NOT_SUPPORT)       /* Not supported */
        CASE_RETURN_STR(SMP_ENC_KEY_SIZE)           /* Encryption key size
                                                     * failure 
                                                     */
        CASE_RETURN_STR(SMP_INVALID_CMD)            /* Invalid command */
        CASE_RETURN_STR(SMP_PAIR_FAIL_UNKNOWN)      /* Unknown failure */
        CASE_RETURN_STR(SMP_REPEATED_ATTEMPTS)      /* Repeated attempts */
        CASE_RETURN_STR(SMP_INVALID_PARAMETERS)     /* Invalid parameters  */
        CASE_RETURN_STR(SMP_DHKEY_CHK_FAIL)         /* DH Key check failed */
        CASE_RETURN_STR(SMP_NUMERIC_COMPAR_FAIL)    /* Numeric comparison
                                                     * failed 
                                                     */
        CASE_RETURN_STR(SMP_BR_PAIRING_IN_PROGR)    /* BR paIring in progress */
        CASE_RETURN_STR(SMP_XTRANS_DERIVE_NOT_ALLOW)/* Cross transport key
                                                     * derivation not allowed */
        /* bte smp status codes */
        CASE_RETURN_STR(SMP_PAIR_INTERNAL_ERR)      /* Internal error */
        CASE_RETURN_STR(SMP_UNKNOWN_IO_CAP)         /* unknown IO capability,
                                                     * unable to decide
                                                     * association model */
        CASE_RETURN_STR(SMP_INIT_FAIL)              /* Initialization failed */
        CASE_RETURN_STR(SMP_CONFIRM_FAIL)           /* Confirmation failed */
        CASE_RETURN_STR(SMP_BUSY)                   /* Busy */
        CASE_RETURN_STR(SMP_ENC_FAIL)               /* Encryption failed */
        CASE_RETURN_STR(SMP_STARTED)                /* Started */
        CASE_RETURN_STR(SMP_RSP_TIMEOUT)            /* Response timeout */
        CASE_RETURN_STR(SMP_FAIL)                   /* Generic failure */
        CASE_RETURN_STR(SMP_CONN_TOUT)              /* Connection timeout */
        CASE_DEFAULT_STR
    }

    return "UNKNOWN_STATUS";
}

/* [] END OF FILE */