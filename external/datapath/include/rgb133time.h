/*
 * rgb133time.h
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

#ifndef RGB133TIME_H
#define RGB133TIME_H

#include <linux/delay.h>

#ifdef RGB133_CONFIG_HAVE_USLEEP_RANGE
#define __usleep_range(msecs, usecs) \
   do \
   { \
      usleep_range((msecs*1000)+usecs,(msecs*1000)+usecs); \
   } \
   while(0);
#else
#define __usleep_range(msecs, usecs) \
   do \
   { \
      msleep(msecs); \
      if(usecs) \
         udelay(usecs); \
   } \
   while(0);
#endif /* RGB133_CONFIG_HAVE_USLEEP_RANGE */

#define RGB133WAIT(msecs, usecs) \
   do \
   { \
      if(msecs) \
      { \
         if(msecs <= 45) \
         { \
            __usleep_range(msecs, usecs); \
         } \
         else \
         { \
            msleep(msecs); \
            if(usecs) \
               udelay(usecs); \
         } \
      } \
      else if(usecs) \
      { \
         udelay(usecs); \
      } \
   } \
   while(0);

#define RGB133BUSYWAIT(msecs, usecs) \
   do \
   { \
      if(msecs) \
      { \
         mdelay(msecs); \
         if(usecs) \
            udelay(usecs); \
      } \
      else if(usecs) \
      { \
         udelay(usecs); \
      } \
   } \
   while(0);

unsigned long TimeGetJiffies(void);
int TimeJiffiesToMsecs(unsigned long jiffies_in);

#endif /* RGB133TIME_H */
