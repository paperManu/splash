/*
 * rgb133control.h
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

#ifndef RGB133CONTROL_H_
#define RGB133CONTROL_H_

#include "rgb133defs.h"

/*!
 * Control defines's
 */
#define VW_DEVICE_NOT_FLASHABLE     0
#define VW_DEVICE_IS_FLASHABLE      1
#define VW_DEVICE_UNKNOWN_FLASHABLE 2

/*!
 * De-interlacing Enum
 */
typedef enum _eVWDeint {
   VW_DEINT_NONE,
   VW_DEINT_BOB,
   VW_DEINT_WEAVE,
} eVWDeint;

/*!
 * Signal Type Enum
 */
typedef enum _eVWSignalType {
   VW_TYPE_NOSIGNAL,
   VW_TYPE_DVI,
   VW_TYPE_DVI_DUAL_LINK,
   VW_TYPE_SDI,
   VW_TYPE_VIDEO,
   VW_TYPE_3WIRE_SOG,
   VW_TYPE_4WIRE_COMPOSITE_SYNC,
   VW_TYPE_5WIRE_SEPARATE_SYNCS,
   VW_TYPE_YPRPB,
   VW_TYPE_CVBS,
   VW_TYPE_YC,
   VW_TYPE_UNKNOWN,
   VW_TYPE_NUM_ITEMS,
} eVWSignalType;

/*!
 * Video Standard Enum
 */
typedef enum _eVWVidStd {
   VW_VIDSTD_UNKNOWN,
   VW_VIDSTD_NTSC_M,
   VW_VIDSTD_NTSC_J,
   VW_VIDSTD_NTSC_4_43_50,
   VW_VIDSTD_NTSC_4_43_60,
   VW_VIDSTD_PAL_I,
   VW_VIDSTD_PAL_M,
   VW_VIDSTD_PAL_NC,
   VW_VIDSTD_PAL_4_43_60,
   VW_VIDSTD_SECAM_L,
   VW_VIDSTD_NUM_ITEMS,
} eVWVidStd;

/*!
 * App Event Enum
 *    - This enumeration is used by the split Kernel/User space
 *      driver for event handshaking.
 */
typedef enum _eAppEvent {
   RGB133_EV_OPEN_CONTROL = 1,
   RGB133_EV_CLOSE_CONTROL,
   RGB133_EV_ENABLE_CONTROL,
   RGB133_EV_REPORT_VID_PARMS_CONTROL,
   RGB133_EV_GET_CURRENT_PARMS,
   RGB133_EV_GET_DEFAULT_PARMS,
   RGB133_EV_GET_MINIMUM_PARMS,
   RGB133_EV_GET_MAXIMUM_PARMS,
   RGB133_EV_GET_ALL_PARMS,
   RGB133_EV_GET_ALL_DEVICE_PARMS,
   RGB133_EV_GET_VID_PARMS,
   RGB133_EV_GET_SET_CURRENT,
   RGB133_EV_GET_SET_VIDEO_TIMINGS,
   RGB133_EV_SET_VIDEO_TIMINGS,
   RGB133_EV_SET_COLOUR_BALANCE,
   RGB133_EV_GET_DETECT_FLAGS,
   RGB133_EV_GET_VID_MEAS_FLAGS,
   RGB133_EV_SET_PARAMETER,
   RGB133_EV_SET_SCALE_AOI,
   RGB133_EV_GET_CROPPING,
   RGB133_EV_SET_CROPPING,
   RGB133_EV_SET_PIXELFORMAT,
   RGB133_EV_REQ_DATA,
   RGB133_EV_WAIT_FOR_DATA,
   RGB133_EV_IS_BAD_SIGNAL,
   RGB133_EV_DRAW_NO_SIG,
   RGB133_EV_GET_CHANNELS,
   RGB133_EV_GET_VHDL_VERSION,
   RGB133_EV_GET_BOARD_TYPE,
   RGB133_EV_ADDRESS_PEEK,
   RGB133_EV_READ_FLASH_IMAGE,
   RGB133_EV_WRITE_FLASH_IMAGE,
   RGB133_EV_RATE_SET,
   RGB133_EV_GET_INPUT_RATE,
   RGB133_EV_GET_SIGNAL_TYPE,
   RGB133_EV_GET_VIDEO_STD,
   RGB133_EV_SET_EQUILISATION,
   RGB133_EV_SET_DEBUG_LEVEL,
} eAppEvent;

/*!
 * Control Device Magic Numbers
 *    - These magic numbers are used by applications to perform
 *      read/write operations on the Vision / VisionLC control device
 *      (/dev/video63 or /dev/video64) .  For a structure to be read/written
 *      successfully its magic number element must be set to the
 *      correct value.
 */
typedef enum _eVWMagic {
   VW_MAGIC_SYSTEM_NUM_DEVICES = 1,
   VW_MAGIC_SYSTEM_INFO,
   VW_MAGIC_DEVICE_INFO,
   VW_MAGIC_DEVICE_INIT_INFO,
   VW_MAGIC_INPUT_INFO,
   VW_MAGIC_DEVICE,
   VW_MAGIC_SET_DEVICE_PARMS,
   VW_MAGIC_READ_REG,
   VW_MAGIC_WRITE_FLASH,
   VW_MAGIC_READ_FLASH,
   VW_MAGIC_BOARD_TYPE,
   VW_MAGIC_SIGNAL_EVENT,
   VW_MAGIC_DEBUG_LEVEL,
   VW_MAGIC_FLASH_INFO,
   VW_MAGIC_INIT_USER,           /*!< Used Internally.*/
   VW_MAGIC_DEVICE_PARMS,        /*!< Used Internally.*/
   VW_MAGIC_CLIENT_PARMS,        /*!< Used Internally.*/
   VW_MAGIC_DEVICE_INFO_INT,     /*!< Used Internally.*/
   VW_MAGIC_CONTROL,             /*!< Used Internally.*/
   VW_MAGIC_IMAGE_DATA,          /*!< Used Internally.*/
   VW_MAGIC_APP_EVENT,           /*!< Used Internally.*/
   VW_MAGIC_IRQ_EVENT,           /*!< Used Internally.*/
   VW_MAGIC_USER_DMA,            /*!< Used Internally.*/
} eVWMagic;

/*!
 * Magic Number Structure
 */
typedef struct _sVWMagic {
   int magic; /*!< Holds an eVWMagic enum.*/
} sVWMagic;

/*!
 * Signal Notification Event Structure
 */
typedef struct _sVWSignalEvent {
   int                     magic;
   struct _sVWSignalEvent *next;
   int                     device;  /*!< Device event was triggered for. */
   int                     channel; /*!< Channel event was triggered for. */
} sVWSignalEvent;

/*!
 * User Mode Initialisation Structure
 */
typedef struct _sVWUserInit {
   /* Magic struct number */
   int           magic;

   int           init;
} sVWUserInit;

/*!
 * Area-Of-Interest Structure
 */
typedef struct _sVWAOI {
   int top;
   int left;
   int bottom;
   int right;
} sVWAOI;

/*!
 * Client Parameters Structure
 */
typedef struct _sVWClientParms {
   /* Magic struct number */
   int           magic;

   int           device;

   int           status;

   // Buffer Characteristics
   int           width;
   int           height;
   int           pixelformat;

   // Cropping
   sVWAOI        aoi;

   // Deinterlacing
   eVWDeint      deint;

   unsigned long SampleRate;
} sVWClientParms;

/*!
 * Video Timings Structure
 */
typedef struct _sVWVideoTimings {
   unsigned long  HorFrequency;     /* Line rate in Hz. */
   unsigned long  VerFrequency;     /* Refresh rate in Hz*1000. */
   unsigned long  PixelClock;       /* Dot clock in Hz. */

   unsigned short Flags;            /* Bitwise OR of VDIF_FLAG_.*. */

   /* The following values are in pixels. */
   unsigned long  HorAddrTime;      /* Amount of active video (resolution). */
   unsigned long  HorRightBorder;
   unsigned long  HorFrontPorch;
   unsigned long  HorSyncTime;
   unsigned long  HorBackPorch;
   unsigned long  HorLeftBorder;

   /* The following values are in lines. */
   unsigned long  VerAddrTime;      /* Amount of active video (resolution). */
   unsigned long  VerBottomBorder;
   unsigned long  VerFrontPorch;
   unsigned long  VerSyncTime;
   unsigned long  VerBackPorch;
   unsigned long  VerTopBorder;

   unsigned long  HScaled;
   unsigned long  VScaled;
} sVWVideoTimings;

typedef struct _sVWVideoMeasurement {
   unsigned long Flags;
} sVWVideoMeasurement;

typedef struct _sVWColourBalance {
   unsigned long RedGain;
   unsigned long GreenGain;
   unsigned long BlueGain;
   unsigned long RedOffset;
   unsigned long GreenOffset;
   unsigned long BlueOffset;
} sVWColourBalance, *psVWColourBalance;

/*!
 * Device Parameters Structure
 */
typedef struct _sVWDeviceParms {
   int                 cap_parm;

   // Signal Type
   eVWSignalType       type;

   // Video Std
   eVWVidStd           VideoStandard;

   // Signal
   int                 no_signal;
   int                 transition;

   // Video Characteristics (ALL)
   unsigned long       Brightness;
   unsigned long       Contrast;

   // Video Characteristics (ANALOG)
   unsigned long       Saturation;
   unsigned long       Hue;
   unsigned long       Blacklevel;
   unsigned long       Phase;

   sVWColourBalance    Colour;

   // Video Std
   unsigned long       flags;
   unsigned long       Format;

   // VideoTimings
   sVWVideoTimings     VideoTimings;
   unsigned long       PixelFormat;

   // VideoMeasurement
   sVWVideoMeasurement VideoMeasurement;

   // AOI
   sVWAOI              aoi[RGB133_MAX_CAP_PER_CHANNEL];

   // Image manipulation
   int                 VFlip;

   // LiveStream
   unsigned long       LiveStream;

} sVWDeviceParms;

/*!
 * Curr/Min/Max Device Parameters Structure
 */
typedef struct _sVWAllDeviceParms {
   /* Magic struct number */
   int               magic;

   int               device;

   int               status;

   sVWDeviceParms    curr;
   sVWDeviceParms    det;
   sVWDeviceParms    def;
   sVWDeviceParms    min;
   sVWDeviceParms    max;
} sVWAllDeviceParms;

/*!
 * Capture Client Structure
 */
typedef struct _sVWClient {
   int            connected;
   sVWClientParms clientParms;
} sVWClient;

/*!
 * Capture Input Structure
 */
typedef struct _sVWInput {
   sVWClient      client[RGB133_MAX_CAP_PER_CHANNEL];
   int            clients;
   sVWDeviceParms curDeviceParms;
   sVWDeviceParms minDeviceParms;
   sVWDeviceParms maxDeviceParms;
   sVWDeviceParms defDeviceParms;
   sVWDeviceParms detDeviceParms;
} sVWInput;

/*!
 * Vision / VisionLC Device Structure
 *    - Readable over the control channel by using
 *      the magic number VW_MAGIC_DEVICE
 */
typedef struct _sVWDevice {
   /* Magic struct number */
   int           magic;

   int           device;
   int           input;
   int           client;
   unsigned long flags;

   sVWInput      inputs[RGB133_MAX_CHANNEL_PER_CARD];
} sVWDevice, *psVWDevice;

/*!
 * Vision / VisionLC Device Input Map
 *    - The min/max input numbers of the device
 */
typedef struct _sVWDeviceInputMap {
   int min;     // The minimum input number for the device
   int max;     // The maximum input number for the device
} sVWDeviceInputMap;

/*!
 * Vision / VisionLC Device Initialisation Information Structure
 *    - Readable over the control channel by using
 *      the magic number VW_MAGIC_DEVICE_INIT_INFO
 */
typedef struct _sVWDeviceInitInfo {
   /* Magic struct number */
   int magic;

   int device;
   int init;
} sVWDeviceInitInfo;

/*!
 * Vision / VisionLC System Information Structure
 *    - Readable over the control channel by using
 *      the magic number VW_MAGIC_SYSTEM_INFO
 */
typedef struct _sVWSystemInfo {
   /* Magic struct number */
   int               magic;

   int               devices;
   int               inputs;
   sVWDeviceInputMap map[RGB133_MAX_DEVICES-1];

   char              version[16];
} sVWSystemInfo, *psVWSystemInfo;

#define MAX_NAME_LEN 32
#define MAX_NODE_LEN 20

/*!
 * Vision / VisionLC Firmware Structure
 */
typedef struct _sVWFW {
   unsigned long Version;
   unsigned long day;
   unsigned long month;
   unsigned long year;
} sVWFW;

/*!
 * Vision / VisionLC Device Information Structure
 *    - Readable over the control channel by using
 *      the magic number VW_MAGIC_DEVICE_INFO
 */
typedef struct _sVWDeviceInfo {
   /* Magic struct number */
   int  magic;

   int  device;

   int  status;

   int  channels;
   char name[MAX_NAME_LEN];
   char node[MAX_NODE_LEN];

   int  link;

   sVWFW FW;

   int  flashable;
} sVWDeviceInfo, *psVWDeviceInfo;

/*!
 * Vision / VisionLC Driver Information Structure
 *    - Local usage only.
 */
typedef struct _sVWDriverInfo {
      unsigned int version;
} sVWDriverInfo, *psDriverInfo;

/*!
 * Vision / VisionLC Input Information Structure
 *    - Readable over the control channel by using
 *      the magic number VW_MAGIC_INPUT_INFO
 */
typedef struct _sVWInputInfo {
   /* Magic struct number */
   int magic;

   int device;
   int input;

   int clients;
} sVWInputInfo, *psVWInputInfo;

/*!
 * Flash info structure
 */
typedef struct _sVWFlashInfo {
    /* Magic struct number */
    int magic;
    int device;

    int flashable;
 } sVWFlashInfo, *psVWFlashInfo;

/*!
 * Split Kernel/User Space Command Structure
 */
typedef struct _sVWCommand {
   /* Magic struct number */
   int           magic;

   unsigned long command;
} sVWCommand;

/*!
 * Split Kernel/User Space IRQ Event Structure
 */
typedef struct _sIrqEvent {
   /* Magic struct number */
   int           magic;

   unsigned long event;
   int           id;
   int           exit;
} sIrqEvent;

enum {
   WAIT_TYPE_CONTROL = 1,
   WAIT_TYPE_PARMS,
   WAIT_TYPE_INFO,
   WAIT_TYPE_IMAGE_DATA,
};

/*!
 * Split Kernel/User Space App Event Structure
 */
typedef struct _sAppEvent {
   /* Magic struct number */
   int               magic;
   unsigned long     wait_type;

   int               id;
   int               device;
   unsigned long     event;
   int               channel;
   int               capture;
   int               users;
   int               force;
   sVWAllDeviceParms deviceParms;
   sVWClientParms    clientParms;

   int               debug_mask;

   int               exit;

   void*             mdl_vaddr;
   unsigned long     mdl_size;

   unsigned long     offset;

   char*             pData;
   unsigned long     dataSize;
} sAppEvent;

/*!
 * Split Kernel/User Space Control Structure
 */
typedef struct _sControl {
   /* Magic struct number */
   int             magic;

   int             device;
   int             status;
   int             command;
   int             init;

   unsigned long   flags;

   unsigned int    value;

   unsigned long   type;

} sControl;

/*!
 * Split Kernel/User Space Image Data Structure
 */
typedef struct _sImageData {
   /* Magic struct number */
   int             magic;

   int             device;
   int             status;

   unsigned int    dataSize;
   char*           pData;
} sImageData;

/*!
 * Split Kernel/User Space Image Structure
 */
typedef struct _sImage {
   /* Magic struct number */
   int             magic;

   int             device;
   int             status;

   char            pBuffer[2*1024*1024];
   unsigned long   dataSize;
} sImage;

/*!
 * Vision / VisionLC Read Register Structure
 *    - Readable over the control channel by using
 *      the magic number VW_MAGIC_READ_REG
 */
typedef struct _sVWReadReg {
   /* Magic struct number */
   int           magic;

   int           device;

   int           offset;
   unsigned int  value;
} sVWReadReg;

/*!
 * Vision / VisionLC Write Flash Structure
 *    - Readable over the control channel by using
 *      the magic number VW_MAGIC_WRITE_FLASH
 */
typedef struct _sVWWriteFlash {
   /* Magic struct number */
   int            magic;

   int            device;

   unsigned int   completed;
   unsigned int   status;

   unsigned int   dataSize;

   /* The 'data' field must be 64-bit aligned, the above
    * aligns to 32-bit on 64-bit systems and can affect
    * flash writes on the VisionAV family. */
   unsigned int   padding;

   unsigned char  data[];
} sVWWriteFlash;

/*!
 * Vision / VisionLC Read Flash Structure
 *    - Readable over the control channel by using
 *      the magic number VW_MAGIC_READ_FLASH
 */
typedef struct _sVWReadFlash {
   /* Magic struct number */
   int            magic;

   int            device;

   unsigned int   status;

   unsigned int   dataSize;
   unsigned char  data[];
} sVWReadFlash;

/*!
 * Vision / VisionLC Board Type Structure
 *    - Readable over the control channel by using
 *      the magic number VW_MAGIC_BOARD_TYPE
 */
typedef struct _sVWBoardType {
   /* Magic struct number */
   int           magic;

   int           device;

   unsigned long type;
} sVWBoardType;

/*! Vision / VisionLC Debug Level
 *    - Sets the debug level
 */
typedef struct _sVWDebugLevel {
   /* Magic struct number */
   int           magic;

   int           level;
} sVWDebugLevel;

#endif /*RGB133CONTROL_H_*/
