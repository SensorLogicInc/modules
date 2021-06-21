/**
@file slmx4_freertos.c

See header

@author
Justin Hadella

@copyright (c) 2021 Sensor Logic
*/
#include <slmx4_freertos.h>
#include <stdbool.h>

// Hardware includes
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MIMXRT1062.h"
#include "fsl_debug_console.h"
#include "fsl_lpspi.h"
#include "fsl_lpspi_freertos.h"
#include "fsl_lpi2c.h"
#include "fsl_lpi2c_freertos.h"
#include "fsl_iomuxc.h"
#include "fsl_pwm.h"
#include "fsl_xbara.h"
#include "fsl_debug_console.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "semphr.h"

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

#define FRAME_BUFFER_SIZE (1535 * 4)

// ?? Move/Refactor the rest of these defines into the board.h

#define CPU_FREQ (BOARD_BOOTCLOCKRUN_CORE_CLOCK / 1000000)
#define SW8_IRQ_HANDLER BOARD_USER_BUTTON_IRQ_HANDLER

#define X4_SPI (kLPSPI_Pcs0)
#define X4_SPI_MASTER_PCS_FOR_TRANSFER (kLPSPI_MasterPcs0)

// Select USB1 PLL PFD0 (720 MHz) as lpspi clock source
//#define EXAMPLE_LPSPI_CLOCK_SOURCE_SELECT (1U)

// Clock divider for master lpspi clock source
#define X4_SPI_CLOCK_SOURCE_DIVIDER (7U)

#define X4_SPI_CLOCK_FREQ (CLOCK_GetFreq(kCLOCK_Usb1PllPfd0Clk) / (X4_SPI_CLOCK_SOURCE_DIVIDER + 1U))

#define X4_SPI_MASTER_CLOCK_FREQ X4_SPI_CLOCK_FREQ

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------

static int init_rgb_led_pwm();

static uint32_t x4driver_local_spi_write_read_one(void *user_reference, uint8_t *wdata, uint32_t wlength, uint8_t *rdata, uint32_t rlength);

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Globals
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Hal_t g_hal = {
	{kGPIO_DigitalOutput, 0, kGPIO_NoIntmode},      // Red LED
	{kGPIO_DigitalOutput, 0, kGPIO_NoIntmode},      // Green LED
	{0},
	{0},
	{kGPIO_DigitalOutput, 0, kGPIO_NoIntmode},      // X4_EN
	{kGPIO_DigitalInput, 0, kGPIO_IntRisingEdge},   // X4_GPIO1
	{kGPIO_DigitalInput, 0, kGPIO_NoIntmode},       // X4_GPIO2
	{0},
	{0}
};

// Buffer used to store radar signal data
uint8_t g_spi_buffer[FRAME_BUFFER_SIZE];

// Stores the platform init results
Platform_Status_t platform_status = {0};

// Flag to enable/disable LED
uint8_t led_enable[2] = {true, true}; // Red & Green LEDs are enabled by default

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Platform Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

PLATFORM_STATUS_e platform__init()
{
	PLATFORM_STATUS_e plat_stat = PLATFORM_SUCCESS;

	// Init board hardware
	BOARD_ConfigMPU();           // board
	BOARD_InitBootPins();        // pin_mux
	BOARD_BootClockRUN();        // clock_config

	// KL for profiling SWO
	*((uint32_t *)(0x400E0600)) = (1 << 11); /* enable TPIU clock */
	CLOCK_EnableClock(kCLOCK_Trace);
	//end edit for SWO

	BOARD_InitBootPeripherals(); // peripherals

	// Init FSL debug console
	BOARD_InitDebugConsole();    // board

	SystemCoreClockUpdate(); // is this needed?

	NVIC_SetPriority(BOARD_X4_SPI_IRQ, 3);
	NVIC_SetPriority(BOARD_I2C2_MASTER_IRQN, 4);
	//NVIC_SetPriority(BOARD_X4_GPIO1_IRQ, 3);

	// Init LEDs
	GPIO_PinInit(BOARD_INIT_LED_RED_GPIO, BOARD_INIT_LED_RED_PIN, &g_hal.gpio_led_red);
	GPIO_PinInit(BOARD_INIT_LED_GREEN_GPIO, BOARD_INIT_LED_GREEN_PIN, &g_hal.gpio_led_green);

	// Init X4 enable
	GPIO_PinInit(BOARD_INIT_X4_EN_GPIO, BOARD_INIT_X4_EN_PIN, &g_hal.x4_en);

	//
	// Init SPI configuration
	//

	LPSPI_MasterGetDefaultConfig(&g_hal.spi_x4);
	g_hal.spi_x4.baudRate = 20000000;
	g_hal.spi_x4.bitsPerFrame = 8;
	g_hal.spi_x4.cpol = kLPSPI_ClockPolarityActiveHigh;
	g_hal.spi_x4.cpha = kLPSPI_ClockPhaseFirstEdge;
	g_hal.spi_x4.direction = kLPSPI_MsbFirst;
	g_hal.spi_x4.pcsToSckDelayInNanoSec = 1000000000 / g_hal.spi_x4.baudRate;
	g_hal.spi_x4.lastSckToPcsDelayInNanoSec = 1000000000 / g_hal.spi_x4.baudRate;
	g_hal.spi_x4.betweenTransferDelayInNanoSec = 1000000000 / g_hal.spi_x4.baudRate;
	g_hal.spi_x4.whichPcs = X4_SPI;
	g_hal.spi_x4.pcsActiveHighOrLow = kLPSPI_PcsActiveLow;
	g_hal.spi_x4.pinCfg = kLPSPI_SdiInSdoOut;
	g_hal.spi_x4.dataOutConfig = kLpspiDataOutTristate;

	uint32_t spi_src_clk = X4_SPI_MASTER_CLOCK_FREQ;

	int status = LPSPI_RTOS_Init(&g_hal.spi_x4_handle, BOARD_X4_SPI_BASEADDR, &g_hal.spi_x4, spi_src_clk);
	if (kStatus_Success != status) {
		plat_stat |= PLATFORM_X4_SPI_OPEN_ERR;
		platform_status.spi_fail = 1;
	}

	//
	// Init I2C configuration
	//

	LPI2C_MasterGetDefaultConfig(&g_hal.i2c_conf);
	g_hal.i2c_conf.baudRate_Hz = BOARD_I2C2_BAUDRATE;

	status = LPI2C_RTOS_Init(&g_hal.i2c_handle, BOARD_I2C_MASTER, &g_hal.i2c_conf, BOARD_I2C_CLOCK_FREQUENCY);
	if (status != kStatus_Success) {
		plat_stat |= PLATFORM_ERR_I2C;
		platform_status.i2c_fail = 1;
	}

	//
	// Init PWM (for RGB LED control)
	//
	status = init_rgb_led_pwm();
	if (status != PLATFORM_SUCCESS) {
		plat_stat |= PLATFORM_RGB_LED_INIT_ERR;
		platform_status.rgb_fail = 1;
	}

	return plat_stat;
}


void platform__delay(uint32_t delay_ms)
{
	const TickType_t delay = delay_ms / portTICK_PERIOD_MS;

	vTaskDelay(delay);
}


void platform__delay_us(uint32_t delay_us)
{
	// The OS tick is > 1 us, so we just delay via loop
	volatile uint32_t delay = CPU_FREQ * delay_us;
	while (delay--)
		continue;
}


int32_t platform__get_timer_ticks()
{
	return xTaskGetTickCount();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LED Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void platform__led_enable(PLATFORM_GPIO_LED_e led, bool en)
{
	if (!en) {
		switch (led) {
		case LED_RED:
			platform__led_off(LED_RED);
			break;
		case LED_GREEN:
			platform__led_off(LED_GREEN);
			break;
		}
	}

	led_enable[led] = en ? 1U : 0U;
}


int platform__led_on(PLATFORM_GPIO_LED_e led)
{
	if (!led_enable[led]) return 0;

	switch (led) {
	case LED_RED:
		GPIO_PinWrite(BOARD_INIT_LED_RED_GPIO, BOARD_INIT_LED_RED_PIN, 1U);
		g_hal.gpio_led_red.outputLogic = 1;
		break;
	case LED_GREEN:
		GPIO_PinWrite(BOARD_INIT_LED_GREEN_GPIO, BOARD_INIT_LED_GREEN_PIN, 1U);
		g_hal.gpio_led_green.outputLogic = 1;
		break;
	}

	return 0;
}


int platform__led_off(PLATFORM_GPIO_LED_e led)
{
	if (!led_enable[led]) return 0;

	switch (led) {
	case LED_RED:
		GPIO_PinWrite(BOARD_INIT_LED_RED_GPIO, BOARD_INIT_LED_RED_PIN, 0U);
		g_hal.gpio_led_red.outputLogic = 0;
		break;
	case LED_GREEN:
		GPIO_PinWrite(BOARD_INIT_LED_GREEN_GPIO, BOARD_INIT_LED_GREEN_PIN, 0U);
		g_hal.gpio_led_green.outputLogic = 0;
		break;
	}

	return 0;
}


int platform__led_toggle(PLATFORM_GPIO_LED_e led)
{
	if (!led_enable[led]) return 0;

	switch (led) {
	case LED_RED:
		if (g_hal.gpio_led_red.outputLogic) {
			GPIO_PinWrite(BOARD_INIT_LED_RED_GPIO, BOARD_INIT_LED_RED_PIN, 0U);
			g_hal.gpio_led_red.outputLogic = 0;
		}
		else {
			GPIO_PinWrite(BOARD_INIT_LED_RED_GPIO, BOARD_INIT_LED_RED_PIN, 1U);
			g_hal.gpio_led_red.outputLogic = 1;
		}
		break;

	case LED_GREEN:
		if (g_hal.gpio_led_green.outputLogic) {
			GPIO_PinWrite(BOARD_INIT_LED_GREEN_GPIO, BOARD_INIT_LED_GREEN_PIN, 0U);
			g_hal.gpio_led_green.outputLogic = 0;
		}
		else {
			GPIO_PinWrite(BOARD_INIT_LED_GREEN_GPIO, BOARD_INIT_LED_GREEN_PIN, 1U);
			g_hal.gpio_led_green.outputLogic = 1;
		}
	}

	return 0;
}


int platform__set_rgb_led(uint32_t rgb)
{
	uint8_t r = (uint8_t)(100.0f * ((rgb >> 16) & 0xff) / 255.0f);
	uint8_t g = (uint8_t)(100.0f * ((rgb >>  8) & 0xff) / 255.0f);
	uint8_t b = (uint8_t)(100.0f * (rgb & 0xff) / 255.0f);

	//PRINTF("r = %d, g = %d, b = %d\t(%06x)\n", r, g, b, rgb);
	PWM_UpdatePwmDutycycle(BOARD_PWM_BASEADDR, kPWM_Module_0, kPWM_PwmB, kPWM_EdgeAligned, 100 - r);
	PWM_UpdatePwmDutycycle(BOARD_PWM_BASEADDR, kPWM_Module_1, kPWM_PwmB, kPWM_EdgeAligned, 100 - b);
	PWM_UpdatePwmDutycycle(BOARD_PWM_BASEADDR, kPWM_Module_2, kPWM_PwmB, kPWM_EdgeAligned, 100 - g);

	PWM_SetPwmLdok(BOARD_PWM_BASEADDR, kPWM_Control_Module_0 | kPWM_Control_Module_1 | kPWM_Control_Module_2, true);

	return 0;
}


static int init_rgb_led_pwm()
{
	XBARA_Init(XBARA1);
	XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputLogicHigh, kXBARA1_OutputFlexpwm2Fault0);
	XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputLogicHigh, kXBARA1_OutputFlexpwm2Fault1);
	XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputLogicHigh, kXBARA1_OutputFlexpwm1234Fault2);
	XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputLogicHigh, kXBARA1_OutputFlexpwm1234Fault3);

	pwm_signal_param_t pwm_signal;

	pwm_signal.deadtimeValue = 0;
	pwm_signal.level = kPWM_HighTrue;
	pwm_signal.pwmChannel = kPWM_PwmB;
	pwm_signal.dutyCyclePercent = 100; // rgb led off by default

	pwm_config_t pwm_config;
	PWM_GetDefaultConfig(&pwm_config);
	pwm_config.enableDebugMode = true;

	if (PWM_Init(BOARD_PWM_BASEADDR, kPWM_Module_0, &pwm_config) == kStatus_Fail) {
		return PLATFORM_RGB_LED_INIT_ERR;
	}

	if (PWM_Init(BOARD_PWM_BASEADDR, kPWM_Module_1, &pwm_config) == kStatus_Fail) {
		return PLATFORM_RGB_LED_INIT_ERR;
	}

	if (PWM_Init(BOARD_PWM_BASEADDR, kPWM_Module_2, &pwm_config) == kStatus_Fail) {
		return PLATFORM_RGB_LED_INIT_ERR;
	}

	uint32_t pwm_src_clock_hz = CLOCK_GetFreq(kCLOCK_IpgClk);
	uint32_t pwm_freq_hz = 1000;

	PWM_SetupPwm(BOARD_PWM_BASEADDR, kPWM_Module_0, &pwm_signal, 1, kPWM_EdgeAligned, pwm_freq_hz, pwm_src_clock_hz);
	PWM_SetupPwm(BOARD_PWM_BASEADDR, kPWM_Module_1, &pwm_signal, 1, kPWM_EdgeAligned, pwm_freq_hz, pwm_src_clock_hz);
	PWM_SetupPwm(BOARD_PWM_BASEADDR, kPWM_Module_2, &pwm_signal, 1, kPWM_EdgeAligned, pwm_freq_hz, pwm_src_clock_hz);

	PWM_SetPwmLdok(BOARD_PWM_BASEADDR, kPWM_Control_Module_0 | kPWM_Control_Module_1 | kPWM_Control_Module_2, true);
	PWM_StartTimer(BOARD_PWM_BASEADDR, kPWM_Control_Module_0 | kPWM_Control_Module_1 | kPWM_Control_Module_2);

	return PLATFORM_SUCCESS;
}

// ~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~
// X4Adapter Functions
// ~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~

int x4adapter_open(X4Driver_t **x4driver)
{
	// Setup mutex
	X4DriverLock_t lock;
	lock.object = (void*)xSemaphoreCreateRecursiveMutex();
	lock.lock = x4driver_callback_take_sem;
	lock.unlock = x4driver_callback_give_sem;

	// Setup timers
	X4DriverTimer_t timer_sweep;
	timer_sweep.configure = NULL;
	timer_sweep.object = NULL;
	X4DriverTimer_t timer_action;
	timer_action.configure = NULL;
	timer_action.object = NULL;

	// Setup X4Driver callbacks
	X4DriverCallbacks_t x4driver_callbacks;
	x4driver_callbacks.pin_set_enable = x4driver_callback_pin_set_enable;
	x4driver_callbacks.spi_read = x4driver_callback_spi_read;
	x4driver_callbacks.spi_write = x4driver_callback_spi_write;
	x4driver_callbacks.spi_write_read = x4driver_callback_spi_write_read;
	x4driver_callbacks.wait_us = x4driver_callback_wait_us;
	x4driver_callbacks.notify_data_ready = x4driver_notify_data_ready;
	x4driver_callbacks.trigger_sweep = x4driver_trigger_sweep_pin;
	x4driver_callbacks.enable_data_ready_isr = x4driver_enable_ISR;

	// Allocate memory and create x4driver handle
	void* x4driver_instance_memory = malloc(x4driver_get_instance_size());
	x4driver_create(x4driver, x4driver_instance_memory, &x4driver_callbacks, &lock, &timer_sweep, &timer_action, (void*)&g_hal);

	// Allocate memory for frame buffer
	(*x4driver)->spi_buffer_size = FRAME_BUFFER_SIZE;
	(*x4driver)->spi_buffer = g_spi_buffer; // using fixed array...

	return 0;
}


void x4adapter_close(X4Driver_t *x4driver)
{
	// Free up allocated memory
	free (x4driver->user_reference);
	vSemaphoreDelete(x4driver->lock.object);
	free (x4driver);
}

// ~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~
// X4DriverCallbacks_t Callback Functions
// ~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~

uint32_t x4driver_callback_pin_set_enable(void *user_reference, uint8_t value)
{
	//Hal_t *hal = user_reference;

	if (value == 0) {
		GPIO_PinWrite(BOARD_INIT_X4_EN_GPIO, BOARD_INIT_X4_EN_PIN, 0U);
	}
	else {
		GPIO_PinWrite(BOARD_INIT_X4_EN_GPIO, BOARD_INIT_X4_EN_PIN, 1U);
	}

	return 0;
}


uint32_t x4driver_callback_spi_write(void *user_reference, uint8_t *data, uint32_t length)
{
	Hal_t *hal = user_reference;

	lpspi_transfer_t xfer = {
		.txData = (uint8_t*)data,
		.rxData = NULL,
		.dataSize = length,
		.configFlags = X4_SPI_MASTER_PCS_FOR_TRANSFER | kLPSPI_MasterPcsContinuous | kLPSPI_MasterByteSwap
	};

	int status = LPSPI_RTOS_Transfer(&hal->spi_x4_handle, &xfer);
	return (status == kStatus_Success) ? 0 : -1;
}


uint32_t x4driver_callback_spi_read(void *user_reference, uint8_t *data, uint32_t length)
{
	// The x4driver doesn't current use this callback!

	Hal_t *hal = user_reference;

	lpspi_transfer_t xfer = {
		.txData = NULL,
		.rxData = (uint8_t*)data,
		.dataSize = length,
		.configFlags = X4_SPI_MASTER_PCS_FOR_TRANSFER | kLPSPI_MasterPcsContinuous | kLPSPI_MasterByteSwap
	};

	int status = LPSPI_RTOS_Transfer(&hal->spi_x4_handle, &xfer);
	return (status == kStatus_Success) ? 0 : -1;
}


uint32_t x4driver_callback_spi_write_read(void *user_reference, uint8_t *wdata, uint32_t wlength, uint8_t *rdata, uint32_t rlength)
{
	if (rlength == 1) {
		// This is the most common case across the x4driver api. The nxp freertos
		// lpspi driver seems to not deal well with situation where the wdata and
		// rdata are just single uint8_t and not arrays. This call deals with this
		// special case
		return x4driver_local_spi_write_read_one(user_reference, wdata, wlength, rdata, rlength);
	}
	else {
		// The only time this code will run is when the user is reading out the
		// radar signal data. The rdata is long array in this case so we can call
		// it conventionally
		Hal_t *hal = user_reference;

		lpspi_transfer_t xfer = {
			.txData = (uint8_t*)wdata,
			.rxData = (uint8_t*)rdata,
			.dataSize = wlength + rlength,
			.configFlags = X4_SPI_MASTER_PCS_FOR_TRANSFER | kLPSPI_MasterPcsContinuous | kLPSPI_MasterByteSwap
		};

		int status = LPSPI_RTOS_Transfer(&hal->spi_x4_handle, &xfer);
		return (status == kStatus_Success) ? 0 : -1;
	}
}


static uint32_t x4driver_local_spi_write_read_one(void *user_reference, uint8_t *wdata, uint32_t wlength, uint8_t *rdata, uint32_t rlength)
{
	Hal_t *hal = user_reference;

	uint8_t rd_tmp[2] = {0, 0};

	lpspi_transfer_t xfer = {
		.txData = (uint8_t*)wdata,
		.rxData = (uint8_t*)rd_tmp,
		.dataSize = 2,
		.configFlags = X4_SPI_MASTER_PCS_FOR_TRANSFER | kLPSPI_MasterPcsContinuous | kLPSPI_MasterByteSwap
	};

	int status = LPSPI_RTOS_Transfer(&hal->spi_x4_handle, &xfer);
	*rdata = rd_tmp[1];
	return (status == kStatus_Success) ? 0 : -1;
}


void x4driver_callback_wait_us(uint32_t us)
{
	platform__delay_us(us);
}


void x4driver_notify_data_ready(void *user_reference)
{
	// do nothing...
}


uint32_t x4driver_trigger_sweep_pin(void *user_reference)
{
	// do nothing...
	return 0;
}


void x4driver_enable_ISR(void *user_reference, uint32_t enable)
{
	if (enable) {
		//PRINTF("Enable x4 isr\n");
		EnableIRQ(BOARD_X4_GPIO1_IRQ);
	}
	else {
		//PRINTF("Disable x4 isr\n");
		DisableIRQ(BOARD_X4_GPIO1_IRQ);
	}
}

// ~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~
// X4DriverLock_t Callback Functions
// ~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~

uint32_t x4driver_callback_take_sem(void *sem, uint32_t timeout)
{
	return xSemaphoreTakeRecursive((SemaphoreHandle_t)sem, timeout);
}


void x4driver_callback_give_sem(void *sem)
{
	xSemaphoreGiveRecursive((SemaphoreHandle_t)sem);
}
