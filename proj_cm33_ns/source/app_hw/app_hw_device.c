/*******************************************************************************
* File Name        : app_hw_device.c
*
* Description      : This file contains functions for initialization and usage 
*                    of device peripheral GPIO for LED and button. It also
*                    contains FreeRTOS timer implementations.
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
#include "timers.h"
#include "app_bt_bonding.h"
#include "cycfg_gap.h"
#include "app_bt_utils.h"
#include "app_bt_event_handler.h"
#include "app_bt_gatt_handler.h"
#include "app_hw_device.h"
#include "retarget_io_init.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* Interrupt priority for GPIO connected to button */
#define GPIO_INTERRUPT_PRIORITY        (4U)
#define BTN_TASK_STACK_SIZE            (512U)
#define BTN_TASK_PRIORITY              ((configMAX_PRIORITIES) - 1U)
#define KEY_RESET_VALUE                (0U)
#define INTERRUPT_MASKED               (1U)
#define BUTTON_SCAN_DELAY_MSEC         (10U)
#define SHORT_PRESS_DELAY_MSEC         (10U)
#define LONG_PRESS_DELAY_MSEC          (3000U)
#define DEBOUNCE_TIME_MS               (300U)
#define CCCD_ARRAY_SIZE                (2U)
#define CCCD_INDEX_INDICATION          (0U)
#define CCCD_INDICATION_ENABLE         (0x02U)
#define INIT_VAL_0                     (0U)
#define RESPONSE_DATA1_LENGTH          (20U)
#define READ_OFFSET_VALUE              (0U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* This is a one shot timer that is used to blink the LED the number of times 
   written in the led_blink characteristic in GATT database  by the client */
TimerHandle_t timer_led_blink;

/* This timer is used to toggle LED to indicate the 10 sec button press
 * duration */
TimerHandle_t ms_timer_led_indicate;

/* ms_timer_btn is a periodic timer that ticks every millisecond.
 * This timer is used to measure the duration of button press events */
TimerHandle_t ms_timer_btn;

/* Variables to hold the timer count values */
uint8_t led_blink_count;

/* Handle of the button task */
TaskHandle_t button_handle;

/* To check if the device has entered pairing mode to connect and bond with a new device */
bool pairing_mode = FALSE;

/*Flag to set based on USER BUTTON press */
bool gpio_intr_flag1 = FALSE;
bool gpio_intr_flag2 = FALSE;
bool long_press_use_btn2 = FALSE;

/* Define the semaphore handle */
static SemaphoreHandle_t button_semaphore;
uint8_t read_resp_data1[RESPONSE_DATA1_LENGTH];
/* Variables for button debouncing */
volatile bool button_debouncing = false;
volatile uint32_t button_debounce_timestamp = INIT_VAL_0;

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: user_btn_interrupt_config
********************************************************************************
* Summary:
*  This function initializes a pin as input that triggers interrupt on
*  falling edges.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void user_btn_interrupt_config(void)
{
    cy_rslt_t result = INIT_VAL_0;

    /* Interrupt config structure */
    cy_stc_sysint_t intrCfg =
    {
        .intrSrc =CYBSP_USER_BTN1_IRQ,
        .intrPriority = GPIO_INTERRUPT_PRIORITY
    };

    /* CYBSP_USER_BTN1 (SW2) and CYBSP_USER_BTN2 (SW4) share the same port and
     * hence they share the same NVIC IRQ line. Since both are configured in the
     * BSP via the Device Configurator, the interrupt flags for both the buttons
     * are set right after they get initialized through the call to cybsp_init().
     * The flags must be cleared before initializing the interrupt, otherwise
     * the interrupt line will be constantly asserted. 
     */
    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN1_PORT, CYBSP_USER_BTN1_PIN);
    Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN);
    NVIC_ClearPendingIRQ(CYBSP_USER_BTN1_IRQ);
    NVIC_ClearPendingIRQ(CYBSP_USER_BTN2_IRQ);

    /* Initialize the interrupt and register interrupt callback */
    cy_en_sysint_status_t btn_interrupt_init_status = Cy_SysInt_Init
            (&intrCfg, &app_bt_gpio_interrupt_handler);
    if(CY_SYSINT_SUCCESS != btn_interrupt_init_status)
    {
        handle_app_error();
    }

    /* Enable the interrupt in the NVIC */
    NVIC_EnableIRQ(intrCfg.intrSrc);

    /* Create Button Task */
    result = xTaskCreate(button_task,"Button task",BTN_TASK_STACK_SIZE,NULL,
                 BTN_TASK_PRIORITY,&button_handle);
    if( pdPASS != result )
    {
        printf("Unable to create Button task\n");
    }
}

/*******************************************************************************
* Function Name: app_bt_gpio_interrupt_handler
********************************************************************************
* Summary:
*  GPIO interrupt handler.
*
* Parameters:
*  void
*
* Return:
*  None
*
*******************************************************************************/
void app_bt_gpio_interrupt_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Check if the interrupt is from USER BUTTON 1 */
    if(INTERRUPT_MASKED == Cy_GPIO_GetInterruptStatusMasked(CYBSP_USER_BTN_PORT,CYBSP_USER_BTN_PIN))
    {
        /* Clear the interrupt and pending IRQ from NVIC*/
        Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN_PORT, CYBSP_USER_BTN_PIN);
        NVIC_ClearPendingIRQ(CYBSP_USER_BTN1_IRQ);

        /* Set the interrupt flag */
        gpio_intr_flag1 = true;
    }

    /* Check if the interrupt is from USER BUTTON 2 */
    if(INTERRUPT_MASKED == Cy_GPIO_GetInterruptStatusMasked(CYBSP_USER_BTN2_PORT,CYBSP_USER_BTN2_PIN))
    {
        /* Clear the interrupt and pending IRQ from NVIC*/
        Cy_GPIO_ClearInterrupt(CYBSP_USER_BTN2_PORT, CYBSP_USER_BTN2_PIN);
        NVIC_ClearPendingIRQ(CYBSP_USER_BTN2_IRQ);

        if (!button_debouncing)
        {
            /* Set the debouncing flag */
            button_debouncing = true;

            /* Record the current timestamp */
            button_debounce_timestamp = (uint32_t) (xTaskGetTickCount()
                * portTICK_PERIOD_MS);
        }

        if (button_debouncing && (((xTaskGetTickCount() * portTICK_PERIOD_MS)) -
                button_debounce_timestamp <= DEBOUNCE_TIME_MS *
                portTICK_PERIOD_MS))
        {
              button_debouncing = false;
              /* Set the interrupt flag */
              gpio_intr_flag2 = true;
        }
    }

    /* Give Semaphore from ISR to the button task */
    xSemaphoreGiveFromISR(button_semaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*******************************************************************************
* Function Name: app_bt_client_read_on_button_press
********************************************************************************
* Summary:
*  Reads the data characteristic from the connected GATT server when a button
*  is pressed. Prioritizes New Custom Service 2 if discovered; otherwise falls
*  back to Custom Service 1.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void app_bt_client_read_on_button_press(void)
{
    if ( INIT_VAL_0 == current_conn_id )
    {
        printf("Client not connected, cannot read.\n");
        return;
    }

    if (new_cs_discovery_data->service_found)
    {
       printf("Reading New Custom Service 2 data characteristic...\n");
    
       wiced_bt_gatt_client_send_read_handle(
            current_conn_id,
            new_cs_discovery_data->char_val_blink_handle,
            READ_OFFSET_VALUE,
        read_resp_data1, sizeof(wiced_bt_gatt_operation_complete_t),
                    GATT_AUTH_REQ_NONE);

    }
    else if (cs_discovery_data->service_found)
    {
        printf("New Custom service 2 is not found. Reading Custom Service 1 data characteristic...\n");

         wiced_bt_gatt_client_send_read_handle(
            current_conn_id,
            cs_discovery_data->char_val_blink_handle,
            READ_OFFSET_VALUE,
            read_resp_data1, sizeof(wiced_bt_gatt_operation_complete_t),
                                        GATT_AUTH_REQ_NONE);
    }
    else
    {
        printf(" NOT FOUND !\n");
    }
}

/*******************************************************************************
* Function Name: button_task
********************************************************************************
* Summary:
*  This is a FreeRTOS task that handles the button press events.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void button_task(void *arg)
{
    wiced_result_t result;
    TickType_t last_button_press_time = INIT_VAL_0;
    TickType_t long_press_start_time = INIT_VAL_0;
    BaseType_t long_press_detected = pdFALSE;

    /* Create the semaphore */
    button_semaphore = xSemaphoreCreateBinary();
    for (;;)
    {
        /* Wait for the semaphore to be taken from the ISR */
        if (xSemaphoreTake(button_semaphore, portMAX_DELAY) == pdTRUE)
        {
            if(true == gpio_intr_flag2)
            {
                gpio_intr_flag2 = false;
                long_press_detected = pdFALSE;
                long_press_start_time = xTaskGetTickCount();
                last_button_press_time = xTaskGetTickCount();

                printf("USER BUTTON 2 press is detected!\r\n");

                while (Cy_GPIO_Read(CYBSP_SW4_PORT, CYBSP_SW4_PIN) ==
                                       CYBSP_BTN_PRESSED)
                {
                    if(!long_press_detected &&
                          ((xTaskGetTickCount() - long_press_start_time) >= pdMS_TO_TICKS(LONG_PRESS_DELAY_MSEC)))
                    {
                        long_press_detected = pdTRUE;
                        uint8_t cccd_enable[CCCD_ARRAY_SIZE]= {INIT_VAL_0, INIT_VAL_0};

                        if(!long_press_use_btn2)
                        {
                            cccd_enable[CCCD_INDEX_INDICATION] = CCCD_INDICATION_ENABLE;
                            long_press_use_btn2 = true;
                        }
                        else
                        {
                            long_press_use_btn2 = false;
                        }
                        app_bt_write_peer_cccd(cccd_enable);
                    }
                    vTaskDelay( pdMS_TO_TICKS(BUTTON_SCAN_DELAY_MSEC) );
                }
                if (!long_press_detected &&
                    ( ( xTaskGetTickCount() - last_button_press_time) >= pdMS_TO_TICKS(SHORT_PRESS_DELAY_MSEC) ) )
                {
                    if (MAX_PERIPHERALS > num_peripherals)
                    {
                        /* Start scanning upon short button press */
                        result = wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_HIGH_DUTY,true, app_scan_result_handler);
                        if ((WICED_BT_PENDING != result) && (WICED_BT_BUSY != result))
                        {
                            printf("Failed to start scanning!, result = %d \n",result);
                            CY_ASSERT(INIT_VAL_0);
                        }
                    }
                    else
                    {
                        printf("Maximum limit for number of peripherals reached!\r\n");
                    }
                }
            }
            else if(true == gpio_intr_flag1)
            {
                gpio_intr_flag1 = false;
                last_button_press_time = xTaskGetTickCount();
                long_press_start_time = xTaskGetTickCount();
                long_press_detected = pdFALSE;

                while (Cy_GPIO_Read(CYBSP_SW2_PORT, CYBSP_SW2_PIN) == CYBSP_BTN_PRESSED)
                {
                    if(!long_press_detected &&
                          ((xTaskGetTickCount() - long_press_start_time) >= pdMS_TO_TICKS(LONG_PRESS_DELAY_MSEC)))
                    {
                        long_press_detected = pdTRUE;
                        printf("USER BUTTON 1 long press is detected!\r\n");

                        /* Reset Kv-store library, this will clear the NVM */
                       cy_rslt_t cy_result = mtb_kvstore_reset(&kvstore_obj);
                        if(CY_RSLT_SUCCESS == cy_result)
                        {
                            printf("Successfully reset kv-store library, "
                                   "please reset the device to generate new "
                                   "keys!\n");
                        }
                        else
                        {
                            printf("failed to reset kv-store libray\n");
                        }

                        memset(&bond_info, KEY_RESET_VALUE, sizeof(bond_info));
                        memset(&identity_keys, KEY_RESET_VALUE,
                               sizeof(identity_keys));
                        handle_app_error();
                    }

                    vTaskDelay(pdMS_TO_TICKS(BUTTON_SCAN_DELAY_MSEC));

                }

                if (!long_press_detected &&
                    ((xTaskGetTickCount() - last_button_press_time) >= pdMS_TO_TICKS(SHORT_PRESS_DELAY_MSEC)))
                {
                    app_bt_client_read_on_button_press();
                }
            }
        }
    }
}

/* [] END OF FILE */
