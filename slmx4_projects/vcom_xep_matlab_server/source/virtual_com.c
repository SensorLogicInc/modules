/**
This project is a XEP Matlab Server which operates using Virtual COM port over
the USB interface

Hardware required:
- SLMX4-HW
- epam-x4 (or other pin-compatible x4 module)
- USB connection between PC and SLMX4

This program works in conjunction with a corresponding Matlab XEP tools; it
uses a USB virtual COM port interface for communications.

@note
The interesting stuff is contained in `mat_handler.c`. The majority of the code
in this file is boilerplate USB code.

@author
Justin Hadella

@copyright (c) 2021 Sensor Logic
*/

/**
@file   virtual_com.c
@brief  xep matlab server
*/
#include "fsl_device_registers.h"
#include "clock_config.h"
#include "board.h"

#include <stdio.h>
#include <stdlib.h>

//#include "fsl_debug_console.h"
#include "virtual_com.h"

#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
#include "fsl_sysmpu.h"
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

#if defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0)
#include "usb_phy.h"
#endif
#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
    defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
    defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
extern uint8_t USB_EnterLowpowerMode(void);
#endif
#include "pin_mux.h"

#include "fsl_semc.h"

// Platform includes
#include "slmx4_freertos.h"

#include "project.h"
#include "mat_handler.h"

#include <cr_section_macros.h>

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

#define EXAMPLE_SEMC SEMC
#define EXAMPLE_SEMC_START_ADDRESS (0x80000000U)
#define EXAMPLE_SEMC_CLK_FREQ CLOCK_GetFreq(kCLOCK_SemcClk)

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------

void BOARD_InitHardware(void);
void USB_DeviceClockInit(void);
void USB_DeviceIsrEnable(void);

status_t BOARD_Init_SEMC(void);

#if USB_DEVICE_CONFIG_USE_TASK
void USB_DeviceTaskFn(void *deviceHandle);
#endif

void BOARD_DbgConsole_Deinit(void);
void BOARD_DbgConsole_Init(void);
usb_status_t USB_DeviceCdcVcomCallback(class_handle_t handle, uint32_t event, void *param);
usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param);

// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Global Variables
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

#define HEAP_REGION1_SIZE (  50 * 1024) /* 50 KB */
#define HEAP_REGION2_SIZE (1000 * 1024) /*  1 MB */

__NOINIT(SRAM_DTC) uint8_t HeapReagion1[HEAP_REGION1_SIZE];
__NOINIT(BOARD_SDRAM) uint8_t HeapReagion2[HEAP_REGION2_SIZE];

const HeapRegion_t xHeapRegions[] =
{
	{HeapReagion1, HEAP_REGION1_SIZE},
	{HeapReagion2, HEAP_REGION2_SIZE},
	{NULL, 0} /* Terminates the array. */
};

// Reference to the X4 device driver
X4Driver_t *x4 = NULL;

// Flag indicates X4 data ready (used by ISR)
int x4_data_ready = 0;

// Flag indicates USB if connected
static int usb_connected = 0;

extern usb_device_endpoint_struct_t g_UsbDeviceCdcVcomDicEndpoints[];
extern usb_device_class_struct_t g_UsbDeviceCdcVcomConfig;

/* Data structure of virtual com device */
usb_cdc_vcom_struct_t s_cdcVcom;
static char const *s_appName = "hep2_usb_vcom_xep_matlab_server";

/* Line coding of cdc device */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE) static uint8_t s_lineCoding[LINE_CODING_SIZE] = {
	/* E.g. 0x00,0xC2,0x01,0x00 : 0x0001C200 is 115200 bits per second */
	(LINE_CODING_DTERATE >> 0U) & 0x000000FFU,
	(LINE_CODING_DTERATE >> 8U) & 0x000000FFU,
	(LINE_CODING_DTERATE >> 16U) & 0x000000FFU,
	(LINE_CODING_DTERATE >> 24U) & 0x000000FFU,
	LINE_CODING_CHARFORMAT,
	LINE_CODING_PARITYTYPE,
	LINE_CODING_DATABITS
};

/* Abstract state of cdc device */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static uint8_t s_abstractState[COMM_FEATURE_DATA_SIZE] = {
	(STATUS_ABSTRACT_STATE >> 0U) & 0x00FFU,
	(STATUS_ABSTRACT_STATE >> 8U) & 0x00FFU
};

/* Country code of cdc device */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static uint8_t s_countryCode[COMM_FEATURE_DATA_SIZE] = {
	(COUNTRY_SETTING >> 0U) & 0x00FFU,
	(COUNTRY_SETTING >> 8U) & 0x00FFU
};

/* CDC ACM information */
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static usb_cdc_acm_info_t s_usbCdcAcmInfo;

/* Data buffer for receiving */
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static uint8_t s_currRecvBuf[DATA_BUFF_SIZE];

/* Data buffer for sending*/
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static uint8_t s_currSendBuf[DATA_BUFF_SIZE];

volatile static uint32_t s_recvSize = 0;
volatile static uint32_t s_sendSize = 0;

/* USB device class information */
static usb_device_class_config_struct_t s_cdcAcmConfig[1] = {{
	USB_DeviceCdcVcomCallback, 0, &g_UsbDeviceCdcVcomConfig,
}};

/* USB device class configuration information */
static usb_device_class_config_list_struct_t s_cdcAcmConfigList = {
	s_cdcAcmConfig, USB_DeviceCallback, 1,
};

#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
	defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
	defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
volatile static uint8_t s_waitForDataReceive = 0;
volatile static uint8_t s_comOpen = 0;
#endif

// -----------------------------------------------------------------------------
// USB Functions
// -----------------------------------------------------------------------------

void USB_OTG1_IRQHandler(void)
{
	USB_DeviceEhciIsrFunction(s_cdcVcom.deviceHandle);
}


void USB_OTG2_IRQHandler(void)
{
	USB_DeviceEhciIsrFunction(s_cdcVcom.deviceHandle);
}


void USB_DeviceClockInit(void)
{
	usb_phy_config_struct_t phyConfig = {
		BOARD_USB_PHY_D_CAL,
		BOARD_USB_PHY_TXCAL45DP,
		BOARD_USB_PHY_TXCAL45DM,
	};

	if (CONTROLLER_ID == kUSB_ControllerEhci0)
	{
		CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
		CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 480000000U);
	}
	else
	{
		CLOCK_EnableUsbhs1PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
		CLOCK_EnableUsbhs1Clock(kCLOCK_Usb480M, 480000000U);
	}
	USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL0_CLK_HZ, &phyConfig);
}


void USB_DeviceIsrEnable(void)
{
	uint8_t irqNumber;

	uint8_t usbDeviceEhciIrq[] = USBHS_IRQS;
	irqNumber = usbDeviceEhciIrq[CONTROLLER_ID - kUSB_ControllerEhci0];

	/* Install isr, set priority, and enable IRQ. */
	NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
	EnableIRQ((IRQn_Type)irqNumber);
}

#if USB_DEVICE_CONFIG_USE_TASK
void USB_DeviceTaskFn(void *deviceHandle)
{
	USB_DeviceEhciTaskFunction(deviceHandle);
}
#endif


/*!
 * @brief CDC class specific callback function.
 *
 * This function handles the CDC class specific requests.
 *
 * @param handle          The CDC ACM class handle.
 * @param event           The CDC ACM class event type.
 * @param param           The parameter of the class specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceCdcVcomCallback(class_handle_t handle, uint32_t event, void *param)
{
	usb_status_t error = kStatus_USB_Error;
	uint32_t len;
	uint8_t *uartBitmap;
	usb_cdc_acm_info_t *acmInfo = &s_usbCdcAcmInfo;
	usb_device_cdc_acm_request_param_struct_t *acmReqParam;
	usb_device_endpoint_callback_message_struct_t *epCbParam;
	acmReqParam = (usb_device_cdc_acm_request_param_struct_t *)param;
	epCbParam = (usb_device_endpoint_callback_message_struct_t *)param;
	switch (event)
	{
		case kUSB_DeviceCdcEventSendResponse:
			{
				if ((epCbParam->length != 0) && (!(epCbParam->length % g_UsbDeviceCdcVcomDicEndpoints[0].maxPacketSize)))
				{
					/* If the last packet is the size of endpoint, then send also zero-ended packet,
					** meaning that we want to inform the host that we do not have any additional
					** data, so it can flush the output.
					*/
					error = USB_DeviceCdcAcmSend(handle, USB_CDC_VCOM_BULK_IN_ENDPOINT, NULL, 0);
				}
				else if ((1 == s_cdcVcom.attach) && (1 == s_cdcVcom.startTransactions))
				{
					if ((epCbParam->buffer != NULL) || ((epCbParam->buffer == NULL) && (epCbParam->length == 0)))
					{
						/* User: add your own code for send complete event */
						/* Schedule buffer for next receive event */
						error = USB_DeviceCdcAcmRecv(handle, USB_CDC_VCOM_BULK_OUT_ENDPOINT, s_currRecvBuf, g_UsbDeviceCdcVcomDicEndpoints[0].maxPacketSize);

#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
	defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
	defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
						s_waitForDataReceive = 1;
						USB0->INTEN &= ~USB_INTEN_SOFTOKEN_MASK;
#endif
					}
				}
			}
			break;

		case kUSB_DeviceCdcEventRecvResponse:
			{
				if ((1 == s_cdcVcom.attach) && (1 == s_cdcVcom.startTransactions))
				{
					s_recvSize = epCbParam->length;

#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
	defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
	defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
					s_waitForDataReceive = 0;
					USB0->INTEN |= USB_INTEN_SOFTOKEN_MASK;
#endif
					if (!s_recvSize)
					{
						/* Schedule buffer for next receive event */
						error = USB_DeviceCdcAcmRecv(handle, USB_CDC_VCOM_BULK_OUT_ENDPOINT, s_currRecvBuf, g_UsbDeviceCdcVcomDicEndpoints[0].maxPacketSize);
#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
	defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
	defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
						s_waitForDataReceive = 1;
						USB0->INTEN &= ~USB_INTEN_SOFTOKEN_MASK;
#endif
					}
				}
			}
			break;

		case kUSB_DeviceCdcEventSerialStateNotif:
			((usb_device_cdc_acm_struct_t *)handle)->hasSentState = 0;
			error = kStatus_USB_Success;
			break;

		case kUSB_DeviceCdcEventSendEncapsulatedCommand:
			break;

		case kUSB_DeviceCdcEventGetEncapsulatedResponse:
			break;

		case kUSB_DeviceCdcEventSetCommFeature:
			if (USB_DEVICE_CDC_FEATURE_ABSTRACT_STATE == acmReqParam->setupValue)
			{
				if (1 == acmReqParam->isSetup)
				{
					*(acmReqParam->buffer) = s_abstractState;
				}
				else
				{
					*(acmReqParam->length) = 0;
				}
			}
			else if (USB_DEVICE_CDC_FEATURE_COUNTRY_SETTING == acmReqParam->setupValue)
			{
				if (1 == acmReqParam->isSetup)
				{
					*(acmReqParam->buffer) = s_countryCode;
				}
				else
				{
					*(acmReqParam->length) = 0;
				}
			}
			error = kStatus_USB_Success;
			break;

		case kUSB_DeviceCdcEventGetCommFeature:
			if (USB_DEVICE_CDC_FEATURE_ABSTRACT_STATE == acmReqParam->setupValue)
			{
				*(acmReqParam->buffer) = s_abstractState;
				*(acmReqParam->length) = COMM_FEATURE_DATA_SIZE;
			}
			else if (USB_DEVICE_CDC_FEATURE_COUNTRY_SETTING == acmReqParam->setupValue)
			{
				*(acmReqParam->buffer) = s_countryCode;
				*(acmReqParam->length) = COMM_FEATURE_DATA_SIZE;
			}
			error = kStatus_USB_Success;
			break;

		case kUSB_DeviceCdcEventClearCommFeature:
			break;

		case kUSB_DeviceCdcEventGetLineCoding:
			*(acmReqParam->buffer) = s_lineCoding;
			*(acmReqParam->length) = LINE_CODING_SIZE;
			error = kStatus_USB_Success;
			break;

		case kUSB_DeviceCdcEventSetLineCoding:
			{
				if (1 == acmReqParam->isSetup)
				{
					*(acmReqParam->buffer) = s_lineCoding;
				}
				else
				{
					*(acmReqParam->length) = 0;
				}
			}
			error = kStatus_USB_Success;
			break;

		case kUSB_DeviceCdcEventSetControlLineState:
			{
				s_usbCdcAcmInfo.dteStatus = acmReqParam->setupValue;
				/* activate/deactivate Tx carrier */
				if (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_CARRIER_ACTIVATION)
				{
					acmInfo->uartState |= USB_DEVICE_CDC_UART_STATE_TX_CARRIER;
				}
				else
				{
					acmInfo->uartState &= (uint16_t)~USB_DEVICE_CDC_UART_STATE_TX_CARRIER;
				}

				/* activate carrier and DTE. Com port of terminal tool running on PC is open now */
				if (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_DTE_PRESENCE)
				{
					acmInfo->uartState |= USB_DEVICE_CDC_UART_STATE_RX_CARRIER;
				}
				/* Com port of terminal tool running on PC is closed now */
				else
				{
					acmInfo->uartState &= (uint16_t)~USB_DEVICE_CDC_UART_STATE_RX_CARRIER;
				}

				/* Indicates to DCE if DTE is present or not */
				acmInfo->dtePresent = (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_DTE_PRESENCE) ? true : false;

				/* Initialize the serial state buffer */
				acmInfo->serialStateBuf[0] = NOTIF_REQUEST_TYPE;                /* bmRequestType */
				acmInfo->serialStateBuf[1] = USB_DEVICE_CDC_NOTIF_SERIAL_STATE; /* bNotification */
				acmInfo->serialStateBuf[2] = 0x00;                              /* wValue */
				acmInfo->serialStateBuf[3] = 0x00;
				acmInfo->serialStateBuf[4] = 0x00; /* wIndex */
				acmInfo->serialStateBuf[5] = 0x00;
				acmInfo->serialStateBuf[6] = UART_BITMAP_SIZE; /* wLength */
				acmInfo->serialStateBuf[7] = 0x00;
				/* Notify to host the line state */
				acmInfo->serialStateBuf[4] = acmReqParam->interfaceIndex;
				/* Lower byte of UART BITMAP */
				uartBitmap = (uint8_t *)&acmInfo->serialStateBuf[NOTIF_PACKET_SIZE + UART_BITMAP_SIZE - 2];
				uartBitmap[0] = acmInfo->uartState & 0xFFu;
				uartBitmap[1] = (acmInfo->uartState >> 8) & 0xFFu;
				len = (uint32_t)(NOTIF_PACKET_SIZE + UART_BITMAP_SIZE);
				if (0 == ((usb_device_cdc_acm_struct_t *)handle)->hasSentState)
				{
					error = USB_DeviceCdcAcmSend(handle, USB_CDC_VCOM_INTERRUPT_IN_ENDPOINT, acmInfo->serialStateBuf, len);
					if (kStatus_USB_Success != error)
					{
						usb_echo("kUSB_DeviceCdcEventSetControlLineState error!");
					}
					((usb_device_cdc_acm_struct_t *)handle)->hasSentState = 1;
				}

				/* Update status */
				if (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_CARRIER_ACTIVATION)
				{
					/*  To do: CARRIER_ACTIVATED */
				}
				else
				{
					/* To do: CARRIER_DEACTIVATED */
				}
				if (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_DTE_PRESENCE)
				{
					/* DTE_ACTIVATED */
					if (1 == s_cdcVcom.attach)
					{
						s_cdcVcom.startTransactions = 1;
#if defined(FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED) && (FSL_FEATURE_USB_KHCI_KEEP_ALIVE_ENABLED > 0U) && \
	defined(USB_DEVICE_CONFIG_KEEP_ALIVE_MODE) && (USB_DEVICE_CONFIG_KEEP_ALIVE_MODE > 0U) &&             \
	defined(FSL_FEATURE_USB_KHCI_USB_RAM) && (FSL_FEATURE_USB_KHCI_USB_RAM > 0U)
						s_waitForDataReceive = 1;
						USB0->INTEN &= ~USB_INTEN_SOFTOKEN_MASK;
						s_comOpen = 1;
						usb_echo("USB_APP_CDC_DTE_ACTIVATED\r\n");
#endif
					}
				}
				else
				{
					/* DTE_DEACTIVATED */
					if (1 == s_cdcVcom.attach)
					{
						s_cdcVcom.startTransactions = 0;
					}
				}
			}
			break;

		case kUSB_DeviceCdcEventSendBreak:
			break;

		default:
			break;
	}

	return error;
}


/*!
 * @brief USB device callback function.
 *
 * This function handles the usb device specific requests.
 *
 * @param handle          The USB device handle.
 * @param event           The USB device event type.
 * @param param           The parameter of the device specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
	usb_status_t error = kStatus_USB_Error;
	uint16_t *temp16 = (uint16_t *)param;
	uint8_t *temp8 = (uint8_t *)param;

	switch (event)
	{
		case kUSB_DeviceEventBusReset:
			{
				s_cdcVcom.attach = 0;
				s_cdcVcom.currentConfiguration = 0U;
#if (defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U)) || \
	(defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
				/* Get USB speed to configure the device, including max packet size and interval of the endpoints. */
				if (kStatus_USB_Success == USB_DeviceClassGetSpeed(CONTROLLER_ID, &s_cdcVcom.speed))
				{
					USB_DeviceSetSpeed(handle, s_cdcVcom.speed);
				}
#endif
			}
			break;

		case kUSB_DeviceEventSetConfiguration:
			if (0U ==(*temp8))
			{
				s_cdcVcom.attach = 0;
				s_cdcVcom.currentConfiguration = 0U;
			}
			else if (USB_CDC_VCOM_CONFIGURE_INDEX == (*temp8))
			{
				s_cdcVcom.attach = 1;
				s_cdcVcom.currentConfiguration = *temp8;
				/* Schedule buffer for receive */
				USB_DeviceCdcAcmRecv(s_cdcVcom.cdcAcmHandle, USB_CDC_VCOM_BULK_OUT_ENDPOINT, s_currRecvBuf, g_UsbDeviceCdcVcomDicEndpoints[0].maxPacketSize);
			}
			else
			{
				error = kStatus_USB_InvalidRequest;
			}
			break;

		case kUSB_DeviceEventSetInterface:
			if (s_cdcVcom.attach)
			{
				uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
				uint8_t alternateSetting = (uint8_t)(*temp16 & 0x00FFU);
				if (interface < USB_CDC_VCOM_INTERFACE_COUNT)
				{
					s_cdcVcom.currentInterfaceAlternateSetting[interface] = alternateSetting;
				}
			}
			break;

		case kUSB_DeviceEventGetConfiguration:
			break;

		case kUSB_DeviceEventGetInterface:
			break;

		case kUSB_DeviceEventGetDeviceDescriptor:
			if (param)
			{
				error = USB_DeviceGetDeviceDescriptor(handle, (usb_device_get_device_descriptor_struct_t *)param);
			}
			break;

		case kUSB_DeviceEventGetConfigurationDescriptor:
			if (param)
			{
				error = USB_DeviceGetConfigurationDescriptor(handle, (usb_device_get_configuration_descriptor_struct_t *)param);
			}
			break;

		case kUSB_DeviceEventGetStringDescriptor:
			if (param)
			{
				/* Get device string descriptor request */
				error = USB_DeviceGetStringDescriptor(handle, (usb_device_get_string_descriptor_struct_t *)param);
			}
			break;

		default:
			break;
	}

	return error;
}


/*!
 * @brief Application initialization function.
 *
 * This function initializes the application.
 *
 * @return None.
 */
void USB_DeviceApplicationInit(void)
{
	USB_DeviceClockInit();
#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
	SYSMPU_Enable(SYSMPU, 0);
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

	s_cdcVcom.speed = USB_SPEED_FULL;
	s_cdcVcom.attach = 0;
	s_cdcVcom.cdcAcmHandle = (class_handle_t)NULL;
	s_cdcVcom.deviceHandle = NULL;

	if (kStatus_USB_Success != USB_DeviceClassInit(CONTROLLER_ID, &s_cdcAcmConfigList, &s_cdcVcom.deviceHandle))
	{
		usb_echo("USB device init failed\r\n");
	}
	else
	{
		usb_echo("USB device init success\r\n");
		s_cdcVcom.cdcAcmHandle = s_cdcAcmConfigList.config->classHandle;
	}

	USB_DeviceIsrEnable();
	USB_DeviceRun(s_cdcVcom.deviceHandle);
}


/*!
 * @brief Application task function.
 *
 * This function runs the task for application.
 *
 * @return None.
 */
void USB_VCOM_Handler_Task(void *handle)
{
	usb_status_t error = kStatus_USB_Error;

	PRINTF("\r\n************************************************\r\n");
	PRINTF("SLMX4 -- VCOM XEP MATLAB SERVER\r\n");
	PRINTF("************************************************\r\n");

	USB_DeviceApplicationInit();

	handle_client_init();

	while (1)
	{
		if ((1 == s_cdcVcom.attach) && (1 == s_cdcVcom.startTransactions))
		{
			usb_connected = 1;

			if ((0 != s_recvSize) && (0xFFFFFFFF != s_recvSize))
			{
				handle_client_request(s_currRecvBuf, s_recvSize);

				// Reset rx count
				s_recvSize = 0;
			}
		}
		else
		{
			usb_connected = 0;
		}
	}
}


// =============================================================================
// Main Program
// =============================================================================

#if defined(__CC_ARM) || (defined(__ARMCC_VERSION)) || defined(__GNUC__)
int main(void)
#else
void main(void)
#endif
{
	// Do this first!
	vPortDefineHeapRegions(xHeapRegions);

	int status = platform__init();
	if (status)
	{
		PRINTF("platform__init() err = %d\n", status);
		return 1;
	}

	// File system doesn't work without this line!
	NVIC_SetPriority(BOARD_SD_HOST_IRQ, 5U);

	// Make sure to init SEMC! (DCD has already initialized SDRAM)
	status = BOARD_Init_SEMC();
	if (status)
	{
		PRINTF("BOARD_Init_SEMC() = %d\n", status);
	}

	// The SDRAM on peak board uses an address column bit width of 8 which is not
	// possible choice in current SEMC! We need to modify this register to make it
	// work correctly.
	SEMC->SDRAMCR0 |= SEMC_SDRAMCR0_COL8_MASK;

	// Init RGB LED to a violet color
	platform__set_rgb_led(RGB_COLOR_VIOLET);

	if (xTaskCreate(USB_VCOM_Handler_Task,           /* pointer to the task                      */
					s_appName,                       /* task name for kernel awareness debugging */
					5000L / sizeof(portSTACK_TYPE),  /* task stack size                          */
					&s_cdcVcom,                      /* optional task startup argument           */
					4,                               /* initial priority                         */
					&s_cdcVcom.applicationTaskHandle /* optional task handle to create           */
					) != pdPASS)
	{
		usb_echo("app task create failed!\r\n");
#if (defined(__CC_ARM) || (defined(__ARMCC_VERSION)) || defined(__GNUC__))
		return 1;
#else
		return;
#endif
	}

	vTaskStartScheduler();

#if (defined(__CC_ARM) || (defined(__ARMCC_VERSION)) || defined(__GNUC__))
	return 1;
#endif
}

// ~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~
// ISR
// ~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~-~~

void GPIO2_Combined_0_15_IRQHandler(void)
{
	// Clear the interrupt
	GPIO_PortClearInterruptFlags(BOARD_INIT_X4_GPIO1_GPIO, 1U << BOARD_INIT_X4_GPIO1_PIN);

	x4_data_ready = 1;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SDRAM
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

status_t BOARD_Init_SEMC(void)
{
	semc_config_t config;
	semc_sdram_config_t sdramconfig;
	uint32_t clockFrq = EXAMPLE_SEMC_CLK_FREQ;

	// Initializes the MAC configure structure to zero
	memset(&config, 0, sizeof(semc_config_t));
	memset(&sdramconfig, 0, sizeof(semc_sdram_config_t));

	// Initialize SEMC
	SEMC_GetDefaultConfig(&config);
	config.dqsMode = kSEMC_Loopbackdqspad; // For more accurate timing
	SEMC_Init(SEMC, &config);

	// Configure SDRAM
	sdramconfig.csxPinMux           = kSEMC_MUXCSX0;
	sdramconfig.address             = 0x80000000;
	sdramconfig.memsize_kbytes      = 8 * 1024; /* 8 MB = 8*1024*1KBytes*/
	sdramconfig.portSize            = kSEMC_PortSize16Bit;
	sdramconfig.burstLen            = kSEMC_Sdram_BurstLen8;
	sdramconfig.columnAddrBitNum    = kSEMC_SdramColunm_9bit;
	sdramconfig.casLatency          = kSEMC_LatencyThree;
	sdramconfig.tPrecharge2Act_Ns   = 18; /* Trp 18ns */
	sdramconfig.tAct2ReadWrite_Ns   = 18; /* Trcd 18ns */
	sdramconfig.tRefreshRecovery_Ns = 67; /* Use the maximum of the (Trfc , Txsr). */
	sdramconfig.tWriteRecovery_Ns   = 12; /* 12ns */
	sdramconfig.tCkeOff_Ns          = 42; /* The minimum cycle of SDRAM CLK off state. CKE is off in self refresh at a minimum period tRAS.*/
	sdramconfig.tAct2Prechage_Ns       = 42; /* Tras 42ns */
	sdramconfig.tSelfRefRecovery_Ns    = 67;
	sdramconfig.tRefresh2Refresh_Ns    = 60;
	sdramconfig.tAct2Act_Ns            = 60;
	sdramconfig.tPrescalePeriod_Ns     = 160 * (1000000000 / clockFrq);
	sdramconfig.refreshPeriod_nsPerRow = 64 * 1000000 / 4096; /* 64ms/4096 */
	sdramconfig.refreshUrgThreshold    = sdramconfig.refreshPeriod_nsPerRow;
	sdramconfig.refreshBurstLen        = 1;

	return SEMC_ConfigureSDRAM(SEMC, kSEMC_SDRAM_CS0, &sdramconfig, clockFrq);
}
