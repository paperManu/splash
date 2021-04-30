/*
 * rgb133kernel.h
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

#ifndef RGB133KERNELAPI_H_
#define RGB133KERNELAPI_H_

#include <linux/device.h>

#include "rgb133.h"
#include "rgb_api_types.h"

#ifndef RGB133_CONFIG_HAVE_PAGE_CACHE_RELEASE
#define page_cache_release(page) put_page(page)
#endif

bool KernelUseWorkLookup(void);

void* KernelVzalloc(unsigned int size);
void KernelVfree(void* ptr);
void* KernelKmalloc(size_t size, int flags);
void KernelKfree(const void* ptr);

void KernelInitWork(void* arg1, void* arg2, void* arg3);
void KernelPrepareWork(void* arg1, void* arg2, void* arg3);

int KernelLoadFirmware(const PFWENTRYAPI* pfw_entry, char* fw_name, PDEAPI _pDE);
int KernelGetFwSize(PFWENTRYAPI fw_entry);
char* KernelGetFwData(PFWENTRYAPI fw_entry);
void KernelReleaseFw(PFWENTRYAPI fw_entry);

PDRIVERDEVAPI KernelGetDriverDevice(struct rgb133_dev* rgb133);

void KernelAtomicSet(atomic_t* pAtomic, int value);
int KernelAtomicRead(const atomic_t* pAtomic);

void KernelGetCurrentTime(PTIMEVALAPI _pTv, unsigned long* pSec, unsigned long* pUsec);
void KernelDoGettimeofday(unsigned long* pSec, unsigned long* pUsec);

int KernelMemcmp(const void* s1, const void* s2, size_t n);

int KernelCopyFromUser(void* to, const __user void* from, unsigned long count);
int KernelCopyToUser(__user void* to, void* from, unsigned long count);

void KernelMutexInit(PMUTEXAPI pLock);
void KernelMutexLock(PMUTEXAPI pLock);
void KernelMutexUnlock(PMUTEXAPI pLock);

void KernelPreemptEnable( );
void KernelPreemptDisable( );

void KernelMemset(void* ptr, int value, unsigned int size);

void KernelAddTimer(PVOID _pTimer);
int KernelDelTimer(PVOID _pTimer);
BOOL KernelInitTimer(PVOID _pTimer, PVOID start_routine, PVOID data, unsigned int flags);
void KernelSetTimerExpires(PVOID _pTimer, BOOL use_jiffies, unsigned long value_jffs);

PMODULEAPI KernelGetModule( );
int KernelGetGFP_DMAFlag( );
unsigned long KernelGetPAGE_OFFSET( );

unsigned int KernelGetDeviceNodeNr(void* _dev, int channel);

void KernelGetTimestamp(struct rgb133_handle* h, int index, eTimestamp ts, unsigned long* pSec, unsigned long* pUsec);
void KernelSetTimestamp(struct rgb133_handle* h, int index, eTimestamp ts, unsigned long sec, unsigned long usec);

HANDLE KernelGetProcessId( );

BOOLEAN KernelGetEventState(PKEVENT pEvent);

#endif /*RGB133KERNELAPI_H_*/
