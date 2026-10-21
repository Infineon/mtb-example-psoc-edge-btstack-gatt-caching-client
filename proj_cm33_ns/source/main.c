/*******************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for non-secure
*                    application in the CM33 CPU for the BLuetooth
*                    LE Caching Client Example for ModusToolbox.
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
#include "app_bt_bonding.h"
#include "cybsp.h"
#include "mtb_kvstore.h"
#include <FreeRTOS.h>
#include <task.h>
#include "cycfg_bt_settings.h"
#include "wiced_bt_stack.h"
#include "cybsp_bt_config.h"
#include "cybt_platform_config.h"
#include "app_bt_event_handler.h"
#include "app_bt_gatt_handler.h"
#include "app_hw_device.h"
#include "app_bt_utils.h"
#include "retarget_io_init.h"
#include "cybt_platform_trace.h"
#include "mtb_hal.h"
#include "cy_time.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* App boot address for CM55 project */
#define CM55_APP_BOOT_ADDR                  (CYMEM_CM33_0_m55_nvm_START + \
                                             CYBSP_MCUBOOT_HEADER_SIZE) 

/* The timeout value in microseconds used to wait for CM55 core to be booted */
#define CM55_BOOT_WAIT_TIME_USEC            (10U)

/* Enabling or disabling a MCWDT requires a wait time of upto 2 CLK_LF cycles
 * to come into effect. This wait time value will depend on the actual CLK_LF
 * frequency set by the BSP */
#define LPTIMER_0_WAIT_TIME_USEC            (62U)

/* Define the LPTimer interrupt priority number. '1' implies highest priority */
#define APP_LPTIMER_INTERRUPT_PRIORITY      (1U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* LPTimer HAL object */
static mtb_hal_lptimer_t lptimer_obj;

/* RTC HAL object */
static mtb_hal_rtc_t rtc_obj;

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: setup_clib_support
********************************************************************************
* Summary:
*    1. This function configures and initializes the Real-Time Clock (RTC).
*    2. It then initializes the RTC HAL object to enable CLIB support library 
*       to work with the provided Real-Time Clock (RTC) module.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void setup_clib_support(void)
{
    /* RTC Initialization */
    Cy_RTC_Init(&CYBSP_RTC_config);
    Cy_RTC_SetDateAndTime(&CYBSP_RTC_config);

    /* Initialize the ModusToolbox CLIB support library */
    mtb_clib_support_init(&rtc_obj);
}

/*******************************************************************************
* Function Name: lptimer_interrupt_handler
********************************************************************************
* Summary:
*  Interrupt handler function for LPTimer instance.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void lptimer_interrupt_handler(void)
{
    mtb_hal_lptimer_process_interrupt(&lptimer_obj);
}

/*******************************************************************************
* Function Name: setup_tickless_idle_timer
********************************************************************************
* Summary:
*   1. This function first configures and initializes an interrupt for LPTimer.
*   2. Then it initializes the LPTimer HAL object to be used in the RTOS
*      tickless idle mode implementation to allow the device enter deep sleep
*      when idle task runs. LPTIMER_0 instance is configured for CM33 CPU.
*   3. It then passes the LPTimer object to abstraction RTOS library that
*      implements tickless idle mode.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void setup_tickless_idle_timer(void)
{
    /* Interrupt configuration structure for LPTimer */
    cy_stc_sysint_t lptimer_intr_cfg =
    {
        .intrSrc = CYBSP_CM33_LPTIMER_0_IRQ,
        .intrPriority = APP_LPTIMER_INTERRUPT_PRIORITY
    };

    /* Initialize the LPTimer interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status =
         Cy_SysInt_Init(&lptimer_intr_cfg,
         lptimer_interrupt_handler);

    /* LPTimer interrupt initialization failed. Stop program execution. */
    if(CY_SYSINT_SUCCESS != interrupt_init_status)
    {
        handle_app_error();
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(lptimer_intr_cfg.intrSrc);

    /* Initialize the MCWDT block */
    cy_en_mcwdt_status_t mcwdt_init_status =
         Cy_MCWDT_Init(CYBSP_CM33_LPTIMER_0_HW,
                       &CYBSP_CM33_LPTIMER_0_config);

    /* MCWDT initialization failed. Stop program execution. */
    if(CY_MCWDT_SUCCESS != mcwdt_init_status)
    {
        handle_app_error();
    }

    /* Enable MCWDT instance */
    Cy_MCWDT_Enable(CYBSP_CM33_LPTIMER_0_HW,
                    CY_MCWDT_CTR_Msk,
                    LPTIMER_0_WAIT_TIME_USEC);

    /* Setup LPTimer using the HAL object and desired configuration as defined
     * in the device configurator. */
    cy_rslt_t result = mtb_hal_lptimer_setup(&lptimer_obj,
                                             &CYBSP_CM33_LPTIMER_0_hal_config);

    /* LPTimer setup failed. Stop program execution. */
    if(CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Pass the LPTimer object to abstraction RTOS library that implements
     * tickless idle mode
     */
    cyabs_rtos_set_lptimer(&lptimer_obj);
}

/*******************************************************************************
* Function Name : main
********************************************************************************
* Summary:
*  Entry point to the application. Set device configuration and start
*  BT stack initialization.  The actual application initialization will happen
*  when stack reports that BT device is ready.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t cy_result;
    wiced_result_t wiced_result;

    /* Initialize the board support package */
    cy_result = cybsp_init();

    if (CY_RSLT_SUCCESS != cy_result)
    {
        handle_app_error();
    }
    /* Initialize retarget-io middleware */
    init_retarget_io();

    /* Setup CLIB support library. */
    setup_clib_support();

    /* Setup the LPTimer instance for CM33 CPU. */
    setup_tickless_idle_timer();

    /* Clear UART Terminal Window*/
    printf("\x1b[2J\x1b[;H");
    /*Display header*/
    printf("****************** "
           "PSOC Edge MCU : Bluetooth GATT CACHING CLIENT"
           " ********************\n\n");

    /* Initialize kv-store library */
    app_kv_store_init();

    /* Register call back and configuration with stack */
    wiced_result = wiced_bt_stack_init(app_bt_management_callback,
                                       &cy_bt_cfg_settings);

    /* Check if stack initialization was successful */
    if(WICED_BT_SUCCESS == wiced_result)
    {
        printf("Bluetooth Stack Initialization Successful \n");
    }
    else
    {
        printf("Bluetooth Stack Initialization failed!! \n");
        handle_app_error();
    }

    /* Enable CM55. CM55_APP_BOOT_ADDR must be updated if CM55 memory layout
     * is changed. 
     */
    Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);

    /* Enable global interrupts */
    __enable_irq();

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    /* Should never get here */
    handle_app_error();
}

/* END OF FILE [] */