#ifndef __RGB133TIMER__H
#define __RGB133TIMER__H

#include "rgb133config.h"
#include "rgb_api_types.h"

typedef struct rgb133_timer_holder {
   struct timer_list  timer;
   void*              data;
} RGB133TIMERHOLDER, *PRGB133TIMERHOLDER;

#ifdef RGB133_CONFIG_HAVE_NEW_TIMERS_API
void rgb133_timer_setup(struct timer_list* pTimer, void (*function)(struct timer_list*), unsigned int flags);
void rgb133_clock_reset_passive_timer(struct timer_list* pTimer);
#else
void rgb133_init_timer(struct timer_list* pTimer);
void rgb133_setup_timer(struct timer_list* pTimer, void (*function)(unsigned long), unsigned long data);
void rgb133_clock_reset_passive_timer(unsigned long data);
#endif

int rgb133_mod_timer(PTIMERAPI _pTimer, unsigned long expires_ms);
void rgb133_del_timer(struct timer_list* pTimer);

#endif /* __RGB133TIMER__H */

