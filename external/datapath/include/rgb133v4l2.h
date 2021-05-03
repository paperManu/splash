/*
 * rgb133v4l2.h
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

#ifndef RGB133V4L2API_H_
#define RGB133V4L2API_H_

#ifdef __KERNEL__
#include "rgb133config.h"

/* V4L2 includes */
#include <media/v4l2-common.h>

#ifdef RGB133_CONFIG_HAVE_V4L2_IOCTL
#include <media/v4l2-ioctl.h>
#else
#include <media/v4l2-dev.h>
#endif

#include "rgb_api_types.h"

#include <linux/videodev2.h>

extern struct video_device rgb133_defaults;

/*!
 * Supported V4L2 std's.
 */
#define RGB133_NORMS    (\
                V4L2_STD_PAL | V4L2_STD_NTSC)
#endif

/* Include outside of __KERNEL__ region */
#include "rgb133pvtioctl.h"

/*!
 * Define the V4L2 extended controls.
 */
#define RGB133_V4L2_CID_HOR_POS           (V4L2_CID_PRIVATE_BASE +  0)
#define RGB133_V4L2_CID_HOR_SIZE          (V4L2_CID_PRIVATE_BASE +  1)
#define RGB133_V4L2_CID_PHASE             (V4L2_CID_PRIVATE_BASE +  2)
#define RGB133_V4L2_CID_VERT_POS          (V4L2_CID_PRIVATE_BASE +  3)
#define RGB133_V4L2_CID_R_COL_CONTRAST    (V4L2_CID_PRIVATE_BASE +  4)
#define RGB133_V4L2_CID_R_COL_BRIGHTNESS  (V4L2_CID_PRIVATE_BASE +  5)
#define RGB133_V4L2_CID_G_COL_CONTRAST    (V4L2_CID_PRIVATE_BASE +  6)
#define RGB133_V4L2_CID_G_COL_BRIGHTNESS  (V4L2_CID_PRIVATE_BASE +  7)
#define RGB133_V4L2_CID_B_COL_CONTRAST    (V4L2_CID_PRIVATE_BASE +  8)
#define RGB133_V4L2_CID_B_COL_BRIGHTNESS  (V4L2_CID_PRIVATE_BASE +  9)
#define RGB133_V4L2_CID_FORCE_DETECT      (V4L2_CID_PRIVATE_BASE + 10)
#define RGB133_V4L2_CID_SCALING           (V4L2_CID_PRIVATE_BASE + 11)
#define RGB133_V4L2_CID_SCALING_AR        (V4L2_CID_PRIVATE_BASE + 12)
#define RGB133_V4L2_CID_HOR_TIME          (V4L2_CID_PRIVATE_BASE + 13)
#define RGB133_V4L2_CID_VER_TIME          (V4L2_CID_PRIVATE_BASE + 14)
#define RGB133_V4L2_CID_HDCP              (V4L2_CID_PRIVATE_BASE + 15)
#define RGB133_V4L2_CID_COLOURDOMAIN      (V4L2_CID_PRIVATE_BASE + 16)
#define RGB133_V4L2_CID_LIVESTREAM        (V4L2_CID_PRIVATE_BASE + 17)
#define RGB133_V4L2_CID_INPUT_GANGING     (V4L2_CID_PRIVATE_BASE + 18)
#define RGB133_V4L2_CID_SIGNAL_TYPE       (V4L2_CID_PRIVATE_BASE + 19)
#define RGB133_V4L2_CID_VIDEO_STANDARD    (V4L2_CID_PRIVATE_BASE + 20)
#define RGB133_V4L2_CID_CLIENT_ID         (V4L2_CID_PRIVATE_BASE + 21) /* Keep last */

#define RGB133_NUM_V4L2_CONTROLS     8
#define RGB133_NUM_PRIVATE_CONTROLS 21
#define RGB133_NUM_CONTROLS         (RGB133_NUM_V4L2_CONTROLS+RGB133_NUM_PRIVATE_CONTROLS)

#define RGB133_NUM_FORCE_DETECT_ITEMS    3
#define RGB133_NUM_SCALING_ITEMS         4
#define RGB133_NUM_SCALING_AR_ITEMS     10
#define RGB133_NUM_HDCP_ITEMS            2
#define RGB133_NUM_COLOURDOMAIN_ITEMS   13
#define RGB133_NUM_INPUT_GANGING_ITEMS   8
#define RGB133_NUM_SIGNAL_TYPE_ITEMS    (VW_TYPE_NUM_ITEMS)
#define RGB133_NUM_VIDEO_STANDARD_ITEMS (VW_VIDSTD_NUM_ITEMS)

#define V4L2_CID_PRIVATE_LASTP1 (V4L2_CID_PRIVATE_BASE + RGB133_NUM_PRIVATE_CONTROLS)

/*!
 * Define extra fourcc pixel formats.
 */
#define V4L2_PIX_FMT_Y800  v4l2_fourcc('Y', '8', '0', '0') /*  8  Greyscale          */
#define V4L2_PIX_FMT_Y8    v4l2_fourcc('Y', '8', ' ', ' ') /*  8  Greyscale          */
#define V4L2_PIX_FMT_R8    v4l2_fourcc('R', '8', ' ', ' ') /*  8  Red Monochrome     */
#define V4L2_PIX_FMT_R800  v4l2_fourcc('R', '8', '0', '0') /*  8  Red Monochrome     */
#define V4L2_PIX_FMT_G8    v4l2_fourcc('G', '8', ' ', ' ') /*  8  Green Monochrome   */
#define V4L2_PIX_FMT_G800  v4l2_fourcc('G', '8', '0', '0') /*  8  Green Monochrome   */
#define V4L2_PIX_FMT_B8    v4l2_fourcc('B', '8', ' ', ' ') /*  8  Blue Monochrome    */
#define V4L2_PIX_FMT_B800  v4l2_fourcc('B', '8', '0', '0') /*  8  Blue Monochrome    */
#define V4L2_PIX_FMT_RGB10 v4l2_fourcc('A', 'R', '3', '0') /*  32 RGB 10-bit         */
#define V4L2_PIX_FMT_Y410  v4l2_fourcc('A', 'Y', '3', '0') /*  32 YUV 10-bit         */

#define rgb133_fourcc_to_char(fcc, fourcc) \
   memcpy(fourcc, &fcc, 4)

/*!
 * Define private ioctl enums.
 */
#define RGB133_VIDIOC_G_SRC_FMT                 RGB133_PRIVATE_IOCTL_G_FMT
#define RGB133_VIDIOC_S_SRC_FMT                 RGB133_PRIVATE_IOCTL_S_FMT
#define RGB133_VIDIOC_G_VIDEO_TIMINGS           RGB133_PRIVATE_IOCTL_G_VIDEO_TIMINGS

/*!
 * Define private buffer type for querying the source resolution with
 * the VIDIOC_G_FMT and VIDIOC_S_FMT ioctl's.
 */
#define V4L2_BUF_TYPE_CAPTURE_SOURCE (V4L2_BUF_TYPE_PRIVATE+0)

/*!
 * Define private BOB deinterlace type.
 */
#define V4L2_FIELD_BOB 128

enum {
   RGB133_UNKNOWN_IO = 0,
   RGB133_READ_IO,
   RGB133_MMAP_IO,
   RGB133_USERPTR_IO,
};

typedef enum _eParmsType {
   RGB133_PARMS_CURRENT,
   RGB133_PARMS_DETECTED,
} eParmsType;

/*!
 * Define private structure for querying video timings with
 * the VIDIOC_G_VIDEO_TIMINGS ioctl.
 */
typedef struct _srgb133VideoTimings {
   unsigned long    size;
   eParmsType       type;
   sVWVideoTimings  VideoTimings;
} srgb133VideoTimings;


#define RGB133_NUM_V4L2_IO_TYPES 4
extern const char* rgb133_v4l2_io_types[RGB133_NUM_V4L2_IO_TYPES];

#define RGB133_V4L2_BAD_TPF_NUMERATOR   -2
#define RGB133_V4L2_BAD_TPF_DENOMINATOR -1

#ifdef __KERNEL__
/************************************************/
int rgb133V4L2IsCapture(PV4L2FMTAPI f);
int rgb133V4L2IsCaptureVideo(PV4L2FMTAPI f);
int rgb133V4L2IsCaptureSource(PV4L2FMTAPI f);

int rgb133V4L2GetCaptureVideo(void);

int rgb133V4L2GetPixFmtRGB24(void);
int rgb133V4L2GetPixFmtBGR24(void);
int rgb133V4L2GetPixFmtRGB32(void);
int rgb133V4L2GetPixFmtBGR32(void);

const char* rgb133V4L2GetIOType(PV4L2EMEMAPI pMemory);

int rgb133V4L2GetFieldNone(void);
int rgb133V4L2GetFieldBob(void);
int rgb133V4L2GetFieldTop(void);
int rgb133V4L2GetFieldBottom(void);
int rgb133V4L2GetFieldInterlaced(void);
int rgb133V4L2GetFieldAlternate(void);

int rgb133V4L2GetCIDForceDetect(void);
int rgb133V4L2GetCIDHDCP(void);
int rgb133V4L2GetCIDInputGanging(void);

void rgb133V4L2AssignDimensions(struct rgb133_handle* h, PV4L2FMTAPI f);

void rgb133V4L2AssignPixFmtFromWin(struct rgb133_handle* h, PV4L2FMTAPI f, unsigned long PixelFormat);

void rgb133V4L2CorrectBGRPixFmt(struct rgb133_handle* h, PV4L2FMTAPI f);

void rgb133V4L2AssignImageSize(struct rgb133_handle* h, PV4L2FMTAPI f);

void rgb133V4L2AssignPriv(PV4L2FMTAPI f, unsigned long VerFrequency, unsigned long DefVerFrequency);

int rgb133V4L2GetSampleRate(PV4L2CAPPARMAPI pCapParm, unsigned long* pSampleRate);
void rgb133V4L2AssignTimePerFrame(PV4L2CAPPARMAPI pCapParm, unsigned long SampleRate, unsigned long VerFrequency);

typedef enum _eControls {
   RGB133_NO_CONTROLS = 1,
   RGB133_ALL_CONTROLS = 2,
   RGB133_DYNAMIC_CONTROLS = 3,
} eControls;

void rgb133V4L2AssignControlLimits(struct rgb133_handle* h, PDEAPI pDE,
      eControls ctrls);
void rgb133V4L2AssignPrivateControlLimits(struct rgb133_handle* h, PDEAPI pDE,
      PDEVPARMSAPI _pCurrent, PDEVPARMSAPI _pDetect, eControls ctrls);

void rgb133V4L2AssignDeinterlace(u32* pField, PSETPARMSINAPI pSetParmsIn);

void rgb133V4L2AssignDeviceCaps(struct rgb133_dev* dev,
                        struct video_device *vdev,
                        struct v4l2_capability* pCap);

#endif /* __KERNEL__ */

#endif /*RGB133V4L2API_H_*/
