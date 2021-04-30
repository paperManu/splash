#ifndef __RGB133_EDID__H
#define __RGB133_EDID__H

typedef struct _rgb133EdidOps {
   unsigned long Bytes;          // Size of EDID
   char          Edid[256];      // The EDID (with room for extension block)
   unsigned long bGetDefault;    // ???
   unsigned long Channel;        // Which channel (Input) on the card to retrieve.
} rgb133EdidOps, *prgb133EdidOps;

#endif /* __RGB133_EDID__H */
