/*
 * rgb133capparms.h
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

#ifndef RGB133CAPPARMS_H_
#define RGB133CAPPARMS_H_

typedef enum _rgb133_cap_parms {
   RGB133_CAP_PARM_BRIGHTNESS,
   RGB133_CAP_PARM_CONTRAST,
   RGB133_CAP_PARM_SATURATION,
   RGB133_CAP_PARM_HUE,
   RGB133_CAP_PARM_BLACKLEVEL,
   RGB133_CAP_PARM_HOR_TIME,
   RGB133_CAP_PARM_VER_TIME,
   RGB133_CAP_PARM_HOR_POS,
   RGB133_CAP_PARM_HOR_SIZE,
   RGB133_CAP_PARM_PHASE,
   RGB133_CAP_PARM_VERT_POS,
   RGB133_CAP_PARM_R_COL_BRIGHTNESS,
   RGB133_CAP_PARM_R_COL_CONTRAST,
   RGB133_CAP_PARM_G_COL_BRIGHTNESS,
   RGB133_CAP_PARM_G_COL_CONTRAST,
   RGB133_CAP_PARM_B_COL_BRIGHTNESS,
   RGB133_CAP_PARM_B_COL_CONTRAST,
   RGB133_CAP_PARM_VIDEO_STD,
   RGB133_CAP_PARM_VID_HTIMINGS,
   RGB133_CAP_PARM_VID_VTIMINGS,
   RGB133_CAP_PARM_VFLIP,
   RGB133_CAP_PARM_LIVESTREAM,
   RGB133_CAP_PARM_COLOURDOMAIN,
} rgb133_cap_parms;

typedef struct _rgb133_cap_parms_aoi {
   int Top;
   int Left;
   int Bottom;
   int Right;
} rgb133_cap_parms_aoi, *prgb133_cap_parms_aoi;

#endif /*RGB133CAPPARMS_H_*/
