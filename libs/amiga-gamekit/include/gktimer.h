/*
 * gktimer.h - jiffy clock for Amiga game ports
 *
 * Both the Battleship and Fujitzee upstreams express move timers in jiffies
 * and ask the platform how many there are per second. The two calls are a
 * matched pair: gamelogic.c only ever does
 *
 *     seconds_left = (maxJifs - getTime()) / getJiffiesPerSecond()
 *
 * so getJiffiesPerSecond() must report the unit getTime() counts in — NOT
 * the display refresh rate. See gktimer.c for why that distinction matters
 * on the Amiga and why display-mode detection would be a bug here.
 *
 * Games wrap these in their own util.c to satisfy the upstream platform API
 * (resetTimer / getTime / getJiffiesPerSecond).
 *
 * See docs/plan-track1c-fujitzee.md (Phase 0).
 */
#ifndef GKTIMER_H
#define GKTIMER_H

#include <stdint.h>

/*
 * Jiffies per second of the gamekit clock. Constant: gk_timer_elapsed()
 * counts dos.library DateStamp ticks, which are 1/50 s of real time on
 * every Amiga regardless of PAL/NTSC.
 */
#define GK_JIFFIES_PER_SECOND 50

/* Mark "now" as the zero point for gk_timer_elapsed(). */
void gk_timer_reset(void);

/* Jiffies since the last gk_timer_reset(), saturating at 0xFFFF. */
uint16_t gk_timer_elapsed(void);

/*
 * Convert elapsed jiffies to whole seconds. Provided so games express the
 * conversion once and cannot accidentally pair gk_timer_elapsed() with a
 * frame-rate constant.
 */
uint16_t gk_timer_seconds(uint16_t jiffies);

#endif /* GKTIMER_H */
