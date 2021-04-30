/*
 * rgb133status.h
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

#ifndef _RGB133WINSTATUS_H
#define _RGB133WINSTATUS_H

#include <linux/errno.h>

#if !defined(STATUS_SUCCESS)
#define STATUS_SUCCESS     0x00000000L
#endif /* !STATUS_SUCCESS */

#define NT_ERROR(x) ((x) != STATUS_SUCCESS)
#define NT_SUCCESS(x) ((x) == STATUS_SUCCESS) 

/* Match the Windows error codes as close as possible
 * to Linux errno equivalents */

/* From asm-generic/errno-base.h */
#define STATUS_NO_SUCH_FILE            -ENOENT     /*   2 */
#define STATUS_IO_DEVICE_ERROR         -EIO        /*   5 */
#define STATUS_NO_SUCH_DEVICE          -ENODEV     /*  19 */
#define STATUS_INVALID_PARAMETER       -EINVAL     /*  22 */
#define STATUS_TOO_MANY_NODES          -EMFILE     /*  24 */

/* From asm-generic/errno.h */
#define STATUS_WAS_LOCKED              -EDEADLK    /*  35 */
#define STATUS_NOT_IMPLEMENTED         -ENOSYS     /*  38 */
#define STATUS_NO_MORE_ENTRIES         -ENODATA+3  /*  58 */
#define STATUS_PENDING                 -ENODATA+2  /*  59 */
#define STATUS_NO_MATCH                -ENODATA+1  /*  60 */
#define STATUS_NO_DATA_DETECTED        -ENODATA    /*  61 */
#define STATUS_TIMEOUT                 -ETIME      /*  62 */
#define STATUS_INSUFFICIENT_RESOURCES  -ENOSR      /*  63 */
#define STATUS_BUFFER_TOO_SMALL        -ENOBUFS    /* 105 */
#define STATUS_CANCELLED               -ECANCELED  /* 125 */

#endif /* _RGB133WINSTATUS_H */

