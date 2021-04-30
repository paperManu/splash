/*
 * rgb133api.h
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

#ifndef RGB133API_H_
#define RGB133API_H_

#include "rgb133config.h"
#include "rgb133.h"

#ifdef RGB133_CONFIG_HAVE_V4L2_FOPS
extern const struct v4l2_file_operations rgb133_fops;
#else
extern const struct file_operations rgb133_fops;
#endif

int APIOpenCapture(struct rgb133_handle* h, PIRP pIrp);
int APICloseCapture(struct rgb133_handle* h);
int APIEnableCapture(struct rgb133_handle* h);
int APIDisableCapture(struct rgb133_handle* h);
int APIWaitForDMA(struct rgb133_handle* h);

int APIFromNoSignal(struct rgb133_handle* h);

int APIRequestAndWaitForData(struct rgb133_handle* h, char __user* data, size_t count);

#endif /*RGB133API_H_*/
