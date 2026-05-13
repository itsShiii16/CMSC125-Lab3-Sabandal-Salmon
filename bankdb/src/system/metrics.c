#include "metrics.h"

#include <stdio.h>

/*
 * Prints a compact summary of transaction outcomes and runtime.
 */
void print_summary(Transaction *transactions, int num_transactions, int total_ticks) {
    int committed = 0;
    int aborted = 0;

    if (transactions == NULL || num_transactions < 0) {
        return;
    }

    for (int i = 0; i < num_transactions; i++) {
        if (transactions[i].status == TX_COMMITTED) {
            committed++;
        } else if (transactions[i].status == TX_ABORTED) {
            aborted++;
        }
    }

    printf("\n=== BankDB Summary ===\n");
    printf("Transactions : %d\n", num_transactions);
    printf("Committed    : %d\n", committed);
    printf("Aborted      : %d\n", aborted);
    printf("Total ticks  : %d\n", total_ticks);
    printf("Avg wait     : %.2f ticks\n",
           compute_average_wait(transactions, num_transactions));
    printf("Throughput   : %.2f tx/tick\n",
           compute_throughput(num_transactions, total_ticks));
}

/*
 * Prints per-transaction timing and final status.
 */
void print_transaction_metrics(Transaction *transactions, int num_transactions) {
    if (transactions == NULL || num_transactions < 0) {
        return;
    }

    printf("\n=== Transaction Metrics ===\n");
    printf("%-6s %-10s %-12s %-10s %-10s %-10s\n",
           "TxID", "Start", "ActualStart", "End", "Wait", "Status");

    for (int i = 0; i < num_transactions; i++) {
        printf("T%-5d %-10d %-12d %-10d %-10d %-10s\n",
               transactions[i].tx_id,
               transactions[i].start_tick,
               transactions[i].actual_start,
               transactions[i].actual_end,
               transactions[i].wait_ticks,
               tx_status_to_string(transactions[i].status));
    }
}

/*
 * Computes the average wait ticks across all transactions.
 */
double compute_average_wait(Transaction *transactions, int num_transactions) {
    long total_wait = 0;

    if (transactions == NULL || num_transactions <= 0) {
        return 0.0;
    }

    for (int i = 0; i < num_transactions; i++) {
        total_wait += transactions[i].wait_ticks;
    }

    return (double) total_wait / (double) num_transactions;
}

/*
 * Computes transaction throughput as completed transactions per tick.
 */
double compute_throughput(int total_transactions, int total_ticks) {
    if (total_transactions <= 0 || total_ticks <= 0) {
        return 0.0;
    }

    return (double) total_transactions / (double) total_ticks;
}

/*
 * Prints buffer pool statistics when Phase 3 provides a real pool.
 */
void print_buffer_pool_report(BufferPool *pool) {
    printf("\n=== Buffer Pool Report ===\n");

    if (pool == NULL) {
        printf("Buffer pool integration is deferred to Phase 3.\n");
        return;
    }

    printf("Loads       : %d\n", pool->load_count);
    printf("Unloads     : %d\n", pool->unload_count);
    printf("Current use : %d\n", pool->current_usage);
    printf("Peak use    : %d\n", pool->peak_usage);
}

/*
 * Compares initial and final total balances for a basic consistency check.
 */
void verify_balance_conservation(Bank *bank, long initial_total) {
    long final_total;

    if (bank == NULL) {
        return;
    }

    final_total = compute_total_balance(bank);

    printf("\n=== Balance Check ===\n");
    printf("Initial total : %ld\n", initial_total);
    printf("Final total   : %ld\n", final_total);

    if (initial_total == final_total) {
        printf("Result        : PASS\n");
    } else {
        printf("Result        : CHANGED by deposits/withdrawals or failed conservation\n");
    }
}
