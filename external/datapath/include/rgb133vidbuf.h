/*
 * rgb133vidbuf.h
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

#ifndef RGB133VIDBUF_H_
#define RGB133VIDBUF_H_

#include <linux/videodev2.h>

#include "rgb133sg.h"
#include "rgb133win.h"
#include "rgb133debug.h"

#define RGB133_MAX_FRAME 4

enum rgb133_buffer_state {
   RGB133_BUF_NEEDS_INIT = 0x00,
   RGB133_BUF_PREPARED   = 0x01,
   RGB133_BUF_QUEUED     = 0x02,
   RGB133_BUF_ACTIVE     = 0x04,
   RGB133_BUF_DONE       = 0x08,
   RGB133_BUF_ERROR      = 0x10,
   RGB133_BUF_IDLE       = 0x20,
};

enum rgb133_buffer_mode {
   RGB133_BUF_READ = 0,
   RGB133_BUF_MMAP = 1,
   RGB133_BUF_UPTR = 2,
};

typedef enum _rgb133_strm_state {
   RGB133_STRM_NOT_STARTED = 0,
   RGB133_STRM_ON = 1,
   RGB133_STRM_STOPPED = 2,
   RGB133_STRM_OFF = 3,
   RGB133_STRM_UNKNOWN = 3,
} rgb133_streaming_state;

typedef enum
{
   RGB133_NOTIFY_RESERVED = 0,
   RGB133_NOTIFY_FRAME_CAPTURED = 1,
   RGB133_NOTIFY_NO_SIGNAL = 2,
   RGB133_NOTIFY_DMA_OVERFLOW = 3,
} rgb133_notify__t;

/*
** All the information which is needed to describe a buffer.
** This is only used in the streaming sense of the capture driver.
*/
struct rgb133_unified_buffer {
   /* Capture info */
   struct rgb133_handle *     h;
   int                        index;
   unsigned int               setup;
   int                        init;

   /* Buffer state */   
   enum rgb133_buffer_state   state;  /* videobuf_state */
   rgb133_notify__t           notify; /* dma notification */

   /* V4L2 info */
   enum v4l2_memory           memory;
   
   /* Buffer stats */
   int bsize;
   int boff;

   union {
      void* pMemory;          // Make the scatter gather generator access generic.
      void* pMMapMemory;      // )
      void* pUserMemory;      // ) Only one of these 3 is valid on a per-handle basis.  Only one *should* be used.
      void* pReadMemory;      // )
   };

   void* pMessageBuffer;

   /* Scatter gather info - extended on arrival of NV12 format */
   PMDL                       pMdlsPlanar[3];
   RGB133_DMA_PAGE_INFO       dma_info[3];
   RGB133_KERNEL_DMA          kernel_dma[3];
   void*                      pWinSGL[3];

   /* Timestamp info */
   sTimestamp  ACQ;
   sTimestamp  Q;
   sTimestamp  DMA;
   sTimestamp  DQ;

   /* Mode */
   KPROCESSOR_MODE            mode;

   /* Private */
   void*                      priv;

   unsigned long long         CapNum;
   u32                        seq;
   u32                        fieldId;

   /* Mock Windows IRP */
   PIRP                       pIrp;

   /* Buffer mmapped? */
   unsigned int               mmapped:1;
};


void rgb133_vm_open(struct vm_area_struct* vma);

void rgb133_q_uninit(struct rgb133_handle* h);

void* rgb133_buffer_alloc(struct rgb133_handle *h);

int rgb133_read_buffer_setup(struct rgb133_unified_buffer** ppBuf, char __user* data, unsigned int size,
      KPROCESSOR_MODE AccessMode);
int rgb133_userptr_buffer_setup(struct rgb133_unified_buffer** ppBuf, unsigned long data, unsigned int size,
      KPROCESSOR_MODE AccessMode);
void rgb133_read_buffer_free(struct rgb133_unified_buffer** ppBuf,
      KPROCESSOR_MODE AccessMode);
void rgb133_userptr_buffer_free(struct rgb133_unified_buffer** ppBuf,
      KPROCESSOR_MODE AccessMode);

int rgb133_mmap_buffer_setup(struct rgb133_handle* h, unsigned int* pCount, unsigned int* pSize);
int __rgb133_buffer_reqbuf_setup(struct rgb133_handle* h, unsigned int count, unsigned int size, enum v4l2_memory memory);
int rgb133_mmap_buffer_status(struct rgb133_handle* h, struct v4l2_buffer* b);
int __rgb133_buffer_mmap_mapper(struct rgb133_handle* h, struct vm_area_struct* vma);
int __rgb133_buffer_mmap_dma_setup(struct rgb133_unified_buffer** ppBuf);
void __rgb133_buffer_mmap_dma_free(struct rgb133_unified_buffer ** ppBuf);

PMDL rgb133_userptr_buffer_get_mdl(struct rgb133_unified_buffer** ppBuf, ULONG plane);

PIRP AllocAndSetupIrp(struct rgb133_handle* h);
void FreeIrp(PIRP pIrp);

/*
** caller api type get/set.
*/
BOOL rgb133_is_mapped(struct rgb133_handle* h);
void rgb133_set_mapped(struct rgb133_handle* h, BOOL value);
BOOL rgb133_is_mapped_buffer(struct rgb133_unified_buffer * b);
void rgb133_set_mapped_buffer(struct rgb133_unified_buffer * b, BOOL value);
BOOL rgb133_is_reading(struct rgb133_handle* h);
void rgb133_set_reading(struct rgb133_handle* h, BOOL value);
BOOL rgb133_is_streaming(struct rgb133_handle* h);
BOOL rgb133_streaming_valid(struct rgb133_handle* h);
BOOL rgb133_streaming_on(struct rgb133_handle* h);
void rgb133_set_streaming(struct rgb133_handle* h, BOOL value);
enum _rgb133_strm_state rgb133_get_streaming(struct rgb133_handle* h);

/*
** Buffer level get/set.
*/
BOOL rgb133_buffer_queued(struct rgb133_handle* h, struct v4l2_buffer *b);
void rgb133_buffer_set_queued(struct rgb133_handle* h, struct rgb133_unified_buffer * buf, BOOL value);

void rgb133_buffer_set_active(struct rgb133_handle* h, struct rgb133_unified_buffer * buf, BOOL value);

RGB133_KERNEL_DMA* rgb133_buffer_get_kernel_dma(struct rgb133_unified_buffer** ppBuf, ULONG plane);

void rgb133_enter_critical_section(struct rgb133_handle* h);
void rgb133_exit_critical_section(struct rgb133_handle* h);

#define LOCK_SPINLOCK      1
#define UNLOCK_SPINLOCK    LOCK_SPINLOCK
#define NO_LOCK_SPINLOCK   0
#define NO_UNLOCK_SPINLOCK NO_LOCK_SPINLOCK

void rgb133_acquire_spinlock(struct rgb133_handle* h, char * caller, int lock_action);
void rgb133_release_spinlock(struct rgb133_handle* h, char * caller, int unlock_action);

void rgb133_invalidate_buffers(struct rgb133_handle* h);

#endif /*RGB133VIDBUF_H_*/
