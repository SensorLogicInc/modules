/**
@file x4_post_norm.h

This is a small utility api to help post normalize raw data from the X4 radar

@note
Once normalized, this should produce identical data to x4 driver's own normalization feature

@par Environment
Environment Independent

@par Compiler
Compiler Independent

@author Justin Hadella

@copyright (c) 2021 Sensor Logic
*/
#ifndef X4_POST_NORM_h
#define X4_POST_NORM_h

#include "x4driver.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Data Structure
// -----------------------------------------------------------------------------

typedef struct {
	bool ddc_en;
	int tx_region;
	int dac_min;
	int dac_max;
	int dac_step;
	int pps;
	int iterations;

} X4NormConfig_t, *X4NormConfig;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Public Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
Function used to calculate the radar signal normalization factors for X4 radar

@note
This requires an *X4* hence only should run on embedded platform!

@note
This calculates all the normalization factors whether or not DDC is enabled. If
the DDC is enabled, then *nregion* and *nfactor* are used. If the DDC is disabled
then *nfactor* and *noffset* are used.

@param [in]  x4  Handle to X4 radar
@param [out] *ddc_en  Flag indicate whether hardware DDC is enabled
@param [out] *nregion  Normalization factor of current region when DDC is enabled
@param [out] *nfactor  Normalization factor multiplier
@param [out[ *noffset  Normalization factor offset (when DDC is disabled)

@return X4_POST_NORM_SUCCESS on success, otherwise non-zero error code
*/
int x4_calc_norm_factors(X4Driver_t* x4, bool *ddc_en, float *nregion, float *nfactor, float *noffset);

/**
Function to set the normalization factors based on given configuration

@note
In a playback, the [header] would contain the necessary radar settings to 
determine the normalization factors -- the user would just need to stuff all
the relavant radar settings into the config structure before calling this.

@param [in] nc  Data structure containing radar settings needed to determine normalization
@param [out] *ddc_en  Flag indicate whether hardware DDC is enabled
@param [out] *nregion  Normalization factor of current region when DDC is enabled
@param [out] *nfactor  Normalization factor multiplier
@param [out[ *noffset  Normalization factor offset (when DDC is disabled)

@return X4_POST_NORM_SUCCESS on success, otherwise non-zero error code
*/
int x4_set_norm_factors(X4NormConfig nc, bool *ddc_en, float *nregion, float *nfactor, float *noffset);

/**
Function to *post* normalize radar data (for DDC data)

@note
Input data *x* will be overwritten with normalized data

@note
The input n represents the #values in array, so in typical X4, this will be 376
samples in length.

@param [in,out] *x        Radar data to normalize
@param [in]      n        The length of the radar data
@param [in]      nregion  The normalization factor for DDC based on tx region
@param [in]      nfactor  The normalization multiplier

@return X4_POST_NORM_SUCCESS on success, otherwise non-zero error code
*/
void x4_norm_data_ddc(float *x, int n, float nregion, float nfactor);

/**
Function to *post* normalize radar data (for raw data)

@param [in,out] *x        Radar data to normalize
@param [in]      n        The length of the radar data
@param [in]      noffset  The normalization offset
@param [in]      nfactor  The normalization multiplier

@return X4_POST_NORM_SUCCESS on success, otherwise non-zero error code
*/
void x4_norm_data(float *x, int n, float noffset, float nfactor);

#ifdef __cplusplus
}
#endif
#endif // X4_POST_NORM_h
