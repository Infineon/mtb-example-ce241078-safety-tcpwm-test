/*******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the Class-B Safety test code
*              example for TCPWM block, for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* $ Copyright 2026 Infineon Technologies AG $
*******************************************************************************/


/*******************************************************************************
* Header Files
*******************************************************************************/

#include "cy_pdl.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "mtb_hal.h"
#include "SelfTest.h"

/*******************************************************************************
* Macros
*******************************************************************************/

/* UART timeout in milliseconds */
#define UART_TIMEOUT_MS (10u)

/* Available commands */
#define SELFTEST_CMD_TIMER         ('1')
#define SELFTEST_CMD_PWM           ('2')
#define SELFTEST_CMD_PWM_GATE_KILL ('3')

/* Pins for self test*/
#define PWM_IN_PIN_PORT    NULL
#define PWM_IN_PIN_NUM     0u

/*******************************************************************************
* Global Variables
*******************************************************************************/

/* For the Retarget -IO (Debug UART) usage */
static cy_stc_scb_uart_context_t    DEBUG_UART_context;           /** UART context */
static mtb_hal_uart_t               DEBUG_UART_hal_obj;           /** Debug UART HAL object  */

/* Variable for storing PWM status */
cy_en_tcpwm_status_t api_status;

/* Variable for storing low-power comparator context */
static cy_stc_lpcomp_context_t context;

/* Variable to store the input command from UART*/
uint8_t cmd;

/*******************************************************************************
* Function Prototypes
*******************************************************************************/

/* Prototypes of test functions */
void timer_test(void);
void pwm_test(void);
void pwm_gate_kill(void);

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function. It performs Class-B safety test for TCPMW block.
* SelfTest is performed for Timer/Counter, PWM and PWM gate Kill based on the
* user command.
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

#if defined (CY_DEVICE_SECURE)
    cyhal_wdt_t wdt_obj;

    /* Clear watchdog timer so that it doesn't trigger a reset */
    result = cyhal_wdt_init(&wdt_obj, cyhal_wdt_get_max_timeout_ms());
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    cyhal_wdt_free(&wdt_obj);
#endif

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Debug UART init */
    result = (cy_rslt_t)Cy_SCB_UART_Init(DEBUG_UART_HW, &DEBUG_UART_config, &DEBUG_UART_context);

    /* UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /*Debug UART enable*/
    Cy_SCB_UART_Enable(DEBUG_UART_HW);

    /* Setup the HAL UART */
    result = mtb_hal_uart_setup(&DEBUG_UART_hal_obj, &DEBUG_UART_hal_config, &DEBUG_UART_context, NULL);

    /* HAL UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initializes redirecting of low-level I/O. */
    result = cy_retarget_io_init(&DEBUG_UART_hal_obj);

    /* HAL retarget_io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("****************** "
           "CLass-B: TCPWM SAFETY TEST "
           "****************** \r\n\n");

    /* Display available commands */
    printf("Available commands \r\n");
    printf("1 : Run SelfTest for Timer/ Counter\r\n");
    printf("2 : Run SelfTest for PWM\r\n");
    printf("3 : Run SelfTest for PWM Gate Kill\r\n\n");

    for (;;)
    {
        /* Storing the data send from the terminal through UART in cmd */
        result = mtb_hal_uart_get(&DEBUG_UART_hal_obj, &cmd, UART_TIMEOUT_MS);

        /* check for timeout if data is not received*/
        if (result != MTB_HAL_UART_RSLT_ERR_CSP_UART_GETC_TIMEOUT)
        {
            if (SELFTEST_CMD_TIMER == cmd)
            {
                printf("\r\n[Command] : Run SelfTest for Timer/ Counter\r\n");
                timer_test();
            }
            else if (SELFTEST_CMD_PWM == cmd)
            {
                printf("\r\n[Command] : Run SelfTest for PWM\r\n");
                pwm_test();
            }
            else if (SELFTEST_CMD_PWM_GATE_KILL == cmd)
            {
                printf("\r\n[Command] : Run SelfTest for PWM Gate Kill\r\n");
                pwm_gate_kill();
            }
            else
            {
                printf("\r\nEnter a valid command\r\n");
            }
        }

    }
}

/*******************************************************************************
* Function Name: timer_test
********************************************************************************
* Summary:
* This function creates configures the block to timer/counter personality.
* The test verifies, if the counter is incrementing the count value, and the
* count value falls within the expected thresholds.
*
* Parameters:
*  none
*
* Return :
*  void
*
*******************************************************************************/
void timer_test(void)
{
    /* Initialize the TCPWM for timer/counter test. */
    SelfTest_Timer_Counter_init(CYBSP_TIMER_COUNTER_TEST_HW,
                                CYBSP_TIMER_COUNTER_TEST_NUM,
                                &CYBSP_TIMER_COUNTER_TEST_config,
                                (IRQn_Type)CYBSP_TIMER_COUNTER_TEST_IRQ);
    /* Run Timer/Counter Self Test... */
    if (OK_STATUS != SelfTest_Counter_Timer())
    {
        /* Process error */
        printf("Error: Timer Counter fail\r\n");
    }
    else
    {
        printf("Success: Timer Counter pass\r\n");
    }
}

/*******************************************************************************
* Function Name: pwm_test
********************************************************************************
* Summary:
* The function configures a 32-bit PWM to run at 1/3 duty ON, 2/3 OFF duty cycle
* with a 1 millisecond period, and then start the PWM. The CPU is then run in a
* loop for 5 milliseconds and the output is polled continuously in the loop.
* The instances of `0` (low) and `1` (high) are counted. The on/off ratio is
* then calculated and checked if it falls within the expected thresholds.
*
* Parameters:
*  none
*
* Return:
*  void
*******************************************************************************/
void pwm_test(void)
{
    /*Initialize the TCPWM for PWM test. If init failed disable global interrupts */
    if (SelfTest_PWM_init(CYBSP_PWM_HW, CYBSP_PWM_NUM, &CYBSP_PWM_config,
            (IRQn_Type)CYBSP_PWM_IRQ) != 0)
    {
        __disable_irq();
    }

    /* Run PWM Self Test... */
    if (OK_STATUS != SelfTest_PWM(PWM_IN_PIN_PORT, PWM_IN_PIN_NUM))
    {
        /* Process error */
        printf("Error: PWM fail\r\n");
        printf("Ensure PWM_IN_PIN is connected to PWM line -"
                " Refer Hardware setup section in Readme\r\n");
    }
    else
    {
        /* Process success */
        printf("Success: PWM pass\r\n");
    }
}

/*******************************************************************************
* Function Name: pwm_gate_kill
********************************************************************************
* Summary:
* The function configures Kill mode of the TCPWM block as `Stop on Kill. The
* low-power comparator output is routed to the Kill signal of TCPWM.
* If the voltage on the positive terminal is greater than the voltage on the
* negative terminal, it will kill the PWM output.
* The TCPWM base and CntNum are passed to check whether
* the counter is stopped or not. If the counter is not incrementing or
* decrementing, it is assumed that the PWM output is inactive and the PWM
* is killed. This test has limited scope since it only checks the counter
* value. It is recommended to create an application-level self-test to
* check the PWM pin state.
*
* Parameters:
*  none
*
* Return:
*  void
*******************************************************************************/
void pwm_gate_kill(void)
{
    /* Initializes low-power comparator */
    Cy_TCPWM_PWM_Disable(CYBSP_PWM_GATEKILL_HW, CYBSP_PWM_GATEKILL_NUM);
    Cy_LPComp_Init_Ext(LPCOMP_HW, LPCOMP_CHANNEL, &LPCOMP_config,&context);
    Cy_LPComp_SetInputs(LPCOMP_HW, LPCOMP_CHANNEL, CY_LPCOMP_SW_GPIO, CY_LPCOMP_SW_LOCAL_VREF);
    Cy_LPComp_UlpReferenceEnable(LPCOMP_HW);

    /* Configure the TCPWM for PWM operation. */
     api_status = Cy_TCPWM_PWM_Init(CYBSP_PWM_GATEKILL_HW,
             CYBSP_PWM_GATEKILL_NUM, &CYBSP_PWM_GATEKILL_config);

     /* TCPWM init failed. Disable the global interrupt */
    if( api_status != CY_TCPWM_SUCCESS)
    {
        __disable_irq();
    }

    /* TCPWM enable */
    Cy_TCPWM_PWM_Enable(CYBSP_PWM_GATEKILL_HW, CYBSP_PWM_GATEKILL_NUM);

    /* Start trigger to TCPWM */
    Cy_TCPWM_TriggerReloadOrIndex_Single(CYBSP_PWM_GATEKILL_HW, CYBSP_PWM_GATEKILL_NUM);

    if (OK_STATUS == SelfTest_PWM_GateKill(CYBSP_PWM_GATEKILL_HW,
            CYBSP_PWM_GATEKILL_NUM))
    {
        printf("Error: PWM GateKill precondition fail - PWM counter is not running\r\n");
        Cy_TCPWM_PWM_Disable(CYBSP_PWM_GATEKILL_HW, CYBSP_PWM_GATEKILL_NUM);
        return;
    }

    /* Power up the comparator with the PWM running. A rising edge on the
     * comparator output (P8.0 above the reference) kills the counter. */
    Cy_LPComp_Enable_Ext(LPCOMP_HW, LPCOMP_CHANNEL, &context);
    Cy_SysLib_Delay(1);

    /* The power-up transient can kill the counter even though the settled
     * output is low. Clear such a false kill so only a real high on P8.0
     * leaves the counter stopped. */
    if (0UL == Cy_LPComp_GetCompare(LPCOMP_HW, LPCOMP_CHANNEL))
    {
        Cy_TCPWM_PWM_Disable(CYBSP_PWM_GATEKILL_HW, CYBSP_PWM_GATEKILL_NUM);
        Cy_TCPWM_PWM_Enable(CYBSP_PWM_GATEKILL_HW, CYBSP_PWM_GATEKILL_NUM);
        Cy_TCPWM_TriggerReloadOrIndex_Single(CYBSP_PWM_GATEKILL_HW, CYBSP_PWM_GATEKILL_NUM);
        Cy_SysLib_DelayUs(100);
    }
    
    /* Run PWM GateKill Self Test... */
    if (OK_STATUS != SelfTest_PWM_GateKill(CYBSP_PWM_GATEKILL_HW,
            CYBSP_PWM_GATEKILL_NUM))
    {
        /* Process error */
        printf("Error: PWM GateKill fail\r\n");
        printf("Ensure LPCOMP +ve pin is connected to VDD -"
                " Refer Hardware setup section in Readme\r\n");
    }
    else
    {
        /* Process success */
        printf("Success: PWM GateKill pass\r\n");
    }

    Cy_LPComp_Disable_Ext(LPCOMP_HW, LPCOMP_CHANNEL, &context);
    Cy_TCPWM_PWM_Disable(CYBSP_PWM_GATEKILL_HW, CYBSP_PWM_GATEKILL_NUM);
}

/* [] END OF FILE */

