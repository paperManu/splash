#ifndef RGB133_TIMESTAMP__H
#define RGB133_TIMESTAMP__H

typedef enum _eTimestamp
{
   RGB133_LINUX_TIMESTAMP_ACQ,
   RGB133_LINUX_TIMESTAMP_Q,
   RGB133_LINUX_TIMESTAMP_DMA,
   RGB133_LINUX_TIMESTAMP_DQ,
} eTimestamp;

typedef struct _sTimestamp
{
   unsigned long sec;
   unsigned long usec;
} sTimestamp, *psTimestamp;

#endif /* RGB133_TIMESTAMP__H */
