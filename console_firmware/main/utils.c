#include "utils.h"

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"


void feed_watchdog_timer(void)
{
    TIMERG0.wdtwprotect.wdt_wkey = TIMG_WDT_WKEY_VALUE;
    TIMERG0.wdtfeed.wdt_feed = 1;
    TIMERG0.wdtwprotect.wdt_wkey = 0;
}