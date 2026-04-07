#ifndef PARSER_H
#define PARSER_H

#include "bank.h"
#include "transaction.h"

/*
 * Parser Module
 *
 * Responsible for reading input files:
 * - accounts file
 * - transaction trace file
 */

/*
 * Parses the accounts file and populates the bank.
 *
 * Format:
 * account_id initial_balance
 *
 * Returns:
 * - true on success
 * - false on failure
 */
bool parse_accounts_file(const char *filename, Bank *bank);

/*
 * Parses the transaction trace file.
 *
 * Populates an array of Transaction structures.
 *
 * Returns:
 * - number of transactions parsed
 * - -1 on error
 */
int parse_trace_file(const char *filename, Transaction *transactions);

/*
 * Parses a single line from the trace file into an operation.
 *
 * Returns:
 * - true if parsing is successful
 * - false otherwise
 */
bool parse_trace_line(const char *line, Transaction *tx);

/*
 * Initializes all transactions before parsing.
 */
void init_transactions(Transaction *transactions, int max_transactions);

#endif