/**
See header

@author
Justin Hadella

@note
For projects which resize the DTC memory, make sure to add this -D to the
project's predefined settings: PEAK_DTC_EMBIGGEN

@copyright
2019 Flat Earth Inc
*/

/**
 * @file   board.c
 * @brief  Board initialization file.
 */
 
/* This is a template for board specific configuration created by MCUXpresso IDE Project Wizard.*/

#include <stdint.h>
#include "board.h"
#include "fsl_iomuxc.h"

/**
 * @brief Set up and initialize all required blocks and functions related to the board hardware.
 */
void BOARD_InitDebugConsole(void) {
	/* The user initialization should be placed here */
}

void BOARD_ConfigMPU(void)
{
  // Disable I cache and D cache
  if (SCB_CCR_IC_Msk == (SCB_CCR_IC_Msk & SCB->CCR)) {
    SCB_DisableICache();
  }
  if (SCB_CCR_DC_Msk == (SCB_CCR_DC_Msk & SCB->CCR)) {
    SCB_DisableDCache();
  }

  // Disable MPU
  ARM_MPU_Disable();

  /* MPU configuration:
   * Use ARM_MPU_RASR(DisableExec, AccessPermission, TypeExtField, IsShareable, IsCacheable, IsBufferable, SubRegionDisable, Size)
   *
   * API in core_cm7.h:
   * param DisableExec       Instruction access (XN) disable bit,0=instruction fetches enabled, 1=instruction fetches disabled
   * param AccessPermission  Data access permissions, allows you to configure read/write access for User and Privileged mode
   *
   * Use MACROS defined in core_cm7.h:
   * ARM_MPU_AP_NONE/ARM_MPU_AP_PRIV/ARM_MPU_AP_URO/ARM_MPU_AP_FULL/ARM_MPU_AP_PRO/ARM_MPU_AP_RO
   * Combine TypeExtField/IsShareable/IsCacheable/IsBufferable to configure MPU memory access attributes.
   *  TypeExtField  IsShareable  IsCacheable  IsBufferable   Memory Attribute    Shareability        Cache
   *     0             x           0           0             Strongly Ordered    shareable
   *     0             x           0           1              Device             shareable
   *     0             0           1           0              Normal             not shareable   Outer and inner write
   * through no write allocate
   *     0             0           1           1              Normal             not shareable   Outer and inner write
   * back no write allocate
   *     0             1           1           0              Normal             shareable       Outer and inner write
   * through no write allocate
   *     0             1           1           1              Normal             shareable       Outer and inner write
   * back no write allocate
   *     1             0           0           0              Normal             not shareable   outer and inner
   * noncache
   *     1             1           0           0              Normal             shareable       outer and inner
   * noncache
   *     1             0           1           1              Normal             not shareable   outer and inner write
   * back write/read allocate
   *     1             1           1           1              Normal             shareable       outer and inner write
   * back write/read allocate
   *     2             x           0           0              Device             not shareable
   * Above are normal use settings, if your want to see more details or want to config different inner/outer cache
   * policy.
   * Please refer to Table 4-55 /4-56 in arm cortex-M7 generic user guide <dui0646b_cortex_m7_dgug.pdf>
   * param SubRegionDisable  Sub-region disable field. 0=sub-region is enabled, 1=sub-region is disabled.
   * param Size              Region size of the region to be configured. use ARM_MPU_REGION_SIZE_xxx MACRO in
   * core_cm7.h.
   */

  // Region 0 setting: Memory with Device type, not shareable, non-cacheable
  MPU->RBAR = ARM_MPU_RBAR(0, 0xC0000000U);
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 2, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_512MB);

  // Region 1 setting: Memory with Device type, not shareable,  non-cacheable
  MPU->RBAR = ARM_MPU_RBAR(1, 0x80000000U);
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 2, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_1GB);

  // Region 2 setting
#if defined(XIP_EXTERNAL_FLASH) && (XIP_EXTERNAL_FLASH == 1)
  // Setting Memory with Normal type, not shareable, outer/inner write back
  MPU->RBAR = ARM_MPU_RBAR(2, 0x60000000U);
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_RO, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_8MB);
#else
  // Setting Memory with Device type, not shareable, non-cacheable
  MPU->RBAR = ARM_MPU_RBAR(2, 0x60000000U);
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_RO, 2, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_8MB);
#endif

  // Region 3 setting: Memory with Device type, not shareable, non-cacheable
  MPU->RBAR = ARM_MPU_RBAR(3, 0x00000000U);
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 2, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_1GB);

  // Region 4 setting: Memory with Normal type, not shareable, outer/inner write back
  MPU->RBAR = ARM_MPU_RBAR(4, 0x00000000U);
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_128KB);

  // Region 5 setting: Memory with Normal type, not shareable, outer/inner write back
  MPU->RBAR = ARM_MPU_RBAR(5, 0x20000000U);
#if defined(PEAK_DTC_EMBIGGEN)
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_512KB);
//#elif defined (PEAK_DTC_EMBIGGEN2) // 256 KB DTC, 768 KB OC
//  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_256KB);
#else
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_128KB);
#endif

  // Region 6 setting: Memory with Normal type, not shareable, outer/inner write back
  MPU->RBAR = ARM_MPU_RBAR(6, 0x20200000U);
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_512KB);

  // Region 7 setting: Memory with Normal type, not shareable, outer/inner write back
  MPU->RBAR = ARM_MPU_RBAR(7, 0x20280000U);
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_256KB);

  /* The define sets the cacheable memory to shareable,
   * this suggestion is referred from chapter 2.2.1 Memory regions,
   * types and attributes in Cortex-M7 Devices, Generic User Guide */
#if defined(SDRAM_IS_SHAREABLE)
  // Region 8 setting: Memory with Normal type, not shareable, outer/inner write back
  MPU->RBAR = ARM_MPU_RBAR(8, 0x80000000U);
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 1, 1, 1, 0, ARM_MPU_REGION_SIZE_32MB);
#else
  // Region 8 setting: Memory with Normal type, not shareable, outer/inner write back
  MPU->RBAR = ARM_MPU_RBAR(8, 0x80000000U);
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_8MB);
#endif

  /* Region 9 setting, set last 2MB of SDRAM can't be accessed by cache, global variables which are not expected to be
   * accessed by cache can be put here */

  // Memory with Normal type, not shareable, non-cacheable
  MPU->RBAR = ARM_MPU_RBAR(9, 0x80600000U);
  MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_2MB);

  // Enable MPU
  ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);

  // Enable I cache and D cache
  SCB_EnableDCache();
  SCB_EnableICache();
}

void BOARD_SD_Pin_Config(uint32_t speed, uint32_t strength)
{
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_00_USDHC1_CMD,
                      IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK |
                      IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
                      IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |
                      IOMUXC_SW_PAD_CTL_PAD_DSE(strength));
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_01_USDHC1_CLK,
                      IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK |
                      IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(0) |
                      IOMUXC_SW_PAD_CTL_PAD_DSE(strength));
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_02_USDHC1_DATA0,
                      IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK |
                      IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
                      IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |
                      IOMUXC_SW_PAD_CTL_PAD_DSE(strength));
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_03_USDHC1_DATA1,
                      IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK |
                      IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
                      IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |
                      IOMUXC_SW_PAD_CTL_PAD_DSE(strength));
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_04_USDHC1_DATA2,
                      IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK |
                      IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
                      IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |
                      IOMUXC_SW_PAD_CTL_PAD_DSE(strength));
  IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_05_USDHC1_DATA3,
                      IOMUXC_SW_PAD_CTL_PAD_SPEED(speed) | IOMUXC_SW_PAD_CTL_PAD_SRE_MASK |
                      IOMUXC_SW_PAD_CTL_PAD_PKE_MASK | IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
                      IOMUXC_SW_PAD_CTL_PAD_HYS_MASK | IOMUXC_SW_PAD_CTL_PAD_PUS(1) |
                      IOMUXC_SW_PAD_CTL_PAD_DSE(strength));
}

void BOARD_MMC_Pin_Config(uint32_t speed, uint32_t strength)
{
}

/* Get debug console frequency. */
uint32_t BOARD_DebugConsoleSrcFreq(void)
{
    uint32_t freq;

    /* To make it simple, we assume default PLL and divider settings, and the only variable
       from application is use PLL3 source or OSC source */
    if (CLOCK_GetMux(kCLOCK_UartMux) == 0) /* PLL3 div6 80M */
    {
        freq = (CLOCK_GetPllFreq(kCLOCK_PllUsb1) / 6U) / (CLOCK_GetDiv(kCLOCK_UartDiv) + 1U);
    }
    else
    {
        freq = CLOCK_GetOscFreq() / (CLOCK_GetDiv(kCLOCK_UartDiv) + 1U);
    }

    return freq;
}
