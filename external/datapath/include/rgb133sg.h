/*
 * rgb133sg.h
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

#ifndef RGB133SG_H_
#define RGB133SG_H_

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/scatterlist.h>
#include <linux/pci.h>
#endif /* __KERNEL__ */

#include "rgb133win.h"

typedef struct _rgb133_sg_element {
   __le64 src;
   __le64 dst;
   __le64 size;
} RGB133_SG_ELEMENT;

typedef struct _rgb133_kernel_dma {
   int                  page_count;
   struct page**        pMap;
   struct page**        pBouncemap;

   RGB133_SG_ELEMENT*   pSGArray;
   uint64_t             SG_handle;
   int                  SG_length;

   struct scatterlist*  pSGList;
} RGB133_KERNEL_DMA;

typedef struct _rgb133_dma_page_info {
   unsigned long uaddr;
   unsigned long first;
   unsigned long last;
   unsigned int  offset;
   unsigned int  tail;
   int           page_count;
} RGB133_DMA_PAGE_INFO;

void rgb133_get_page_info(RGB133_DMA_PAGE_INFO* dma_page,
         unsigned long first, unsigned long size);

int rgb133_kernel_dma_fill_sg_list(RGB133_KERNEL_DMA* dma,
         RGB133_DMA_PAGE_INFO* dma_page, int map_offset);

void rgb133_kernel_dma_fill_sg_array(RGB133_KERNEL_DMA* dma,
         unsigned long buffer_offset, unsigned int buffer_offset_2,
         unsigned int split);

struct rgb133_dev;

#ifdef __KERNEL__
void rgb133_kernel_dma_sync_for_device(struct pci_dev* pdev, struct scatterlist* pSGList, unsigned long page_count);
#endif /* __KERNEL__ */

int rgb133_init_kernel_dma(RGB133_DMA_PAGE_INFO* dma_info, RGB133_KERNEL_DMA* kernel_dma, PMDL pMdl, KPROCESSOR_MODE AccessMode);
void rgb133_uninit_kernel_dma(RGB133_DMA_PAGE_INFO* dma_info, RGB133_KERNEL_DMA* kernel_dma, PMDL pMdl, KPROCESSOR_MODE AccessMode);

#endif /*RGB133SG_H_*/
