/*
 * rgb133ioctl.h
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

#ifndef _RGB133IOCTL_H
#define _RGB133IOCTL_H

extern struct v4l2_queryctrl rgb133_qctrl_defaults[RGB133_NUM_CONTROLS];      

/* v4l2 ioctl handling definitions */
int rgb133_querycap(struct file*, void*, struct v4l2_capability*);
int rgb133_enum_fmt_vid_cap(struct file*, void*, struct v4l2_fmtdesc*);
int rgb133_g_fmt_vid_cap(struct file*, void*, struct v4l2_format*);
int rgb133_try_fmt_vid_cap(struct file*, void*, struct v4l2_format*);
int rgb133_s_fmt_vid_cap(struct file*, void*, struct v4l2_format*);
int rgb133_reqbufs(struct file*, void*, struct v4l2_requestbuffers*);
int rgb133_querybuf(struct file*, void*, struct v4l2_buffer*);
int rgb133_qbuf(struct file*, void*, struct v4l2_buffer*);
int rgb133_dqbuf(struct file*, void*, struct v4l2_buffer*);
#ifdef RGB133_CONFIG_HAVE_VIDIOC_S_STD_FIX
int rgb133_s_std(struct file*, void* priv, v4l2_std_id norm);
#else
int rgb133_s_std(struct file*, void* priv, v4l2_std_id* norm);
#endif
int rgb133_enum_input(struct file*, void*, struct v4l2_input*);
int rgb133_g_input(struct file*, void*, unsigned int*);
int rgb133_s_input(struct file*, void*, unsigned int);
int rgb133_queryctrl(struct file*, void*, struct v4l2_queryctrl*);
int rgb133_querymenu(struct file*, void*, struct v4l2_querymenu*);
int rgb133_g_ctrl(struct file*, void*, struct v4l2_control*);
int rgb133_s_ctrl(struct file*, void*, struct v4l2_control*);
int rgb133_streamon(struct file*, void*, enum v4l2_buf_type);
int rgb133_streamoff(struct file*, void*, enum v4l2_buf_type);
int rgb133_g_parm(struct file*, void*, struct v4l2_streamparm*);
int rgb133_s_parm(struct file*, void*, struct v4l2_streamparm*);


#ifdef RGB133_CONFIG_HAVE_CROP_API
int rgb133_cropcap(struct file*, void*, struct v4l2_cropcap*);
int rgb133_g_crop(struct file*, void*, struct v4l2_crop*);
#ifdef RGB133_CONFIG_S_CROP_IS_CONST
int rgb133_s_crop(struct file*, void*, const struct v4l2_crop*);
#else /* !RGB133_CONFIG_S_CROP_IS_CONST */
int rgb133_s_crop(struct file*, void*, struct v4l2_crop*);
#endif /* RGB133_CONFIG_S_CROP_IS_CONST */
#endif /* RGB133_CONFIG_HAVE_CROP_API */

#ifdef RGB133_CONFIG_HAVE_SELECTION_API
int rgb133_g_selection(struct file*, void*, struct v4l2_selection*);
int rgb133_s_selection(struct file*, void*, struct v4l2_selection*);
#endif /* RGB133_CONFIG_HAVE_SELECTION_API */

#ifdef RGB133_CONFIG_HAVE_ENUM_FRMAEINTERVALS
int rgb133_enum_framesizes(struct file* file, void* priv, struct v4l2_frmsizeenum* fsize);
int rgb133_enum_frameintervals(struct file* file, void* priv, struct v4l2_frmivalenum* fival);
#endif /* RGB133_CONFIG_HAVE_ENUM_FRMAEINTERVALS */
#ifdef RGB133_CONFIG_HAVE_VIDIOC_DEFAULT
#ifdef RGB133_CONFIG_HAVE_VIDIOC_DEFAULT_EXT
#ifdef RGB133_CONFIG_DEFAULT_IOCTL_UNSIGNED
long rgb133_default_ioctl(struct file*, void*, bool, unsigned int, void*);
#else
long rgb133_default_ioctl(struct file*, void*, bool, int, void*);
#endif
#else
long rgb133_default_ioctl(struct file*, void*, int, void*);
#endif /* RGB133_CONFIG_HAVE_VIDIOC_DEFAULT_EXT */
#endif /* RGB133_CONFIG_HAVE_VIDIOC_DEFAULT */

struct rgb133_handle;

void CompleteActiveDMABuffer(struct rgb133_handle *h, void *pRGBCapture, int index, rgb133_notify__t notify, u32 seq, u32 fieldId);
void CompleteActiveDMABuffers(struct rgb133_handle* h, int lock_action, rgb133_notify__t notify);

void StopDmaStreamingAndFreeBuffers(struct rgb133_handle * h);
int CountQueuedBuffers(struct rgb133_handle* h, int lock_action);
int CountActiveDMABuffers(struct rgb133_handle *, int locking);
int CountActiveOrDoneDMABuffers(struct rgb133_handle* h, int lock_action);

int ReportFPS(struct rgb133_handle* h);
unsigned long CalculateLostReadTime(struct rgb133_handle *h);

void DumpBufferStates(unsigned int dbgLvl, struct rgb133_handle * h, char * pMsg);

int NoSignalDrawMessage(struct rgb133_handle* h, struct rgb133_unified_buffer * buf, struct v4l2_buffer* b);
void NoSignalBufferWait(struct rgb133_handle* h);

void SignalTransition(struct rgb133_handle* h);
void NoSignalTransition(struct rgb133_handle* h);

#endif /* _RGB133IOCTL_H */
