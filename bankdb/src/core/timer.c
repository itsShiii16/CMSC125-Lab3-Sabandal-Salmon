#include "timer.h"

#include <unistd.h>

/*
 * Global simulation clock state.
 */
volatile int global_tick = 0;
pthread_mutex_t tick_lock;
pthread_cond_t tick_changed;
volatile bool simulation_running = false;
int tick_interval_ms = 100;

/*
 * Initializes timer-related synchronization primitives.
 */
void init_timer() {
    global_tick = 0;
    simulation_running = false;

    pthread_mutex_init(&tick_lock, NULL);
    pthread_cond_init(&tick_changed, NULL);
}

/*
 * Timer thread:
 * sleeps for one tick interval, increments the global tick,
 * then wakes up all waiting transaction threads.
 */
void *timer_thread(void *arg) {
    (void) arg;

    while (simulation_running) {
        usleep(tick_interval_ms * 1000);

        pthread_mutex_lock(&tick_lock);
        global_tick++;
        pthread_cond_broadcast(&tick_changed);
        pthread_mutex_unlock(&tick_lock);
    }

    return NULL;
}

/*
 * Blocks the caller until the global tick reaches target_tick.
 * Uses a condition variable to avoid busy waiting.
 */
void wait_until_tick(int target_tick) {
    pthread_mutex_lock(&tick_lock);

    while (global_tick < target_tick) {
        pthread_cond_wait(&tick_changed, &tick_lock);
    }

    pthread_mutex_unlock(&tick_lock);
}

/*
 * Cleans up timer-related synchronization resources.
 */
void destroy_timer() {
    pthread_mutex_destroy(&tick_lock);
    pthread_cond_destroy(&tick_changed);
}