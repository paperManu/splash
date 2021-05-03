/*
 * rgb133.h
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

#ifndef RGB133_H
#define RGB133_H

#ifdef __KERNEL__
#include <linux/version.h>

#include <linux/time.h>

# define DRIVER_AUTHOR "Datapath Limited <support@datapath.co.uk>"

#ifdef INCLUDE_VISION
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,0))
#error "Vision Driver supports kernels > 2.6 ONLY"
#endif
# define DRIVER_DESC            "Device Driver for Vision Capture Cards"
# define DRIVER_NAME            "rgb133"
# define DRIVER_TAG             "Vision"
# define RGB133_MODULE_NAME     "Vision-7.21.0.60886"
# define RGB133_CHAR_VERSION    "7.21.0.60886"
# define RGB133_WIDE_VERSION   L"7.21.0.60886"
#ifdef RGB133_RELEASE_BUILD
# define RGB133_MAJOR_VERSION   7
# define RGB133_MINOR_VERSION   21
# define RGB133_RELEASE_VERSION 0
#else
# define RGB133_MAJOR_VERSION   9
# define RGB133_MINOR_VERSION   9
# define RGB133_RELEASE_VERSION 9
#endif
#else
#ifdef INCLUDE_VISIONLC
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,0))
#error "VisionLC Driver supports kernels > 2.6 ONLY"
#endif
# define DRIVER_DESC            "Device Driver for VisionLC Capture Cards"
# define DRIVER_NAME            "rgb200"
# define DRIVER_TAG             "VisionLC"
# define RGB133_MODULE_NAME     "VisionLC-1.2.5.60886"
# define RGB133_CHAR_VERSION    "1.2.5.60886"
# define RGB133_WIDE_VERSION   L"1.2.5.60886"
#ifdef RGB133_RELEASE_BUILD
# define RGB133_MAJOR_VERSION   1
# define RGB133_MINOR_VERSION   2
# define RGB133_RELEASE_VERSION 5
#else
# define RGB133_MAJOR_VERSION   9
# define RGB133_MINOR_VERSION   9
# define RGB133_RELEASE_VERSION 9
#endif
#endif
#endif

/* RGB133 Capture Driver Version */
#define RGB133_VERSION\
   KERNEL_VERSION(RGB133_MAJOR_VERSION, RGB133_MINOR_VERSION, RGB133_RELEASE_VERSION)

#include <linux/version.h>

/* v4l2 API */
#include <linux/videodev2.h>

#endif /* __KERNEL__ */

#include "rgb133defs.h"
#include "rgb133timestamp.h"
#include "rgb133vidbuf.h"

#include "rgb133control.h"

#include "rgb_windows_types.h"
#include "rgb_api_types.h"

#include "rgb133v4l2.h"

#ifdef __KERNEL__
#ifdef RGB133_CONFIG_HAVE_ASM_SEMA4
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#endif /* __KERNEL__ */

#ifdef __KERNEL__
#ifdef RGB133_CONFIG_HAVE_ASM_SPIN
#include <asm/spinlock.h>
#else
#include <linux/spinlock.h>
#endif
#endif /* __KERNEL__ */

/* Check macro for diagnostics mode */
#define IsDiagnosticsMode(mode_diag) (mode_diag ? 1 : 0)

/* RGB133 device structure */
struct rgb133_core {
   /* PCI device struct */
   struct pci_dev*   pci;
   unsigned long long pci_base;

   /* device config */
   unsigned int      nr;
   unsigned int      type;
   char              name[256];
   char              node[256];
};

struct rgb133_control {
   int device;
   int input;
};

#ifdef RGB133_USER_MODE
struct rgb133_irq_event {
   struct rgb133_irq_event* next;
   unsigned long            active;
   volatile int             init;
   volatile unsigned long   flags;
};

struct rgb133_irq_event_queue {
   wait_queue_head_t        q;
   struct rgb133_irq_event* event;
   spinlock_t               lock;
   unsigned long            flags;
   volatile int             exit;
};

struct rgb133_app_event {
   struct rgb133_app_event* next;
   unsigned long            wait_type;
   unsigned long            active;
   unsigned long            flags;
   int                      init;
   unsigned long            command;
   int                      channel;
   int                      capture;
   void*                    mdl_vaddr;
   unsigned long            mdl_size;
   unsigned long            offset;
   int                      users;
   int                      force;
   void*                    pData;
   unsigned long            dataSize;
   int                      debug_mask;
};

struct rgb133_app_event_queue {
   wait_queue_head_t        q;
   struct rgb133_app_event* event;
   spinlock_t               lock;
   unsigned long            flags;
   int                      exit;
};

struct rgb133_parms_event {
   wait_queue_head_t      q;
   spinlock_t             lock;
   volatile int           signalled;
};

struct rgb133_control_event {
   wait_queue_head_t      q;
   spinlock_t             lock;
   volatile int           signalled;
};

struct rgb133_image_data_event {
   wait_queue_head_t      q;
   spinlock_t             lock;
   volatile int           signalled;
};

struct rgb133_info_event {
   wait_queue_head_t      q;
   spinlock_t             lock;
   volatile int           signalled;
};

#endif /* RGB133_USER_MODE */

struct rgb133_handle;

typedef void (*CriticalSection)(struct rgb133_handle * h);
typedef void (*SpinlockAccess)(struct rgb133_handle * h, char * caller, int action);

struct _DGC133DE;

struct rgb133_rate {
   struct timeval tv_now;
   struct timeval tv_later;
   unsigned long  timeout;
   unsigned long  msdelay;
   unsigned long  adjust;
   unsigned long  source;
};

struct rgb133_devmap {
   int  index;
   int  minor;
   int  channel;
   char name[256];
   char node[256];
};

struct rgb133_dev {
   unsigned int                  init[RGB133_MAX_CHANNEL_PER_CARD];
   unsigned int                  audioInit[RGB133_MAX_CHANNEL_PER_CARD];
   unsigned int                  control;
   unsigned int                  forced;

   struct rgb133_core            core;
   struct rgb133_devmap          pDevMap[RGB133_MAX_CHANNEL_PER_CARD];
   struct v4l2_device *          v4l2_dev;

   struct _sVWSignalEvent*       events;

#ifdef RGB133_USER_MODE
   /* initialisation */
   struct _sVWUserInit            userInit;

   /* control interface */
   struct rgb133_app_event_queue  appEvQ;
   struct rgb133_irq_event_queue  irqEvQ;
   struct rgb133_control_event    ctrlEv;
   struct rgb133_image_data_event imageDataEv;
   struct rgb133_parms_event      parmsEv;
   struct rgb133_info_event       infoEv;

   struct _sVWDeviceInfo          deviceInfo;
   struct _sVWAllDeviceParms      deviceParms;
   struct _sVWClientParms         clientParms;
   struct _sControl               ctrl;
   struct _sImageData             imageData;

#endif /* RGB133_USER_MODE */

   /* pci device config */
   unsigned short          id;
   unsigned char           revision;
#ifdef __KERNEL__
   unsigned char __iomem*  rgb133_mmio;
#else
   volatile unsigned char* rgb133_mmio;
#endif /* __KERNEL__ */

   /* Device info */
   int                     devices;
   struct video_device*    pVfd[RGB133_MAX_CHANNEL_PER_CARD];

   /* Detection Mode */
   unsigned long           detectMode[RGB133_MAX_CHANNEL_PER_CARD];

   /* Channel info */
   int                     channels;
   int                     fd;
   int                     index;

   BOOL                    transition[RGB133_MAX_CHANNEL_PER_CARD];

   void*                   pDetVdif[RGB133_MAX_CHANNEL_PER_CARD];
   void*                   pCurVdif[RGB133_MAX_CHANNEL_PER_CARD];
   signed long             ColourDomain[RGB133_MAX_CHANNEL_PER_CARD];

   /* User info */
   int                     ctrlUsers;

   /* Debug */
   int                     debug;

   /* From Windows Driver Device Structure */
   PDEAPI                  pDE;

   /* Equilisation */
   int                     bSetEquilisation;

   /* Mutex for access to the driver */
   PMUTEXAPI               pLock;

   struct v4l2_queryctrl   V4L2Ctrls[RGB133_MAX_CHANNEL_PER_CARD][RGB133_NUM_CONTROLS];

   /////would MAX_AUDIO_CHANNELS be correct here?
   struct snd_card*       snd_card[RGB133_MAX_CHANNEL_PER_CARD];
};

/* Structure Definitions */
typedef struct _rgb133_capture {
      unsigned int SourceWidth;
      unsigned int SourceHeight;

      unsigned int CaptureWidth;
      unsigned int CaptureHeight;

      /* Coordinates of image in capture buffer */
      unsigned int ComposeLeft;
      unsigned int ComposeTop;
      unsigned int ComposeWidth;
      unsigned int ComposeHeight;

      unsigned int BufferOffset;

      unsigned int CropTop;
      unsigned int CropLeft;
      unsigned int CropBottom;
      unsigned int CropRight;

      unsigned int bpp;
      unsigned int pixelformat;
      unsigned int planes;
      unsigned int pixelformat_swapped;

      unsigned int imageSize;

      unsigned int ScalingMode;
      unsigned int ScalingAR;
      unsigned int LiveStream;
} rgb133_capture, *prgb133capture;

typedef struct _rgb133_frame {
      int seq;
      struct timeval ts;
} rgb133_frame, *prgb133_frame;

struct rgb133_handle {
#ifdef __KERNEL__ /* Scoped inside the structure for doxygen's benefit. */
   /* Driver device */
   struct rgb133_dev*  dev;
   int                 minor;

   /* System Flags */
   BOOL                bSysFSOpen; /*Opened as a SysFS device? */
   BOOL                bClosing;

   /* v4l2 - are we mmap or user buffered? */
   enum v4l2_buf_type  buffertype;
   enum v4l2_memory    memorytype;

	/* for the vid capture */
   struct _rgb133_capture sCapture;

   /* Capture Info */
   int                 channel;
   int                 capture;
   int                 no_signal;

   /* No signal data */
   unsigned long        no_sig_jiffies;
   int                  day, hr, m, s, ms;

   /* FPS data */
   int                  fps;
   int                  frame_count;
   struct timeval       now, later;

   /* Frame info */
   struct _rgb133_frame frame;

   int                            numbuffers; // How many buffers (below).
   struct rgb133_unified_buffer **buffers; // Actually an array of buffers.
   void                         **ioctlsin; // Array of IO In request structures (one per buffer)
   char                          *pNoSignalBufIn;
   char                          *pNoSignalBufOut;

   /* Locking */
   //   struct mutex               mlock;
   struct semaphore*               pSem;

   spinlock_t*                     pSpinlock;

   /* Critical Section enter/exit functions */
   CriticalSection            EnterCriticalSection;
   CriticalSection            ExitCriticalSection;

   /* spinlock enter/exit functions */
   /* Before altering the state of the queued buffers, a call to
   ** AcquireSpinLock needs to be made.  After modification, call
   ** ReleaseSpinLock.
   ** These functions will lock against BH handlers running, too.
   */
   SpinlockAccess             AcquireSpinLock;
   SpinlockAccess             ReleaseSpinLock;

   char* pMessagesIn[3];
   char* pMessagesOut[4];

   BOOL no_signal_available;

   PIRP                       pIrp;

   // char * vmalloc;

   /* V4L info */   
   enum v4l2_field            field;

   enum _rgb133_strm_state    streaming;

   unsigned int               queued:1;
   unsigned int               mmapped:1;

   /* Read capture */
   unsigned int               read_off;
   unsigned int               reading:1;

   /* Rate Data */
   struct rgb133_rate         rate;

   BOOL blocking;
   
#if defined(USE_SCALING_BUFFERS)
   /* Scaling */
   struct rgb133_unified_buffer*      scaling;
#endif
#endif
};

#endif /* RGB133_H */
