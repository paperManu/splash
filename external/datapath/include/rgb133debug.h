/*
 * rgb133debug.h
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

#ifndef _RGB133DEBUG_H
#define _RGB133DEBUG_H

#ifndef __KERNEL__
#include <stdio.h>
#endif /* !__KERNEL__ */

#include <linux/delay.h>

/* Windows driver debug defines */
#define DBG 0

/* DEBUGGING SWITCH */
#define RGB133_LINUX_DEBUG

#ifdef RGB133_DEBUG_WAIT
extern int first_frame;
#define FRAMES_PER_SEC    2
#define FRAME_MULTIPLIER  1
#else
#define FRAMES_PER_SEC    60
#define FRAME_MULTIPLIER  1
#endif /* RGB133_DEBUG_WAIT */

#define fpsToJiffies(fps) ((FRAME_MULTIPLIER * HZ) / (fps))

/* DEBUG LEVELS */
#define RGB133_LINUX_DBG_OVERRIDE 0x3FFF

#define RGB133_LINUX_DBG_FAULT    0x80000
#define RGB133_LINUX_DBG_QUEUE    0x40000
#define RGB133_LINUX_DBG_INOUT    0x20000
#define RGB133_LINUX_DBG_MEM      0x10000
#define RGB133_LINUX_DBG_UNIQUE   0x08000
#define RGB133_LINUX_DBG_STUPID   0x04000
#define RGB133_LINUX_DBG_SPIN     0x00080
#define RGB133_LINUX_DBG_IRQ      0x00040
#define RGB133_LINUX_DBG_WORK     0x00020
#define RGB133_LINUX_DBG_DEBUG    0x00010
#define RGB133_LINUX_DBG_TRACE    0x00008
#define RGB133_LINUX_DBG_LOG      0x00004
#define RGB133_LINUX_DBG_WARNING  0x00002
#define RGB133_LINUX_DBG_ERROR    0x00001
#define RGB133_LINUX_DBG_NONE     0x00000

#define RGB133_LINUX_DBG_EVENTS   RGB133_LINUX_DBG_LOG
#define RGB133_LINUX_DBG_CORE     RGB133_LINUX_DBG_WARNING
#define RGB133_LINUX_DBG_TODO     RGB133_LINUX_DBG_ERROR
#define RGB133_LINUX_DBG_INIT     RGB133_LINUX_DBG_ERROR

/* Mux'ed levels */
#define RGB133_LINUX_DBG_L0       ( RGB133_LINUX_DBG_NONE )
#define RGB133_LINUX_DBG_L1       ( RGB133_LINUX_DBG_ERROR | RGB133_LINUX_DBG_WARNING )
#define RGB133_LINUX_DBG_L2       ( RGB133_LINUX_DBG_L1    | RGB133_LINUX_DBG_LOG )
#define RGB133_LINUX_DBG_L3       ( RGB133_LINUX_DBG_L2    | RGB133_LINUX_DBG_TRACE | RGB133_LINUX_DBG_DEBUG )
#define RGB133_LINUX_DBG_L4       ( RGB133_LINUX_DBG_L3    | RGB133_LINUX_DBG_WORK  | RGB133_LINUX_DBG_IRQ )
#define RGB133_LINUX_DBG_L5       ( RGB133_LINUX_DBG_L4    | RGB133_LINUX_DBG_SPIN )
#define RGB133_LINUX_DBG_L6       ( RGB133_LINUX_DBG_L1    | RGB133_LINUX_DBG_MEM )
#define RGB133_LINUX_DBG_L7       ( RGB133_LINUX_DBG_L2    | RGB133_LINUX_DBG_MEM )
#define RGB133_LINUX_DBG_L8       ( RGB133_LINUX_DBG_L3    | RGB133_LINUX_DBG_MEM )
#define RGB133_LINUX_DBG_L9       ( RGB133_LINUX_DBG_L4    | RGB133_LINUX_DBG_MEM )
#define RGB133_LINUX_DBG_L10      ( RGB133_LINUX_DBG_L5    | RGB133_LINUX_DBG_MEM )
#define RGB133_LINUX_DBG_L11      ( RGB133_LINUX_DBG_L6    | RGB133_LINUX_DBG_MEM )
#define RGB133_LINUX_DBG_L12      ( RGB133_LINUX_DBG_L1    | RGB133_LINUX_DBG_INOUT )
#define RGB133_LINUX_DBG_L13      ( RGB133_LINUX_DBG_L2    | RGB133_LINUX_DBG_INOUT )
#define RGB133_LINUX_DBG_L14      ( RGB133_LINUX_DBG_L3    | RGB133_LINUX_DBG_INOUT )
#define RGB133_LINUX_DBG_L15      ( RGB133_LINUX_DBG_L4    | RGB133_LINUX_DBG_INOUT )
#define RGB133_LINUX_DBG_L16      ( RGB133_LINUX_DBG_L5    | RGB133_LINUX_DBG_INOUT )
#define RGB133_LINUX_DBG_L17      ( RGB133_LINUX_DBG_L6    | RGB133_LINUX_DBG_INOUT )
#define RGB133_LINUX_DBG_L18      ( RGB133_LINUX_DBG_L1    | RGB133_LINUX_DBG_QUEUE )
#define RGB133_LINUX_DBG_L19      ( RGB133_LINUX_DBG_L17   | RGB133_LINUX_DBG_FAULT )
#define RGB133_LINUX_DBG_L20      ( RGB133_LINUX_DBG_L6    | RGB133_LINUX_DBG_INOUT )
#define RGB133_LINUX_DBG_L21      ( RGB133_LINUX_DBG_L7    | RGB133_LINUX_DBG_INOUT )
#define RGB133_LINUX_DBG_L22      ( RGB133_LINUX_DBG_L8    | RGB133_LINUX_DBG_INOUT )
#define RGB133_LINUX_DBG_L23      ( RGB133_LINUX_DBG_L9    | RGB133_LINUX_DBG_INOUT )
#define RGB133_LINUX_DBG_L24      ( RGB133_LINUX_DBG_L10   | RGB133_LINUX_DBG_INOUT )
#define RGB133_LINUX_DBG_L25      ( RGB133_LINUX_DBG_L11   | RGB133_LINUX_DBG_INOUT )

/* Comment in one line from below to turn on
 * various levels of debugging
 */
extern unsigned long rgb133_debug_mask;

#ifndef __KERNEL__
extern FILE* oFile;
#endif /* !__KERNEL__ */

/* RGB133 Debugging macro */
#ifdef RGB133_LINUX_DEBUG

#define RGB133PRINT(__x__) __RGB133PRINT __x__
#define RGB133PRINTNS(__x__) __RGB133PRINTNS __x__

#ifdef __KERNEL__
//#define __RGB133PRINT(level, fmt, arg...)
#ifdef INCLUDE_VISIONLC
#define __RGB133PRINT(level, fmt, arg...) \
   do \
   { \
      if((level) & rgb133_debug_mask) \
      { \
         printk(KERN_INFO "rgb200: " fmt, ## arg); \
      } \
   } while(0);
#define __RGB133PRINTNS(level, fmt, arg...) \
   do \
   { \
      if((level) & rgb133_debug_mask) \
      { \
         printk(KERN_INFO "rgb200: " fmt, ## arg); \
      } \
   } while(0);
#else /* INCLUDE_VISION */
#define __RGB133PRINT(level, fmt, arg...) \
   do \
   { \
      if((level) & rgb133_debug_mask) \
      { \
         printk(KERN_INFO "rgb133: " fmt, ## arg); \
      } \
   } while(0);
#define __RGB133PRINTNS(level, fmt, arg...) \
   do \
   { \
      if((level) & rgb133_debug_mask) \
      { \
         printk(KERN_INFO "rgb133: " fmt, ## arg); \
      } \
   } while(0);
#endif

#ifdef RGB133_REALLY_PANIC
#define RGB133_DEBUG_ASSERT(n) \
   do {\
      if(!(n))\
      {\
         panic("DGC133_DEBUG_ASSERT: %s - %d\n", __FILE__, __LINE__);\
      }\
   } while(0);
#else
#define RGB133_DEBUG_ASSERT(n)
#endif /* RGB133_REALLY_PANIC */
#else
//#define __RGB133PRINT(level, fmt, arg...)
#define __RGB133PRINT(level, fmt, arg...) \
   do \
   { \
      if((level) & rgb133_debug_mask) \
         if(oFile) \
            fprintf(oFile, "DGC133: " fmt, ## arg); \
         else \
            printf("DGC133: " fmt, ## arg); \
   } while(0);
#endif /* __KERNEL__ */
#else

#define RGB133PRINT(level, fmt, arg...)

#endif /* !RGB133_LINUX_DEBUG */

#endif /* RGB133DEBUG_H */
