/**
Board definitions for the PEAK_EP0 hardware platform

@author
Justin Hadella

@copyright
2019 Flat Earth Inc
*/

/**
@file   board.h
@brief  Board initialization header file.
*/
 
/* This is a template for board specific configuration created by MCUXpresso IDE Project Wizard.*/

#ifndef _BOARD_H_
#define _BOARD_H_

#include "clock_config.h"
#include "pin_mux.h"

#include "fsl_common.h"
#include "fsl_gpio.h"
#include "fsl_clock.h"

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

/**
 * @brief	The board name 
 */
#define BOARD_NAME "hep2"
#define BOARD_VERSION 0

/*! @brief The board flash size */
#define BOARD_FLASH_SIZE (0x400000U)

/* RGB LED uses PWM2 */
#define BOARD_PWM_BASEADDR PWM2

#define BOARD_USDHC1_BASEADDR USDHC1
#define BOARD_USDHC2_BASEADDR USDHC2

#define BOARD_USDHC1_CLK_FREQ (CLOCK_GetSysPfdFreq(kCLOCK_Pfd0) / (CLOCK_GetDiv(kCLOCK_Usdhc1Div) + 1U))
#define BOARD_USDHC2_CLK_FREQ (CLOCK_GetSysPfdFreq(kCLOCK_Pfd0) / (CLOCK_GetDiv(kCLOCK_Usdhc2Div) + 1U))

// -----------------------------------------------------------------------------
// SDRAM
// -----------------------------------------------------------------------------

#define BOARD_SEMC SEMC
#define BOARD_SEMC_START_ADDRESS (0x80000000U)
#define BOARD_SEMC_CLK_FREQ CLOCK_GetFreq(kCLOCK_SemcClk)

// -----------------------------------------------------------------------------
// X4
// -----------------------------------------------------------------------------

#define BOARD_X4_SPI_BASEADDR (LPSPI4)
#define BOARD_X4_SPI_IRQ   (LPSPI4_IRQn)
#define BOARD_X4_GPIO1_IRQ (GPIO2_Combined_0_15_IRQn)

// -----------------------------------------------------------------------------
// I2C (HTS221, LTR308)
// -----------------------------------------------------------------------------

#define BOARD_I2C2_MASTER_IRQN (LPI2C1_IRQn)
#define BOARD_I2C_MASTER ((LPI2C_Type *)LPI2C1_BASE)
#define BOARD_I2C_CLOCK_FREQUENCY BOARD_BOOTCLOCKRUN_LPI2C_CLK_ROOT

#define BOARD_I2C2_BAUDRATE (400000)
#define BOARD_HTS221_I2C_ADDR (0x5f)
#define BOARD_LTR308_I2C_ADDR (0x53)

// -----------------------------------------------------------------------------
// FLASH
// -----------------------------------------------------------------------------

#define FLASH_SIZE          (0x01000000) /* 16 MB */
#define FLASH_BASE_ADDR     (0x60000000)
#define FLASH_SECTOR_SIZE   (4 * 1024)   /* 4 KB */
#define FLASH_PAGE_SIZE     (256)        /* 256 B */

#define FLASH_CONFIG_OPTION (0xc0000008)
#define FLASH_INSTANCE      (0)          /* Flexspi Instance 0 */

// -----------------------------------------------------------------------------
// SD-CARD
// -----------------------------------------------------------------------------

#define BOARD_USDHC_CD_GPIO_BASE BOARD_INIT_SD_CD_GPIO
#define BOARD_USDHC_CD_GPIO_PIN BOARD_INIT_SD_CD_PIN

#define BOARD_USDHC_CD_PORT_IRQ GPIO2_Combined_16_31_IRQn
#define BOARD_USDHC_CD_PORT_IRQ_HANDLER GPIO2_Combined_16_31_IRQHandler

#define BOARD_USDHC_CD_STATUS() (GPIO_PinRead(BOARD_USDHC_CD_GPIO_BASE, BOARD_USDHC_CD_GPIO_PIN))

#define BOARD_USDHC_CD_INTERRUPT_STATUS() (GPIO_PortGetInterruptFlags(BOARD_USDHC_CD_GPIO_BASE))
#define BOARD_USDHC_CD_CLEAR_INTERRUPT(flag) (GPIO_PortClearInterruptFlags(BOARD_USDHC_CD_GPIO_BASE, flag))

#define BOARD_USDHC_CD_GPIO_INIT()                                                          \
    {                                                                                       \
        gpio_pin_config_t sw_config = {                                                     \
            kGPIO_DigitalInput,                                                             \
            0,                                                                              \
            kGPIO_IntRisingOrFallingEdge,                                                   \
        };                                                                                  \
        GPIO_PinInit(BOARD_USDHC_CD_GPIO_BASE, BOARD_USDHC_CD_GPIO_PIN, &sw_config);        \
        GPIO_PortEnableInterrupts(BOARD_USDHC_CD_GPIO_BASE, 1U << BOARD_USDHC_CD_GPIO_PIN); \
        GPIO_PortClearInterruptFlags(BOARD_USDHC_CD_GPIO_BASE, ~0);                         \
    }
#define BOARD_HAS_SDCARD (1U)

#define BOARD_SD_POWER_RESET_GPIO BOARD_INIT_SD_SD_PWR_EN_GPIO
#define BOARD_SD_POWER_RESET_GPIO_PIN BOARD_INIT_SD_SD_PWR_EN_PIN

#define BOARD_USDHC_CD_GPIO_PIN BOARD_INIT_SD_CD_PIN

#define BOARD_USDHC_MMCCARD_POWER_CONTROL_INIT()                                            \
    {                                                                                       \
        gpio_pin_config_t sw_config = {                                                     \
            kGPIO_DigitalOutput,                                                            \
            0,                                                                              \
            kGPIO_NoIntmode,                                                                \
        };                                                                                  \
        GPIO_PinInit(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PIN, &sw_config); \
        GPIO_PinWrite(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PIN, true);      \
    }

#define BOARD_USDHC_SDCARD_POWER_CONTROL_INIT()                                             \
    {                                                                                       \
        gpio_pin_config_t sw_config = {                                                     \
            kGPIO_DigitalOutput,                                                            \
            0,                                                                              \
            kGPIO_NoIntmode,                                                                \
        };                                                                                  \
        GPIO_PinInit(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PIN, &sw_config); \
    }

#define BOARD_USDHC_SDCARD_POWER_CONTROL(state) \
    (GPIO_PinWrite(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PIN, state))

#define BOARD_SD_HOST_BASEADDR BOARD_USDHC1_BASEADDR
#define BOARD_SD_HOST_CLK_FREQ BOARD_USDHC1_CLK_FREQ
#define BOARD_SD_HOST_IRQ USDHC1_IRQn

// -----------------------------------------------------------------------------
// WICED
// -----------------------------------------------------------------------------

/*! @brief The ENET PHY address. */
#define BOARD_ENET0_PHY_ADDRESS (0x02U) /* Phy address of enet port 0. */

/* USB PHY configuration */
#define BOARD_USB_PHY_D_CAL (0x0CU)
#define BOARD_USB_PHY_TXCAL45DP (0x06U)
#define BOARD_USB_PHY_TXCAL45DM (0x06U)

// tmp -- just to compile the wifi
//#define BOARD_USDHC2_CD_GPIO_BASE GPIO2
//#define BOARD_USDHC2_CD_GPIO_PIN 28
//#define BOARD_USDHC2_CD_PORT_IRQ GPIO2_Combined_16_31_IRQn
//#define BOARD_USDHC2_CD_PORT_IRQ_HANDLER GPIO2_Combined_16_31_IRQHandler

/* USB PHY configuration */
#define BOARD_USB_PHY_D_CAL (0x0CU)
#define BOARD_USB_PHY_TXCAL45DP (0x06U)
#define BOARD_USB_PHY_TXCAL45DM (0x06U)

#define BOARD_WIFI_HOST_BASEADDR BOARD_USDHC2_BASEADDR
#define BOARD_WIFI_HOST_CLK_FREQ BOARD_USDHC2_CLK_FREQ
#define BOARD_WIFI_HOST_IRQ USDHC2_IRQn

//#define BOARD_USDHC_CD_STATUS() (0)
//#define BOARD_USDHC_CD_GPIO_INIT() (0)

//#define BOARD_USDHC_CD_CLEAR_INTERRUPT(flag) (GPIO_PortClearInterruptFlags(BOARD_USDHC_CD_GPIO_BASE, flag))
//#define BOARD_USDHC_CD_STATUS() (GPIO_PinRead(BOARD_USDHC_CD_GPIO_BASE, BOARD_USDHC_CD_GPIO_PIN))
//#define BOARD_USDHC_CD_INTERRUPT_STATUS() (GPIO_PortGetInterruptFlags(BOARD_USDHC_CD_GPIO_BASE))
/*
#define BOARD_USDHC_CD_GPIO_INIT()                                                          \
    {                                                                                       \
        gpio_pin_config_t sw_config = {                                                     \
            kGPIO_DigitalInput,                                                             \
            0,                                                                              \
            kGPIO_IntRisingOrFallingEdge,                                                   \
        };                                                                                  \
        GPIO_PinInit(BOARD_USDHC_CD_GPIO_BASE, BOARD_USDHC_CD_GPIO_PIN, &sw_config);        \
        GPIO_PortEnableInterrupts(BOARD_USDHC_CD_GPIO_BASE, 1U << BOARD_USDHC_CD_GPIO_PIN); \
        GPIO_PortClearInterruptFlags(BOARD_USDHC_CD_GPIO_BASE, ~0);                         \
    }
*/

//#define BOARD_USDHC_MMCCARD_POWER_CONTROL(state)

/*
#define BOARD_USDHC_MMCCARD_POWER_CONTROL_INIT()                                            \
    {                                                                                       \
        gpio_pin_config_t sw_config = {                                                     \
            kGPIO_DigitalOutput,                                                            \
            0,                                                                              \
            kGPIO_NoIntmode,                                                                \
        };                                                                                  \
        GPIO_PinInit(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PIN, &sw_config); \
        GPIO_PinWrite(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PIN, true);      \
    }
*/


#define BOARD_WIFI_POWER_RESET_GPIO BOARD_INIT_WICED_WL_REG_ON_GPIO
#define BOARD_WIFI_POWER_RESET_GPIO_PIN BOARD_INIT_WICED_WL_REG_ON_PIN

#define BOARD_USDHC2_POWER_CONTROL_INIT()                                             \
    {                                                                                       \
        gpio_pin_config_t sw_config = {                                                     \
            kGPIO_DigitalOutput,                                                            \
            0,                                                                              \
            kGPIO_NoIntmode,                                                                \
        };                                                                                  \
        GPIO_PinInit(BOARD_WIFI_POWER_RESET_GPIO, BOARD_WIFI_POWER_RESET_GPIO_PIN, &sw_config); \
    }

#define BOARD_USDHC2_POWER_CONTROL(state) \
    (GPIO_PinWrite(BOARD_WIFI_POWER_RESET_GPIO, BOARD_WIFI_POWER_RESET_GPIO_PIN, state))



#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

// -----------------------------------------------------------------------------
// BLUETOOTH
// -----------------------------------------------------------------------------

#define BOARD_BT_UART_BASEADDR LPUART1
#define BOARD_BT_UART_CLK_FREQ BOARD_DebugConsoleSrcFreq()
#define BOARD_BT_UART_IRQ LPUART1_IRQn

#define BOARD_INITPINS_BT_REG_ON_GPIO BOARD_INIT_WICED_BT_REG_ON_GPIO
#define BOARD_INITPINS_BT_REG_ON_PIN BOARD_INIT_WICED_BT_REG_ON_PIN

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// API
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
 * @brief 	Initialize board specific settings.
 */
void BOARD_InitDebugConsole(void);

void BOARD_ConfigMPU(void);

void BOARD_SD_Pin_Config(uint32_t speed, uint32_t strength);
void BOARD_MMC_Pin_Config(uint32_t speed, uint32_t strength);

uint32_t BOARD_DebugConsoleSrcFreq(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* _BOARD_H_ */
