#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "bank.h"
#include "buffer_pool.h"
#include "metrics.h"
#include "parser.h"
#include "timer.h"
#include "transaction.h"
#include "utils.h"

/*
 * Global bank instance.
 * transaction.c currently uses this through:
 *   extern Bank bank;
 */
Bank bank;

/*
 * Global bounded buffer pool.
 * transaction.c uses this through extern BufferPool buffer_pool.
 */
BufferPool buffer_pool;

/*
 * Stores command-line configuration.
 */
typedef struct {
    const char *accounts_file;
    const char *trace_file;
    const char *deadlock_mode;
    int tick_ms;
    int verbose;
} Config;

/*
 * Prints basic usage instructions.
 */
static void print_usage(const char *program_name) {
    fprintf(stderr,
        "Usage: %s --accounts=FILE --trace=FILE "
        "--deadlock=prevention [--tick-ms=N] [--verbose]\n",
        program_name
    );
}

/*
 * Parses command-line arguments into the Config struct.
 * Returns true on success, false on invalid or missing required arguments.
 */
static bool parse_args(int argc, char *argv[], Config *config) {
    if (config == NULL) {
        return false;
    }

    config->accounts_file = NULL;
    config->trace_file = NULL;
    config->deadlock_mode = NULL;
    config->tick_ms = 100;
    config->verbose = 0;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--accounts=", 11) == 0) {
            config->accounts_file = argv[i] + 11;
        } else if (strncmp(argv[i], "--trace=", 8) == 0) {
            config->trace_file = argv[i] + 8;
        } else if (strncmp(argv[i], "--deadlock=", 11) == 0) {
            config->deadlock_mode = argv[i] + 11;
        } else if (strncmp(argv[i], "--tick-ms=", 10) == 0) {
            config->tick_ms = atoi(argv[i] + 10);
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config->verbose = 1;
        } else {
            return false;
        }
    }

    if (config->accounts_file == NULL ||
        config->trace_file == NULL ||
        config->deadlock_mode == NULL) {
        return false;
    }

    if (strcmp(config->deadlock_mode, "prevention") != 0) {
        fprintf(stderr, "Unsupported deadlock mode: only prevention is supported.\n");
        return false;
    }

    if (config->tick_ms <= 0) {
        return false;
    }

    return true;
}

/*
 * Computes money entering or leaving the bank from committed transactions.
 * Transfers are internal movements and do not change total bank balance.
 */
static long compute_committed_external_flow(
    Transaction *transactions,
    int num_transactions
) {
    long net_external_flow = 0;

    if (transactions == NULL || num_transactions <= 0) {
        return 0;
    }

    for (int i = 0; i < num_transactions; i++) {
        if (transactions[i].status != TX_COMMITTED) {
            continue;
        }

        for (int j = 0; j < transactions[i].num_ops; j++) {
            Operation *op = &transactions[i].ops[j];

            if (op->type == OP_DEPOSIT) {
                net_external_flow += op->amount_centavos;
            } else if (op->type == OP_WITHDRAW) {
                net_external_flow -= op->amount_centavos;
            }
        }
    }

    return net_external_flow;
}

/*
 * Program entry point.
 * - parses CLI args
 * - loads accounts
 * - initializes account locks
 * - parses transactions
 * - initializes timer
 * - runs one thread per transaction
 * - prints metrics and cleans up resources
 */
int main(int argc, char *argv[]) {
    Config config;
    Transaction transactions[MAX_TRANSACTIONS];
    pthread_t timer_thread_id;
    long initial_total = 0;
    long net_external_flow = 0;
    int timer_started = 0;
    int exit_code = EXIT_FAILURE;

    if (!parse_args(argc, argv, &config)) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    verbose_mode = config.verbose;
    tick_interval_ms = config.tick_ms;

    init_bank(&bank);

    if (!parse_accounts_file(config.accounts_file, &bank)) {
        fprintf(stderr, "Failed to parse accounts file.\n");
        return EXIT_FAILURE;
    }

    init_account_locks(&bank);
    initial_total = compute_total_balance(&bank);

    int num_transactions = parse_trace_file(config.trace_file, transactions);
    if (num_transactions < 0) {
        fprintf(stderr, "Failed to parse trace file.\n");
        goto cleanup_bank;
    }

    init_buffer_pool(&buffer_pool);
    init_timer();

    /* Start the timer thread to increment the global tick. */
    simulation_running = true;
    if (pthread_create(&timer_thread_id, NULL, timer_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create timer thread.\n");
        goto cleanup_timer;
    }
    timer_started = 1;

    /* Start one worker thread per parsed transaction. */
    for (int i = 0; i < num_transactions; i++) {
        if (pthread_create(&transactions[i].thread, NULL,
                           execute_transaction, &transactions[i]) != 0) {
            fprintf(stderr, "Failed to create transaction thread T%d.\n",
                    transactions[i].tx_id);
            transactions[i].status = TX_ABORTED;
        }
    }

    /* Wait for all transaction threads that were successfully started. */
    for (int i = 0; i < num_transactions; i++) {
        if (transactions[i].thread != 0) {
            pthread_join(transactions[i].thread, NULL);
        }
    }

    /* Stop the timer thread after all transactions have completed. */
    pthread_mutex_lock(&tick_lock);
    simulation_running = false;
    pthread_cond_broadcast(&tick_changed);
    pthread_mutex_unlock(&tick_lock);

    if (timer_started) {
        pthread_join(timer_thread_id, NULL);
    }

    net_external_flow = compute_committed_external_flow(
        transactions,
        num_transactions
    );

    /* Print the execution summary and buffer pool report. */
    print_summary(transactions, num_transactions, global_tick);
    print_transaction_metrics(transactions, num_transactions);
    print_buffer_pool_report(&buffer_pool);
    verify_balance_conservation(&bank, initial_total, net_external_flow);

    exit_code = EXIT_SUCCESS;

cleanup_timer:
    destroy_timer();
    destroy_buffer_pool(&buffer_pool);
cleanup_bank:
    destroy_account_locks(&bank);

    return exit_code;
}
