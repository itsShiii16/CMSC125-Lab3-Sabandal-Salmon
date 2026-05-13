#include "buffer_pool.h"

#include <errno.h>
#include <stddef.h>

/*
 * Initializes all buffer slots to an empty state.
 * Synchronization: caller must ensure no other thread is using the pool.
 */
void init_buffer_slots(BufferPool *pool) {
    if (pool == NULL) {
        return;
    }

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        pool->slots[i].account_id = -1;
        pool->slots[i].data = NULL;
        pool->slots[i].in_use = false;
        pool->slots[i].pin_count = 0;
    }
}

/*
 * Initializes the bounded buffer pool and its synchronization primitives.
 * Synchronization: semaphores bound capacity and the mutex protects slot state.
 */
void init_buffer_pool(BufferPool *pool) {
    if (pool == NULL) {
        return;
    }

    init_buffer_slots(pool);
    sem_init(&pool->empty_slots, 0, BUFFER_POOL_SIZE);
    sem_init(&pool->full_slots, 0, 0);
    pthread_mutex_init(&pool->pool_lock, NULL);

    pool->load_count = 0;
    pool->unload_count = 0;
    pool->blocked_count = 0;
    pool->current_usage = 0;
    pool->peak_usage = 0;
}

/*
 * Loads an account into the bounded buffer pool, blocking when full.
 * Synchronization: empty_slots enforces capacity, full_slots tracks occupied
 * slots, and pool_lock protects slot lookup, insertion, and pin counts.
 */
void load_account(BufferPool *pool, Account *account) {
    bool inserted = false;

    if (pool == NULL || account == NULL) {
        return;
    }

    pthread_mutex_lock(&pool->pool_lock);
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (pool->slots[i].in_use &&
            pool->slots[i].account_id == account->account_id) {
            pool->slots[i].pin_count++;
            pthread_mutex_unlock(&pool->pool_lock);
            return;
        }
    }
    pthread_mutex_unlock(&pool->pool_lock);

    if (sem_trywait(&pool->empty_slots) != 0) {
        if (errno == EAGAIN) {
            pthread_mutex_lock(&pool->pool_lock);
            pool->blocked_count++;
            pthread_mutex_unlock(&pool->pool_lock);
        }
        sem_wait(&pool->empty_slots);
    }
    pthread_mutex_lock(&pool->pool_lock);

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (pool->slots[i].in_use &&
            pool->slots[i].account_id == account->account_id) {
            pool->slots[i].pin_count++;
            pthread_mutex_unlock(&pool->pool_lock);
            sem_post(&pool->empty_slots);
            return;
        }
    }

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (!pool->slots[i].in_use) {
            pool->slots[i].account_id = account->account_id;
            pool->slots[i].data = account;
            pool->slots[i].in_use = true;
            pool->slots[i].pin_count = 1;
            pool->load_count++;
            pool->current_usage++;
            if (pool->current_usage > pool->peak_usage) {
                pool->peak_usage = pool->current_usage;
            }
            inserted = true;
            break;
        }
    }

    pthread_mutex_unlock(&pool->pool_lock);

    if (inserted) {
        sem_post(&pool->full_slots);
    } else {
        sem_post(&pool->empty_slots);
    }
}

/*
 * Releases one transaction's use of an account from the buffer pool.
 * Synchronization: pool_lock protects pin counts and slot state; when the final
 * pin is released, the occupied count is consumed and one empty slot is posted.
 */
void unload_account(BufferPool *pool, int account_id) {
    if (pool == NULL) {
        return;
    }

    pthread_mutex_lock(&pool->pool_lock);

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (pool->slots[i].in_use && pool->slots[i].account_id == account_id) {
            pool->slots[i].pin_count--;
            if (pool->slots[i].pin_count <= 0) {
                pool->slots[i].account_id = -1;
                pool->slots[i].data = NULL;
                pool->slots[i].in_use = false;
                pool->slots[i].pin_count = 0;
                pool->unload_count++;
                pool->current_usage--;
                pthread_mutex_unlock(&pool->pool_lock);
                sem_wait(&pool->full_slots);
                sem_post(&pool->empty_slots);
                return;
            }

            pthread_mutex_unlock(&pool->pool_lock);
            return;
        }
    }

    pthread_mutex_unlock(&pool->pool_lock);
}

/*
 * Checks whether an account is currently present in the buffer pool.
 * Synchronization: pool_lock protects the read of shared slot state.
 */
bool is_account_loaded(BufferPool *pool, int account_id) {
    bool loaded = false;

    if (pool == NULL) {
        return false;
    }

    pthread_mutex_lock(&pool->pool_lock);

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (pool->slots[i].in_use && pool->slots[i].account_id == account_id) {
            loaded = true;
            break;
        }
    }

    pthread_mutex_unlock(&pool->pool_lock);
    return loaded;
}

/*
 * Destroys synchronization resources owned by the buffer pool.
 * Synchronization: caller must ensure all transaction threads have completed.
 */
void destroy_buffer_pool(BufferPool *pool) {
    if (pool == NULL) {
        return;
    }

    sem_destroy(&pool->empty_slots);
    sem_destroy(&pool->full_slots);
    pthread_mutex_destroy(&pool->pool_lock);
}
