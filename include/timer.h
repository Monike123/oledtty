#ifndef OLEDTTY_TIMER_H
#define OLEDTTY_TIMER_H

#include <signal.h>
#include "types.h"

u32 timer_now_ms(void);
void timer_sleep_ms(u32 ms, const volatile sig_atomic_t *running);

#endif /* OLEDTTY_TIMER_H */
