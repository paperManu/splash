/*
 * rgb133vdif.h
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

#ifndef RGB133VDIF_H_
#define RGB133VDIF_H_

#define RGB133_VDIF_FLAG_INTERLACED        0x0001
#define RGB133_VDIF_FLAG_HSYNC_POSITIVE    0x0002
#define RGB133_VDIF_FLAG_VSYNC_POSITIVE    0x0004
#define RGB133_VDIF_FLAG_READ_ONLY         0x0008
#define RGB133_VDIF_FLAG_NOSIGNAL          0x0010
#define RGB133_VDIF_FLAG_OUTOFRANGE        0x0020
#define RGB133_VDIF_FLAG_UNRECOGNISABLE    0x0040
#define RGB133_VDIF_FLAG_NONVESA           0x0080
#define RGB133_VDIF_FLAG_SDI               0x0100
#define RGB133_VDIF_FLAG_DVI               0x0200
#define RGB133_VDIF_FLAG_DVI_DUAL_LINK     0x0400
#define RGB133_VDIF_FLAG_DISPLAYPORT       0x0800
#define RGB133_VDIF_FLAG_VIDEO             0x1000

#define VDIF_FLAG_DVI                      0x0200
#define VDIF_FLAG_VIDEO                    0x1000

#endif /*RGB133VDIF_H_*/
