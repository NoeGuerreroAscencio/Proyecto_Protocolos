/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"


#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "Can.h"

int main(void)
{
	Can_Init(&_flexcan_config);

	Can_Write(8, &mbConfig);

	Can_MainFunctionRead();
}
