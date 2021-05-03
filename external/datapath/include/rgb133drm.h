#ifndef __RGB133_SECURE__H
#define __RGB133_SECURE__H

typedef struct _rgb133drm {
      unsigned long EnableHDCP;
} rgb133drm;

LONG SecureMemoryTargetCount(VOID);

BOOLEAN GetLaneWidthAndLinkSpeed(PRGB133DEVAPI _rgb133dev, ERESOURCE *PCIResource, USHORT *LinkSpeed, USHORT *LinkWidth);

#endif /* __RGB133_SECURE__H */
