/**
@file x4_post_norm.c

See header

@par Environment
Environment Independent

@par Compiler
Compiler Independent

@author Justin Hadella

@copyright (c) 2021 Sensor Logic
*/

#include "x4_post_norm.h"

#include <stdlib.h> // for NULL

// -----------------------------------------------------------------------------
// Error Codes
// -----------------------------------------------------------------------------

#define X4_POST_NORM_SUCCESS  0
#define X4_POST_NORM_NULL_PTR 1

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------

static float calc_nregion(int tx_region);
static float calc_nfactor(bool ddc_en, int dac_step, int pps, int iterations);
static float calc_noffset(int dac_min, int dac_max);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Public Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int x4_calc_norm_factors(X4Driver_t* x4, bool *ddc_en, float *nregion, float *nfactor, float *noffset)
{
	if (NULL == x4) return X4_POST_NORM_NULL_PTR;

	// Is the DDC enabled?
	uint8_t tmp;
	x4driver_get_downconversion(x4, &tmp);
	*ddc_en = tmp;

	// Determine the region
	xtx4_tx_center_frequency_t tx_region;
	x4driver_get_tx_center_frequency(x4, &tx_region);

	// Read out the various sweep controller settings
	xtx4_dac_step_t dac_step;
	x4driver_get_dac_step(x4, &dac_step);

	uint16_t dac_min;
	x4driver_get_dac_min(x4, &dac_min);

	uint16_t dac_max;
	x4driver_get_dac_max(x4, &dac_max);

	uint16_t pps;
	x4driver_get_pulses_per_step(x4, &pps);

	uint8_t iterations;
	x4driver_get_iterations(x4, &iterations);

	// Calc the normalization factors
	*nregion = calc_nregion((int)tx_region);
	*nfactor = calc_nfactor(*ddc_en, 1 << (int)dac_step, (int)pps, (int)iterations);
	*noffset = calc_noffset((int)dac_min, (int)dac_max);

	return X4_POST_NORM_SUCCESS;
}


int x4_set_norm_factors(X4NormConfig nc, bool *ddc_en, float *nregion, float *nfactor, float *noffset)
{
	if (NULL == nc) return X4_POST_NORM_NULL_PTR;

	*ddc_en = nc->ddc_en;

	// Grab the normalization config values
	int tx_region = nc->tx_region;
	int dac_min = nc->dac_min;
	int dac_max = nc->dac_max;
	int dac_step = nc->dac_step;
	int pps = nc->pps;
	int iterations = nc->iterations;

	// Calc the normalization factors
	*nregion = calc_nregion(tx_region);
	*nfactor = calc_nfactor(*ddc_en, dac_step, pps, iterations);
	*noffset = calc_noffset(dac_min, dac_max);

	return X4_POST_NORM_SUCCESS;
}


void x4_norm_data_ddc(float *x, int n, float nregion, float nfactor)
{
	int i;
	for (i = 0; i < n; i++)
	{
		x[i] *= nregion * nfactor;

		// Conjugate the imaginary (the odd index values)
		x[i] *= (i % 2 == 1) ? -1.0 : 1.0;
	}
}


void x4_norm_data(float *x, int n, float noffset, float nfactor)
{
	int i;
	for (i = 0; i < n; i++)
	{
		x[i] = x[i] / nfactor + noffset;
	}
}

// ~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~
// Local Functions
// ~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~

static float calc_nregion(int tx_region)
{
	float nregion = (tx_region == 3) ? 1.0f / 306.0f : 1.0f / 316.0f;
	return nregion;
}


static float calc_nfactor(bool ddc_en, int dac_step, int pps, int iterations)
{
	float nfactor = 1024.0f * (float)(iterations * pps) / (float)(dac_step);

	if (ddc_en)
	{
		nfactor = 1.0f / nfactor;
	}

	return nfactor;
}


static float calc_noffset(int dac_min, int dac_max)
{
	float noffset = (float)(2048.0f - dac_max + dac_min) / 2.0f;
	return noffset;
}
