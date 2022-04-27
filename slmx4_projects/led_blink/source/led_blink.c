/**
This program is simple demonstration of the LEDs

Hardware required:
- SLMX4-HW
- epam-x4 (or other pin-compatible x4 module)

This program uses the Red, Green, and RGB LED in a few modes. Under the "Modes"
section below, uncomment one mode and comment out all the others. _Some modes
can be mixed though._

Modes:
* BLINK_RED_LED
  This mode blinks the Red LED at a fixed rate
* BLINK_GREEN_LED
  This mode blinks the Green LED at a fixed rate
* BLINK_RGB_LED
  This blinks the RGB LED at a fixed rate. This essentially is setting the RGB
  LED to either the current color or "off"
* COLOR_CYCLE_RGB_LED
  This mode cycles the RGB LED through the HSV color space by manipulating the
  hue value
* THROB_RGB_LED
  This mode color cycles the RGB LED by manipulating the hue as in the color
  cycle mode. However, this mode also manipulates the value in the HSV color
  space, which changes the brightness
* NIGHT_THROB_LED
  This mode throbs the brightness for night time use

@author
Justin Hadella

@copyright (c) 2021 Sensor Logic
*/

/**
@file    led_blink.c
@brief   demonstration of led usage
*/
#include "fsl_debug_console.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

// Platform includes
#include "slmx4_freertos.h"

// -----------------------------------------------------------------------------
// Modes
// -----------------------------------------------------------------------------

//#define BLINK_RED_LED
//#define BLINK_GREEN_LED
//#define BLINK_RGB_LED
#define COLOR_CYCLE_RGB_LED
//#define THROB_RGB_LED
//#define NIGHT_THROB_LED

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

#define blink_task_PRIORITY (configMAX_PRIORITIES - 1)

#define NORMAL_DELAY (100)
#define THROB_DELAY  (10)

typedef struct rgb_color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb_color;

typedef struct hsv_color
{
	uint8_t h;
	uint8_t s;
	uint8_t v;
} hsv_color;

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------

static void blink_task(void *pvParameters);

static rgb_color hsv_2_rgb(hsv_color hsv);

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

	if (xTaskCreate(blink_task, "blink_task", configMINIMAL_STACK_SIZE + 1000, NULL, blink_task_PRIORITY, NULL) != pdPASS)
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

static void blink_task(void *pvParameters)
{
	hsv_color hsv = {200, 255, 255}; // violet
	rgb_color rgb;
	uint32_t rgb_val = 0xff00ff; // violet
	uint32_t delay = NORMAL_DELAY;
	uint8_t idx = 0;

#ifdef BLINK_RGB_LED
	int i = 0;
#endif

#ifdef THROB_RGB_LED
	uint8_t throb_lookup[64] =
	{
		0,1,2,5,10,15,21,29,37,47,57,67,79,90,103,115,
		128,140,152,165,176,188,198,208,218,226,234,240,245,250,253,254,
		255,254,253,250,245,240,234,226,218,208,198,188,176,165,152,140,
		128,115,103,90,79,67,57,47,37,29,21,15,10,5,2,1
	};

	hsv.h = 0; // red

	delay = THROB_DELAY;
#endif

#ifdef NIGHT_THROB_LED
	// Heartbeat pattern
	uint8_t throb_lookup[64] =
	{
		0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
		1,2,2,2,2,2,3,3,4,4,4,4,3,3,2,2,
		2,2,3,3,4,4,4,4,4,3,3,2,2,2,2,1,
		1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0
	};

	hsv.h = 0; // red

	delay = THROB_DELAY;
#endif

  for (;;)
  {
#ifdef BLINK_RED_LED
	  platform__led_toggle(LED_RED);
#endif

#ifdef BLINK_GREEN_LED
	  platform__led_toggle(LED_GREEN);
#endif

#ifdef COLOR_CYCLE_RGB_LED
		// Determine the RGB value
		rgb = hsv_2_rgb(hsv);
		rgb_val = (rgb.r << 16) | (rgb.g << 8) | rgb.b;

		platform__set_rgb_led(rgb_val);

		// Update hue (incrementing hue only will color cycle)
		hsv.h++;
#endif

#ifdef BLINK_RGB_LED
		i++;
		platform__set_rgb_led(i % 2 ? rgb_val : 0);
#endif

#ifdef THROB_RGB_LED
		hsv.v = throb_lookup[idx];

		idx++;
		if (idx == 64)
		{
			idx = 0;
			hsv.h += 8;
		}

		// Determine the RGB value
		rgb = hsv_2_rgb(hsv);
		rgb_val = (rgb.r << 16) | (rgb.g << 8) | rgb.b;

		platform__set_rgb_led(rgb_val);

#endif

#ifdef NIGHT_THROB_LED
		hsv.v = throb_lookup[idx];;

		idx++;
		if (idx == 64)
		{
			idx = 0;
		}

		// Determine the RGB value
		rgb = hsv_2_rgb(hsv);
		rgb_val = (rgb.r << 16) | (rgb.g << 8) | rgb.b;

		platform__set_rgb_led(rgb_val);
#endif
		platform__delay(delay);
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

