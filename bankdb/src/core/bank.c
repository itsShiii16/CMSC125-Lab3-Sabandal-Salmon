#include "bank.h"

#include <stdio.h>

#include "lock_mgr.h"

/*
 * Initializes the bank structure.
 * Starts with zero accounts and initializes the bank-wide mutex.
 */
void init_bank(Bank *bank) {
    if (bank == NULL) {
        return;
    }

    bank->num_accounts = 0;
    pthread_mutex_init(&bank->bank_lock, NULL);

    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        bank->accounts[i].account_id = -1;
        bank->accounts[i].balance_centavos = 0;
    }
}

/*
 * Adds a new account to the bank.
 * Returns false if the bank is full or the account ID already exists.
 */
bool add_account(Bank *bank, int account_id, int initial_balance) {
    if (bank == NULL || account_id < 0 || initial_balance < 0) {
        return false;
    }

    pthread_mutex_lock(&bank->bank_lock);

    if (bank->num_accounts >= MAX_ACCOUNTS) {
        pthread_mutex_unlock(&bank->bank_lock);
        return false;
    }

    for (int i = 0; i < bank->num_accounts; i++) {
        if (bank->accounts[i].account_id == account_id) {
            pthread_mutex_unlock(&bank->bank_lock);
            return false;
        }
    }

    int idx = bank->num_accounts;
    bank->accounts[idx].account_id = account_id;
    bank->accounts[idx].balance_centavos = initial_balance;
    bank->num_accounts++;

    pthread_mutex_unlock(&bank->bank_lock);
    return true;
}

/*
 * Finds an account by ID.
 * Returns NULL if the account does not exist.
 */
Account *get_account(Bank *bank, int account_id) {
    if (bank == NULL || account_id < 0) {
        return NULL;
    }

    for (int i = 0; i < bank->num_accounts; i++) {
        if (bank->accounts[i].account_id == account_id) {
            return &bank->accounts[i];
        }
    }

    return NULL;
}

/*
 * Returns the current balance of one account.
 * Uses a read lock so multiple balance checks can run concurrently.
 */
int get_balance(Bank *bank, int account_id) {
    Account *acc = get_account(bank, account_id);
    if (acc == NULL) {
        return -1;
    }

    read_lock_account(acc);
    int balance = acc->balance_centavos;
    read_unlock_account(acc);

    return balance;
}

/*
 * Deposits money into one account.
 * Uses a write lock because the balance is modified.
 */
void deposit(Bank *bank, int account_id, int amount_centavos) {
    if (bank == NULL || amount_centavos < 0) {
        return;
    }

    Account *acc = get_account(bank, account_id);
    if (acc == NULL) {
        return;
    }

    lock_account(acc);
    acc->balance_centavos += amount_centavos;
    unlock_account(acc);
}

/*
 * Withdraws money from one account.
 * Returns false if the account is invalid or funds are insufficient.
 */
bool withdraw(Bank *bank, int account_id, int amount_centavos) {
    if (bank == NULL || amount_centavos < 0) {
        return false;
    }

    Account *acc = get_account(bank, account_id);
    if (acc == NULL) {
        return false;
    }

    lock_account(acc);

    if (acc->balance_centavos < amount_centavos) {
        unlock_account(acc);
        return false;
    }

    acc->balance_centavos -= amount_centavos;
    unlock_account(acc);

    return true;
}

/*
 * Transfers money from one account to another.
 * Deadlock prevention is handled by locking both accounts in a fixed order.
 */
bool transfer(Bank *bank, int from_id, int to_id, int amount_centavos) {
    if (bank == NULL || amount_centavos < 0 || from_id == to_id) {
        return false;
    }

    Account *from_acc = get_account(bank, from_id);
    Account *to_acc = get_account(bank, to_id);

    if (!validate_accounts(from_acc, to_acc)) {
        return false;
    }

    if (!lock_accounts_in_order(from_acc, to_acc)) {
        return false;
    }

    if (from_acc->balance_centavos < amount_centavos) {
        unlock_accounts(from_acc, to_acc);
        return false;
    }

    from_acc->balance_centavos -= amount_centavos;
    to_acc->balance_centavos += amount_centavos;

    unlock_accounts(from_acc, to_acc);
    return true;
}

/*
 * Initializes all per-account reader-writer locks.
 * Should be called after accounts have been loaded.
 */
void init_account_locks(Bank *bank) {
    if (bank == NULL) {
        return;
    }

    for (int i = 0; i < bank->num_accounts; i++) {
        pthread_rwlock_init(&bank->accounts[i].lock, NULL);
    }
}

/*
 * Destroys all per-account reader-writer locks.
 * Called before program exit.
 */
void destroy_account_locks(Bank *bank) {
    if (bank == NULL) {
        return;
    }

    for (int i = 0; i < bank->num_accounts; i++) {
        pthread_rwlock_destroy(&bank->accounts[i].lock);
    }

    pthread_mutex_destroy(&bank->bank_lock);
}
