/*
 * Can_cfg.c
 *
 *  Created on: 18 may 2023
 *      Author: noeas
 */


#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)

/* Array of FlexCAN clock name. */

//static const clock_ip_name_t s_flexcanClock[] = FLEXCAN_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
static inline void DisableMbInterrupts(CAN_Type *base, uint32_t mask);
/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Array of FlexCAN peripheral base address. */
static CAN_Type *const s_flexcanBases[] = CAN_BASE_PTRS;


static inline uint32_t GetMbStatusFlags(CAN_Type *base, uint32_t mask);


/*!
 * brief Get the FlexCAN instance from peripheral base address.
 *
 * param base FlexCAN peripheral base address.
 * return FlexCAN instance.
 */
uint32_t CAN_GetInstanceAutosar(CAN_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_flexcanBases); instance++)
    {
        if (s_flexcanBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_flexcanBases));

    return instance;
}

/*!
 * brief Set bit rate of FlexCAN classical CAN frame or CAN FD frame nominal phase.
 *
 * This function set the bit rate of classical CAN frame or CAN FD frame nominal phase base on the value of the
 * parameter passed in. Users need to ensure that the timing segment values (phaseSeg1, phaseSeg2 and propSeg) match the
 * clock and bit rate, if not match, the final output bit rate may not equal the bitRate_Bps value. Suggest use
 * FLEXCAN_CalculateImprovedTimingValues() to get timing configuration.
 *
 * param base FlexCAN peripheral base address.
 * param sourceClock_Hz Source Clock in Hz.
 * param bitRate_Bps Bit rate in Bps.
 * param timingConfig FlexCAN timingConfig.
 */
static void FLEXCAN_SetBitRate(CAN_Type *base,
                               uint32_t sourceClock_Hz,
                               uint32_t bitRate_Bps,
                               flexcan_timing_config_t timingConfig)
{
    /* FlexCAN classical CAN frame or CAN FD frame nominal phase timing setting formula:
     * quantum = 1 + (phaseSeg1 + 1) + (phaseSeg2 + 1) + (propSeg + 1);
     */
    uint32_t quantum = (1U + ((uint32_t)timingConfig.phaseSeg1 + 1U) + ((uint32_t)timingConfig.phaseSeg2 + 1U) +
                        ((uint32_t)timingConfig.propSeg + 1U));

    /* Assertion: Desired bit rate is too high. */
    assert(bitRate_Bps <= 1000000U);
    /* Assertion: Source clock should greater than or equal to bit rate * quantum. */
    assert((bitRate_Bps * quantum) <= sourceClock_Hz);
    /* Assertion: Desired bit rate is too low, the bit rate * quantum * max prescaler divider value should greater than
       or equal to source clock. */

    assert((bitRate_Bps * quantum * MAX_PRESDIV) >= sourceClock_Hz);
    if (quantum < (MIN_TIME_SEGMENT1 + MIN_TIME_SEGMENT2 + 1U))
    {
        /* No valid timing configuration. */
        timingConfig.preDivider = 0U;
    }
    else
    {
        timingConfig.preDivider = (uint16_t)((sourceClock_Hz / (bitRate_Bps * quantum)) - 1U);
    }

    /* Update actual timing characteristic. */
    FLEXCAN_SetTimingConfig(base, (const flexcan_timing_config_t *)(uint32_t)&timingConfig);
}

/*!
 * @name Bus Operations
 * @{
 */

/*!
 * @brief Enables or disables the FlexCAN module operation.
 *
 * This function enables or disables the FlexCAN module.
 *
 * @param base FlexCAN base pointer.
 * @param enable true to enable, false to disable.
 */
static inline void FLEXCAN_Enable(CAN_Type *base, bool enable)
{
    if (enable)
    {
        base->MCR &= ~CAN_MCR_MDIS_MASK;

        /* Wait FlexCAN exit from low-power mode. */
        while (0U != (base->MCR & CAN_MCR_LPMACK_MASK))
        {
        }
    }
    else
    {
        base->MCR |= CAN_MCR_MDIS_MASK;

        /* Wait FlexCAN enter low-power mode. */
        while (0U == (base->MCR & CAN_MCR_LPMACK_MASK))
        {
        }
    }
}

/*!
 * brief Reset the FlexCAN Instance.
 *
 * Restores the FlexCAN module to reset state, notice that this function
 * will set all the registers to reset state so the FlexCAN module can not work
 * after calling this API.
 *
 * param base FlexCAN peripheral base address.
 */
static void FLEXCAN_Reset(CAN_Type *base)
{
    /* The module must should be first exit from low power
     * mode, and then soft reset can be applied.
     */
    assert(0U == (base->MCR & CAN_MCR_MDIS_MASK));

    uint8_t i;

    /* Wait until FlexCAN exit from any Low Power Mode. */
    while (0U != (base->MCR & CAN_MCR_LPMACK_MASK))
    {
    }

    /* Assert Soft Reset Signal. */
    base->MCR |= CAN_MCR_SOFTRST_MASK;
    /* Wait until FlexCAN reset completes. */
    while (0U != (base->MCR & CAN_MCR_SOFTRST_MASK))
    {
    }

/* Reset MCR register. */
#if (defined(FSL_FEATURE_FLEXCAN_HAS_GLITCH_FILTER) && FSL_FEATURE_FLEXCAN_HAS_GLITCH_FILTER)
    base->MCR |= CAN_MCR_WRNEN_MASK | CAN_MCR_WAKSRC_MASK |
                 CAN_MCR_MAXMB((uint32_t)FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base) - 1U);

    /* Reset CTRL1 and CTRL2 register, default to eanble SMP feature which enable three sample point to determine the
     * received bit's value of the. */
    base->CTRL1 = CAN_CTRL1_SMP_MASK;
    base->CTRL2 = CAN_CTRL2_TASD(0x16) | CAN_CTRL2_RRS_MASK | CAN_CTRL2_EACEN_MASK;


    /* Only need clean all Message Buffer memory. */
    (void)memset((void *)&base->MB[0], 0, sizeof(base->MB));

    /* Clean all individual Rx Mask of Message Buffers. */
    for (i = 0; i < (uint32_t)FSL_FEATURE_FLEXCAN_HAS_MESSAGE_BUFFER_MAX_NUMBERn(base); i++)
    {
        base->RXIMR[i] = 0x3FFFFFFF;
    }

    /* Clean Global Mask of Message Buffers. */
    base->RXMGMASK = 0x3FFFFFFF;
    /* Clean Global Mask of Message Buffer 14. */
    base->RX14MASK = 0x3FFFFFFF;
    /* Clean Global Mask of Message Buffer 15. */
    base->RX15MASK = 0x3FFFFFFF;
    /* Clean Global Mask of Rx FIFO. */
    base->RXFGMASK = 0x3FFFFFFF;
#endif
}

void _FLEXCAN_SetTimingConfig(CAN_Type *base, const flexcan_timing_config_t *pConfig)
	{
	    /* Assertion. */
	    assert(NULL != pConfig);

	    /* Enter Freeze Mode. */
	    FLEXCAN_EnterFreezeMode(base);


	    /* Cleaning previous Timing Setting. */
	    base->CTRL1 &= ~(CAN_CTRL1_PRESDIV_MASK | CAN_CTRL1_RJW_MASK | CAN_CTRL1_PSEG1_MASK | CAN_CTRL1_PSEG2_MASK |
	                     CAN_CTRL1_PROPSEG_MASK);

	    /* Updating Timing Setting according to configuration structure. */
	    base->CTRL1 |= (CAN_CTRL1_PRESDIV(pConfig->preDivider) | CAN_CTRL1_RJW(pConfig->rJumpwidth) |
	                    CAN_CTRL1_PSEG1(pConfig->phaseSeg1) | CAN_CTRL1_PSEG2(pConfig->phaseSeg2) |
	                    CAN_CTRL1_PROPSEG(pConfig->propSeg));
	    /* Exit Freeze Mode. */
	    FLEXCAN_ExitFreezeMode(base);

}

static bool FLEXCAN_IsMbOccupied(CAN_Type *base, uint8_t mbIdx)
{
    uint8_t lastOccupiedMb;
    bool fgRet;

    /* Is Rx FIFO enabled? */
    if (0U != (base->MCR & CAN_MCR_RFEN_MASK))
    {
        /* Get RFFN value. */
        lastOccupiedMb = (uint8_t)((base->CTRL2 & CAN_CTRL2_RFFN_MASK) >> CAN_CTRL2_RFFN_SHIFT);
        /* Calculate the number of last Message Buffer occupied by Rx FIFO. */
        lastOccupiedMb = ((lastOccupiedMb + 1U) * 2U) + 5U;

        fgRet = (mbIdx <= lastOccupiedMb);
    }
    else
    {
        {
            fgRet = false;
        }
    }

    return fgRet;
}
void FLEXCAN_EnterFreezeMode(CAN_Type *base)
{
    /* Set Freeze, Halt bits. */
    base->MCR |= CAN_MCR_FRZ_MASK;
    base->MCR |= CAN_MCR_HALT_MASK;
    while (0U == (base->MCR & CAN_MCR_FRZACK_MASK))
    {
    }
}


/*!
 * brief Exit FlexCAN Freeze Mode.
 *
 * This function makes the FlexCAN leave Freeze Mode.
 *
 * param base FlexCAN peripheral base address.
 */
void FLEXCAN_ExitFreezeMode(CAN_Type *base)
{

    /* Clear Freeze, Halt bits. */
    base->MCR &= ~CAN_MCR_HALT_MASK;
    base->MCR &= ~CAN_MCR_FRZ_MASK;

    /* Wait until the FlexCAN Module exit freeze mode. */
    while (0U != (base->MCR & CAN_MCR_FRZACK_MASK))
    {
    }
}

status_t FLEXCAN_TransferSendBlocking(CAN_Type *base, uint8_t mbIdx, flexcan_frame_t *pTxFrame)
{
    status_t status;

    /* Write Tx Message Buffer to initiate a data sending. */
    if (kStatus_Success == FLEXCAN_WriteTxMb(base, mbIdx, (const flexcan_frame_t *)(uint32_t)pTxFrame))
    {
/* Wait until CAN Message send out. */

        uint32_t u32flag = 1;
        while (0U == GetMbStatusFlags(base, u32flag << mbIdx))

        {
        }

/* Clean Tx Message Buffer Flag. */

        ClearMbStatusFlags(base, u32flag << mbIdx);
        /*After TX MB tranfered success, update the Timestamp from MB[mbIdx].CS register*/
        pTxFrame->timestamp = (uint16_t)((base->MB[mbIdx].CS & CAN_CS_TIME_STAMP_MASK) >> CAN_CS_TIME_STAMP_SHIFT);

        status = kStatus_Success;
    }
    else
    {
        status = kStatus_Fail;
    }

    return status;
}

static inline void ClearMbStatusFlags(CAN_Type *base, uint32_t mask)
{


    base->IFLAG1 = mask;

}
