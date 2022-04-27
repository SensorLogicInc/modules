/**
@file ltr308_driver.c

See header

@author
Justin Hadella

@copyright
2019 Flat Earth Inc
*/
#include "ltr308_driver.h"

// Platform includes
#include "slmx4_freertos.h"

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Global Variables
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

extern Hal_t g_hal;

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

#define ALS_MAIN_CTRL_REG (0x0)
#define ALS_MEAS_RATE_REG (0x4)
#define ALS_GAIN_REG      (0x5)
#define ALS_PART_ID_REG   (0x6)
#define ALS_STATUS_REG    (0x7)
#define ALS_DATA_L_REG    (0xD)
#define ALS_DATA_M_REG    (0xE)
#define ALS_DATA_H_REG    (0xF)

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------

static float als_calc_gain(PLATFORM_ALS_GAIN_e gain);
static float als_calc_int(PLATFORM_ALS_RESOLUTION_e res);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Public Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int ltr308__set_active()
{
  uint8_t reg_data = 1 << 1; // Set ALS active

  int status = ltr308__write_reg(&g_hal.i2c_handle, ALS_MAIN_CTRL_REG, &reg_data);

  // Wait 5 ms for the voltages/current to settle
  platform__delay(5);

  return status;
}


int ltr308__set_standby()
{
  uint8_t reg_data = 0; // Set ALS standby

  return ltr308__write_reg(&g_hal.i2c_handle, ALS_MAIN_CTRL_REG, &reg_data);
}


int ltr308__set_meas_rate(PLATFORM_ALS_RESOLUTION_e res, PLATFORM_ALS_MEAS_RATE_e rate)
{
  uint8_t reg_data = ((uint8_t)res << 4) | (uint8_t)rate;

  g_hal.als_int = als_calc_int(res);

  return ltr308__write_reg(&g_hal.i2c_handle, ALS_MEAS_RATE_REG, &reg_data);
}


int ltr308__get_meas_rate(PLATFORM_ALS_RESOLUTION_e *res, PLATFORM_ALS_MEAS_RATE_e *rate)
{
  uint8_t reg_data;
  int status = ltr308__read_reg(&g_hal.i2c_handle, ALS_MEAS_RATE_REG, &reg_data);

  uint8_t tmp = reg_data >> 4;
  *res = (PLATFORM_ALS_RESOLUTION_e)tmp;

  tmp = reg_data & 0x7;
  if (tmp == 7) tmp = 6;
  *rate = (PLATFORM_ALS_MEAS_RATE_e)tmp;

  return status;
}


int ltr308__set_gain(PLATFORM_ALS_GAIN_e gain)
{
  uint8_t reg_data = (uint8_t)gain;

  g_hal.als_gain = als_calc_gain(gain);

  return ltr308__write_reg(&g_hal.i2c_handle, ALS_GAIN_REG, &reg_data);
}


int ltr308__get_gain(PLATFORM_ALS_GAIN_e *gain)
{
  uint8_t reg_data;
  int status = ltr308__read_reg(&g_hal.i2c_handle, ALS_GAIN_REG, &reg_data);

  *gain = (PLATFORM_ALS_GAIN_e)(reg_data & 0x7);

  return status;
}


int ltr308__read_id(uint8_t *id)
{
  return ltr308__read_reg(&g_hal.i2c_handle, ALS_PART_ID_REG, id);
}


int ltr308__read_lux(float *lux)
{
  uint8_t reg_data[3];
  int status = 0;
  status |= ltr308__read_reg(&g_hal.i2c_handle, ALS_DATA_L_REG, &reg_data[0]);
  status |= ltr308__read_reg(&g_hal.i2c_handle, ALS_DATA_M_REG, &reg_data[1]);
  status |= ltr308__read_reg(&g_hal.i2c_handle, ALS_DATA_H_REG, &reg_data[2]);

  reg_data[2] &= 0xf;
  uint32_t raw_data = (reg_data[2] << 16) | (reg_data[1] << 8) | reg_data[0];

  *lux = (0.6f * (float)raw_data) / (g_hal.als_gain * g_hal.als_int);

  return status;
}


bool ltr308__is_data_new()
{
  uint8_t reg_data;
  ltr308__read_reg(&g_hal.i2c_handle, ALS_STATUS_REG, reg_data);

  return (bool)((reg_data >> 3) & 0x1);
}


int ltr308__read_reg(void *handle, uint8_t rd_addr, uint8_t *data)
{
  return (int)ltr308_io_read(&g_hal.i2c_handle, rd_addr, data);
}


int ltr308__write_reg(void *handle, uint8_t wr_addr, uint8_t *data)
{
  return (int)ltr308_io_write(&g_hal.i2c_handle, wr_addr, data);
}

// ~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~
// Local Functions
// ~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~

static float als_calc_gain(PLATFORM_ALS_GAIN_e gain)
{
  float als_gain = 1.0f;
  switch (gain) {
  case ALS_GAIN_1:
    als_gain = 1.0f;
    break;

  case ALS_GAIN_3:
    als_gain = 3.0f;
    break;

  case ALS_GAIN_6:
    als_gain = 6.0f;
    break;

  case ALS_GAIN_9:
    als_gain = 9.0f;
    break;

  case ALS_GAIN_18:
    als_gain = 18.0f;
    break;
  }
  return als_gain;
}


static float als_calc_int(PLATFORM_ALS_RESOLUTION_e res)
{
  float alt_int = 1.0f;
  switch (res) {
  case ALS_RESOLUTION_20BIT:
    alt_int = 4.0f;
    break;

  case ALS_RESOLUTION_19BIT:
    alt_int = 2.0f;
    break;

  case ALS_RESOLUTION_18BIT:
    alt_int = 1.0f;
    break;

  case ALS_RESOLUTION_17BIT:
    alt_int = 0.5f;
    break;

  case ALS_RESOLUTION_16BIT:
    alt_int = 0.25f;
    break;
  }
  return alt_int;
}
