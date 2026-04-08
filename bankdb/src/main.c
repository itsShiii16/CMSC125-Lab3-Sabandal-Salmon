#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "bank.h"
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
        "--deadlock=prevention|detection [--tick-ms=N] [--verbose]\n",
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

    if (strcmp(config->deadlock_mode, "prevention") != 0 &&
        strcmp(config->deadlock_mode, "detection") != 0) {
        return false;
    }

    if (config->tick_ms <= 0) {
        return false;
    }

    return true;
}

/*
 * Starter main program:
 * - parses CLI args
 * - loads accounts
 * - initializes account locks
 * - parses transactions
 * - initializes timer
 *
 * For Phase 1 this is enough to validate structure and parsing.
 * Thread creation and full execution can be extended in the next phase.
 */
int main(int argc, char *argv[]) {
    Config config;
    Transaction transactions[MAX_TRANSACTIONS];

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

    int num_transactions = parse_trace_file(config.trace_file, transactions);
    if (num_transactions < 0) {
        fprintf(stderr, "Failed to parse trace file.\n");
        destroy_account_locks(&bank);
        return EXIT_FAILURE;
    }

    init_timer();

    /*
     * Phase 1 note:
     * We stop here after verifying setup and parsing.
     * next phase, this will be extended to:
     * - start timer thread
     * - create transaction threads
     * - join threads
     * - print metrics and summary
     */
    printf("BankDB setup successful.\n");
    printf("Loaded %d account(s).\n", bank.num_accounts);
    printf("Loaded %d transaction(s).\n", num_transactions);
    printf("Deadlock mode: %s\n", config.deadlock_mode);
    printf("Tick interval: %d ms\n", tick_interval_ms);
    printf("Verbose mode: %s\n", verbose_mode ? "ON" : "OFF");

    destroy_timer();
    destroy_account_locks(&bank);

    return EXIT_SUCCESS;
}