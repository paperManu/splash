#ifndef __RGB133_PEEKPOKE__H
#define __RGB133_PEEKPOKE__H

typedef struct _srgb133_peek
{
   int device;
   int subdevice;
   int address;
   int status;
   int data;
} srgb133_peek, *psrgb133_peek;

typedef struct _srgb133_poke
{
      int device;
      int subdevice;
      int address;
      int status;
      int data;
} srgb133_poke, *psrgb133_poke;

typedef struct _srgb133_dump
{
   int   offset;
   int   size;
   char* pData;
} srgb133_dump, *psrgb133_dump;

#ifdef __KERNEL__
int rgb133_peekpoke_peek(struct file* file, void* priv, struct _srgb133_peek* psPeek);
int rgb133_peekpoke_poke(struct file* file, void* priv, struct _srgb133_poke* psPoke);
int rgb133_peekpoke_dump(struct file* file, void* priv, struct _srgb133_dump* psDump);
#endif

#endif /* __RGB133_PEEKPOKE__H */
