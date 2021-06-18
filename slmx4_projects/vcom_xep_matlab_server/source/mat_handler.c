/**
@file mat_handler.c

See header

@par Environment
Environment Independent

@par Compiler
Compiler Independent

@author Justin Hadella

@copyright (c) 2021 Sensor Logic
*/
#include "project.h"

// Platform include
#include "slmx4_freertos.h"

// Driver include
#include "x4driver.h"

// App include (which includes necessary usb headers)
#include "virtual_com.h"

// CMSIS DSP include
#include "arm_const_structs.h"
#include "arm_math.h"
#include <math.h>

// Local include
#include "x4_post_norm.h"

#include <cr_section_macros.h>

#include "mat_handler.h"

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

#define ERR_GET_X4_REG_BAD_TAG  (-1000)
#define ERR_SET_X4_REG_BAD_TAG  (-1001)
#define ERR_GET_X4F_REG_BAD_TAG (-1002)
#define ERR_SET_X4F_REG_BAD_TAG (-1003)

// The speed of light (m/s)
#define C (299792458.0)

// The X4 PLL is locked to 243 MHz
#define X4_FIXED_PLL (243e6)

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// External Variables
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

extern uint8_t ucHeap[configTOTAL_HEAP_SIZE];

// Handle to x4 driver
extern X4Driver_t *x4;

// USB VCOM
extern usb_cdc_vcom_struct_t s_cdcVcom;

// Flag used by x4 ISR
extern int x4_data_ready;

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Variables
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

int include_packet_length_flag = 0; // externally accessed!

// Flag indicates if radar handle is open/active
static bool isOpen = false;

// Flag indicates whether DDC is enabled
static bool ddc_en = false;

// Storage for USB data to transmit
static uint8_t usb_tx_buf[4 * 1536 + 5];

// Stores the radar signal data
static float x[1536]; // DDC_EN == 1

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------

static bool memory_allocation_ok();
static void free_memory();

static void parse_user_command(char* buf, int n, char* cmd, char* arg1, char* arg2, char* arg3);

static int InitHandle_x4();
static int OpenRadar_x4();
static int CloseRadar_x4();

static int VarGetValue_ByName_x4(char* var_name);
static int VarSetValue_ByName_x4(char* var_name, char* var_value);

static int GetFrameRaw_x4();
static int GetFrameNormalized_x4();

static int ListVariables_x4();
static int RegisterRead_x4(int address);
static int ResetAllVars_x4();

static int GetRegisterProperties_x4(char *name);

static int get_frame_normalized(X4Driver_t* x4driver, float *frame, int n);
static int get_frame_raw(X4Driver_t* x4driver, float *frame, int n);

static int connector_version();
static int write_warning(const char* warning);
static int include_packet_length(int enable);
static int write_data_nack(const char* data);
static int write_binary_nack(const void* data, int data_len);
static int write_ack();
static int write_error(const char* error);
static int write_binary(const void* data, int data_len);
static int write_data(const char* data);
static int set_io_pin_dir(int bank, int pin, int direction);
static int write_io_pin(int bank, int pin, int val);
static int read_io_pin(int bank, int pin, int* val);

static void usb_write_buf(uint8_t *buf, uint32_t buf_len, uint32_t *offset);
static int  usb_write(size_t n);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Public Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void handle_client_init()
{
	bool mem_alloc_ok = memory_allocation_ok();
	if (!mem_alloc_ok)
	{
		PRINTF("unable to allocate DSP memory!\n");
		return;
	}

	int status = x4adapter_open(&x4);
	if (status) {
		write_error("x4adapter_open() error");
	}

}


void handle_client_request(uint8_t *buf, int n)
{
	// Define some vars to use in parsing....
	int ind = 0;

	char cmd[100];     // Parsed command string
	char arg1[100];    // First argument of the command
	char arg2[100];    // Second argument of the command
	char arg3[100];    // Third argument of the command

	int dummy;

	parse_user_command(buf, n, cmd, arg1, arg2, arg3);
//	printf("~~ cmd = <%s>\n", buf);
	memset(buf, 0, n);

	// Handle the user's command
	if (strcmp("VarGetValue_ByName", cmd) == 0)
		VarGetValue_ByName_x4(arg1);
	else if (strcmp("NVA_CreateHandle", cmd) == 0)
		InitHandle_x4();
	else if (strcmp("OpenRadar", cmd) == 0)
		OpenRadar_x4();
	else if (strcmp("GetFrameRaw", cmd) == 0)
		GetFrameRaw_x4();
	else if (strcmp("GetFrameNormalized", cmd) == 0)
		GetFrameNormalized_x4();
	else if (strcmp("VarSetValue_ByName", cmd) == 0)
		VarSetValue_ByName_x4(arg1, arg2);
	else if (strcmp("ListVariables", cmd) == 0)
		ListVariables_x4();
	else if (strcmp("RegisterRead", cmd) == 0)
		RegisterRead_x4(atoi(arg1));
	else if (strcmp("VarsResetAllToDefault", cmd) == 0)
		ResetAllVars_x4();
	else if (strcmp("ConnectorVersion", cmd) == 0)
		connector_version();
	else if (strcmp("Close", cmd) == 0)
	{
		CloseRadar_x4();
	}
	else if (strcmp("SendPacketLengths", cmd) == 0)
		include_packet_length(atoi(arg1));
	else if (strcmp("SetIOPinDirection", cmd) == 0)
		set_io_pin_dir(atoi(arg1), atoi(arg2), atoi(arg3));
	else if (strcmp("WriteIOPin", cmd) == 0)
		write_io_pin(atoi(arg1), atoi(arg2), atoi(arg3));
	else if (strcmp("ReadIOPin", cmd) == 0)
		read_io_pin(atoi(arg1), atoi(arg2), &dummy);//?? need a return -- this won't compile!
	else if (strcmp("GetRegisterProperties", cmd) == 0)
		GetRegisterProperties_x4(arg1);
	else
		write_error("Invalid and/or Unimplemented Command");
}

// ~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~
// Local Functions
// ~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~

static bool memory_allocation_ok()
{
	bool ok = true;

	return ok;
}


static void free_memory()
{

}


static void parse_user_command(char *buf, int n, char *cmd, char *arg1, char *arg2, char *arg3)
{
	int i;
	int start_idx = 0;

	// Parse the cmd
	for (i = 0; i < n; i++)
	{
		if (buf[i] == '(')
			break;

		cmd[i] = buf[i];
	}
	cmd[i] = '\0';
	start_idx = i + 1;  // Move to the position after the '('

	// parse the first arg
	for (i = start_idx; i < n; i++)
	{
		if ((buf[i] == ')') || (buf[i] == ','))
			break;

		arg1[i - start_idx] = buf[i];
	}
	arg1[i - start_idx] = '\0';
	start_idx = i + 1;  // Move to the position after the '('

	// parse the second arg
	for (i = start_idx; i < n; i++)
	{
		if ((buf[i] == ')') || (buf[i] == ','))
			break;

		arg2[i - start_idx] = buf[i];
	}
	arg2[i - start_idx] = '\0';
	start_idx = i + 1;  // Move to the position after the '('

	// parse the third arg
	for (i = start_idx; i < n; i++)
	{
		if ((buf[i] == ')') || (buf[i] == ','))
			break;

		arg3[i - start_idx] = buf[i];
	}
	arg3[i - start_idx] = '\0';

//	PRINTF(">>>> parse_user_command():\r\n cmd: %s \r\n arg1: %s \r\n arg2: %s \r\n arg3: %s \r\n", cmd, arg1, arg2, arg3);
}


static int InitHandle_x4()
{
	write_ack();
	return 0;
}


static int OpenRadar_x4()
{
	if (isOpen == 1)
	{
		write_error("Connection already open");
		return 0;
	}

	int status = x4driver_init(x4);
	if (status)
	{
		char buf[256];

		int reason = x4driver_check_configuration(x4);
		snprintf(buf, 256, "x4driver_init() error %d, check config = %d", status, reason);
		write_error(buf);

		return 0;
	}

	isOpen = 1;

	write_ack();

	return 0;
}


static int CloseRadar_x4()
{
	if (isOpen == 0)
	{
		write_error("ERROR: Radar is closed");
		return 1;
	}

	isOpen = 0;

	write_ack();

	return 0;
}


static int VarGetValue_ByName_x4(char *var_name)
{
	if (isOpen == 0)
	{
		write_error("ERROR: Radar is closed");
		return 1;
	}

	int status;
	char buf[80];

	if (strcmp("DACMin", var_name) == 0 || strcmp("dac_min", var_name) == 0)
	{
		uint16_t tmp;
		status = x4driver_get_dac_min(x4, &tmp);
		sprintf(buf, "%d", (int)tmp);
	}
	else if (strcmp("DACMax", var_name) == 0 || strcmp("dac_max", var_name) == 0)
	{
		uint16_t tmp;
		status = x4driver_get_dac_max(x4, &tmp);
		sprintf(buf, "%d", (int)tmp);
	}
	else if (strcmp("DACStep", var_name) == 0 || strcmp("dac_step", var_name) == 0)
	{
		xtx4_dac_step_t tmp;
		status = x4driver_get_dac_step(x4, &tmp);
		sprintf(buf, "%d", (int)tmp);
	}
	else if (strcmp("PPS", var_name) == 0 || strcmp("pps", var_name) == 0)
	{
		uint16_t tmp;
		status = x4driver_get_pulses_per_step(x4, &tmp);
		sprintf(buf, "%d", (int)tmp);
	}
	else if (strcmp("Iterations", var_name) == 0 || strcmp("iterations", var_name) == 0)
	{
		uint8_t tmp;
		status = x4driver_get_iterations(x4, &tmp);
		sprintf(buf, "%d", (int)tmp);
	}
	else if (strcmp("PRF", var_name) == 0 || strcmp("prf", var_name) == 0)
	{
		uint8_t prf_div;
		status = x4driver_get_prf_div(x4, &prf_div);
		sprintf(buf, "%e", X4_FIXED_PLL / (float)prf_div);
	}
	else if (strcmp("prf_div", var_name) == 0)
	{
		uint8_t tmp;
		status = x4driver_get_prf_div(x4, &tmp);
		sprintf(buf, "%d", (int)tmp);
	}
	else if (strcmp("SamplingRate", var_name) == 0 || strcmp("fs", var_name) == 0)
	{
		float tmp;
		status = x4driver_get_sampler_frequency(x4, &tmp);
		sprintf(buf, "%e", tmp);
	}
	else if (strcmp("SamplersPerFrame", var_name) == 0 || strcmp("num_samples", var_name) == 0)
	{
		uint32_t tmp;
		status = x4driver_get_frame_bin_count(x4, &tmp);
		sprintf(buf, "%d", (int)tmp);
	}
	else if (strcmp("frame_length", var_name) == 0)
	{
		uint32_t tmp;
		status = x4driver_get_frame_length(x4, &tmp);
		sprintf(buf, "%d", (int)tmp);
	}
	else if (strcmp("RxWait", var_name) == 0 || strcmp("rx_wait", var_name) == 0)
	{
		uint8_t tmp;
		status = x4driver_get_rx_wait(x4, &tmp);
		sprintf(buf, "%d", (int)tmp);
	}
	else if (strcmp("tx_region", var_name) == 0)
	{
		xtx4_tx_center_frequency_t tmp;
		status = x4driver_get_tx_center_frequency(x4, &tmp);
		sprintf(buf, "%d", (int)tmp);
	}
	else if (strcmp("tx_power", var_name) == 0)
	{
		xtx4_tx_power_t tmp;
		status = x4driver_get_tx_power(x4, &tmp);
		sprintf(buf, "%d", (int)tmp);
	}
	else if (strcmp("DownConvert", var_name) == 0 || strcmp("ddc_en", var_name) == 0)
	{
		uint8_t tmp;
		status = x4driver_get_downconversion(x4, &tmp);
		sprintf(buf, "%d", (int)tmp);
	}
	else if (strcmp("frame_offset", var_name) == 0)
	{
		float tmp;
		status = x4driver_get_frame_area_offset(x4, &tmp);
		sprintf(buf, "%e", tmp);
	}
	else if (strcmp("frame_start", var_name) == 0)
	{
		float tmp1, tmp2;
		status = x4driver_get_frame_area(x4, &tmp1, &tmp2);
		sprintf(buf, "%e", tmp1);
	}
	else if (strcmp("frame_end", var_name) == 0)
	{
		float tmp1, tmp2;
		status = x4driver_get_frame_area(x4, &tmp1, &tmp2);
		sprintf(buf, "%e", tmp2);
	}
	else if (strcmp("sweep_time", var_name) == 0)
	{
		float tmp;
		status = x4driver_get_sweep_time(x4, &tmp);
		sprintf(buf, "%e", tmp);
	}
	else if (strcmp("unambiguous_range", var_name) == 0 || strcmp("ur", var_name) == 0)
	{
		uint8_t prf_div;
		status = x4driver_get_prf_div(x4, &prf_div);

		float prf = X4_FIXED_PLL / (float)prf_div;
		sprintf(buf, "%e", C / (2.0 * prf));
	}
	else if (strcmp("res", var_name) == 0)
	{
		float tmp;
		status = x4driver_get_bin_length(x4, &tmp);
		sprintf(buf, "%e", tmp);
	}
	else if (strcmp("fs_rf", var_name) == 0)
	{
		float tmp;
		status = x4driver_get_sampler_frequency_rf(x4, &tmp);
		sprintf(buf, "%e", tmp);
	}
	else
	{
		sprintf(buf, "<ERR>Unknown Variable Name");
	}

	if (status)
	{
		char error[256];
		snprintf(error, 256, "ERROR: Set var error code = %d", status);
		write_error(error);
	}
	else
		write_data(buf);

	return 0;
}


int VarSetValue_ByName_x4(char* var_name, char* var_value)
{
	if (isOpen == 0)
	{
		write_error("ERROR: Radar is closed");
		return 1;
	}

	int status;

	if (strcmp("DACMin", var_name) == 0 || strcmp("dac_min", var_name) == 0)
	{
		int tmp = atoi(var_value);

		status = x4driver_set_dac_min(x4, (uint16_t)tmp);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("DACMax", var_name) == 0 || strcmp("dac_max", var_name) == 0)
	{
		int tmp = atoi(var_value);

		status = x4driver_set_dac_max(x4, (uint16_t)tmp);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("DACStep", var_name) == 0 || strcmp("dac_step", var_name) == 0)
	{
		xtx4_dac_step_t dac_step;

		int tmp = atoi(var_value);

		switch(tmp)
		{
			case 0:
				dac_step = DAC_STEP_1;
				break;
			case 1:
				dac_step = DAC_STEP_2;
				break;
			case 2:
				dac_step = DAC_STEP_4;
				break;
			case 3:
				dac_step = DAC_STEP_8;
				break;
			default: // enforce legal setting?
				dac_step = DAC_STEP_1;
				break;
		}
		status = x4driver_set_dac_step(x4, dac_step);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("PPS", var_name) == 0 || strcmp("pps", var_name) == 0)
	{
		int tmp = atoi(var_value);

		status = x4driver_set_pulses_per_step(x4, (uint16_t)tmp);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("Iterations", var_name) == 0 || strcmp("iterations", var_name) == 0)
	{
		int tmp = atoi(var_value);

		// Novelda **Highly Recommends** that Iterations be divisible by 2^noiseless_ghost_order*2^trx_auto_bidir_enable
		uint8_t trx_auto_bidir_enable;
		uint8_t noiseless_ghost_order;

		x4driver_get_pif_register(x4, 0x34, &trx_auto_bidir_enable);
		x4driver_get_pif_register(x4, 0x3e, &noiseless_ghost_order);

		trx_auto_bidir_enable = (trx_auto_bidir_enable >> 5) & 0x01;
		noiseless_ghost_order = (noiseless_ghost_order >> 4) & 0x07;

		int recMult = (1 << noiseless_ghost_order) * (1 << trx_auto_bidir_enable);
		if ((tmp % recMult) != 0)
		{
			char message[100];
			sprintf(message, "It is recommended to set iterations to a multiple of %d with these radar settings", recMult);
			write_warning(message);
		}

		status = x4driver_set_iterations(x4, (uint8_t)tmp);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("prf_div", var_name) == 0)
	{
		int tmp = atoi(var_value);

		status = x4driver_set_prf_div(x4, (uint8_t)tmp);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("RxWait", var_name) == 0 || strcmp("rx_wait", var_name) == 0)
	{
		int tmp = atoi(var_value);

		status = x4driver_set_rx_wait(x4, (uint8_t)tmp);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("TxRegion", var_name) == 0 || strcmp("tx_region", var_name) == 0)
	{
		xtx4_tx_center_frequency_t tx_center_frequency;

		int tmp = atoi(var_value);
		switch(tmp)
		{
			case 3:
				tx_center_frequency = TX_CENTER_FREQUENCY_EU_7_290GHz;
				break;
			case 4:
				tx_center_frequency = TX_CENTER_FREQUENCY_KCC_8_748GHz;
				break;
			default:
				tx_center_frequency = TX_CENTER_FREQUENCY_EU_7_290GHz;
				break;
		}
		status = x4driver_set_tx_center_frequency(x4, tx_center_frequency);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("DownConvert", var_name) == 0 || strcmp("ddc_en", var_name) == 0)
	{
		int tmp = atoi(var_value);

		ddc_en = (tmp == 1) ? true : false;

		status = x4driver_set_downconversion(x4, (uint8_t)tmp);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("frame_length", var_name) == 0)
	{
		int tmp = atoi(var_value);

		status = x4driver_set_frame_length(x4, (uint8_t)tmp);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("tx_power", var_name) == 0)
	{
		xtx4_tx_power_t tx_power;

		int tmp = atoi(var_value);
		switch(tmp)
		{
			case 0:
				tx_power = TX_POWER_OFF;
				break;
			case 1:
				tx_power = TX_POWER_LOW;
				break;
			case 2:
				tx_power = TX_POWER_MEDIUM;
				break;
			case 3:
				tx_power= TX_POWER_HIGH;
				break;
			default:
				tx_power = TX_POWER_MEDIUM;
				break;
		}
		status = x4driver_set_tx_power(x4, tx_power);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}

	}
	else if (strcmp("ddc_en", var_name) == 0)
	{
		int tmp = atoi(var_value);

		status = x4driver_set_downconversion(x4, (uint8_t)tmp);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("frame_offset", var_name) == 0)
	{
		float tmp = atof(var_value);

		status = x4driver_set_frame_area_offset(x4, tmp);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("frame_start", var_name) == 0)
	{
		float start, end;

		x4driver_get_frame_area(x4, &start, &end);

		start = atof(var_value);

		status = x4driver_set_frame_area(x4, start, end);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else if (strcmp("frame_end", var_name) == 0)
	{
		float start, end;

		x4driver_get_frame_area(x4, &start, &end);

		end = atof(var_value);

		status = x4driver_set_frame_area(x4, start, end);
		if (status)
		{
			write_error("Error setting register\n");
			return 1;
		}
	}
	else
	{
		write_error("Unknown/Invalid Variable Name");
		return 1;
	}

	write_ack();

	return 0;
}


static int GetFrameRaw_x4()
{
	if (isOpen == 0)
	{
		write_error("ERROR: Radar is closed");
		return 1;
	}

	// Get the number of bins in the sample
	int bins;
	x4driver_get_frame_bin_count(x4, &bins);

	if (ddc_en)
		bins *= 2;

	// Get a new frame
	get_frame_raw(x4, x, bins);

	// Send the radar frame to the client
	write_binary(x, bins * sizeof(float));

	return 0;
}


static int GetFrameNormalized_x4()
{
	if (isOpen == 0)
	{
		write_error("ERROR: Radar is closed");
		return 1;
	}

	// Get the number of bins in the sample
	int bins;
	x4driver_get_frame_bin_count(x4, &bins);

	if (ddc_en)
		bins *= 2;

	// Get a new frame
	get_frame_normalized(x4, x, bins);

	//Send the radar frame to the client
	write_binary(x, bins * sizeof(float));

	return 0;
}


static int ListVariables_x4()
{
	if (isOpen == 0)
	{
		write_error("ERROR: Radar is closed");
		return 1;
	}

	char *regList = "DACMin,dac_min,DACMax,dac_max,DACStep,dac_step,PPS,pps,Iterations,iterations,PRF,prf,prf_div,SamplingRate,fs,SamplersPerFrame,num_samples,frame_length,RxWait,rx_wait,tx_region,tx_power,DownConvert,ddc_en,frame_offset,frame_start,frame_end,sweep_time,unambiguous_range,ur,fs_rf,frame_offset,res";
	write_data(regList);

	return 0;
}


static int RegisterRead_x4(int address)
{
	if (isOpen == 0)
	{
		write_error("ERROR: Radar is closed");
		return 1;
	}

	uint8_t registerValue = 0;

	int status = x4driver_get_spi_register(x4, (uint8_t)address, &registerValue);
	if (status)
	{
		char radarErrorMsg[] = "Cannot read register";
		write_error(radarErrorMsg);
		return 1;
	}
	else
	{
		char temp[10];
		sprintf(temp, "%u", registerValue);
		write_data(temp);
	}

	return 0;
}


static int ResetAllVars_x4()
{
	if (isOpen == 0)
	{
		write_error("ERROR: Radar is closed");
		return 1;
	}

	x4driver_setup_default(x4);

	write_ack();

	return 0;
}


static int GetRegisterProperties_x4(char *name)
{
	write_ack();
	return 0;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Radar Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
Function to get single normalized radar frame

@param [in]  *x4driver  Pointer to X4 driver instance
@param [out] *frame     Radar frame which will be written to
@param [in]   n         Number of bins in single radar frame
*/
static int get_frame_normalized(X4Driver_t* x4driver, float *frame, int n)
{
	int status;

	// Start radar sweep
	status = x4driver_start_sweep(x4driver);

	// Wait for sweep to complete
	uint8_t trx_ctrl_done = 0;
	do {
		status |= x4driver_get_pif_register(x4driver, ADDR_PIF_TRX_CTRL_DONE_R, &trx_ctrl_done);
	} while (trx_ctrl_done == 0);

	// Read the radar data
	uint32_t fc = 0;
	status |= x4driver_read_frame_normalized(x4driver, &fc, frame, n);

	return status;
}

/**
Function to get single normalized radar frame

@param [in]  *x4driver  Pointer to X4 driver instance
@param [out] *frame     Radar frame which will be written to
@param [in]   n         Number of bins in single radar frame
*/
static int get_frame_raw(X4Driver_t* x4driver, float *frame, int n)
{
	int status;

	// Start radar sweep
	status = x4driver_start_sweep(x4driver);

	// Wait for sweep to complete
	uint8_t trx_ctrl_done = 0;
	do {
		status |= x4driver_get_pif_register(x4driver, ADDR_PIF_TRX_CTRL_DONE_R, &trx_ctrl_done);
	} while (trx_ctrl_done == 0);

	// Read the radar data
	uint32_t fc = 0;
	status |= x4driver_read_frame_raw(x4driver, &fc, frame, n);

	return status;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MAT Helper Functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static int connector_version()
{
	char buf[100];
	snprintf(buf, 100, "%s", MAT_HANDLER_VERSION);
	write_data(buf);

	return 0;
}


static int write_warning(const char* warning)
{
	size_t n = 0;
	size_t dlen = strlen(warning);

	uint32_t offset = 0;

	if (include_packet_length_flag != 0)
	{
		uint32_t len = 5 + dlen + 5;
		usb_write_buf(&len, 4, &offset);
	}

	usb_write_buf("<WRN>", 5, &offset);
	usb_write_buf(warning, dlen, &offset);
	usb_write_buf("<ACK>", 5, &offset);

	n = usb_write(offset);
	if (n != offset)
		PRINTF("Failed to write warning message to client\n");

	return 0;
}


static int include_packet_length(int enable)
{
	include_packet_length_flag = enable;

  	write_ack();

	return 0;
}


static int write_data_nack(const char* data)
{
	size_t n = 0;
	size_t dlen = strlen(data);

	uint32_t offset = 0;

	usb_write_buf(data, dlen, &offset);

	n = usb_write(offset);
	if (n != offset)
		PRINTF("Failed to write data nack message to client\n");

	return 0;
}


static int write_binary_nack(const void* data, int data_len)
{
	size_t n = 0;

	uint32_t offset = 0;

	usb_write_buf(data, data_len, &offset);

	n = usb_write(offset);
	if (n != offset)
		PRINTF("Failed to write binary nack message to client\n");

	return 0;
}


static int write_ack()
{
	write_data("");

	return 0;
}


static int write_error(const char* error)
{
	size_t n = 0;
	size_t dlen = strlen(error);

	uint32_t offset = 0;

	if (include_packet_length_flag != 0)
	{
		uint32_t len = 5 + dlen + 5;
		usb_write_buf(&len, 4, &offset);
	}

	usb_write_buf("<ERR>", 5, &offset);
	usb_write_buf(error, dlen, &offset);
	usb_write_buf("<ACK>", 5, &offset);

	n = usb_write(offset);
	if (n != offset)
		PRINTF("Failed to write error message to client\n");

	return 0;
}


static int write_binary(const void* data, int data_len)
{
	size_t n = 0;

	uint32_t offset = 0;

	if (include_packet_length_flag)
	{
		uint32_t dlen = data_len + 5;
		usb_write_buf(&dlen, 4, &offset);
	}

	usb_write_buf(data, data_len, &offset);
	usb_write_buf("<ACK>", 5, &offset);

	n = usb_write(offset);
	if (n != offset)
		PRINTF("Failed to write binary message to client\n");

	return 0;
}


static int write_data(const char* data)
{
	size_t n = 0;
	size_t dlen = strlen(data);

	uint32_t offset = 0;

	if (include_packet_length_flag)
	{
		uint32_t len = dlen + 5;
		usb_write_buf(&len, 4, &offset);
	}

	if (dlen > 0)
	{
		usb_write_buf(data, dlen, &offset);
	}

	usb_write_buf("<ACK>", 5, &offset);

	n = usb_write(offset);
	if (n != offset)
		PRINTF("Failed to write data message to client\n");

	return 0;
}

//
// These GPIO functions are relic of BBB version. Not used on SLMX4
//

static int set_io_pin_dir(int bank, int pin, int direction)
{
	// Do nothing
	return 0;
}


static int write_io_pin(int bank, int pin, int val)
{
	// Do nothing
	return 0;
}


static int read_io_pin(int bank, int pin, int* val)
{
	// Do nothing
	return 0;
}


static void usb_write_buf(uint8_t *buf, uint32_t buf_len, uint32_t *offset)
{
	memcpy(usb_tx_buf + *offset, buf, buf_len);
	*offset += buf_len;
}


static int usb_write(size_t n)
{
	// Transmit the message via USB
	int error = USB_DeviceCdcAcmSend(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_IN_ENDPOINT, usb_tx_buf, n);
	if (error != kStatus_USB_Success)
	{
		// error
		PRINTF("send_response() err = %d\n", error);
		return 0;
	}

	return n;
}
