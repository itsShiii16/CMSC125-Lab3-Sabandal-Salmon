#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <pthread.h>
#include <stdbool.h>

#define MAX_OPS_PER_TX 256
#define MAX_TRANSACTIONS 256

/*
 * Operation types supported by the banking system.
 * Each transaction is made up of one or more of these operations.
 */
typedef enum {
    OP_DEPOSIT,
    OP_WITHDRAW,
    OP_TRANSFER,
    OP_BALANCE
} OpType;

/*
 * Represents one banking operation inside a transaction.
 *
 * account_id:
 *   Primary account used by the operation.
 *
 * amount_centavos:
 *   Amount involved in the operation, stored as integer centavos.
 *
 * target_account:
 *   Secondary account used only for transfer operations.
 *   Set to -1 for non-transfer operations.
 */
typedef struct {
    OpType type;
    int account_id;
    int amount_centavos;
    int target_account;
} Operation;

/*
 * Execution status of a transaction.
 */
typedef enum {
    TX_RUNNING,
    TX_COMMITTED,
    TX_ABORTED
} TxStatus;

/*
 * Represents one transaction to be executed by its own thread.
 *
 * tx_id:
 *   Numeric transaction identifier.
 *
 * ops / num_ops:
 *   List of operations that belong to this transaction.
 *
 * start_tick:
 *   Scheduled simulation tick when the transaction should begin.
 *
 * thread:
 *   pthread handle for the transaction worker.
 *
 * actual_start / actual_end:
 *   Tick values recorded during runtime for metrics.
 *
 * wait_ticks:
 *   Total ticks spent waiting, usually due to locks or scheduling delays.
 *
 * status:
 *   Final execution status of the transaction.
 */
typedef struct {
    int tx_id;
    Operation ops[MAX_OPS_PER_TX];
    int num_ops;
    int start_tick;

    pthread_t thread;

    int actual_start;
    int actual_end;
    int wait_ticks;

    TxStatus status;
} Transaction;

/*
 * Converts a string such as "DEPOSIT" or "TRANSFER" into an OpType.
 * Returns true on success, false if the string is invalid.
 */
bool parse_op_type(const char *op_str, OpType *out_type);

/*
 * Returns a readable string version of an OpType.
 * Useful for logs and debug output.
 */
const char *op_type_to_string(OpType type);

/*
 * Returns a readable string version of a transaction status.
 * Useful for summary tables and logs.
 */
const char *tx_status_to_string(TxStatus status);

/*
 * Initializes one transaction structure with safe default values.
 * This helps avoid garbage values before parsing real data into it.
 */
void init_transaction(Transaction *tx, int tx_id, int start_tick);

/*
 * Adds one operation to the given transaction.
 * Returns true if the operation was added successfully,
 * or false if the transaction is already full.
 */
bool add_operation_to_transaction(
    Transaction *tx,
    OpType type,
    int account_id,
    int amount_centavos,
    int target_account
);

/*
 * Thread entry point for executing a single transaction.
 * Actual logic will be implemented in transaction.c.
 */
void *execute_transaction(void *arg);

#endif