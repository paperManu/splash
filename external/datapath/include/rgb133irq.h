/*
 * rgb133irq.h
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

#ifndef _RGB133IRQ_H
#define _RGB133IRQ_H

#include <linux/interrupt.h>

#ifdef RGB133_CONFIG_IRQ_HAS_PT_REGS
irqreturn_t rgb133_irq(int irq, void* dev_id, struct pt_regs*);
#else
irqreturn_t rgb133_irq(int irq, void* dev_id);
#endif

#endif /* _RGB133IRQ_H */
