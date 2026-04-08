#ifndef METRICS_H
#define METRICS_H

#include "transaction.h"
#include "buffer_pool.h"
#include "bank.h"

/*
 * Metrics Module
 *
 * Responsible for collecting and reporting system performance.
 */

/*
 * Prints a summary of all transactions.
 *
 * Includes:
 * - total transactions
 * - committed / aborted counts
 * - total ticks
 */
void print_summary(Transaction *transactions, int num_transactions, int total_ticks);

/*
 * Prints detailed transaction metrics in tabular form.
 *
 * Includes:
 * - start tick
 * - actual start
 * - end tick
 * - wait time
 * - status
 */
void print_transaction_metrics(Transaction *transactions, int num_transactions);

/*
 * Computes average wait time across all transactions.
 */
double compute_average_wait(Transaction *transactions, int num_transactions);

/*
 * Computes throughput (transactions per tick).
 */
double compute_throughput(int total_transactions, int total_ticks);

/*
 * Prints buffer pool statistics.
 */
void print_buffer_pool_report(BufferPool *pool);

/*
 * Verifies money conservation:
 * total initial balance == total final balance
 */
void verify_balance_conservation(Bank *bank, long initial_total);

/*
 * Computes total balance across all accounts.
 */
long compute_total_balance(Bank *bank);

#endif