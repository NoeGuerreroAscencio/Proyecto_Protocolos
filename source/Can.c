/*
 * Can.c
 *
 *  Created on: 18 may 2023
 *      Author: noeas
 */

#include "Can_cfg.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define EXAMPLE_CAN            CAN0
#define EXAMPLE_CAN_CLK_SOURCE (kFLEXCAN_ClkSrc1)
#define EXAMPLE_CAN_CLK_FREQ   CLOCK_GetFreq(kCLOCK_BusClk)
/* Set USE_IMPROVED_TIMING_CONFIG macro to use api to calculates the improved CAN / CAN FD timing values. */
#define USE_IMPROVED_TIMING_CONFIG (1U)
#define EXAMPLE_FLEXCAN_IRQn       CAN0_ORed_Message_buffer_IRQn
#define EXAMPLE_FLEXCAN_IRQHandler CAN0_ORed_Message_buffer_IRQHandler
#define RX_MESSAGE_BUFFER_NUM      (9)
#define TX_MESSAGE_BUFFER_NUM      (8)
#define DLC                        (8)
#define CAN_CS_CODE(x)                           (((uint32_t)(((uint32_t)(x)) << CAN_CS_CODE_SHIFT)) & CAN_CS_CODE_MASK)
flexcan_frame_t txFrame, rxFrame;
static const clock_ip_name_t s_flexcanClock[] = FLEXCAN_CLOCKS;
#ifndef CAN_CLOCK_CHECK_NO_AFFECTS
/* If no define such MACRO, it mean that the CAN in current device have no clock affect issue. */
#define CAN_CLOCK_CHECK_NO_AFFECTS (true)
volatile bool rxComplete = false;
#endif
void Can_Init(const Can_ConfigType *flexcanConfig) {
	CAN_Type *base = EXAMPLE_CAN;
	uint32_t sourceClock_Hz = EXAMPLE_CAN_CLK_FREQ;
	/* Assertion. */
	assert(NULL != flexcanConfig);
	assert(
			(flexcanConfig->maxMbNum > 0U) && (flexcanConfig->maxMbNum <= (uint8_t)FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base)));
	assert(flexcanConfig->bitRate > 0U);

	uint32_t mcrTemp;
	uint32_t ctrl1Temp;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
	uint32_t instance;
#endif

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
	instance = CAN_GetInstance(base);
	/* Enable FlexCAN clock. */
	(void) CLOCK_EnableClock(s_flexcanClock[instance]);
	/*
	 * Check the CAN clock in this device whether affected by Other clock gate
	 * If it affected, we'd better to change other clock source,
	 * If user insist on using that clock source, user need open these gate at same time,
	 * In this scene, User need to care the power consumption.
	 */
	assert(CAN_CLOCK_CHECK_NO_AFFECTS);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if defined(CAN_CTRL1_CLKSRC_MASK)
	{
		/* Disable FlexCAN Module. */
		FLEXCAN_Enable(base, false);

		/* Protocol-Engine clock source selection, This bit must be set
		 * when FlexCAN Module in Disable Mode.
		 */
		base->CTRL1 =
				(kFLEXCAN_ClkSrc0 == flexcanConfig->clkSrc) ?
						(base->CTRL1 & ~CAN_CTRL1_CLKSRC_MASK) :
						(base->CTRL1 | CAN_CTRL1_CLKSRC_MASK);
	}
#endif /* CAN_CTRL1_CLKSRC_MASK */

	/* Enable FlexCAN Module for configuration. */
	FLEXCAN_Enable(base, true);

	/* Reset to known status. */
	FLEXCAN_Reset(base);

	/* Save current CTRL1 value and enable to enter Freeze mode(enabled by default). */
	ctrl1Temp = base->CTRL1;

	/* Save current MCR value and enable to enter Freeze mode(enabled by default). */
	mcrTemp = base->MCR;

	/* Enable Loop Back Mode? */
	ctrl1Temp =
			(flexcanConfig->enableLoopBack) ?
					(ctrl1Temp | CAN_CTRL1_LPB_MASK) :
					(ctrl1Temp & ~CAN_CTRL1_LPB_MASK);

	/* Enable Timer Sync? */
	ctrl1Temp =
			(flexcanConfig->enableTimerSync) ?
					(ctrl1Temp | CAN_CTRL1_TSYN_MASK) :
					(ctrl1Temp & ~CAN_CTRL1_TSYN_MASK);

	/* Enable Listen Only Mode? */
	ctrl1Temp =
			(flexcanConfig->enableListenOnlyMode) ?
					ctrl1Temp | CAN_CTRL1_LOM_MASK :
					ctrl1Temp & ~CAN_CTRL1_LOM_MASK;

#if !(defined(FSL_FEATURE_FLEXCAN_HAS_NO_SUPV_SUPPORT) && FSL_FEATURE_FLEXCAN_HAS_NO_SUPV_SUPPORT)
	/* Enable Supervisor Mode? */
	mcrTemp =
			(flexcanConfig->enableSupervisorMode) ?
					mcrTemp | CAN_MCR_SUPV_MASK : mcrTemp & ~CAN_MCR_SUPV_MASK;
#endif

	/* Set the maximum number of Message Buffers */
	mcrTemp = (mcrTemp & ~CAN_MCR_MAXMB_MASK)
			| CAN_MCR_MAXMB((uint32_t )flexcanConfig->maxMbNum - 1U);

	/* Enable Self Wake Up Mode and configure the wake up source. */
	mcrTemp =
			(flexcanConfig->enableSelfWakeup) ?
					(mcrTemp | CAN_MCR_SLFWAK_MASK) :
					(mcrTemp & ~CAN_MCR_SLFWAK_MASK);
	mcrTemp =
			(kFLEXCAN_WakeupSrcFiltered == flexcanConfig->wakeupSrc) ?
					(mcrTemp | CAN_MCR_WAKSRC_MASK) :
					(mcrTemp & ~CAN_MCR_WAKSRC_MASK);

	/* Enable Individual Rx Masking and Queue feature? */
	mcrTemp =
			(flexcanConfig->enableIndividMask) ?
					(mcrTemp | CAN_MCR_IRMQ_MASK) :
					(mcrTemp & ~CAN_MCR_IRMQ_MASK);

	/* Disable Self Reception? */
	mcrTemp =
			(flexcanConfig->disableSelfReception) ?
					mcrTemp | CAN_MCR_SRXDIS_MASK :
					mcrTemp & ~CAN_MCR_SRXDIS_MASK;

	/* Write back CTRL1 Configuration to register. */
	base->CTRL1 = ctrl1Temp;

	/* Write back MCR Configuration to register. */
	base->MCR = mcrTemp;

	/* Bit Rate Configuration.*/
	FLEXCAN_SetBitRate(base, sourceClock_Hz, flexcanConfig->bitRate,
			flexcanConfig->timingConfig);
}

Std_ReturnType Can_Write(Can_HwHandleType Hth, const Can_PduType *PduInfo) {
	//Se prepara para enviar en mensaje
	CAN_Type *base = EXAMPLE_CAN;
	uint8_t mbIdx = TX_MESSAGE_BUFFER_NUM;
	Std_ReturnType status;
	/* Assertion. */
	assert(mbIdx <= (base->MCR & CAN_MCR_MAXMB_MASK));
#if !defined(NDEBUG)
	assert(!FLEXCAN_IsMbOccupied(base, mbIdx));
#endif

	/* Inactivate Message Buffer. */
	base->MB[mbIdx].CS = CAN_CS_CODE(kFLEXCAN_TxMbInactive);

	/* Clean Message Buffer content. */
	base->MB[mbIdx].ID = 0x0;
	base->MB[mbIdx].WORD0 = 0x0;
	base->MB[mbIdx].WORD1 = 0x0;
	/* Prepare Tx Frame for sending. */
	if (CAN_CS_CODE(kFLEXCAN_TxMbDataOrRemote)
			!= (base->MB[Hth].CS & CAN_CS_CODE_MASK)) {
		/* Prepare Tx Frame for sending. */
		txFrame.format = (uint8_t) kFLEXCAN_FrameFormatStandard;
		txFrame.type = (uint8_t) kFLEXCAN_FrameTypeData;
		txFrame.id = FLEXCAN_ID_STD(0x123);
		txFrame.length = (uint8_t) DLC;

		txFrame.dataWord0 =
				CAN_WORD0_DATA_BYTE_0(
						0x11) | CAN_WORD0_DATA_BYTE_1(0x22) | CAN_WORD0_DATA_BYTE_2(0x33) |
						CAN_WORD0_DATA_BYTE_3(0x44);
		txFrame.dataWord1 =
				CAN_WORD1_DATA_BYTE_4(
						0x55) | CAN_WORD1_DATA_BYTE_5(0x66) | CAN_WORD1_DATA_BYTE_6(0x77) |
						CAN_WORD1_DATA_BYTE_7(0x88);

		status = FLEXCAN_TransferSendBlocking(EXAMPLE_CAN,
				TX_MESSAGE_BUFFER_NUM, &txFrame);

	} else {
		/* Tx Message Buffer is activated, return immediately. */
		status = kStatus_Fail;
	}

	return status;
}

void Can_MainFunctionRead() {
	uint32_t flag = 1U;
	/* Waiting for Message receive finish. */
	while (!rxComplete) {
	}
	/* Stop FlexCAN Send & Receive. */
	DisableMbInterrupts(EXAMPLE_CAN, flag << RX_MESSAGE_BUFFER_NUM);
}
