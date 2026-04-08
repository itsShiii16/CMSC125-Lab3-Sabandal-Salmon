#include "transaction.h"

#include <stdio.h>
#include <string.h>

#include "bank.h"
#include "timer.h"
#include "utils.h"

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
 * Current starter behavior:
 * - waits until scheduled start tick
 * - records timing
 * - executes each operation in order
 * - commits on success
 * - aborts on failed withdraw/transfer
 *
 * NOTE:
 * This implementation assumes a global/shared Bank will later be made accessible.
 * For now, it references an external global bank instance.
 */
extern Bank bank;

void *execute_transaction(void *arg) {
    if (arg == NULL) {
        return NULL;
    }

    Transaction *tx = (Transaction *) arg;

    /* Wait until the transaction's scheduled start tick */
    wait_until_tick(tx->start_tick);

    pthread_mutex_lock(&tick_lock);
    tx->actual_start = global_tick;
    pthread_mutex_unlock(&tick_lock);

    tx->status = TX_RUNNING;

    for (int i = 0; i < tx->num_ops; i++) {
        Operation *op = &tx->ops[i];

        int tick_before;
        pthread_mutex_lock(&tick_lock);
        tick_before = global_tick;
        pthread_mutex_unlock(&tick_lock);

        switch (op->type) {
            case OP_DEPOSIT:
                deposit(&bank, op->account_id, op->amount_centavos);
                debug_log("T%d: DEPOSIT acc=%d amount=%d\n",
                          tx->tx_id, op->account_id, op->amount_centavos);
                break;

            case OP_WITHDRAW:
                if (!withdraw(&bank, op->account_id, op->amount_centavos)) {
                    tx->status = TX_ABORTED;

                    pthread_mutex_lock(&tick_lock);
                    tx->actual_end = global_tick;
                    pthread_mutex_unlock(&tick_lock);

                    debug_log("T%d: WITHDRAW failed, transaction aborted\n", tx->tx_id);
                    return NULL;
                }

                debug_log("T%d: WITHDRAW acc=%d amount=%d\n",
                          tx->tx_id, op->account_id, op->amount_centavos);
                break;

            case OP_TRANSFER:
                if (!transfer(&bank, op->account_id, op->target_account,
                              op->amount_centavos)) {
                    tx->status = TX_ABORTED;

                    pthread_mutex_lock(&tick_lock);
                    tx->actual_end = global_tick;
                    pthread_mutex_unlock(&tick_lock);

                    debug_log("T%d: TRANSFER failed, transaction aborted\n", tx->tx_id);
                    return NULL;
                }

                debug_log("T%d: TRANSFER from=%d to=%d amount=%d\n",
                          tx->tx_id, op->account_id,
                          op->target_account, op->amount_centavos);
                break;

            case OP_BALANCE: {
                int balance = get_balance(&bank, op->account_id);

                printf("T%d: Account %d balance = PHP %d.%02d\n",
                       tx->tx_id,
                       op->account_id,
                       balance / 100,
                       balance % 100);

                debug_log("T%d: BALANCE acc=%d\n", tx->tx_id, op->account_id);
                break;
            }

            default:
                tx->status = TX_ABORTED;

                pthread_mutex_lock(&tick_lock);
                tx->actual_end = global_tick;
                pthread_mutex_unlock(&tick_lock);

                fprintf(stderr, "T%d: unknown operation type\n", tx->tx_id);
                return NULL;
        }

        int tick_after;
        pthread_mutex_lock(&tick_lock);
        tick_after = global_tick;
        pthread_mutex_unlock(&tick_lock);

        tx->wait_ticks += (tick_after - tick_before);
    }

    pthread_mutex_lock(&tick_lock);
    tx->actual_end = global_tick;
    pthread_mutex_unlock(&tick_lock);

    tx->status = TX_COMMITTED;
    return NULL;
}