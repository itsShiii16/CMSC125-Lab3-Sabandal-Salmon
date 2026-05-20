#ifndef BANK_H
#define BANK_H

#include <pthread.h>
#include <stdbool.h>

/*
 * Maximum number of accounts supported by the system.
 * This can be adjusted if needed.
 */
#define MAX_ACCOUNTS 100

/*
 * Represents a single bank account.
 *
 * account_id:
 *   Unique identifier of the account.
 *
 * balance_centavos:
 *   Current balance stored as integer centavos.
 *
 * lock:
 *   Reader-writer lock to protect access to this account.
 *   - Multiple readers allowed (balance checks)
 *   - Single writer for updates (deposit/withdraw/transfer)
 */
typedef struct {
    int account_id;
    int balance_centavos;
    pthread_rwlock_t lock;
} Account;

/*
 * Represents the entire bank database.
 *
 * accounts:
 *   Array of all accounts in the system.
 *
 * num_accounts:
 *   Total number of valid accounts loaded.
 *
 * bank_lock:
 *   Mutex used to protect global bank metadata
 *   (e.g., adding accounts, initialization).
 */
typedef struct {
    Account accounts[MAX_ACCOUNTS];
    int num_accounts;
    pthread_mutex_t bank_lock;
} Bank;

/*
 * Initializes the bank structure.
 * Sets account count to zero and initializes the bank mutex.
 */
void init_bank(Bank *bank);

/*
 * Adds a new account to the bank.
 *
 * account_id:
 *   ID of the account to create.
 *
 * initial_balance:
 *   Starting balance in centavos.
 *
 * Returns true if successful, false if the bank is full.
 */
bool add_account(Bank *bank, int account_id, int initial_balance);

/*
 * Retrieves a pointer to an account by ID.
 *
 * Returns:
 *   Pointer to Account if found
 *   NULL if account does not exist
 */
Account *get_account(Bank *bank, int account_id);

/*
 * Reads the balance of an account.
 *
 * Uses a reader lock to allow concurrent reads.
 *
 * Returns:
 *   Balance in centavos
 *   -1 if account is invalid
 */
int get_balance(Bank *bank, int account_id);

/*
 * Deposits money into an account.
 *
 * Uses a writer lock to ensure exclusive access.
 */
void deposit(Bank *bank, int account_id, int amount_centavos);

/*
 * Withdraws money from an account.
 *
 * Returns:
 *   true if successful
 *   false if insufficient funds or invalid account
 */
bool withdraw(Bank *bank, int account_id, int amount_centavos);

/*
 * Transfers money from one account to another.
 *
 * Deadlock prevention (lock ordering) will be handled
 * inside the implementation (bank.c or lock_mgr.c).
 *
 * Returns:
 *   true if successful
 *   false if insufficient funds or invalid accounts
 */
bool transfer(Bank *bank, int from_id, int to_id, int amount_centavos);

/*
 * Initializes all account locks.
 * Should be called after loading accounts.
 */
void init_account_locks(Bank *bank);

/*
 * Destroys all account locks.
 * Should be called before program termination.
 */
void destroy_account_locks(Bank *bank);

#endif
