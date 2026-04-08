#ifndef LOCK_MGR_H
#define LOCK_MGR_H

#include <pthread.h>
#include <stdbool.h>
#include "bank.h"

/*
 * Lock Manager Module
 *
 * This module is responsible for handling deadlock prevention
 * using lock ordering.
 *
 * Instead of acquiring locks arbitrarily, accounts are always
 * locked in a consistent order (ascending account ID).
 *
 * This eliminates the circular wait condition and prevents deadlocks.
 */

/*
 * Acquires write locks on two accounts in a consistent order.
 *
 * Parameters:
 * - acc1, acc2: pointers to the accounts involved
 *
 * Behavior:
 * - Determines ordering based on account_id
 * - Locks the lower ID first, then the higher ID
 *
 * Returns:
 * - true if locks are acquired successfully
 * - false if inputs are invalid
 */
bool lock_accounts_in_order(Account *acc1, Account *acc2);

/*
 * Releases write locks on two accounts.
 *
 * IMPORTANT:
 * Must be called after lock_accounts_in_order()
 * to ensure proper unlocking sequence.
 */
void unlock_accounts(Account *acc1, Account *acc2);

/*
 * Acquires a write lock for a single account.
 *
 * This is a simple wrapper to standardize locking behavior
 * across the system.
 */
void lock_account(Account *acc);

/*
 * Releases a write lock for a single account.
 */
void unlock_account(Account *acc);

/*
 * Acquires a read lock for a single account.
 *
 * Used for balance queries where concurrent reads are allowed.
 */
void read_lock_account(Account *acc);

/*
 * Releases a read lock for a single account.
 */
void read_unlock_account(Account *acc);

/*
 * Validates that two account pointers are safe to lock.
 *
 * Checks:
 * - Non-null pointers
 * - Distinct accounts (for transfer)
 *
 * Returns:
 * - true if valid
 * - false otherwise
 */
bool validate_accounts(Account *acc1, Account *acc2);

#endif