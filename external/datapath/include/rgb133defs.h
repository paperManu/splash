/*
 * rgb133defs.h
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

#ifndef RGB133DEFS_H
#define RGB133DEFS_H

/* Device Limits */
#ifdef INCLUDE_VISIONLC
#define RGB133_MAX_DEVICES                   65
#define RGB133_CONTROL_DEVICE_NUM            64
#else /* INCLUDE_VISION */
#define RGB133_MAX_DEVICES                   64
#define RGB133_CONTROL_DEVICE_NUM            63
#endif
#define RGB133_MAX_VIDEO_DEVICES             (RGB133_MAX_DEVICES - 1)

#define RGB133_MAX_CAP_PER_CHANNEL           16
#define RGB133_MAX_CHANNEL_PER_CARD           8
#define RGB133_CAPTURE_NOT_IN_USE    0x00000000
#define RGB133_CAPTURE_IN_USE        0xFFFFFFFF

/* Device defaults */
#define RGB133_DEFAULT_CAP_WIDTH            640
#define RGB133_DEFAULT_CAP_HEIGHT           480

/* Module load option defines */
#define RGB133_NO_EXPOSE_INPUTS               0
#define RGB133_EXPOSE_INPUTS                  1

#define RGB133_ENABLE_NON_V4L2_PIX_FMTS       1
#define RGB133_DISABLE_NON_V4L2_PIX_FMTS      0

#define RGB133_RGB_BYTEORDER                  0
#define RGB133_BGR_BYTEORDER                  1

#define RGB133_VID_TIMINGS_DEFAULT            0
#define RGB133_VID_TIMINGS_LIMITED            1

#define RGB133_DUMB_BUFFER_WIDTH_DEFAULT     -1
#define RGB133_DUMB_BUFFER_HEIGHT_DEFAULT    -1
#define RGB133_DUMB_BUFFER_RATE_DEFAULT      -1

#define RGB133_TIMESTAMP_INFO_SILENT          0
#define RGB133_TIMESTAMP_INFO_VERBOSE         1
#define RGB133_TIMESTAMP_INFO_NOISY           2

#define RGB133_NO_FRAME_DEBUG                 0
#define RGB133_FRAME_DEBUG                    1

#define RGB133_NO_GTF_OFFSET                  0
#define RGB133_GTF_OFFSET                     1

#define RGB133_DISPLAY_NO_SIGNAL_MSG          0
#define RGB133_NO_DISPLAY_NO_SIGNAL_MSG       1

#define RGB133_NO_SHOW_COUNTER                0
#define RGB133_SHOW_COUNTER                   1

#ifdef RGB133_CONFIG_HAVE_INTERLACED_TB
#define RGB133_MAX_INTERLACING_MODES         12 /* Include Bob & Unknown */
#else
#define RGB133_MAX_INTERLACING_MODES         10 /* Include Bob & Unknown - no V4L2_FIELD_INTERLACED_[TB] */
#endif

#define RGB133_NO_SHOW_FPS                    0
#define RGB133_SHOW_FPS                       1

#define RGB133_NO_COLOURED_BUFFERS            0
#define RGB133_COLOURED_BUFFERS               1

/* Force Detect Mode Definitions */
#define RGB133_DETECT_MODE_UNSET              0
#define RGB133_DETECT_MODE_SET                1

#define RGB133_DETECT_MODE_DEFAULT            0
#define RGB133_DETECT_MODE_ANALOGUE           1
#define RGB133_DETECT_MODE_DVI                2

/* Aspect Ratio Definitions */
#define RGB133_SCALING_ASPECT_RATIO_DEFAULT   0
#define RGB133_SCALING_ASPECT_RATIO_SOURCE    1
#define RGB133_SCALING_ASPECT_RATIO_3_2       2
#define RGB133_SCALING_ASPECT_RATIO_4_3       3
#define RGB133_SCALING_ASPECT_RATIO_5_3       4
#define RGB133_SCALING_ASPECT_RATIO_5_4       5
#define RGB133_SCALING_ASPECT_RATIO_8_5       6
#define RGB133_SCALING_ASPECT_RATIO_16_9      7
#define RGB133_SCALING_ASPECT_RATIO_16_10     8
#define RGB133_SCALING_ASPECT_RATIO_17_9      9

/* Scaling Behaviour Definitions */
#define RGB133_SCALING_DEFAULT                0
#define RGB133_SCALING_UP_ONLY                1
#define RGB133_SCALING_DOWN_ONLY              2
#define RGB133_SCALING_NONE                   3

/* Scaling Returns */
#define RGB133_SCALING_NO_CHANGE              0
#define RGB133_SCALING_CHANGED                1

/* HDCP */
#define RGB133_HDCP_OFF                       0
#define RGB133_HDCP_ON                        1

/* Overscan Cropping */
#define RGB133_OVERSCAN_CROPPING_OFF          0
#define RGB133_OVERSCAN_CROPPING_ON           1

/* Colour Domain */
#define RGB133_COLOUR_DOMAIN_AUTO             0          /* Use detected domain */
#define RGB133_COLOUR_DOMAIN_RGB709_FULL      1          /* Full Range RGB BT.709 */
#define RGB133_COLOUR_DOMAIN_YUV709_FULL      2          /* Full Range YUV BT.709 */
#define RGB133_COLOUR_DOMAIN_YUV601_FULL      3          /* Full Range YUV BT.601 */
#define RGB133_COLOUR_DOMAIN_YUV709_STUDIO    4          /* Studio Range YUV BT.709 */
#define RGB133_COLOUR_DOMAIN_YUV601_STUDIO    5          /* Studio Range YUV BT.601 */
#define RGB133_COLOUR_DOMAIN_RGB709_STUDIO    6          /* Studio Range RGB BT.709 */
#define RGB133_COLOUR_DOMAIN_YUV2020_FULL     7          /* Full Range YUV BT.2020 */
#define RGB133_COLOUR_DOMAIN_YUV2020_STUDIO   8          /* Studio Range YUV BT.2020 */
#define RGB133_COLOUR_DOMAIN_RGB601_FULL      9          /* Full Range RGB BT.601 */
#define RGB133_COLOUR_DOMAIN_RGB601_STUDIO    10         /* Studio Range RGB BT.601 */
#define RGB133_COLOUR_DOMAIN_RGB2020_FULL     11         /* Full Range RGB BT.2020 */
#define RGB133_COLOUR_DOMAIN_RGB2020_STUDIO   12         /* Studio Range RGB BT.2020 */

/* Backwards compatible */
#define RGB133_COLOUR_DOMAIN_RGB              RGB133_COLOUR_DOMAIN_RGB709_FULL
#define RGB133_COLOUR_DOMAIN_HDYPRPB          RGB133_COLOUR_DOMAIN_YUV709_FULL
#define RGB133_COLOUR_DOMAIN_YPRPB            RGB133_COLOUR_DOMAIN_YUV601_FULL
#define RGB133_COLOUR_DOMAIN_HDYCRCB          RGB133_COLOUR_DOMAIN_YUV709_STUDIO
#define RGB133_COLOUR_DOMAIN_YCRCB            RGB133_COLOUR_DOMAIN_YUV601_STUDIO

#define RGB133_NO_DIAGNOSTICS_MODE            0
#define RGB133_DIAGNOSTICS_MODE               1

#define RGB133_NO_FULL_RANGE_YUV              0
#define RGB133_FULL_RANGE_YUV                 1

#define RGB133_AUDIO_EMBEDDED                 0
#define RGB133_AUDIO_EXTERNAL                 1

/* Audio Digital Mute */
#define RGB133_AUDIO_MUTE_OFF                 0
#define RGB133_AUDIO_MUTE_ON                  1

/* Audio Analog Mute */
#define RGB133_AUDIO_MUTE_NONE                0
#define RGB133_AUDIO_MUTE_UNBALANCED          1
#define RGB133_AUDIO_MUTE_BALANCED            2
#define RGB133_AUDIO_MUTE_BOTH                3

/* Volume */
#define RGB133_AUDIO_VOLUME_DEFAULT          -1
#define RGB133_AUDIO_VOLUME_MIN               0
#define RGB133_AUDIO_VOLUME_MAX             100

#endif /* RGB133DEFS_H */
