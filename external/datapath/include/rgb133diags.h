#ifndef __RGB133_DIAGS__H
#define __RGB133_DIAGS__H

#define COUNT_NAME_LENGTH 32

#define RGB133_DIAGS_UNINIT  0
#define RGB133_DIAGS_INIT    1

#define RGB133_DIAGS_NON_PCI 0
#define RGB133_DIAGS_PCI     1

#define NUM_PCI_COUNTS 16

typedef enum _eCountFmt {
   RGB133_COUNT_FMT_DECIMAL = 0,
   RGB133_COUNT_FMT_HEX,
   RGB133_COUNT_FMT_RATE,
   RGB133_COUNT_FMT_DIFFERENCE,
   RGB133_COUNT_FMT_FPGAVER,
} eCountFmt, *peCountFmt;

typedef struct _rgb133count {
   struct _rgb133count* prev;
   struct _rgb133count* next;
   struct _rgb133count* mprev;
   struct _rgb133count* mnext;
   unsigned int        CountOffset;
   unsigned int         bPCI;
   char                 CountName[COUNT_NAME_LENGTH+1];
   eCountFmt            eFmt;
   unsigned int*       pValue;
} rgb133count, *prgb133count;

typedef struct _rgb133DiagItem {
   char          CountName[COUNT_NAME_LENGTH];
   unsigned int dwCount;
} rgb133diagitem, *prgb133diagitem;

typedef struct _rgb133Diags {
      unsigned int   ItemSize;
      unsigned int   NumberToFetch;
      unsigned int   Offset;
      unsigned int   NumCounters;
      unsigned int   StartAddress;
      prgb133diagitem pDiagItems;
} rgb133Diags, *prgb133Diags;

#endif /* __RGB133_DIAGS__H */
