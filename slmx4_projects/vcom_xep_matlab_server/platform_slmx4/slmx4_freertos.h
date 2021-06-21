/**
@file slmx4_freertos.h

Definitions for platform-specific device-driver functions targeted toward the
project SLMX4 hardware

@note
The I2C and SPI implementations use the FreeRTOS version of the I2C and SPI
drivers. This means that any calls which use these two drivers must be done
in a task, or *after* vTaskStartScheduler() has been called!

@author
Justin Hadella

@copyright (c) 2021 Sensor Logic
*/
#ifndef SLMX4_FREERTOS_h
#define SLMX4_FREERTOS_h

#include <stdint.h>
#include <stdbool.h>

#include "x4driver.h"

// Hardware includes
#include "board.h"
#include "fsl_lpspi_freertos.h"
#include "fsl_lpi2c_freertos.h"
#include "fsl_common.h"
#include "fsl_gpio.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// HAL Definitions (platform-specific)
// -----------------------------------------------------------------------------

/**
@struct Hal_t
*/
typedef struct Hal_t
{
	gpio_pin_config_t gpio_led_red;
	gpio_pin_config_t gpio_led_green;

	lpspi_master_config_t spi_x4;
	lpspi_rtos_handle_t spi_x4_handle;

	gpio_pin_config_t x4_en;
	gpio_pin_config_t x4_gpio1;
	gpio_pin_config_t x4_gpio2;

	lpi2c_master_config_t i2c_conf;
	lpi2c_rtos_handle_t i2c_handle;
  
} Hal_t;

// -----------------------------------------------------------------------------
// Platform Definitions
// -----------------------------------------------------------------------------

/**
@enum PLATFORM_USER_LED_e
*/
typedef enum GPIO_LED {
	LED_RED,
	LED_GREEN

} GPIO_LED, PLATFORM_GPIO_LED_e;

/**
@enum PLATFORM_STATUS_e
This enumeration is used for return calls to the platform functions
*/
typedef enum {
	PLATFORM_SUCCESS                 = 0,
	PLATFORM_X4_DRIVER_CREATE_ERR    = 1 << 1,
	PLATFORM_X4_DRIVER_OPEN_ERR      = 1 << 2,
	PLATFORM_X4_SPI_OPEN_ERR         = 1 << 3,
	PLATFORM_X4_SPI_XFER_ERR         = 1 << 4,
	PLATFORM_X4_LOCK_INIT_ERR        = 1 << 5,
	PLATFORM_ERR_INIT                = 1 << 6,
	PLATFORM_ERR_ENUM                = 1 << 7,
	PLATFORM_ERR_SPI                 = 1 << 8,
	PLATFORM_ERR_BOUNDS              = 1 << 9,
	PLATFORM_WRONG_REG               = 1 << 10,
	PLATFORM_ERR_I2C                 = 1 << 11,
	PLATFORM_RGB_LED_INIT_ERR        = 1 << 12

} PLATFORM_STATUS_e;

/**
@struct Platform_Status_t
This struct encapsulates result of system initialization results
 */
typedef struct {
	uint8_t spi_fail;
	uint8_t i2c_fail;
	uint8_t rgb_fail;
	uint8_t sem_fail;
	uint8_t x4_fail;
	uint8_t sd_fail;
	uint8_t rtc_fail;
	uint8_t wifi_fail;
	uint8_t wifi_scan_fail;
	uint8_t rsvd;

	uint32_t x4_error;

} Platform_Status_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Platform Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
Function initializes the platform's x4 driver for use

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
PLATFORM_STATUS_e platform__init();

/**
A simple delay function

@param [in] millis  Number of milliseconds to delay
*/
void platform__delay(uint32_t delay_ms);

/**
A simple delay function

@param [in] millis  Number of microseconds to delay
*/
void platform__delay_us(uint32_t delay_us);

/**
Function used to get local timer value specific to the hardware platform

@return value representing timer value (relative)
*/
int32_t platform__get_timer_ticks();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LED Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*
There is no function to *disable* the RGB LED. This could be added if we think
it's needed..
*/

/**
Function to globally disable LED usage

@note
The LEDs are enabled by default

@note
As a side-effect of disabling an LED it will also be turned off automatically

@param [in] led The LED to enable/disable
@param [in] en  True to enable LED (default), or False to disable
*/
void platform__led_enable(PLATFORM_GPIO_LED_e led, bool en);

/**
Function used to turn on the platform's user LED

@note
If LEDs are disabled then command is ignored

@param [in] led The LED to turn on

@return PLATFORM_SUCCESS on success, else a non-zero error code
*/
int platform__led_on(PLATFORM_GPIO_LED_e led);

/**
Function used to turn off the platform's user LED

@note
If LEDs are disabled then command is ignored

@param [in] led The LED to turn off

@return PLATFORM_SUCCESS on success, else a non-zero error code
*/
int platform__led_off(PLATFORM_GPIO_LED_e led);

/**
Function used to toggle the platform's user LED

@note
If LEDs are disabled then command is ignored

@param [in] led The LED to toggle

@return PLATFORM_SUCCESS on success, else a non-zero error code
*/
int platform__led_toggle(PLATFORM_GPIO_LED_e led);

/**
Function used to set the RGB LED color

@note
If LEDs are disabled then command is ignored

@param [in] rgb  The RGB color to set

@return PLATFORM_SUCCESS on success, else a non-zero error code
*/
int platform__set_rgb_led(uint32_t rgb);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// X4 Adapter Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
Function used to open x4 adapter for use

This function allocates the necessary memory and sets all the callbacks and
locks needed to use the driver.

Example:  
@code
X4Driver_t *x4 = NULL;

int status = x4adapter_open(&x4);
if (status) {
  // something went wrong...
}

status = x4driver_init(x4);
if (status) {
  // driver initialization error...
}

// x4 driver is ready to use
@endcode

@param [in] **x4driver  Handle to x4 device driver

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
int x4adapter_open(X4Driver_t **x4driver);

/**
Function used to free resources used by x4 adapter

@note
Once called, the driver will not be useable!

@param [in] *x4driver  Handle to x4 device driver

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
void x4adapter_close(X4Driver_t *x4driver);

// ~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~
// X4DriverCallbacks_t Callback Functions
//
// Note: These functions are not called directly by user code; they are called
// indirectly by using the x4driver itself!
// ~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~

uint32_t x4driver_callback_pin_set_enable(void *user_reference, uint8_t value);
uint32_t x4driver_callback_spi_write(void *user_reference, uint8_t *data, uint32_t length);
uint32_t x4driver_callback_spi_read(void *user_reference, uint8_t *data, uint32_t length);
uint32_t x4driver_callback_spi_write_read(void *user_reference, uint8_t *wdata, uint32_t wlength, uint8_t *rdata, uint32_t rlength);
void x4driver_callback_wait_us(uint32_t us);
void x4driver_notify_data_ready(void *user_reference);
uint32_t x4driver_trigger_sweep_pin(void *user_reference);
void x4driver_enable_ISR(void *user_reference, uint32_t enable);

// ~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~
// X4DriverLock_t Callback Functions
//
// Note: These functions are not called directly by user code; they are called
// indirectly by using the x4driver itself!
// ~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~

uint32_t x4driver_callback_take_sem(void *sem,uint32_t timeout);
void x4driver_callback_give_sem(void *sem);

#ifdef __cplusplus
}
#endif
#endif // SLMX4_FREERTOS_h
