/* gkclock.c — pure jiffy arithmetic (T1 host-testable, see gktimer.h).
 *
 * Split out of gktimer.c so the conversion the countdown clock depends on
 * can be tested natively; gktimer.c is all AmigaOS calls and cannot be.
 */
#include "gktimer.h"

uint16_t gk_timer_seconds(uint16_t jiffies)
{
    return (uint16_t)(jiffies / GK_JIFFIES_PER_SECOND);
}
