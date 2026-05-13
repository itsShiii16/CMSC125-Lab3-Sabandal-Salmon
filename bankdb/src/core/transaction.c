#include "transaction.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bank.h"
#include "buffer_pool.h"
#include "lock_mgr.h"
#include "timer.h"
#include "utils.h"

#define MAX_LOADED_ACCOUNTS_PER_TX (MAX_OPS_PER_TX * 2)

/*
 * Converts a string such as "DEPOSIT" into the matching OpType.
 *
 * Returns:
 * - true if the operation string is valid
 * - false otherwise
 */
bool parse_op_type(const char *op_str, OpType *out_type) {
    if (op_str == NULL || out_type == NULL) {
        return false;
    }

    if (strcmp(op_str, "DEPOSIT") == 0) {
        *out_type = OP_DEPOSIT;
        return true;
    }

    if (strcmp(op_str, "WITHDRAW") == 0) {
        *out_type = OP_WITHDRAW;
        return true;
    }

    if (strcmp(op_str, "TRANSFER") == 0) {
        *out_type = OP_TRANSFER;
        return true;
    }

    if (strcmp(op_str, "BALANCE") == 0) {
        *out_type = OP_BALANCE;
        return true;
    }

    return false;
}

/*
 * Returns a readable string version of an OpType.
 * Useful for logs and debugging output.
 */
const char *op_type_to_string(OpType type) {
    switch (type) {
        case OP_DEPOSIT:
            return "DEPOSIT";
        case OP_WITHDRAW:
            return "WITHDRAW";
        case OP_TRANSFER:
            return "TRANSFER";
        case OP_BALANCE:
            return "BALANCE";
        default:
            return "UNKNOWN_OP";
    }
}

/*
 * Returns a readable string version of a transaction status.
 */
const char *tx_status_to_string(TxStatus status) {
    switch (status) {
        case TX_RUNNING:
            return "RUNNING";
        case TX_COMMITTED:
            return "COMMITTED";
        case TX_ABORTED:
            return "ABORTED";
        default:
            return "UNKNOWN_STATUS";
    }
}

/*
 * Initializes a transaction with safe default values.
 *
 * This avoids uninitialized fields before parsing or execution.
 */
void init_transaction(Transaction *tx, int tx_id, int start_tick) {
    if (tx == NULL) {
        return;
    }

    tx->tx_id = tx_id;
    tx->num_ops = 0;
    tx->start_tick = start_tick;
    tx->thread = 0;

    tx->actual_start = -1;
    tx->actual_end = -1;
    tx->wait_ticks = 0;

    tx->status = TX_RUNNING;

    for (int i = 0; i < MAX_OPS_PER_TX; i++) {
        tx->ops[i].type = OP_BALANCE;
        tx->ops[i].account_id = -1;
        tx->ops[i].amount_centavos = 0;
        tx->ops[i].target_account = -1;
    }
}

/*
 * Appends one operation to a transaction.
 *
 * Returns:
 * - true if successful
 * - false if the transaction has reached its maximum capacity
 */
bool add_operation_to_transaction(
    Transaction *tx,
    OpType type,
    int account_id,
    int amount_centavos,
    int target_account
) {
    if (tx == NULL) {
        return false;
    }

    if (tx->num_ops >= MAX_OPS_PER_TX) {
        return false;
    }

    Operation *op = &tx->ops[tx->num_ops];

    op->type = type;
    op->account_id = account_id;
    op->amount_centavos = amount_centavos;
    op->target_account = target_account;

    tx->num_ops++;

    return true;
}

/*
 * Thread entry point for executing a single transaction.
 *
 * The logic performs each operation sequentially in a transaction:
 * - waits until the scheduled start tick
 * - records the start time
 * - executes each operation (deposit, withdraw, transfer, balance inquiry)
 * - commits or aborts based on success/failure
 */
extern Bank bank;
extern BufferPool buffer_pool;

/*
 * Records the current global tick as a transaction's end time.
 * Synchronization: tick_lock protects reads from the shared global timer.
 */
static void record_transaction_end(Transaction *tx) {
    if (tx == NULL) {
        return;
    }

    pthread_mutex_lock(&tick_lock);
    tx->actual_end = global_tick;
    pthread_mutex_unlock(&tick_lock);
}

/*
 * Checks whether a transaction has already loaded an account.
 * Synchronization: the loaded account list is local to one transaction thread.
 */
static bool transaction_has_loaded_account(
    int *loaded_accounts,
    int loaded_count,
    int account_id
) {
    for (int i = 0; i < loaded_count; i++) {
        if (loaded_accounts[i] == account_id) {
            return true;
        }
    }

    return false;
}

/*
 * Loads one account into the buffer pool for the current transaction.
 * Synchronization: buffer pool internals use semaphores and a mutex; this helper
 * only updates the transaction-local list after a successful load.
 */
static bool load_account_for_transaction(
    int *loaded_accounts,
    int *loaded_count,
    int account_id
) {
    Account *account;

    if (loaded_accounts == NULL || loaded_count == NULL) {
        return false;
    }

    if (transaction_has_loaded_account(loaded_accounts, *loaded_count, account_id)) {
        return true;
    }

    if (*loaded_count >= MAX_LOADED_ACCOUNTS_PER_TX) {
        return false;
    }

    account = get_account(&bank, account_id);
    if (account == NULL) {
        return false;
    }

    load_account(&buffer_pool, account);
    loaded_accounts[*loaded_count] = account_id;
    (*loaded_count)++;

    return true;
}

/*
 * Unloads all accounts loaded by a transaction.
 * Synchronization: each unload updates shared buffer pool state under its mutex.
 */
static void unload_transaction_accounts(int *loaded_accounts, int loaded_count) {
    for (int i = loaded_count - 1; i >= 0; i--) {
        unload_account(&buffer_pool, loaded_accounts[i]);
    }
}

/*
 * Aborts a transaction, records its end tick, and releases loaded accounts.
 * Synchronization: timer and buffer pool helpers handle their own shared state.
 */
static void abort_transaction(
    Transaction *tx,
    int *loaded_accounts,
    int loaded_count,
    const char *message
) {
    tx->status = TX_ABORTED;
    record_transaction_end(tx);
    unload_transaction_accounts(loaded_accounts, loaded_count);

    if (message != NULL) {
        debug_log("T%d: %s\n", tx->tx_id, message);
    }
}

void *execute_transaction(void *arg) {
    if (arg == NULL) {
        return NULL;
    }

    Transaction *tx = (Transaction *) arg;
    int loaded_accounts[MAX_LOADED_ACCOUNTS_PER_TX];
    int loaded_count = 0;

    /* Wait until the transaction's scheduled start tick */
    wait_until_tick(tx->start_tick);

    pthread_mutex_lock(&tick_lock);
    tx->actual_start = global_tick;
    tx->wait_ticks = tx->actual_start - tx->start_tick;
    pthread_mutex_unlock(&tick_lock);

    tx->status = TX_RUNNING;

    for (int i = 0; i < tx->num_ops; i++) {
        Operation *op = &tx->ops[i];

        switch (op->type) {
            case OP_DEPOSIT:
                if (!load_account_for_transaction(loaded_accounts, &loaded_count,
                                                  op->account_id)) {
                    abort_transaction(tx, loaded_accounts, loaded_count,
                                      "DEPOSIT account load failed, transaction aborted");
                    return NULL;
                }

                deposit(&bank, op->account_id, op->amount_centavos);
                debug_log("T%d: DEPOSIT acc=%d amount=%d\n",
                          tx->tx_id, op->account_id, op->amount_centavos);
                break;

            case OP_WITHDRAW:
                if (!load_account_for_transaction(loaded_accounts, &loaded_count,
                                                  op->account_id)) {
                    abort_transaction(tx, loaded_accounts, loaded_count,
                                      "WITHDRAW account load failed, transaction aborted");
                    return NULL;
                }

                if (!withdraw(&bank, op->account_id, op->amount_centavos)) {
                    abort_transaction(tx, loaded_accounts, loaded_count,
                                      "WITHDRAW failed, transaction aborted");
                    return NULL;
                }

                debug_log("T%d: WITHDRAW acc=%d amount=%d\n",
                          tx->tx_id, op->account_id, op->amount_centavos);
                break;

            case OP_TRANSFER:
                if (!load_account_for_transaction(loaded_accounts, &loaded_count,
                                                  op->account_id) ||
                    !load_account_for_transaction(loaded_accounts, &loaded_count,
                                                  op->target_account)) {
                    abort_transaction(tx, loaded_accounts, loaded_count,
                                      "TRANSFER account load failed, transaction aborted");
                    return NULL;
                }

                if (!transfer(&bank, op->account_id, op->target_account,
                              op->amount_centavos)) {
                    abort_transaction(tx, loaded_accounts, loaded_count,
                                      "TRANSFER failed, transaction aborted");
                    return NULL;
                }

                debug_log("T%d: TRANSFER from=%d to=%d amount=%d\n",
                          tx->tx_id, op->account_id,
                          op->target_account, op->amount_centavos);
                break;

            case OP_BALANCE: {
                if (!load_account_for_transaction(loaded_accounts, &loaded_count,
                                                  op->account_id)) {
                    abort_transaction(tx, loaded_accounts, loaded_count,
                                      "BALANCE account load failed, transaction aborted");
                    return NULL;
                }

                int balance = get_balance(&bank, op->account_id);

                if (balance < 0) {
                    abort_transaction(tx, loaded_accounts, loaded_count,
                                      "BALANCE failed, transaction aborted");
                    return NULL;
                }

                printf("T%d: Account %d balance = PHP %d.%02d\n",
                       tx->tx_id,
                       op->account_id,
                       balance / 100,
                       balance % 100);

                debug_log("T%d: BALANCE acc=%d\n", tx->tx_id, op->account_id);
                break;
            }

            default:
                fprintf(stderr, "T%d: unknown operation type\n", tx->tx_id);
                abort_transaction(tx, loaded_accounts, loaded_count,
                                  "unknown operation type, transaction aborted");
                return NULL;
        }

    }

    record_transaction_end(tx);
    tx->status = TX_COMMITTED;
    unload_transaction_accounts(loaded_accounts, loaded_count);
    return NULL;
}
