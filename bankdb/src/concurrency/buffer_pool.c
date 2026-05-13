#include "buffer_pool.h"

#include <stddef.h>

/*
 * Initializes all buffer slots to an empty state.
 */
void init_buffer_slots(BufferPool *pool) {
    if (pool == NULL) {
        return;
    }

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        pool->slots[i].account_id = -1;
        pool->slots[i].data = NULL;
        pool->slots[i].in_use = false;
    }
}

/*
 * Initializes the Phase 3 buffer pool structure without integrating it.
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
    pool->current_usage = 0;
    pool->peak_usage = 0;
}

/*
 * Loads an account into the first available stub buffer slot.
 */
void load_account(BufferPool *pool, Account *account) {
    if (pool == NULL || account == NULL) {
        return;
    }

    sem_wait(&pool->empty_slots);
    pthread_mutex_lock(&pool->pool_lock);

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (!pool->slots[i].in_use) {
            pool->slots[i].account_id = account->account_id;
            pool->slots[i].data = account;
            pool->slots[i].in_use = true;
            pool->load_count++;
            pool->current_usage++;
            if (pool->current_usage > pool->peak_usage) {
                pool->peak_usage = pool->current_usage;
            }
            break;
        }
    }

    pthread_mutex_unlock(&pool->pool_lock);
    sem_post(&pool->full_slots);
}

/*
 * Unloads an account from the stub buffer pool if it is present.
 */
void unload_account(BufferPool *pool, int account_id) {
    if (pool == NULL) {
        return;
    }

    sem_wait(&pool->full_slots);
    pthread_mutex_lock(&pool->pool_lock);

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (pool->slots[i].in_use && pool->slots[i].account_id == account_id) {
            pool->slots[i].account_id = -1;
            pool->slots[i].data = NULL;
            pool->slots[i].in_use = false;
            pool->unload_count++;
            pool->current_usage--;
            break;
        }
    }

    pthread_mutex_unlock(&pool->pool_lock);
    sem_post(&pool->empty_slots);
}

/*
 * Checks whether an account is currently present in the stub buffer pool.
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
 */
void destroy_buffer_pool(BufferPool *pool) {
    if (pool == NULL) {
        return;
    }

    sem_destroy(&pool->empty_slots);
    sem_destroy(&pool->full_slots);
    pthread_mutex_destroy(&pool->pool_lock);
}
