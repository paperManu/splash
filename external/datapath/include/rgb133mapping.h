/*
 * rgb133mapping.h
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

#ifndef _RGB133MAPPING_H
#define _RGB133MAPPING_H

#ifdef INCLUDE_VISIONLC
#define RGB133_SPPT_PIXEL_FORMATS 22
#else
#define RGB133_SPPT_PIXEL_FORMATS 24
#define RGB133_10_BIT_PIXEL_FORMATS 2
#endif
#define RGB133_DUPLICATE_PIXEL_FORMATS 2

enum rgb133_pix_fmt {
   RGB133_PIX_FMT_RGB15,
   RGB133_PIX_FMT_RGB16,
   RGB133_PIX_FMT_RGB24,
   RGB133_PIX_FMT_RGB32
};

enum rgb133_mapping_type {
   __UNKNOWN__ = 0,
   __PIXELFORMAT__,
   __PIXELFORMATDESC__,
   __VDIFFLAGS__,
   __BUFFER_NOTIFICATIONS__,
   __MAPPING_TYPE_COUNT__          /* MUST BE LAST */
};

enum rgb133_mapping_dir {
   __MAP_TO_WIN__    = 0,
   __MAP_TO_V4L__    = 1,
   __MAP_TO_STRING__ = 2,
};

enum rgb133_ganging_type {
   RGB133_GANGING_TYPE_DISABLED = 0,
   RGB133_GANGING_TYPE_2x1      = 4,
   RGB133_GANGING_TYPE_1x2      = 5,
   RGB133_GANGING_TYPE_3x1      = 6,
   RGB133_GANGING_TYPE_1x3      = 7,
   RGB133_GANGING_TYPE_4x1      = 2,
   RGB133_GANGING_TYPE_1x4      = 3,
   RGB133_GANGING_TYPE_2x2      = 1,
};

int v4lBytesPerPixel(unsigned int PixelFormat);

int v4lSupportedPixelFormats(void);
int v4lSupportedPixelFormatsReal(struct rgb133_handle* h);
unsigned int v4lGetExistingFormatIndex(struct rgb133_handle* h, unsigned int index);

unsigned int mapUItoUI(enum rgb133_mapping_type type, enum rgb133_mapping_dir dir, unsigned int win_value);
const char* mapUItoCC(enum rgb133_mapping_type type, enum rgb133_mapping_dir dir, unsigned int win_value);
const char* __mapUItoCC(enum rgb133_mapping_type type, enum rgb133_mapping_dir dir, unsigned int win_value, char* messagesIn[], char* messagesOut[], int* pNumMessages);

#endif /* _RGB133MAPPING_H */
