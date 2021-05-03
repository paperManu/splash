/*
 * rgb133irqapi.h
 *
 * Copyright (c) 2009 Datapath Limited All rights reserved.
 *
 * All information contained herein is proprietary and
 * confidential to Datapath Limited and is licensed under
 * the terms of the Datapath Limited Software License.
 * Please read the LICENCE file for full license terms
 * and conditions.
 *
 * http://www.datapath.co.uk/
 * support@datapath.co.uk
 *
 */

#ifndef RGB133IRQAPI_H_
#define RGB133IRQAPI_H_

#include "rgb_windows_types.h"
#include "rgb_api_types.h"

int IRQ_ISR(PKINTERRUPTAPI Interrupt, PDEAPI _pDE);

#endif /* RGB133IRQAPI_H_ */
