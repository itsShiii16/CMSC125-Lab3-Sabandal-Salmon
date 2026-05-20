# Design Documentation — BankDB

## 1. System Overview
BankDB is a concurrent banking simulation built in C using POSIX threads.  
It executes multiple transactions in parallel while maintaining correctness through synchronization mechanisms.

The system models real-world banking operations such as deposits, withdrawals, transfers, and balance inquiries, and demonstrates how classical concurrency problems are handled in practice.

---

## 2. System Architecture

### 2.1 High-Level Design
The system is divided into modular components:

- **Core Layer**
  - Banking operations and transaction execution
- **Concurrency Layer**
  - Synchronization, locking, and deadlock handling
- **System Layer**
  - Parsing, metrics, and reporting
- **Utility Layer**
  - Helper functions and shared utilities

This separation ensures:
- Low coupling between modules  
- High readability and maintainability  
- Easier debugging and testing  

### 2.2 Module Responsibilities

- bank - handles account state and money operations
- transaction - handles per-transaction execution
- timer - manages simulation time
- lock_mgr - enforces lock ordering
- buffer_pool - manages bounded account loading
- metrics reports - performance and correctness
- parser - loads input files

### 2.3 System Interface

- bank.h / bank.c 
account and bank structures, deposit, withdraw, balance, transfer
- transaction.h / transaction.c
transaction structs, operation enums, transaction thread execution
- timer.h / timer.c
global tick, timer thread, wait-until-tick logic
- lock_mgr.h / lock_mgr.c
lock ordering helpers for transfer deadlock prevention
- buffer_pool.h / buffer_pool.c
bounded buffer structs, init/load/unload helpers
- metrics.h / metrics.c
transaction stats, throughput, summary printing
- parser.h / parser.c
accounts file parser, trace parser
- utils.h / utils.c
generic helpers only
---

### 2.2 Thread Model
The system uses multiple threads:

- **Transaction Threads**
  - One thread per transaction
  - Executes operations sequentially within the transaction

- **Timer Thread**
  - Maintains a global simulation clock
  - Controls when transactions begin execution

- **Synchronization Mechanisms**
  - Reader-writer locks (per account)
  - Mutexes (shared structures)
  - Semaphores (buffer pool)
  - Condition variables (tick coordination)

---

## 3. Deadlock Strategy

### 3.1 Selected Approach
Deadlock prevention via **lock ordering**

### 3.2 Implementation
- Locks are always acquired in ascending order of account IDs
- Ensures consistent locking order across all transactions

### 3.3 Justification
- Simpler and more predictable than deadlock detection
- Eliminates circular wait condition
- Reduces implementation complexity
- Easier to validate and debug

### 3.4 Coffman Condition Broken
- **Circular wait condition** is eliminated

---

## 4. Buffer Pool Design

### 4.1 Purpose
Simulates limited system resources using a bounded buffer model.

### 4.2 Strategy
- Accounts are loaded into the buffer pool on first access
- Accounts are unloaded when the transaction completes

### 4.3 Synchronization
- `empty_slots` semaphore tracks available slots
- `full_slots` semaphore tracks occupied slots
- Mutex protects access to buffer pool data

### 4.4 Behavior When Full
- Transactions block until a buffer slot becomes available
- `tests/trace_buffer_blocking.txt` starts six reader transactions at tick 1.
  Five transactions pin accounts 1-5, and the sixth requests account 6 while
  the pool is full. The saved test output shows `Blocked     : 1`.

### 4.5 Justification
This approach provides:
- Simplicity of implementation  
- Predictable behavior  
- Balanced performance and correctness  

---

## 5. Reader-Writer Lock Design

### 5.1 Structure
Each account has its own reader-writer lock.

### 5.2 Behavior
- Multiple threads can read simultaneously
- Write operations are exclusive

### 5.3 Benefits
- Improves performance in read-heavy workloads
- Reduces unnecessary blocking

### 5.4 Measured Lock Comparison

The comparison below used the committed `pthread_rwlock_t` build and a temporary
copy in `/tmp/bankdb-mutex` where account locks were mechanically changed to
`pthread_mutex_t`. The same reader trace was used for both runs.

| Lock Strategy | Trace | Total Ticks | Elapsed Runtime | Notes |
|---|---|---:|---:|---|
| `pthread_rwlock_t` | `trace_readers.txt` | 0 | 0.01s | Allows concurrent readers |
| `pthread_mutex_t` temporary build | `trace_readers.txt` | 0 | 0.01s | Serializes account reads |

This small trace does not prove a runtime improvement; it only confirms both
lock strategies completed the reader-heavy workload successfully in this
environment.

---

## 6. Timer Thread Design

### 6.1 Purpose
Simulates time progression in discrete ticks.

### 6.2 Behavior
- Increments a global tick at fixed intervals
- Signals waiting threads using condition variables

### 6.3 Importance
Without the timer:
- Transactions would execute sequentially
- Concurrency behavior cannot be properly simulated

---

## 7. Transaction Execution Flow

1. Transaction thread is created  
2. Thread waits until its scheduled start tick  
3. Operations are executed sequentially  
4. Synchronization mechanisms are applied  
5. Timing and wait metrics are recorded  
6. Transaction commits or aborts  

---

## 8. Metrics Collection

The system records:
- Transaction start and end times  
- Wait time from scheduled start to actual start (tick-based)  
- Throughput (transactions per tick)  
- Buffer pool usage statistics  
- Buffer pool blocked-load count  

---

## 9. Correctness Guarantees

The system ensures:
- No deadlocks (via prevention strategy)  
- Balance conservation using `final_total == initial_total + net_external_flow`,
  where committed deposits add to external flow, committed withdrawals subtract
  from it, and transfers do not change it.

---

## 10. Performance Considerations

- Reader-writer locks improve concurrency for read-heavy workloads  
- Lock ordering prevents expensive deadlock recovery  
- Buffer pool introduces controlled contention  
- Tick-based scheduling enables consistent benchmarking  

## 10.1 Test Evidence

- Manual test outputs are saved in docs/test_outputs_2026-05-13.txt
- ThreadSanitizer build/runtime output is saved in docs/tsan_outputs_2026-05-13.txt.
  In this environment the runtime fails before executing the trace with
  `FATAL: ThreadSanitizer: unexpected memory mapping`, so this file is not a
  zero-warning pass.

---

## 11. Limitations

- No persistent storage (in-memory system only)  
- Fixed buffer pool size  
- No advanced buffer replacement strategies (e.g., LRU)  
- Deadlock detection strategy not implemented  
- ThreadSanitizer failed in this environment with "unexpected memory mapping"

---

## 12. Development Phases (4-Week Plan)

### Phase 1 — Week 1: System Setup & Architecture
**Objective:** Establish foundation and design

- Setup project structure  
- Implement data structures (Account, Transaction, Bank)  
- Implement CLI argument parsing  
- Implement file parsing (accounts and trace files)  
- Implement timer thread skeleton  
- Draft design.md documentation  

**Deliverable:**
- Working parsing system  
- Initial timer logic  
- Clear architecture explanation  

---

### Phase 2 — Week 2: Core Concurrency Implementation
**Objective:** Build functional prototype

- Implement reader-writer locks for accounts  
- Implement deposit, withdraw, and balance operations  
- Implement transaction execution threads  
- Implement deadlock prevention via lock ordering  
- Add transaction logging and status tracking  

**Deliverable:**
- Functional concurrent system  
- Correct execution of basic test cases  

---

### Phase 3 — Week 3: Full System Integration
**Objective:** Complete all required features

- Implement bounded buffer pool using semaphores  
- Integrate buffer pool with transactions  
- Implement metrics collection and reporting  
- Implement all test cases  
- Validate correctness using ThreadSanitizer  
- Ensure money conservation  

**Deliverable:**
- Fully working system  
- All features implemented and tested  

---

### Phase 4 — Week 4: Optimization & Defense Preparation
**Objective:** Finalize system and prepare for evaluation

- Optimize synchronization and performance  
- Clean and refactor code  
- Complete documentation (README and design.md)  
- Prepare logs and test evidence  
- Validate all required outputs  
- Practice defense explanations  

**Deliverable:**
- Clean, production-ready codebase  
- Complete documentation  
- Ready for laboratory defense  

---

## 13. Future Improvements

- Implement deadlock detection strategy  
- Add dynamic buffer pool management  
- Introduce persistent storage layer  
- Improve scheduling and fairness mechanisms  
