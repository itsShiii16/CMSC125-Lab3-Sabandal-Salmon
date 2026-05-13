#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "bank.h"

/*
 * Size of the buffer pool.
 * Limits how many accounts can be loaded at once.
 */
#define BUFFER_POOL_SIZE 5

/*
 * Represents one slot in the buffer pool.
 *
 * account_id:
 *   ID of the loaded account
 *
 * data:
 *   Pointer to the actual account in the bank
 *
 * in_use:
 *   Indicates whether this slot is currently occupied
 *
 * pin_count:
 *   Number of active transactions currently using this loaded account.
 *   The slot is released only when the count returns to zero.
 */
typedef struct {
    int account_id;
    Account *data;
    bool in_use;
    int pin_count;
} BufferSlot;

/*
 * Represents the entire buffer pool.
 *
 * slots:
 *   Fixed-size array of buffer slots
 *
 * empty_slots:
 *   Semaphore tracking available slots
 *
 * full_slots:
 *   Semaphore tracking occupied slots
 *
 * pool_lock:
 *   Mutex protecting access to slots array
 *
 * stats (optional but useful for metrics):
 *   load_count: total loads performed
 *   unload_count: total unloads performed
 *   blocked_count: number of times a load had to wait for space
 *   peak_usage: max simultaneous slots used
 */
typedef struct {
    BufferSlot slots[BUFFER_POOL_SIZE];

    sem_t empty_slots;
    sem_t full_slots;

    pthread_mutex_t pool_lock;

    int load_count;
    int unload_count;
    int blocked_count;
    int current_usage;
    int peak_usage;
} BufferPool;

/*
 * Initializes the buffer pool.
 *
 * - Sets all slots to unused
 * - Initializes semaphores and mutex
 */
void init_buffer_pool(BufferPool *pool);

/*
 * Loads an account into the buffer pool.
 *
 * Behavior:
 * - Waits if no empty slot is available (producer)
 * - Finds an empty slot and marks it in use
 * - Updates usage statistics
 *
 * Blocks if the pool is full.
 */
void load_account(BufferPool *pool, Account *account);

/*
 * Unloads an account from the buffer pool.
 *
 * Behavior:
 * - Waits if no full slot exists (consumer)
 * - Finds the corresponding slot and frees it
 * - Updates usage statistics
 *
 * Blocks if the pool is empty.
 */
void unload_account(BufferPool *pool, int account_id);

/*
 * Checks whether an account is already in the buffer pool.
 *
 * Returns:
 * - true if found
 * - false otherwise
 */
bool is_account_loaded(BufferPool *pool, int account_id);

/*
 * Initializes all buffer slot fields.
 * (Helper function used internally or during setup)
 */
void init_buffer_slots(BufferPool *pool);

/*
 * Destroys buffer pool resources.
 *
 * - Cleans up semaphores and mutex
 */
void destroy_buffer_pool(BufferPool *pool);

#endif
