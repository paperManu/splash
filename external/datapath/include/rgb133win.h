/*
 * rgb133win.h
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

#ifndef RGB133WIN_H
#define RGB133WIN_H

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#endif /* __KERNEL__ */

#include "rgb133status.h"
#include "rgb_windows_types.h"

#define TAG4( ch0, ch1, ch2, ch3 )  (   (DWORD)(BYTE)(ch0)         | ( (DWORD)(BYTE)(ch1) << 8  ) |    \
                                      ( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 )   )

// Tags for ExAllocatePoolWithTag
#define TagRGB1  TAG4('R','G','B','1') // FS0 VSync DPC
#define TagRGB2  TAG4('R','G','B','2') // FS1 VSync DPC
#define TagRGBA  TAG4('R','G','B','A') // Accelerated SGList
#define TagRGBB  TAG4('R','G','B','B') // Software scaling buffers (large).
#define TagRGBC  TAG4('R','G','B','C') // Complete DPC
#define TagRGBD  TAG4('R','G','B','D') // pDD
#define TagRGBE  TAG4('R','G','B','E') // DMA finished Event
#define TagRGBH  TAG4('R','G','B','H') // History, Sync History and LatencyLog
#define TagRGBL  TAG4('R','G','B','L') // spinLock
#define TagRGBM  TAG4('R','G','B','M') // Streaming No Signal buffer
#define TagRGBN  TAG4('R','G','B','N') // Pinfo
#define TagRGBO  TAG4('R','G','B','O') // composite SGList
#define TagRGBP  TAG4('R','G','B','P') // PRGBCapture
#define TagRGBQ  TAG4('R','G','B','Q') // Queue
#define TagRGBR  TAG4('R','G','B','R') // Ready DPC
#define TagRGBS  TAG4('R','G','B','S') // SoftReset DPC
#define TagRGBT  TAG4('R','G','B','T') // Timeout
#define TagRGBU  TAG4('R','G','B','U') // pTp
#define TagRGBV  TAG4('R','G','B','V') // Streaming structures.
#define TagRGBW  TAG4('R','G','B','W') // VSync WatchDog Timeout
#define TagRGBZ  TAG4('R','G','B','Z') // very temporary
#define TagHNDL  TAG4('H','N','D','L') // Handle table

/****************************************************************************
 * Windows Function Definitions                                             *
 ****************************************************************************/

/***** Executive Library (EX) Support Routine Abstractions - START *****/

/* Abstracted memory allocation
 * PoolType       - type of memory to be allocated (paged, non-paged)
 *                  only non-paged makes sense in the linux kernel
 * NumberfBytes   - how much to allocate
 * Tag            - specifiec to memory tag (do we need this in linux?)
 *                  ignored for the moment!?
 * returns        - NULL if insufficient memory is available,
 *                  otherwise a pointed to the allocated space
 */
PVOID ExAllocatePoolWithTag(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag);

/* Abstracted memory allocation - wrapper for above
 * PoolType       - type of memory to be allocated (paged, non-paged)
 *                  only non-paged makes sense in the linux kernel
 * NumberfBytes   - how much to allocate
 * returns        - NULL if insufficient memory is available,
 *                  otherwise a pointed to the allocated space
 */
PVOID ExAllocatePool(POOL_TYPE PoolType, SIZE_T NumberOfBytes);

/* Abstracted memory free
 * P     - The beginning addrsss of memory allocated by ExAllocatePoolWithTag  
 * Tag   - specifiec to memory tag (do we need this in linux?)
 */
VOID ExFreePoolWithTag(PVOID P, ULONG Tag);

/* Abstracted memory free - wrapper for above
 * P     - The beginning addrsss of memory allocated by ExAllocatePoolWithTag  
 */
VOID ExFreePool(PVOID P);

/* Abstracted atomic increment
 * count    - atomic value to be incremented
 * returns  - the incremented value
 */
UINT InterlockedIncrement(ATOMIC_LONG_T* count);

/* Abstracted atomic decrement
 * count    - atomic value to be decremented
 * returns  - the decremented value
 */
UINT InterlockedDecrement(ATOMIC_LONG_T* count);

/* Abstracted locked list_add
 * ListHead    - Pointer to the list head for the linked list
 * ListEntry   - Pointer to the caller-alocated entry to be inserted
 * Lock        - spinlock to be used
 * returns     - Pointer to the first entry in the list before the
 *               new entry was inserted.  If the list was empty the
 *               routine returns NULL
 */
PSLIST_ENTRY ExInterlockedPushEntrySList(PSLIST_HEADER ListHead,
               PSLIST_ENTRY ListEntry, PKSPIN_LOCK Lock);

/* Abstracted locked list_entry
 * ListHead - Pointer to the list head for the linked list
 * Lock     - spinlock to be used
 * returns  - Pointer to the first entry in the list.  If the list is
 *            empty it returns NULL
 */
PSLIST_ENTRY ExInterlockedPopEntrySList(PSLIST_HEADER ListHead,
               PKSPIN_LOCK Lock);

/* Abstracted list_init
 * SListHead   - Pointer to list_head struct to initialise
 */
VOID ExInitializeSListHead(PSLIST_HEADER SListHead);

/* Abstracted deletion of all entries in a list
 * ListHead - Pointer to the list head to be operate on
 */
PSLIST_ENTRY ExInterlockedFlushSList(PSLIST_HEADER ListHead);

/* List helper function, is the list empty
 * ListHead - Pointer to a list head
 * returns  - true if list is empty, otherwise false
 */

/* Abstracted initialise resource
 * Resource - Pointer to resource to initialise
 * returns  - STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
 */
NTSTATUS ExInitializeResourceLite(PERESOURCE Resource);

/* Abstracted delete resource
 * Resource - Pointer to resource to be deleted
 * returns  - STATUS_SUCCESS if resource was deleted
 */
NTSTATUS ExDeleteResourceLite(PERESOURCE Resource);

/* Abstracted reinitialise resource
 * Resource - Pointer to resource to reinitialise
 * returns  - STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES
 */
NTSTATUS ExReinitializeResourceLite(PERESOURCE Resource);

/* Abstracted acquire resource for exclusive access
 * Resource - Pointer to resource to acquire
 * Wait     - Specifies routine's behaviour whenever resource cannot be acquired immediately
 *            If TRUE, caller is put into a wait state until resource can be acquired
 *            If FALSE, routine immediately returns, regardless of whether resource can be acquired
 * returns  - TRUE if resource is acquired
 *            FALSE if input Wait is FALSE and exclusive access cannot be granted immediately
 */
BOOLEAN ExAcquireResourceExclusiveLite(PERESOURCE Resource, BOOLEAN Wait);

/* Abstracted acquire resource for shared access
 * Resource - Pointer to resource to acquire
 * Wait     - Specifies routine's behavior whenever resource cannot be acquired immediately
 *            If TRUE, caller is put into a wait state until resource can be acquired
 *            If FALSE, routine immediately returns, regardless of whether resource can be acquired
 * returns  - TRUE if resource is acquired
 *            FALSE if argument Wait is FALSE and exclusive access cannot be granted immediately
 */
BOOLEAN ExAcquireResourceSharedLite(PERESOURCE Resource, BOOLEAN Wait);

/* Abstracted release acquired resource
 * Resource - Pointer to resource to release
 */
VOID ExReleaseResourceLite(PERESOURCE Resource);

/***** Executive Library (EX) Support Routine Abstractions - END   *****/


/***** Core Kernel Library (KE) Support Routine Abstractions - START *****/

/* Abstracted spinlock_t initialisation
 * SpinLock - spinlock to be initialised (allocated by caller)
 */
VOID KeInitializeSpinLock(PVOID _SpinLock);

/* Abstracted spin_lock_irqsave
 * SpinLock - The spin lock to be operated on
 * Flags    - IRQ Flags to be saved
 */
VOID KeAcquireSpinLock(PVOID _SpinLock, PULONG pFlags);

/* Abstracted spin_lock - used inside an isr
 * SpinLock - The spin lock to be operated on
 */
VOID KeAcquireSpinLockAtDpcLevel(PVOID _SpinLock);

/* Abstracted spin_lock - used inside an isr
 * SpinLock - The spin lock to be operated on
 */
KIRQL KeAcquireInterruptSpinLock(PKINTERRUPT pInterruptObject);

/* Abstracted spin_unlock_irqrestore
 * SpinLock - The spin lock to be operated on
 * Flags    - Pointer to the IRQ Flags to be restored
 */
VOID KeReleaseSpinLock(PVOID _SpinLock, ULONG Flags);

/* Abstracted spin_unlock - used inside an isr
 * SpinLock - The spin lock to be operated on
 */
VOID KeReleaseSpinLockFromDpcLevel(PVOID _SpinLock);

/* Abstracted spin_unlock - used inside an isr
 * SpinLock - The spin lock to be operated on
 */
VOID KeReleaseInterruptSpinLock(PKINTERRUPT pInterruptObject, KIRQL flags);

//LARGE_INTEGER KeQueryPerformanceCounter(PLARGE_INTEGER li);
unsigned long long KeQueryPerformanceCounter(PVOID arg);

/* Abstracted udelay
 * MicroSeconds - The number of micro-seconds to wait for
 */
VOID KeStallExecutionProcessor(ULONG MicroSeconds);

/* Abstracted udelay
 * WaitMode  - Processor mode
 * Alertable - Is the call alertable
 * Interval  - The number of 100ns unit to sleep for
 */
NTSTATUS KeDelayExecutionThread(KPROCESSOR_MODE WaitMode,
         BOOLEAN Alertable, PLARGE_INTEGER Interval);

/* Abstracted BUG_ON macro
 * !!! Will cause a kernel panic to avoid a corrupt system
 */
VOID KeBugCheckEx(ULONG BugCheckCode,
         ULONG_PTR BugCheckParameter1, ULONG_PTR BugCheckParameter2,
         ULONG_PTR BugCheckParameter3, ULONG_PTR BugCheckParameter4);

/* Abstracted workqueue init for DPC port
 * Dpc               - Pointer to kDPC structute
 * DeferredRoutine   - Pointer to custom DPC routine
 * DeferredContext   -  Value to pass to the DeferredContext
 */
VOID KeInitializeDpc(PKDPC Dpc, PKDEFERRED_ROUTINE DeferredRoutine, PVOID DeferredContext);

/* Abstracted schedule_work for Windows DPC port
 * Dpc               - Pointer to the DPC object
 * SystemArgument1   - Driver determined context data
 * SystemArgument2   - Driver determined context data
 * returns           - If
 */
BOOLEAN KeInsertQueueDpc(PKDPC Dpc, PVOID arg1, PVOID arg2);

/* Abstracted flush for Windows DPC port
 * Dpc   - Pointer to the DPC object
 */
BOOLEAN KeRemoveQueueDpc(PKDPC Dpc);

/* Enable/Disable preemption (if it is enabled)
 */
#define KeRaiseIrql(x, y)  KernelPreemptDisable( )
#define KeLowerIrql(x)     KernelPreemptEnable( )

/* Abstracted do_gettimeofday
 * CurrentTime - Pointer to the current time on return
 */
VOID KeQuerySystemTime(PLARGE_INTEGER CurrentTime);

/* Abstraction for obtaining the jiffies count
 * TickCount - Pointer to the jiffies value on return from
 *             KeQueryTickCount
 */
VOID KeQueryTickCount(PLARGE_INTEGER TickCount);

/* Abstraction for obtaining the number of 100ns units
 * between clock ticks
 */
ULONG KeQueryTimeIncrement(VOID);

/* Abstraction of init_waitqueue_head with some
 * initial data setting
 * Event - Pointer to the structure containing the
 *         wait queue
 * Type  - Notfication or Synchronization
 * State - The initial state
 */
VOID KeInitializeEvent(PKEVENT Event, EVENT_TYPE Type, BOOLEAN State);

/* Abstraction of data setting in the KEVENT struct
 * Event       - Pointer to the KEVENT struct
 * Increment   - Not used
 * Wait        - Followed by wait?
 */
LONG KeSetEvent(PKEVENT Event, KPRIORITY Increment, BOOLEAN Wait);

/* Abstraction of data clearing in the KEVENT struct
 * Event       - Pointer to the KEVENT struct
 */
LONG KeResetEvent(PKEVENT Event);

/* Abstraction of data clearing in the KEVENT struct
 * Event       - Pointer to the KEVENT struct
 */
VOID KeClearEvent(PKEVENT Event);

/* Abstraction of wait_event
 * Object      - KEVENT object
 * WaitReason  - User work, kernel work
 * WaitMode    - Kernel or User
 * Alertable   - Interruptible??
 * Timeout     - How long to wait for
 */
NTSTATUS KeWaitForSingleObject(PVOID Object, KWAIT_REASON WaitReason,
            KPROCESSOR_MODE WaitMode, BOOLEAN Alertable,
            PLARGE_INTEGER Timeout);

BOOL KeTestSingleObject(PVOID Object);

/* Dummy function
 */
NTSTATUS KeSaveFloatingPointState(KFLOATING_SAVE* arg);

/* Dummy function
 */
NTSTATUS KeRestoreFloatingPointState(KFLOATING_SAVE* arg);

/* Dummy function
 */
KIRQL KeGetCurrentIrql(void);

/* Dummy function
 */
VOID KeFlushIoBuffers(PMDL, BOOLEAN ReadOperation, BOOLEAN DmaOperation);

/* Dummy function
 */
VOID KeEnterCriticalRegion(VOID);

/* Dummy function
 */
VOID KeLeaveCriticalRegion(VOID);

/* Dummy function
 */
PKTHREAD KeGetCurrentThread(void);

/***** Core Kernel Library (KE) Support Routine Abstractions - END   *****/


/***** Run Time Library (RTL) Abstractions - START *****/

/* Abstracted memset
 * Destination - Pointer to memory to be filled
 * Length      - Specifies the number of bytes to be filled
 * Fill        - Specifies the value to fill the memory
 */
VOID RtlFillMemory(VOID* Destination, SIZE_T Length, UCHAR Fill);

/* Abstracted memcpy
 * Destination - Pointer to the destination of the copy
 * Source      - Pointer to the memory to be copied
 * Length      - Specifies the number of bytes to be copied
 */
VOID RtlCopyMemory(VOID* Destination, const VOID* Source, SIZE_T Length);

/* Abstraction of memset with zero's
 * Destination - Pointer to memory to be filled
 * Length      - Specifies the number of bytes to be filled
 */
VOID RtlZeroMemory(VOID* Destination, SIZE_T Length);

/* Abstraction of obsolete memset with zero's
 * Destination - Pointer to memory to be filled
 * Length      - Specifies the number of bytes to be filled
 */
VOID RtlZeroBytes(PVOID Destination, SIZE_T Length);

/* Abstracted method of converting native tsc time
 * to Year, Month, Day etc (hopefully!!)
 * Time        - The current time
 * TimeFields  - structure pointer to fill in
 */
VOID RtlTimeToTimeFields(PLARGE_INTEGER Time, PTIME_FIELDS TimeFields);

/* Compare blocks of memory
 * Source1, Source2 - Blocks of memory to compare
 * Length           - Number of bytes to compare
 */
SIZE_T RtlCompareMemory(const PVOID* Source1, const PVOID* Source2, SIZE_T Length);

/* Initialise an empty unicode string
 * DestinationString - Pointer to string to be initialised
 * Buffer            - Pointer to caller allocated buffer to be used to contain a WCHAR string
 * BufferSize        - Length, in bytes of the buffer that Buffer points to
 */
VOID RtlInitEmptyUnicodeString(PUNICODE_STRING DestinationString,
      PWCHAR Buffer, USHORT BufferSize);

/* Initialise a unicode string
 * DestinationString - Pointer to string to be initialised
 * SourceString      - Optional pointer to null-terminated string to initialize the counted string
 */
VOID RtlInitUnicodeString(PUNICODE_STRING DestinationString,
      PWSTR SourceString);

/* List helper function, is the list empty
 * ListHead - Pointer to a list head
 * returns  - true if list is empty, otherwise false
 */
BOOLEAN IsListEmpty(PLIST_ENTRY ListHead);

/***** Run Time Library (RTL) Abstractions - END   *****/


/***** Driver Support Abstractions - START *****/

/* Abstracted INIT_LIST_HEAD
 * ListHead - Pointer to LIST_ENTRY struct that serves as the
 *            list header.
 */
VOID InitializeListHead(PLIST_ENTRY ListHead);

VOID InsertTailList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);

BOOLEAN RemoveEntryList(PLIST_ENTRY Entry);
PLIST_ENTRY RemoveHeadList(PLIST_ENTRY ListHead);
PLIST_ENTRY RemoveTailList(PLIST_ENTRY ListHead);
VOID InsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);
/***** Driver Support Abstractions - END   *****/

/***** Registry (REG) Abstractions - START *****/

/* Dummy Reg Open Key
 * Ignore all params and return TRUE.
 */
NTSTATUS RegOpenKeyEx(HANDLE hKey, WCHAR* lpSubKey, UINT ulOptions,
         DWORD samDesired, PHANDLE phKey);

/* Dummy Reg Query Value
 * Ignore all params and return TRUE.
 */
NTSTATUS RegQueryValueEx(HANDLE hKey, WCHAR* lpValueName, DWORD lpReserved,
         DWORD* lpType, BYTE* lpData, DWORD* lpcbData);

/* Dummy Reg Close Key
 * Ignore all params and return TRUE.
 */
NTSTATUS RegCloseKey(PHANDLE hKey);

/* Dummy Reg Enum Key
 * hKey - Pointer to array of strings
 * dwIndex - Array index
 * lpName - String to hold array name
 */
NTSTATUS RegEnumKey(PVOID pDE, HANDLE hKey, DWORD dwIndex, WCHAR* lpName, DWORD not_used);

/* Dummy RegSetValueEx
 * Ignore all params and return TRUE.
 */
NTSTATUS RegSetValueEx(HANDLE hKey, WCHAR *pszValueName, DWORD Unused, DWORD dwType, BYTE *pDataBuffer, DWORD dwBufferSize);

/***** Registry (REG) Abstractions - END   *****/

/***** IO Manager (IO) Abstractions - START *****/

/* Dummy abstraction
 */
VOID IoDisconnectInterrupt(PKINTERRUPT InterruptObject);

/* Abstracted PREPARE_WORK
 * NOT_USED - Not used
 * returns  - NULL (not used)
 */
PIO_WORKITEM IoAllocateWorkItem(PDEVICE_OBJECT pWI);

/* Abstracted queue work
 * IoWorkItem  - Not used
 * WorkerRoutine  - Pointer to the routine to run on the workqueue
 * QueueType      - Name of the workqueue to use
 * Context        - Driver specific info
 */
VOID IoQueueWorkItem(PIO_WORKITEM IoWorkItem,
      PIO_WORKITEM_ROUTINE WorkerRoutine, WORK_QUEUE_TYPE QueueType,
      PVOID Context);

/* Abstraction for clearing scheduled work
 */
VOID IoFreeWorkItem(PIO_WORKITEM pWI);

/* Abstraction for setup of a MemoryDescriptorList
 * VirtualAddress    - Pointer to the virtual address of the buffer
 * Length            - Length, in bytes, of the buffer
 * SecondaryBuffer   - SecondaryBuffer, is this Mdl to be chained to
 *                     existing Mdl structures.
 * ChargeQuota       - Not Used
 * Irp               - Irp, set if this Mdl is to be tagged with an Irp
 * returns           - The new Mdl
 */
PMDL IoAllocateMdl(PVOID VirtualAddress, ULONG Length, BOOLEAN SecondaryBuffer,
      BOOLEAN ChargeQuota, PIRP Irp);

/* Abstraction for tear down of a PAGE_STRUCT virtual memory struct
 * Mdl - Pointer to struct to be torn down
 */
VOID IoFreeMdl(PMDL Mdl);

/* Get a pointer to a DMA Adapter structure
 * PhysicalDeviceObject - 
 * DeviceDescription    - Description of the device
 * NumberOfMapRegisters - Pointer to, on output, the max number of registers
 *                        that the driver can allocate for any DMA transfer
 *                        operation
 * returns              - A pointer to a DMA_ADAPTEr object
 */
PDMA_ADAPTER IoGetDmaAdapter(PDEVICE_OBJECT PhysicalDeviceObject,
                             PDEVICE_DESCRIPTION DeviceDescription,
                             PULONG NumberOfMapRegisters);

/* Free pointer to a DMA Adapter structure
 * pDmaAdapter - The DMA_ADAPTER ptr to free.
 * returns     - None.
 */
VOID LinuxPutDmaAdapter(PDMA_ADAPTER pDmaAdapter);

/* Dummy function, does nothing
 * pIrp - Pointer to an IRP
 */
VOID IoMarkIrpPending(PIRP Irp);

/* Dummy function, does nothing
 * pIrp          - Pointer to an IRP
 * PriorityBoost - System defined constant by which to increment the run-time
 *                 priority of the original thread
 */
VOID IoCompleteRequest(PIRP pIrp, CCHAR PriorityBoost); 

NTSTATUS IoGetDeviceProperty(PDEVICE_OBJECT DeviceObject, DEVICE_REGISTRY_PROPERTY DeviceProperty,
                             ULONG BufferLength, PVOID PropertyBuffer, PULONG ResultLength);

/* Dummy function, does nothing
 * pIrp          - Pointer to Irp
 * Always returns false.
 */
BOOLEAN IoIs32bitProcess(PIRP pIrp);

#define PtrToPtr64

/* Abstraction for getting a pointer to the caller's I/O stack location from the specified IRP
 * Irp     - A pointer to the IRP
 * returns - A pointer to an IO_STACK_LOCATION structure that contains the I/O stack location for the driver
 */
PIO_STACK_LOCATION IoGetCurrentIrpStackLocation(_In_ PIRP Irp);

/***** IO Manager (IO) Abstractions - END   *****/

/***** Memory Manager (MM) Abstractions - START *****/

/* Abstraction for get_free_pages
 * MemoryDescriptorList - Pointer to the virtual memory buffer structure
 * AccessMode           - GFP_KERNEL, GFP_ATOMIC, etc
 * Operation            - Not used
 */
VOID MmProbeAndLockPages(PMDL MemoryDescriptorList, KPROCESSOR_MODE AccessMode,
      LOCK_OPERATION Operation);
      
VOID __MmProbeAndLockPages(PMDL MemoryDescriptorList, KPROCESSOR_MODE AccessMode,
      LOCK_OPERATION Operation);
      
/* Abstraction for free_pages
 * MemoryDescriptorList - Pointer to a description of the memory buffer
 */
VOID MmUnlockPages(PMDL MemoryDescriptorList);

#define NormalPagePriority GFP_KERNEL
PVOID MmGetSystemAddressForMdlSafe(PMDL Mdl, ULONG Priority);
VOID MmUnmapLockedPages(PVOID BaseAddress, PMDL Mdl);

/* Return the base virtual address of an Mdl buffer 
 * Mdl - Pointer to MDL
 */
PVOID MmGetMdlVirtualAddress(PMDL Mdl);

/* Return the byte count of the Mdl 
 * Mdl - Pointer to MDL
 */
ULONG MmGetMdlByteCount(PMDL Mdl);
/***** Memory Manager (MM) Abstractions - END   *****/

/***** Stream Class (Stream) Abstractions - START *****/
VOID StreamRegistryOverrides (PVOID pDE);

/***** Stream Class (Stream) Abstractions - END   *****/

/***** String Functions - START *****/
/* Wide string length */
int wcslen(const PWSTR wstr);

/* Wide string printk */
VOID wprintk(WCHAR* wstr);

/* Wide string compare against std char */
BOOLEAN wstrcmp(WCHAR* wcompare, const char* with);

/* Normal string copy to wide string */
VOID wstrcpy(char* src, WCHAR* dest, int Length);

/* Wide string copy to normal string */
VOID strwcpy(WCHAR* src, char* dest, int Length);

/* Wide string copy with length */
VOID wcsncpy(PWSTR dest, const PWSTR src, int n);

/***** String Functions - END   *****/

/***** Process and Thread Manager (Ps) Routines Abstractions - START *****/

/* Abstraction for getting current process id
 * MemoryDescriptorList - Pointer to a description of the memory buffer
 * returns  - process ID of the current thread's process */
HANDLE PsGetCurrentProcessId(void);

/***** Process and Thread Manager (Ps) Routines Abstractions - END *****/

/***** Windows Management Instrumentation (WMI) Routines Abstractions - START *****/

VOID WmiInitTemperatureAddresses(PVOID pDE);

NTSTATUS WmiRegisterDriver(PDRIVER_OBJECT pDriverObject,
                           PUNICODE_STRING pRegistryPath);

NTSTATUS WmiUnregisterDriver(PDRIVER_OBJECT pDriverObject);

NTSTATUS WmiRegisterDevice(PDRIVER_OBJECT pDriverObject,
                           PDEVICE_OBJECT pDeviceObject,
                           PDEVICE_OBJECT pPhysicalDeviceObject,
                           PVOID pDE);

NTSTATUS WmiUnregisterDevice(PDEVICE_OBJECT pDeviceObject,
                             PVOID pDE);

NTSTATUS WmiRaiseInputSignalEvent(PVOID pDE,
                                  ULONG instanceIndex);

/***** Windows Management Instrumentation (WMI) Routines Abstractions - END *****/

#endif /* RGB133WIN_H */
