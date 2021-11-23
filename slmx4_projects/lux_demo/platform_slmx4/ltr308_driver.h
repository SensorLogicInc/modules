/**
@file ltr308_driver.h

Platform device driver for the Lite-On LTR308 ambient light sensor

@author
Justin Hadella

@copyright
2019 Flat Earth Inc
*/
#ifndef LTR308_DRIVER_h
#define LTR308_DRIVER_h

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

/**
@enum PLATFORM_ALS_RESOLUTION_e
This enumeration is used to set the ambient light sensor's resolution
*/
typedef enum {
  ALS_RESOLUTION_20BIT, ///< conversion time = 400 ms
  ALS_RESOLUTION_19BIT, ///< conversion time = 200 ms
  ALS_RESOLUTION_18BIT, ///< conversion time = 100 ms (default)
  ALS_RESOLUTION_17BIT, ///< conversion time = 50 ms
  ALS_RESOLUTION_16BIT  ///< conversion time = 25 ms

} PLATFORM_ALS_RESOLUTION_e;

/**
@enum PLATFORM_ALS_MEAS_RATE_e
This enumeration is used to set the ambient light sensor's measurement rate
*/
typedef enum {
  ALS_MEAS_RATE_25MS,   ///< measurement rate = 25 ms
  ALS_MEAS_RATE_50MS,   ///< measurement rate = 50 ms
  ALS_MEAS_RATE_100MS,  ///< measurement rate = 100 ms (default)
  ALS_MEAS_RATE_500MS,  ///< measurement rate = 500 ms
  ALS_MEAS_RATE_1000MS, ///< measurement rate = 1000 ms
  ALS_MEAS_RATE_2000MS  ///< measurement rate = 2000 ms

} PLATFORM_ALS_MEAS_RATE_e;

/**
@enum PLATFORM_ALS_GAIN_e
This enumeration is used to set the ambient light sensor's gain range
*/
typedef enum {
  ALS_GAIN_1, ///< gain range = 1
  ALS_GAIN_3, ///< gain range = 3 (default)
  ALS_GAIN_6, ///< gain range = 6
  ALS_GAIN_9, ///< gain range = 9
  ALS_GAIN_18 ///< gain range = 18

} PLATFORM_ALS_GAIN_e;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Public Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
Function sets the LTR308 ambient light sensor to active operating mode

In this mode, the sensor will start to make automatic light measurements based
on its configured measurement rate.

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
int ltr308__set_active();

/**
Function sets the LTR308 ambient light sensor to standby operating mode

In this mode, the sensor will be in a low power state.

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
int ltr308__set_standby();

/**
Function sets the LTR308 ambient light sensor's resolution and measurement rate

@note
When the measurement rate is faster than the measurement conversion time, the
rate will be lower than it's programmed for.

@param [in] res   The desired resolution (effects the conversion time)
@param [in] rate  The desired measurement rate

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
int ltr308__set_meas_rate(PLATFORM_ALS_RESOLUTION_e res, PLATFORM_ALS_MEAS_RATE_e rate);

/**
Function gets the LTR308 ambient light sensor's resolution and measurement rate

@param [out] *res   The current resolution
@param [out] *rate  The current measurement rate

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
int ltr308__get_meas_rate(PLATFORM_ALS_RESOLUTION_e *res, PLATFORM_ALS_MEAS_RATE_e *rate);

/**
Function sets the LTR308 ambient light sensor's gain range

@param [in] gain  The desired gain range

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
int ltr308__set_gain(PLATFORM_ALS_GAIN_e gain);

/**
Function gets the LTR308 ambient light sensor's gain range

@param [out] *gain  The current gain range

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
int ltr308__get_gain(PLATFORM_ALS_GAIN_e *gain);

/**
Function reads the LTR308 ambient light sensor's identification code

@note
The LTR308 identification code should read as 0xB1

@param [out] *id  The identification code

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
int ltr308__read_id(uint8_t *id);

/**
Function reads the LTR308 ambient light sensor's lux level

@param [out] *lux  The current lux level

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
int ltr308__read_lux(float *lux);

/**
Predicate function indicates whether or not new LTR308 ambient light sensor data
is ready to read

@return true if there is new data to read, otherwise false
*/
bool ltr308__is_data_new();

/**
Low-level function to read LTR308 ambient light sensor register data

@param [in]  *handle   I2C device driver handle
@param [in]   rd_addr  The I2C address to read from
@param [out] *data     Stores the data read at the given address

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
int ltr308__read_reg(void *handle, uint8_t rd_addr, uint8_t *data);

/**
Low-level function to write LTR308 ambient light sensor register data

@param [in] *handle   I2C device driver handle
@param [in]  wr_addr  The I2C address to write to
@param [in] *data     The data to write to the given address

@return PLATFORM_SUCCESS on success, non-zero error code on failure
*/
int ltr308__write_reg(void *handle, uint8_t wr_addr, uint8_t *data);

#ifdef __cplusplus
}
#endif
#endif // LTR308_DRIVER_h
