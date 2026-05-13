#include "lock_mgr.h"

#include <pthread.h>
#include <stdbool.h>

#include "bank.h"

/*
 * Acquires write locks on two accounts in ascending order of account ID.
 * This prevents deadlocks by ensuring a consistent lock ordering.
 */
bool lock_accounts_in_order(Account *acc1, Account *acc2) {
    if (!validate_accounts(acc1, acc2)) {
        return false;
    }

    // Lock the account with the smaller ID first
    if (acc1->account_id < acc2->account_id) {
        lock_account(acc1);
        lock_account(acc2);
    } else {
        lock_account(acc2);
        lock_account(acc1);
    }

    return true;
}

/*
 * Releases write locks on two accounts.
 * Called after lock_accounts_in_order.
 */
void unlock_accounts(Account *acc1, Account *acc2) {
    if (acc1 == NULL || acc2 == NULL) {
        return;
    }

    unlock_account(acc1);
    unlock_account(acc2);
}

/*
 * Acquires a write lock for a single account.
 * Used for deposit, withdrawal, and transfer operations.
 */
void lock_account(Account *acc) {
    if (acc == NULL) {
        return;
    }
    pthread_rwlock_wrlock(&acc->lock);
}

/*
 * Releases a write lock for a single account.
 */
void unlock_account(Account *acc) {
    if (acc == NULL) {
        return;
    }
    pthread_rwlock_unlock(&acc->lock);
}

/*
 * Acquires a read lock for a single account.
 * Used for balance checks where concurrent reads are allowed.
 */
void read_lock_account(Account *acc) {
    if (acc == NULL) {
        return;
    }
    pthread_rwlock_rdlock(&acc->lock);
}

/*
 * Releases a read lock for a single account.
 */
void read_unlock_account(Account *acc) {
    if (acc == NULL) {
        return;
    }
    pthread_rwlock_unlock(&acc->lock);
}

/*
 * Validates that two account pointers are valid and distinct.
 * Ensures we're locking two different accounts for transfers.
 */
bool validate_accounts(Account *acc1, Account *acc2) {
    if (acc1 == NULL || acc2 == NULL || acc1 == acc2) {
        return false;
    }
    return true;
}
