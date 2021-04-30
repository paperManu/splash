/*
 * rgb133capapi.h
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

#ifndef RGB133CAPAPI_H_
#define RGB133CAPAPI_H_

#include <linux/kernel.h>

#include "rgb133.h"
#include "rgb133capparms.h"
#include "rgb133timestamp.h"
#include "rgb133vidbuf.h"
#include "rgb133colours.h"
#include "rgb133peekpoke.h"
#include "rgb133forcedetect.h"
#include "rgb133diags.h"
#include "rgb133edid.h"

#include "rgb_api_types.h"

#include "rgb133control.h"

#define GET_DEV_PARMS_CUR 0x01
#define GET_DEV_PARMS_MIN 0x02
#define GET_DEV_PARMS_MAX 0x04
#define GET_DEV_PARMS_DEF 0x08
#define GET_DEV_PARMS_DET 0x10

typedef enum _eCopyUser {
   RGB133_CAP_NO_COPY_USER = 0,
   RGB133_CAP_COPY_USER,
} eCopyUser;

#ifndef RGB133_USER_MODE
int CaptureOpen(struct rgb133_handle* h, PIRPAPI pIrp);
int CaptureClose(struct rgb133_handle* h);
#endif /* RGB133_USER_MODE */

PDEVPARMSAPI CaptureAllocDevParms(void);
void CaptureFreeDevParms(PDEVPARMSAPI p);

int CaptureGetParameters(struct rgb133_dev* dev, int channel, int capture, unsigned long inFlags, PGETPARMSOUTAPI pOut);
int CaptureSetParameters(struct rgb133_dev* dev, int channel, int capture, PSETPARMSINAPI pIn);

unsigned long CaptureGetParameterCurrent(struct rgb133_handle* h, rgb133_cap_parms cap_parm);
unsigned long CaptureGetParameterMinimum(struct rgb133_handle* h, rgb133_cap_parms cap_parm);
unsigned long CaptureGetParameterMaximum(struct rgb133_handle* h, rgb133_cap_parms cap_parm);
unsigned long CaptureGetParameterDefault(struct rgb133_handle* h, rgb133_cap_parms cap_parm);
unsigned long __CaptureGetParameter(struct rgb133_handle* h, unsigned long inFlags, rgb133_cap_parms cap_parm);
int CaptureSetParameter(struct rgb133_handle* h, rgb133_cap_parms cap_parm, int value);
int CaptureSetVideoTimingParameter(struct rgb133_handle* h, rgb133_cap_parms cap_parm, int value);
int CaptureSetColourBalanceParameter(struct rgb133_handle* h, rgb133_cap_parms cap_parm, int value);

int CaptureGetSetCurrent(struct rgb133_handle* h, bool force);
int CaptureGetSetVideoTimings(struct rgb133_handle* h);
int CaptureGetSetDefaultVideoTimings(struct rgb133_handle* h);

BOOLEAN CaptureGangingSupported(struct rgb133_dev* dev, int channel);
BOOLEAN CaptureGangingTypeSupported(struct rgb133_dev* dev, unsigned long value);
int CaptureGetGangingType(struct rgb133_handle* h);
int CaptureSetGangingType(PDEAPI _pDE, unsigned long channel, unsigned long value);

#ifndef RGB133_USER_MODE
int CaptureEnable(struct rgb133_handle* h);
int CaptureReportVideoParameters(struct rgb133_handle* h);
int __CaptureReportVideoParameters(struct rgb133_handle* h, PVIDPARMSOUTAPI pVidParmsOut);
#endif /* RGB133_USER_MODE */

void CaptureResolveAspectRatio(unsigned long ar, unsigned int SourceWidth, unsigned int SourceHeight,
      unsigned int pComposeWidth, unsigned int pComposeHeight,
      int* pNumerator, int* pDenominator);
void CaptureAdjustScaleValues(int numerator, int denominator, int SourceWidth,
      int SourceHeight, int CaptureWidth, int CaptureHeight, int* _pWidthScaled,
      int* _pHeightScaled, unsigned int alignMask, unsigned int PixelFormatLinux);
int CaptureCalculateScalingAndCropping(struct rgb133_handle* h,
      unsigned int ScalingMode, unsigned int ScalingAR,
      unsigned int SourceWidth, unsigned int SourceHeight,
      unsigned int CaptureWidth, unsigned int CaptureHeight,
      unsigned int* pCropTop, unsigned int* pCropLeft,
      unsigned int* pCropBottom, unsigned int* pCropRight,
      unsigned int* pBufferOffset, unsigned int bpp,
      unsigned int alignMask, unsigned int PixelFormatLinux,
      unsigned int* pComposeLeft, unsigned int* pComposeTop,
      unsigned int* pComposeWidth, unsigned int* pComposeHeight,
      bool centre_image);
int CaptureSetScaleAndAOI(struct rgb133_handle* h,
      unsigned int SourceHeight, unsigned int SourceWidth,
      unsigned int ScaledHeight, unsigned int ScaledWidth,
      unsigned int CropTop, unsigned int CropLeft,
      unsigned int CropBottom, unsigned int CropRight);
int CaptureGetMinMaxCropping(struct rgb133_handle* h, rgb133_cap_parms_aoi* pMinAOI, rgb133_cap_parms_aoi* pMaxAOI);
int CaptureGetCropping(struct rgb133_handle* h,
      int* pTop, int* pLeft, int* pWidth, int* pHeight);
int CaptureSetCropping(struct rgb133_handle* h,
      int Top, int Left, int Width, int Height);
int CaptureSetImageComposition(struct rgb133_handle* h,
      int* pLeft, int* pTop, int* pWidth, int* pHeight);
void CaptureDumpVideoTimings(struct rgb133_handle* h, PCHANNELAPI pChannelParms, PVDIFKAPI pDetVdifApi);
void CaptureDumpVideoTimingsFromDeviceParms(struct rgb133_handle* h, PGETPARMSOUTAPI pParmsOut);
void CaptureCalculatePitchOffsets(unsigned int* pBufferOffset,
      unsigned int compose_left, unsigned int compose_top,
      unsigned int cap_width, unsigned int bpp,
      unsigned int alignMask, unsigned int PixelFormatLinux);
int CaptureRedetectAndSetVideoParams(struct rgb133_handle* h);
int CaptureSetPixelFormat(struct rgb133_handle* h, unsigned int PixelFormat);

int CaptureGetVidCapFormat(struct rgb133_handle* h, PV4L2FMTAPI f);

eVWSignalType CaptureMapSignalTypeToVW(struct rgb133_handle* h, int channel,
                                     unsigned long flags, unsigned long Format);
eVWVidStd CaptureMapVideoStandardToVW(unsigned short rgb133VidStd);

int CaptureGetVWSignalType(struct rgb133_handle* h);
int CaptureGetVideoStandard(struct rgb133_handle* h);

void CaptureCopyDeviceData(sVWDeviceParms* deviceParms, PDEVPARMSAPI _pDeviceParms);
PGETPARMSOUTAPI CaptureGetDeviceParametersReturnDeviceParms(struct rgb133_dev* dev,
      sVWDeviceParms* curDeviceParms, sVWDeviceParms* minDeviceParms,
      sVWDeviceParms* maxDeviceParms, sVWDeviceParms* defDeviceParms,
      sVWDeviceParms* detDeviceParms, int channel);
int CaptureGetDeviceParameters(struct rgb133_dev* dev,
      sVWDeviceParms* curDeviceParms, sVWDeviceParms* minDeviceParms,
      sVWDeviceParms* maxDeviceParms, sVWDeviceParms* defDeviceParms,
      sVWDeviceParms* detDeviceParms, int channel, PGETPARMSOUTAPI _DeviceParmsOut);
int CaptureSetDeviceParameters(struct rgb133_dev* dev, int channel, int capture, sVWDeviceParms* pDeviceParms);

int CaptureShowMessage(struct rgb133_handle* h, char* message, char* output_buffer, char* input_buffer,
      size_t count, eCopyUser copy);
int __CaptureShowMessage(struct rgb133_handle* h, char** ppMessageTxt, int messages, char* output_buffer, char* input_buffer,
      size_t count, eCopyUser copy);

void CaptureSetControls(struct rgb133_handle* h);
void CaptureSetDynamicControls(struct rgb133_handle* h);
void CaptureGetBrightnessLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef);
void CaptureGetContrastLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef);
void CaptureGetSaturationLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef);
void CaptureGetHueLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef);
void CaptureGetBlackLevelLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef);
void CaptureGetHorTimeLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef, PDEVPARMSAPI _pDetect);
void CaptureGetVerTimeLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef, PDEVPARMSAPI _pDetect);
void CaptureGetHorPosLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef, PDEVPARMSAPI _pCurrent, PDEVPARMSAPI _pDetect);
void CaptureGetHorSizeLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef, PDEVPARMSAPI _pCurrent, PDEVPARMSAPI _pDetect);
void CaptureGetPhaseLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef);
void CaptureGetVerPosLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef, PDEVPARMSAPI _pCurrent, PDEVPARMSAPI _pDetect);
void CaptureGetRedBrightnessLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef);
void CaptureGetRedContrastLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef);
void CaptureGetGreenBrightnessLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef);
void CaptureGetGreenContrastLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef);
void CaptureGetBlueBrightnessLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef);
void CaptureGetBlueContrastLimits(PDEAPI pDE, int* pMin, int* pMax, int* pDef);

PCHANNELAPI CaptureGetChannelPtr(struct rgb133_handle* h);
void CaptureSetLinuxCapture(struct rgb133_handle *h);
struct rgb133_handle * CaptureGetLinuxCapture(PRGBCAPTUREAPI pRGBCaptureAPI);

bool CaptureIsBadSignal(struct rgb133_handle* h, int channel);
unsigned long CaptureGetDetectedFlags(struct rgb133_handle* h, int channel, int* pError);

unsigned long CaptureGetVidMeasFlags(struct rgb133_handle* h);
const char* CaptureGetVidMeasString(struct rgb133_handle* h, int channel, int* pError);

BOOL CaptureVidMeasIsInterlaced(unsigned long flags);
BOOL CaptureClientInterlaced(struct rgb133_handle* h);
int CaptureGetClientInterlaced(struct rgb133_handle* h, unsigned long* _pType,
      unsigned int* _pSeq, unsigned int* _pField, unsigned int* _pFieldId);
void CaptureSetClientInterlaced(struct rgb133_handle* h, u32* pField);
void CaptureSetDeinterlaceBob(PSETPARMSINAPI pSetParmsIn);
void CaptureSetDeinterlaceAlternate(PSETPARMSINAPI pSetParmsIn);
void CaptureSetDeinterlaceField0(PSETPARMSINAPI pSetParmsIn);
void CaptureSetDeinterlaceField1(PSETPARMSINAPI pSetParmsIn);
void CaptureSetDeinterlaceFrame(PSETPARMSINAPI pSetParmsIn);

//unsigned long CaptureGetVDIFFlag(unsigned long flag_mask);

void CaptureRequestForData(struct rgb133_handle* h, struct rgb133_unified_buffer *buf);
void CaptureWaitForData(struct rgb133_handle* h);
PDMAIDLEAPI CaptureGetDMAIdleEvent(PRGBCAPTUREAPI _pRGBCapture);
BOOLEAN CaptureGetDMAIdleEventState(PRGBCAPTUREAPI _pRGBCapture);
void CaptureSetDMAIdleEvent(struct rgb133_handle* h);

NTSTATUS CaptureWaitForMultiBufferEvent(struct rgb133_handle* h);
PMBAPI CaptureGetMultiBufferEvent(PRGBCAPTUREAPI _pRGBCapture);
BOOLEAN CaptureGetMultiBufferEventState(PRGBCAPTUREAPI _pRGBCapture);
void CaptureSetMultiBufferEvent(struct rgb133_handle* h, unsigned int increment);
void CaptureClearMultiBufferEvent(struct rgb133_handle* h);

PCLOSEALLOWEDAPI CaptureGetCloseAllowedEvent(PRGBCAPTUREAPI pRGBCapture);
void CaptureSetCloseAllowedEvent(struct rgb133_handle* h);

void CaptureWaitForSignalTransition(struct rgb133_handle* h);
void CaptureSetSignalTransition(struct rgb133_handle* h);
void CaptureClearSignalTransition(struct rgb133_handle* h);
void CaptureWaitForEventPort(struct rgb133_handle* h);

int CaptureGetNumPixelFmts(void);

PDEVICEAPI CaptureGetDevicePtr(PRGBCAPTUREAPI _pRGBCapture);
PRGBCAPTUREAPI CaptureGetCapturePtr(struct rgb133_handle* h);
int CaptureGetDeviceIndex(PRGBCAPTUREAPI _pRGBCapture);
int CaptureGetChannelNumber(PRGBCAPTUREAPI _pRGBCapture);
int CaptureGetCaptureNumber(PRGBCAPTUREAPI _pRGBCapture);

void CaptureInitBuffer(struct rgb133_handle* h, char* data, char* buffer, int bsize,
      unsigned int pixelformat, enum __rgb133_colour colour, eCopyUser eCopy);
void CaptureInitBufferAndCopyUser(struct rgb133_handle* h, char* data, int bsize, int pixelformat, enum __rgb133_colour colour);

void CaptureAddressPeek(struct rgb133_dev* dev, unsigned long regOffset, unsigned long bRegister, unsigned int* pValue);
void CaptureGetBoardType(struct rgb133_dev* dev, unsigned long* pType);
int CaptureWriteFlashImage(struct rgb133_dev* dev, unsigned char* pData, unsigned int dataSize);
int CaptureReadFlashImage(struct rgb133_dev* dev, unsigned char* pData, unsigned int* dataSize);

//BOOL CaptureSeenValidSignal(struct rgb133_handle* h, int channel, int capture);
unsigned long CaptureRateSet(struct rgb133_handle* h);
unsigned long CaptureGetInputRate(struct rgb133_handle* h);
BOOL CaptureNoSignalFramePending(struct rgb133_handle* h);
VOID CaptureSetNoSignalFramePending(struct rgb133_handle* h);
VOID CaptureClearNoSignalFramePending(struct rgb133_handle* h);
BOOL CaptureNoSignalFrameAvailable(struct rgb133_handle* h);
VOID CaptureSetNoSignalFrameAvailable(struct rgb133_handle* h);
VOID CaptureClearNoSignalFrameAvailable(struct rgb133_handle* h);
BOOL CaptureSignalFramePending(struct rgb133_handle* h);
VOID CaptureSetSignalFramePending(struct rgb133_handle* h);
VOID CaptureClearSignalFramePending(struct rgb133_handle* h);
BOOL CaptureStartNoSignalTimer(struct rgb133_handle* h, unsigned long rate);

BOOL CaptureHaveOutstandingDMA(struct rgb133_handle* h);

int CaptureGetStreamingParms(struct rgb133_handle* h, PV4L2CAPPARMAPI pCapParm);
int CaptureSetStreamingParms(struct rgb133_handle* h, PV4L2CAPPARMAPI pCapParm);

void CaptureSetEquilisation(struct rgb133_handle* h);

unsigned int CaptureGetRGBFlip(struct rgb133_handle* h);
void CaptureSetRGBFlip(struct rgb133_handle* h, unsigned long FlipRGB);
void CaptureSetPixelRange(struct rgb133_handle* h, unsigned long FullRange);

#define RGB133_HW_TICK_TIMER_TIMEOUT 5000

void CaptureClockResetPassiveTimer(PTIMERAPI pTimer);
void CaptureClockResetPassiveThread(PVOID pParam);
void CaptureSetTimestamp(PRGBCAPTUREAPI _pRGBCapture, int index, eTimestamp ts, unsigned long sec, unsigned long usec);


void CaptureSetDebugLevel(struct rgb133_handle* h, int level);

BOOL CaptureControlEnabled(struct rgb133_handle* h, unsigned int id);

unsigned int CapturePeekRegister(struct rgb133_handle* h, struct _srgb133_peek* psPeek);
unsigned int CapturePokeRegister(struct rgb133_handle* h, struct _srgb133_poke* psPoke);

unsigned long CaptureMapColourDomainToWin(unsigned long domainLinux);
unsigned long CaptureMapColourDomainFromWin(unsigned long domainWin);
int CaptureSetColourDomain(struct rgb133_handle* h, PDEAPI _pDE, int channel, unsigned long domain);

int CaptureGetDetectMode(struct rgb133_handle* h, int channel, unsigned long* pMode);
int CaptureSetDetectMode(struct rgb133_handle* h, PDEAPI _pDE, int channel, unsigned long mode);
int CaptureIoctlSetDetectMode(struct rgb133_handle* h, prgb133ForceDetect pfd);

int CaptureGetEdid(struct rgb133_handle* h, struct _rgb133EdidOps *pEdid);
int CaptureSetEdid(struct rgb133_handle* h, struct _rgb133EdidOps *pEdid);

int CaptureIoctlGetDiagNames(struct rgb133_handle* h, PDEAPI _pDE, prgb133Diags pDiags);
int CaptureIoctlGetDiag(struct rgb133_handle* h, PDEAPI _pDE, prgb133count pDiags);

void CaptureGetParmsSignalUnrecognisable(struct rgb133_handle* h, unsigned long* pVerFreq, unsigned long* pHorFreq, unsigned long* pHTotal,
                                        unsigned long* pActivePixels, unsigned long* pActiveLines, unsigned long* pLineDuration,
                                        unsigned long* pFrameDuration, unsigned long* pHSyncDuration, unsigned long* pVSyncDuration);

void CaptureFudgeBuffer(struct rgb133_handle* h);
void CaptureDumpBuffer(struct rgb133_handle* h);

unsigned long CaptureGetSizeIoctlIn();
unsigned long CaptureGetSizeIoctlOut();

BOOL CaptureIsEnabled(struct rgb133_handle* h);

#endif /*RGB133CAPAPI_H_*/
