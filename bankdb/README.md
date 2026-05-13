# BankDB — Concurrent Banking System

## Overview
BankDB is a multi-threaded banking system implemented in C using POSIX threads.  
It simulates concurrent financial transactions such as deposits, withdrawals, transfers, and balance inquiries while ensuring correctness through synchronization mechanisms.

The system demonstrates real-world concurrency challenges including:
- Race conditions
- Deadlock scenarios
- Bounded buffer coordination
- Thread synchronization

---

## Features
### Core Functionality
- Multi-threaded transaction execution (one thread per transaction)
- Scheduled execution using a global timer (tick-based system)
- Support for:
  - Deposit
  - Withdraw
  - Transfer
  - Balance inquiry

### Concurrency & Synchronization
- Reader-writer locks for account access
- Deadlock handling (prevention via lock ordering)
- Semaphore-based bounded buffer pool
- Condition variables for time coordination

### System Metrics
- Transaction timing (start, end, wait time)
- Throughput calculation
- Buffer pool usage statistics
- Balance conservation verification

---

## Project Structure
bankdb/
├── include/ # Header files (interfaces)
├── src/ # Implementation files
│ ├── core/ # Core logic (bank, transaction, timer)
│ ├── concurrency/# Locks and buffer pool
│ ├── system/ # Parser and metrics
│ └── utils/ # Helper functions
├── tests/ # Input files and test cases
├── docs/ # Design documentation
└── build/ # Compiled output


## Build Instructions

- make

### Debug Mode (ThreadSanitizer)

- make debug

Note: In this environment, ThreadSanitizer fails at runtime with
"unexpected memory mapping". See docs/tsan_outputs_2026-05-13.txt.

### Clean Build Files

- make clean

### Usage

- ./bankdb --accounts=tests/accounts.txt --trace=tests/trace_simple.txt --deadlock=prevention --tick-ms=100

## Required Arguments
- --accounts=FILE : Initial account balances
- --trace=FILE : Transaction workload file
- --deadlock=prevention|detection : Deadlock handling strategy
- --tick-ms=N : Duration of one simulation tick (default: 100)
## Optional
- --verbose : Enable detailed logging

## Test Cases
The system supports the following test scenarios:

- trace_simple.txt — Sequential operations
- trace_readers.txt — Concurrent reads
- trace_deadlock.txt — Deadlock scenario
- trace_abort.txt — Insufficient funds
- trace_buffer.txt — Buffer pool saturation
- trace_stress_buffer.txt — Buffer pool stress
- trace_stress_mix.txt — Mixed operation stress

Manual test outputs are saved in docs/test_outputs_2026-05-13.txt.

## Output

The system produces:

- Transaction execution logs
- Performance metrics
- Buffer pool report
- Balance conservation check
- Known Limitations
- Fixed maximum number of accounts
- No persistent storage (in-memory only)
- Deadlock detection strategy not implemented (if using prevention)
- ThreadSanitizer runtime failure in this environment

## Authors
Sherwin Paul Sabandal
Antonio Gabriel Salmon

## Notes
This project is part of CMSC 125 (Operating Systems) and focuses on concurrency, synchronization, and systems-level programming using POSIX threads.

