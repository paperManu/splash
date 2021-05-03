/*
 * rgb133deviceapi.h
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

#ifndef RGB133CARDAPI_H_
#define RGB133CARDAPI_H_

#include "rgb133.h"
#include "rgb_windows_types.h"
#include "rgb_api_types.h"
#include "rgb133control.h"

#define DeviceIsControl(control) (control ? 1 : 0)

extern void* AllocMem(unsigned int size);
extern void FreeMem(PDATAAPI pData);

VOID
DGCDriverUnload (
   PDRIVER_OBJECT  pDriverObject );

void* DeviceAllocMemory(int* pSize);

void DeviceSetDevice(PDEAPI _pDE, struct rgb133_dev* dev);

int DeviceGetChannels(struct rgb133_dev* dev, struct rgb133_dev* ctrl);
int DeviceGetCurrentUsers(struct rgb133_dev* dev);
int DeviceGetUsersOnChannel(struct rgb133_dev* dev, int channel);

void DeviceInitializeHardware(PHW_STREAM_REQUEST_BLOCK pSrb);
void DeviceUninitializeHardware(PHW_STREAM_REQUEST_BLOCK pSrb);

void DeviceEnableAllInterrupts(PDEAPI pDE);
void DeviceDisableAllInterrupts(PDEAPI pDE);

void DeviceSetInterruptObject(PDEAPI _pDE, KINTERRUPT* rgb133_interruptObject);

PHANDLETABLEAPI DeviceAllocHandleTable(void);
void DeviceInitHandleTable(PHANDLETABLEAPI _HandleTable);
void DeviceCleanupHandleTable(void** pPtr);

unsigned long DeviceGetVHDLVersion(struct rgb133_dev* dev, sVWDeviceInfo* pDeviceInfo);
unsigned long DeviceGetVHDLVersionFromPDE(PDEAPI _pDE, sVWDeviceInfo* pDeviceInfo);
unsigned long DeviceGetVHDLMinVersion(PDEAPI _pDE);

void DeviceGetLinkID(struct rgb133_dev* dev, sVWDeviceInfo* pDeviceInfo);

struct rgb133_dev* DeviceGetRGB133(PDEAPI _pDE);
int DeviceGetIndex(PDEAPI _pDE);

void DeviceDriverEntry(void);
void DeviceDGCDriverUnload(void);

PKEVENTAPI DeviceGetSignalNotificationEvent(PDEAPI _pDE, int event);
VOID DeviceClearSignalNotificationEvent(PDEAPI _pDE, int event);
BOOLEAN DeviceSignalNotificationEventSignalled(PDEAPI _pDE, int event);

VOID DeviceLockMutex(struct rgb133_dev* dev);
VOID DeviceUnlockMutex(struct rgb133_dev* dev);

VOID DeviceIsFlashable(struct rgb133_dev* dev, int channel, int* pFlashable);
int DeviceIsInputOrdinalZero(PDEAPI _pDE);

BOOL DeviceHasFirmwareLoaded(PDEAPI _pDE);

DWORD DeviceGetAlignmentMask(PDEAPI _pDE);

unsigned int DeviceGetType(void* dev);
char* DeviceGetName(void* dev);
char* DeviceGetNode(void* dev);

PDESTDESCAPI DeviceGetPDDFromContext(PVOID _pContext);
ULONG DeviceGetPlaneFromContext(PVOID _pContext);

BOOL DeviceIsNiosAlive(PDEAPI _pDE, long* pErrorLinux);

VOID DeviceSetDeviceObject(PDEAPI _pDE, PIO_STACK_LOCATION irpStack);

const char* DeviceGetConnectorType(PDEAPI _pDE, int channel);
BOOLEAN DeviceIs10BitSupported(struct rgb133_handle* h);
BOOLEAN DeviceIsYV12Supported(struct rgb133_handle* h);

#endif /*RGB133CARDAPI_H_*/
