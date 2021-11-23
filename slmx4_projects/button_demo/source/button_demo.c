/**
This program is demonstrates a timed button press

Hardware required:
- SLMX4-HW
- epam-x4 (or other pin-compatible x4 module)

This is a very simple demonstration of a "one-shot" timer in FREERTOS

This program is based on the buttontime demo for the RT 1060 EVK board, but
this uses two buttons:
- User Button 1 -- Controls the Red LED
- User Button 2 -- Controls the Green LED

Initially, both LEDs will blink slow at 1 Hz rate. However, if a button is
pressed briefly, that button's corresponding LED will then blink at a much
higher rate. If the button is pressed for > 2 s, then it will blink at the
slower rate again.

@author
Justin Hadella

@copyright (c) 2021 Sensor Logic
*/

/**
@file  button_demo.c
@brief Demonstration of software timer in FREERTOS
*/
#include "fsl_debug_console.h"

#include "board.h"
#include "fsl_gpio.h"
#include "fsl_iomuxc.h"

#include "pin_mux.h"
#include "system_MIMXRT1062.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

// Platform includes
#include "slmx4_freertos.h"

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

#define button_task_PRIORITY (configMAX_PRIORITIES - 1)
#define led_task_PRIORITY    (configMAX_PRIORITIES - 2)

#define LED_SHORT_DELAY (pdMS_TO_TICKS(50))
#define LED_LONG_DELAY  (pdMS_TO_TICKS(500))

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Variables
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// Start off both LEDs using the long delay
volatile uint32_t red_led_delay = LED_LONG_DELAY;
volatile uint32_t green_led_delay = LED_LONG_DELAY;

// Flag indicates button interrupt
volatile bool button_pressed = false;

TimerHandle_t user_button1_timer = NULL;
TimerHandle_t user_button2_timer = NULL;

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------

static void user_button1_timer_callback(xTimerHandle pxTimer);
static void user_button2_timer_callback(xTimerHandle pxTimer);

static void red_led_task(void *pvParameters);
static void green_led_task(void *pvParameters);

static void button_task(void *pvParameters);

// =============================================================================
// Main Program
// =============================================================================

/**
@brief Application entry point
*/
int main(void)
{

	int status = platform__init();
	if (status)
	{
		PRINTF("platform__init() err = %d\n", status);
		return 1;
	}

	// Enable interrupt on buttons
	EnableIRQ(GPIO6_7_8_9_IRQn);

	// Create the software timer(s)
	user_button1_timer = xTimerCreate("user_button1_timer", // Text name
		pdMS_TO_TICKS(2000),          // Timer period
		pdFALSE,                      // Disable auto reload
		NULL,                         // ID is not used
		user_button1_timer_callback); // The callback function

	user_button2_timer = xTimerCreate("user_button2_timer", // Text name
		pdMS_TO_TICKS(2000),          // Timer period
		pdFALSE,                      // Disable auto reload
		NULL,                         // ID is not used
		user_button2_timer_callback); // The callback function

	if (xTaskCreate(button_task, "button_task", configMINIMAL_STACK_SIZE + 1000, NULL, button_task_PRIORITY, NULL) != pdPASS)
	{
		PRINTF("Task creation failed!.\r\n");
		while (1)
		{
			continue;
		}
	}

	if (xTaskCreate(red_led_task, "red_led_task", configMINIMAL_STACK_SIZE, NULL, led_task_PRIORITY, NULL) != pdPASS)
	{
		PRINTF("Task creation failed!.\r\n");
		while (1)
		{
			continue;
		}
	}

	if (xTaskCreate(green_led_task, "green_led_task", configMINIMAL_STACK_SIZE, NULL, led_task_PRIORITY, NULL) != pdPASS)
	{
		PRINTF("Task creation failed!.\r\n");
		while (1)
		{
			continue;
		}
	}

	vTaskStartScheduler();
	for (;;)
	{
		continue;
	}
}

// ~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~-
// ISR
// ~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~--~-

void GPIO6_7_8_9_IRQHandler(void)
{
	// Clear the interrupt status
	GPIO_PortClearInterruptFlags(BOARD_INITPINS_USER_BUTTON2_GPIO, 1U << BOARD_INITPINS_USER_BUTTON2_PIN);
	GPIO_PortClearInterruptFlags(BOARD_INITPINS_USER_BUTTON1_GPIO, 1U << BOARD_INITPINS_USER_BUTTON1_PIN);

	button_pressed = true;
}


static void user_button1_timer_callback(xTimerHandle pxTimer)
{
	green_led_delay = LED_LONG_DELAY;
}


static void user_button2_timer_callback(xTimerHandle pxTimer)
{
	red_led_delay = LED_LONG_DELAY;
}

// ~~=~~=~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~=
// FreeRTOS Tasks
// ~~=~~=~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~=

static void red_led_task(void *pvParameters)
{
	bool pin_set = false;

	for (;;)
	{
		if (pin_set)
		{
			GPIO_PinWrite(BOARD_INIT_LED_RED_GPIO, BOARD_INIT_LED_RED_PIN, 0U);
			pin_set = false;
		}
		else
		{
			GPIO_PinWrite(BOARD_INIT_LED_RED_GPIO, BOARD_INIT_LED_RED_PIN, 1U);
			pin_set = true;
		}

		vTaskDelay(red_led_delay);
	}
}


static void green_led_task(void *pvParameters)
{
	bool pin_set = false;

	for (;;)
	{
		if (pin_set)
		{
			GPIO_PinWrite(BOARD_INIT_LED_GREEN_GPIO, BOARD_INIT_LED_GREEN_PIN, 0U);
			pin_set = false;
		}
		else
		{
			GPIO_PinWrite(BOARD_INIT_LED_GREEN_GPIO, BOARD_INIT_LED_GREEN_PIN, 1U);
			pin_set = true;
		}

		vTaskDelay(green_led_delay);
	}
}


static void button_task(void *pvParameters)
{
	for (;;)
	{
		if (button_pressed)
		{
			//
			// Check user button 2 (red led)   [[ facreset button on older revs ]]
			//
			if (GPIO_PinRead(BOARD_INITPINS_USER_BUTTON2_GPIO, BOARD_INITPINS_USER_BUTTON2_PIN))
			{
				xTimerStop(user_button2_timer, 0);
			}
			else
			{
				xTimerReset(user_button2_timer, 0);
				red_led_delay = LED_SHORT_DELAY;
			}

			//
			// Check user button 1 (green led)  [[ sleepair button on older revs ]]
			//
			if (GPIO_PinRead(BOARD_INITPINS_USER_BUTTON1_GPIO, BOARD_INITPINS_USER_BUTTON1_PIN))
			{
				xTimerStop(user_button1_timer, 0);
			}
			else
			{
				xTimerReset(user_button1_timer, 0);
				green_led_delay = LED_SHORT_DELAY;
			}

			// Clear the button state
			button_pressed = false;
		}

		vTaskDelay(10);
	}
}
