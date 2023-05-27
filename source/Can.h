/*
 * Can.h
 *
 *  Created on: 18 may 2023
 *      Author: noeas
 */

#ifndef CAN_H_
#define CAN_H_

#include "stdbool.h"
#include "stdint.h"


#include "Can_cfg.h"
#include "Can_cfg.c"

extern void Can_Init(const Can_ConfigType* Config);

extern Std_ReturnType Can_Write(Can_HwHandleType Hth,   const Can_PduType* PduInfo);

extern void Can_MainFuntionRead( void );
#include "Can.c"
#endif /* CAN_H_ */
