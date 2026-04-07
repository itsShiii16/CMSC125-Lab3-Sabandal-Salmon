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
- Wait time due to lock contention  
- Throughput (transactions per tick)  
- Buffer pool usage statistics  

---

## 9. Correctness Guarantees

The system ensures:
- No race conditions (validated via ThreadSanitizer)  
- No deadlocks (via prevention strategy)  
- Conservation of total money across all accounts  

---

## 10. Performance Considerations

- Reader-writer locks improve concurrency for read-heavy workloads  
- Lock ordering prevents expensive deadlock recovery  
- Buffer pool introduces controlled contention  
- Tick-based scheduling enables consistent benchmarking  

---

## 11. Limitations

- No persistent storage (in-memory system only)  
- Fixed buffer pool size  
- No advanced buffer replacement strategies (e.g., LRU)  
- Deadlock detection strategy not implemented  

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