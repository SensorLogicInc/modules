/**
This program is simple demonstration of the LTR308 Ambient Light Sensor

Hardware required:
- SLMX4-HW
- epam-x4 (or other pin-compatible x4 module)

When the program runs, the lux reading is displayed and the RGB LED is lit
green with its brightness controlled by the lux value.

The ambient lux value should be relatively constant normally. When you block
the light (with you hand or finger) the lux value will drop. When you shine a
light at the sensor, the lux value should increase.

@author
Justin Hadella

@copyright (c) 2021 Sensor Logic
*/

/**
@file    demo.c
@brief   demonstration of ltr308 usage
*/
#include "fsl_debug_console.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

// Platform includes
#include "slmx4_freertos.h"
#include "ltr308_driver.h"

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

#define lux_task_PRIORITY (configMAX_PRIORITIES - 1)

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------

static void lux_task(void *pvParameters);

// =============================================================================
// Main Program
// =============================================================================

/**
@brief  Application entry point
*/
void main(void)
{
	int status = platform__init();
	if (status)
	{
		PRINTF("platform__init() = %d\n", status);
		return;
	}

	if (xTaskCreate(lux_task, "lux_task", configMINIMAL_STACK_SIZE + 1000, NULL, lux_task_PRIORITY, NULL) != pdPASS)
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

// ~~=~~=~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~=
// FreeRTOS Tasks
// ~~=~~=~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~~~=~~=

static void lux_task(void *pvParameters)
{
	float lux = 0.0f;

	// Activate lux sensor
	int  status = ltr308__set_active();
	if (status)
	{
		PRINTF("ltr308__set_active() = %d\n", status);
		return;
	}

	// Read ID
	uint8_t lux_id = 0;
	status = ltr308__read_id(&lux_id);
	if (status)
	{
		PRINTF("ltr308__read_id() = %d\n", status);
		return;
	}
	PRINTF("ltr308 id = 0x%x (should be 0xB1)\n", lux_id);

	// Customize
	ltr308__set_meas_rate(ALS_RESOLUTION_17BIT, ALS_MEAS_RATE_50MS);
	ltr308__set_gain(ALS_GAIN_18);

	// Test loop
	for (;;)
	{
		ltr308__read_lux(&lux);
		PRINTF("lux = %.2f\n", lux);

		// Clamp lux
		if (lux > 1000.0f) lux = 1000.0f;

		// Set RGB LED to green but intensity is based on lux level
		uint8_t green_level = (uint8_t)(lux * 255.0f / 1000.0f);
		platform__set_rgb_led(green_level << 8);

		platform__delay(50);
	}
}
