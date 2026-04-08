#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>
#include <stdbool.h>

/*
 * Global simulation clock.
 * Shared across all threads.
 */
extern volatile int global_tick;

/*
 * Mutex protecting access to the global tick.
 */
extern pthread_mutex_t tick_lock;

/*
 * Condition variable used to notify threads
 * when the tick value changes.
 */
extern pthread_cond_t tick_changed;

/*
 * Controls whether the simulation is still running.
 * When set to false, the timer thread should stop.
 */
extern volatile bool simulation_running;

/*
 * Duration of one tick in milliseconds.
 * Configurable via CLI.
 */
extern int tick_interval_ms;

/*
 * Timer thread function.
 *
 * Behavior:
 * - Sleeps for tick_interval_ms
 * - Increments global_tick
 * - Broadcasts condition variable to wake waiting threads
 *
 * This function runs in a loop until simulation_running is false.
 */
void *timer_thread(void *arg);

/*
 * Blocks the calling thread until the global tick
 * reaches the specified target_tick.
 *
 * Uses condition variables to avoid busy waiting.
 */
void wait_until_tick(int target_tick);

/*
 * Initializes timer-related synchronization primitives.
 * Must be called before starting the timer thread.
 */
void init_timer();

/*
 * Cleans up timer-related resources.
 * Should be called before program exit.
 */
void destroy_timer();

#endif