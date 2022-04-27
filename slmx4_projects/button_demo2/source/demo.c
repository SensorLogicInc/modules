/**
This program is demonstrates a timed button press

Hardware required:
- SLMX4-HW
- epam-x4 (or other pin-compatible x4 module)
- USB connection between PC and SLMX4

This program demonstrates another way to GPIO interrupts. In situations where
a number of actions may be executed based on how long a button has been pressed
the typical *Timer* is not adequate. This example uses the xTaskGetTickCount()
to measure elapsed time around a button press, then takes appropriate action.

When the program runs, the RGB LED will be one of three colors:
- Red   (user button 2 pressed < 2 s)
- Green (user button 2 pressed > 2 s and < 4 s)
- Blue  (user button 2 pressed > 4 s)

User button 2 is also used as a separate task. This shows how the single ISR
can serve multiple FreeRTOS tasks in an independent fashion.

User button 1 is used to toggle a "throb" which manipulates the brightness
of the RGB LED color. User button 1 presses < 2 s are ignored, and > 2 s will
toggle the throbbing feature.

@author
Justin Hadella

@copyright (c) 2021 Sensor Logic
*/

/**
@file    demo.c
@brief   demonstration of a timed button press
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

#define button_task_PRIORITY  (configMAX_PRIORITIES - 1)
#define rgb_led_task_PRIORITY (configMAX_PRIORITIES - 2)

typedef struct rgb_color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb_color;

typedef struct hsv_color {
	uint8_t h;
	uint8_t s;
	uint8_t v;
} hsv_color;

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Variables
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// Flag indicates button interrupt
volatile bool user_button2_pressed = false; // FACRESET on legacy hardware
volatile bool user_button1_pressed = false; // SLEEPAIR on legacy hardware

volatile bool throb_enabled = false;

volatile hsv_color hsv = {0, 255, 255}; // red

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------

void GPIO6_7_8_9_IRQHandler(void);

static rgb_color hsv_2_rgb(hsv_color hsv);

// FreeRTOS tasks
static void user_button2_task(void *pvParameters);
static void user_button1_task(void *pvParameters);
static void rgb_led_task(void *pvParameters);

// =============================================================================
// Main Program
// =============================================================================

/**
@brief  Application entry point
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

	// Turn off the RGB LED
	platform__set_rgb_led(0);

	PRINTF("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	PRINTF("Peak EP1 -- Timed button press demo:\n");
	PRINTF("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	if (xTaskCreate(user_button2_task, "user_button2_task", configMINIMAL_STACK_SIZE + 200, NULL, button_task_PRIORITY, NULL) != pdPASS)
	{
		PRINTF("user_button2_task creation failed!.\r\n");
		while (1)
		{
			continue;
		}
	}

	if (xTaskCreate(user_button1_task, "user_button1_task", configMINIMAL_STACK_SIZE + 200, NULL, button_task_PRIORITY, NULL) != pdPASS)
	{
		PRINTF("user_button1_task creation failed!.\r\n");
		while (1)
		{
			continue;
		}
	}

	if (xTaskCreate(rgb_led_task, "rgb_led_task", configMINIMAL_STACK_SIZE, NULL, rgb_led_task_PRIORITY, NULL) != pdPASS)
	{
		PRINTF("rgb_led_task creation failed!.\r\n");
		while (1)
		{
			continue;
		}
	}

	PRINTF("Reset Button:\n  t < 2 s -> set RGB to Red\n  2 s < t < 4 s -> set RGB to Green\n  t > 4 s -> set RGB to Blue\n");
	PRINTF("Sleep Button:\n  t < 2 s -> (ignore)\n  t > 2 s -> toggle RGB throb\n\n");

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
	uint32_t gpio_interrupts;

	gpio_interrupts = GPIO_GetPinsInterruptFlags(BOARD_INITPINS_USER_BUTTON1_GPIO);
	if ((1U << BOARD_INITPINS_USER_BUTTON1_PIN) & ~gpio_interrupts)
	{
		user_button2_pressed = true;
	}

	gpio_interrupts = GPIO_GetPinsInterruptFlags(BOARD_INITPINS_USER_BUTTON2_GPIO);
	if ((1U << BOARD_INITPINS_USER_BUTTON2_PIN) & ~gpio_interrupts)
	{
		user_button1_pressed = true;
	}

	// Clear the interrupt status
	GPIO_PortClearInterruptFlags(BOARD_INITPINS_USER_BUTTON2_GPIO, 1U << BOARD_INITPINS_USER_BUTTON2_PIN);
	GPIO_PortClearInterruptFlags(BOARD_INITPINS_USER_BUTTON1_GPIO, 1U << BOARD_INITPINS_USER_BUTTON1_PIN);
}

// ~~=~~=~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~=
// FreeRTOS Tasks
// ~~=~~=~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~=

static void user_button2_task(void *pvParameters)
{
	bool reset_time = 0;
	uint32_t start_ticks = 0;

	for (;;)
	{
		if (user_button2_pressed)
		{
			static float button_time = -1;

			if (GPIO_PinRead(BOARD_INITPINS_USER_BUTTON2_GPIO, BOARD_INITPINS_USER_BUTTON2_PIN))
			{
				button_time = (float)(xTaskGetTickCount() - start_ticks) / configTICK_RATE_HZ;
				reset_time = 1;
			}
			else
			{
				start_ticks = xTaskGetTickCount();
			}

			if (button_time > 4.0)
			{
				PRINTF("reset task: t > 10 s -> Blue\n");
				hsv.h = 160;
			}
			else if (button_time > 2.0)
			{
				PRINTF("reset task: 4 s < t < 10 s -> Green\n");
				hsv.h = 80;
			}
			else if (button_time > 0)
			{
				PRINTF("reset task: t < 4 s -> Red\n");
				hsv.h = 0;
			}

			if (reset_time)
			{
				reset_time = 0;
				button_time = -1;
			}

			// Clear the button state
			user_button2_pressed = false;
		}

		vTaskDelay(10);
	}
}


static void user_button1_task(void *pvParameters)
{
	bool reset_time = 0;
	uint32_t start_ticks = 0;

	for (;;)
	{
		if (user_button1_pressed)
		{
			static float button_time = -1;

			//
			// Check sleep/pair button
			//
			if (GPIO_PinRead(BOARD_INITPINS_USER_BUTTON1_GPIO, BOARD_INITPINS_USER_BUTTON1_PIN))
			{
				button_time = (float)(xTaskGetTickCount() - start_ticks) / configTICK_RATE_HZ;
				reset_time = 1;
			}
			else
			{
				start_ticks = xTaskGetTickCount();
			}

			if (button_time > 2.0)
			{
				PRINTF("sleep task: t > 2 s -> toggle led throb\n");
				throb_enabled = !throb_enabled;
			}
			else if (button_time > 0)
			{
				PRINTF("sleep task: t < 2 s -> (ignoring)\n");
			}

			if (reset_time)
			{
				reset_time = 0;
				button_time = -1;
			}

			// Clear the button state
			user_button1_pressed = false;
		}

		vTaskDelay(10);
	}
}


static void rgb_led_task(void *pvParameters)
{
	uint8_t throb_lookup[64] =
	{
		0,1,2,5,10,15,21,29,37,47,57,67,79,90,103,115,
		128,140,152,165,176,188,198,208,218,226,234,240,245,250,253,254,
		255,254,253,250,245,240,234,226,218,208,198,188,176,165,152,140,
		128,115,103,90,79,67,57,47,37,29,21,15,10,5,2,1
	};

	uint8_t idx = 0;

	rgb_color rgb;
	uint32_t rgb_val;

	while (1)
	{
		if (throb_enabled)
		{
			hsv.v = throb_lookup[idx];

			idx++;
			if (idx == 64)
			{
				idx = 0;
			}
		}
		else
		{
			// Just set the value to 100%
			hsv.v = 255;
		}

		// Determine the RGB value
		rgb = hsv_2_rgb(hsv);
		rgb_val = (rgb.r << 16) | (rgb.g << 8) | rgb.b;

		platform__set_rgb_led(rgb_val);

		vTaskDelay(10);
	}
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Local Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static rgb_color hsv_2_rgb(hsv_color hsv)
{
	rgb_color rgb;
	uint8_t region, p, q, t;
	uint32_t h, s, v, remainder;

	if (hsv.s == 0)
	{
		rgb.r = hsv.v;
		rgb.g = hsv.v;
		rgb.b = hsv.v;
		return rgb;
	}

	h = hsv.h;
	s = hsv.s;
	v = hsv.v;

	region = h / 43;
	remainder = (h - (region * 43)) * 6;

	p = (v * (255 - s)) >> 8;
	q = (v * (255 - ((s * remainder) >> 8))) >> 8;
	t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

	switch (region)
	{
	case 0:
		rgb.r = v;
		rgb.g = t;
		rgb.b = p;
		break;
	case 1:
		rgb.r = q;
		rgb.g = v;
		rgb.b = p;
		break;
	case 2:
		rgb.r = p;
		rgb.g = v;
		rgb.b = t;
		break;
	case 3:
		rgb.r = p;
		rgb.g = q;
		rgb.b = v;
		break;
	case 4:
		rgb.r = t;
		rgb.g = p;
		rgb.b = v;
		break;
	default:
		rgb.r = v;
		rgb.g = p;
		rgb.b = q;
		break;
	}

	return rgb;
}
