#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "utils.h"

/*
 * Internal helper:
 * Finds an existing transaction by tx_id, or creates a new one if it does not exist yet.
 *
 * Returns:
 * - pointer to the matching/new transaction on success
 * - NULL if the transaction array is already full
 */
static Transaction *find_or_create_transaction(
    Transaction *transactions,
    int *num_transactions,
    int tx_id,
    int start_tick
) {
    for (int i = 0; i < *num_transactions; i++) {
        if (transactions[i].tx_id == tx_id) {
            return &transactions[i];
        }
    }

    if (*num_transactions >= MAX_TRANSACTIONS) {
        return NULL;
    }

    Transaction *tx = &transactions[*num_transactions];
    init_transaction(tx, tx_id, start_tick);
    (*num_transactions)++;

    return tx;
}

/*
 * Internal helper:
 * Skips leading whitespace characters.
 */
static const char *skip_leading_spaces(const char *s) {
    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

/*
 * Internal helper:
 * Checks whether a line is ignorable.
 *
 * A line is ignored if it is:
 * - NULL
 * - empty
 * - whitespace only
 * - a comment starting with '#'
 */
static int should_skip_line(const char *line) {
    if (line == NULL) {
        return 1;
    }

    line = skip_leading_spaces(line);

    if (*line == '\0' || *line == '\n' || *line == '#') {
        return 1;
    }

    return 0;
}

/*
 * Initializes all transactions with safe default values.
 *
 * This is useful before parsing so every slot starts in a known state.
 */
void init_transactions(Transaction *transactions, int max_transactions) {
    if (transactions == NULL || max_transactions <= 0) {
        return;
    }

    for (int i = 0; i < max_transactions; i++) {
        init_transaction(&transactions[i], -1, 0);
    }
}

/*
 * Parses the accounts file and populates the bank.
 *
 * Expected line format:
 *   account_id initial_balance
 *
 * Example:
 *   10 50000
 *
 * Ignores:
 * - blank lines
 * - comment lines starting with '#'
 *
 * Returns:
 * - true on success
 * - false on any parsing or file error
 */
bool parse_accounts_file(const char *filename, Bank *bank) {
    if (filename == NULL || bank == NULL) {
        fprintf(stderr, "parse_accounts_file: invalid argument\n");
        return false;
    }

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Failed to open accounts file");
        return false;
    }

    char line[256];
    int line_no = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        line_no++;
        trim_newline(line);

        if (should_skip_line(line)) {
            continue;
        }

        int account_id = -1;
        int initial_balance = 0;

        int matched = sscanf(line, "%d %d", &account_id, &initial_balance);
        if (matched != 2) {
            fprintf(stderr,
                    "parse_accounts_file: invalid format at line %d: %s\n",
                    line_no, line);
            fclose(fp);
            return false;
        }

        if (account_id < 0) {
            fprintf(stderr,
                    "parse_accounts_file: invalid account id at line %d: %d\n",
                    line_no, account_id);
            fclose(fp);
            return false;
        }

        if (initial_balance < 0) {
            fprintf(stderr,
                    "parse_accounts_file: negative initial balance at line %d\n",
                    line_no);
            fclose(fp);
            return false;
        }

        if (!add_account(bank, account_id, initial_balance)) {
            fprintf(stderr,
                    "parse_accounts_file: failed to add account at line %d\n",
                    line_no);
            fclose(fp);
            return false;
        }
    }

    fclose(fp);
    return true;
}

/*
 * Parses one trace line and appends one operation into the given transaction.
 *
 * Expected line formats:
 *   T1 0 DEPOSIT 10 5000
 *   T2 1 WITHDRAW 10 2000
 *   T3 2 TRANSFER 10 20 3000
 *   T4 5 BALANCE 10
 *
 * This function assumes:
 * - the target transaction has already been created
 * - tx->tx_id and tx->start_tick are already set correctly
 *
 * Returns:
 * - true if the line was parsed and appended successfully
 * - false if the line is malformed
 */
bool parse_trace_line(const char *line, Transaction *tx) {
    if (line == NULL || tx == NULL) {
        return false;
    }

    char tx_label[32];
    char op_str[32];

    int tx_id = -1;
    int start_tick = -1;
    int account_id = -1;
    int amount = 0;
    int target_account = -1;

    /*
     * First, parse the common prefix:
     * T<number> <start_tick> <operation>
     */
    int prefix_count = sscanf(line, "%31s %d %31s", tx_label, &start_tick, op_str);
    if (prefix_count != 3) {
        return false;
    }

    if (tx_label[0] != 'T' || sscanf(tx_label + 1, "%d", &tx_id) != 1) {
        return false;
    }

    /*
     * Sanity check:
     * The parsed line should belong to the transaction passed in.
     */
    if (tx->tx_id != tx_id) {
        return false;
    }

    if (tx->start_tick != start_tick) {
        /*
         * If the same transaction appears on multiple lines,
         * we expect the start tick to stay consistent.
         */
        return false;
    }

    OpType type;
    if (!parse_op_type(op_str, &type)) {
        return false;
    }

    switch (type) {
        case OP_DEPOSIT: {
            /*
             * Format:
             * T1 0 DEPOSIT 10 5000
             */
            int matched = sscanf(line, "%31s %d %31s %d %d",
                                 tx_label, &start_tick, op_str,
                                 &account_id, &amount);
            if (matched != 5) {
                return false;
            }

            return add_operation_to_transaction(
                tx, OP_DEPOSIT, account_id, amount, -1
            );
        }

        case OP_WITHDRAW: {
            /*
             * Format:
             * T2 1 WITHDRAW 10 2000
             */
            int matched = sscanf(line, "%31s %d %31s %d %d",
                                 tx_label, &start_tick, op_str,
                                 &account_id, &amount);
            if (matched != 5) {
                return false;
            }

            return add_operation_to_transaction(
                tx, OP_WITHDRAW, account_id, amount, -1
            );
        }

        case OP_TRANSFER: {
            /*
             * Format:
             * T3 2 TRANSFER 10 20 3000
             */
            int matched = sscanf(line, "%31s %d %31s %d %d %d",
                                 tx_label, &start_tick, op_str,
                                 &account_id, &target_account, &amount);
            if (matched != 6) {
                return false;
            }

            return add_operation_to_transaction(
                tx, OP_TRANSFER, account_id, amount, target_account
            );
        }

        case OP_BALANCE: {
            /*
             * Format:
             * T4 5 BALANCE 10
             */
            int matched = sscanf(line, "%31s %d %31s %d",
                                 tx_label, &start_tick, op_str,
                                 &account_id);
            if (matched != 4) {
                return false;
            }

            return add_operation_to_transaction(
                tx, OP_BALANCE, account_id, 0, -1
            );
        }

        default:
            return false;
    }
}

/*
 * Parses the trace file and fills the transaction array.
 *
 * Behavior:
 * - Groups multiple lines with the same TxID into one Transaction
 * - Creates new transactions as needed
 * - Appends parsed operations into the correct transaction
 *
 * Returns:
 * - number of transactions parsed on success
 * - -1 on error
 */
int parse_trace_file(const char *filename, Transaction *transactions) {
    if (filename == NULL || transactions == NULL) {
        fprintf(stderr, "parse_trace_file: invalid argument\n");
        return -1;
    }

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("Failed to open trace file");
        return -1;
    }

    init_transactions(transactions, MAX_TRANSACTIONS);

    char line[256];
    int line_no = 0;
    int num_transactions = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        line_no++;
        trim_newline(line);

        if (should_skip_line(line)) {
            continue;
        }

        /*
         * We first parse just enough to identify which transaction this line belongs to.
         */
        char tx_label[32];
        char op_str[32];
        int tx_id = -1;
        int start_tick = -1;

        int matched = sscanf(line, "%31s %d %31s", tx_label, &start_tick, op_str);
        if (matched != 3) {
            fprintf(stderr,
                    "parse_trace_file: invalid trace format at line %d: %s\n",
                    line_no, line);
            fclose(fp);
            return -1;
        }

        if (tx_label[0] != 'T' || sscanf(tx_label + 1, "%d", &tx_id) != 1) {
            fprintf(stderr,
                    "parse_trace_file: invalid transaction label at line %d: %s\n",
                    line_no, tx_label);
            fclose(fp);
            return -1;
        }

        if (start_tick < 0) {
            fprintf(stderr,
                    "parse_trace_file: invalid start tick at line %d\n",
                    line_no);
            fclose(fp);
            return -1;
        }

        Transaction *tx = find_or_create_transaction(
            transactions,
            &num_transactions,
            tx_id,
            start_tick
        );

        if (tx == NULL) {
            fprintf(stderr,
                    "parse_trace_file: transaction limit exceeded at line %d\n",
                    line_no);
            fclose(fp);
            return -1;
        }

        if (!parse_trace_line(line, tx)) {
            fprintf(stderr,
                    "parse_trace_file: failed to parse operation at line %d: %s\n",
                    line_no, line);
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return num_transactions;
}