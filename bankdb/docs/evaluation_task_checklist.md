# Evaluation Task Checklist

## System Architecture Mistakes
- [x] Removed `compute_total_balance` from `bank.h`; it is declared only in `metrics.h`.
- [x] Kept `compute_total_balance` implemented in `metrics.c`, matching the metrics module boundary.
- [x] Added a conservative direct account lookup fast path while preserving the existing linear fallback.
- [ ] Add a full hash map for account lookup. Deferred because the current fixed account limit is small and the safer direct-index fast path covers the existing sequential ID traces.

## Robustness Mistakes
- [x] Corrected balance conservation to check `final_total == initial_total + net_external_flow`.
- [x] Counted `net_external_flow` only from committed `DEPOSIT` and `WITHDRAW` operations.
- [x] Excluded transfers from external flow because they only move money between accounts.
- [x] Excluded aborted transactions from external flow.
- [x] Replaced unsafe `atoi` parsing for `--tick-ms=` with validated `strtol`.
- [x] Rejected invalid tick input such as `--tick-ms=100abc`.
- [x] Added a reliable buffer pool blocking trace that shows `Blocked` greater than 0.

## Code Engineering Mistakes
- [x] Routed debug logs to `stderr` with `vfprintf(stderr, format, args)`.
- [x] Kept `va_start` and `va_end` usage intact in `debug_log`.
- [x] Added a small scheduler yield when the buffer pool reaches capacity so same-tick contention can exercise the blocking path reliably.
- [x] Re-ran compilation and tests after code changes.

## Collaboration Mistakes
- [x] Added `docs/test_outputs_2026-05-13.txt` with normal test output.
- [x] Added `docs/tsan_outputs_2026-05-13.txt` with ThreadSanitizer build/runtime output.
- [x] Documented the ThreadSanitizer runtime failure honestly instead of claiming a pass.
- [x] Added measured lock comparison data to `design.md`.
- [x] Ensured README and design documentation reference evidence files that exist.

## Highest-Priority Fixes
- [x] Fixed the `compute_total_balance` module boundary issue.
- [x] Fixed the balance conservation logic for committed deposits and withdrawals.
- [x] Added account lookup improvement without breaking existing behavior.
- [x] Fixed debug logging output stream.
- [x] Replaced unsafe `atoi` parsing.
- [x] Added missing evidence files.
- [ ] Produce a zero-warning ThreadSanitizer run. Not completed because the local environment aborts TSan at startup with `FATAL: ThreadSanitizer: unexpected memory mapping`; the committed evidence file records that failure.
- [x] Added a reliable buffer pool blocking trace and captured output showing `Blocked     : 1`.
- [x] Added pthread rwlock vs mutex benchmark data.
- [x] Cleaned README and design documentation to match the corrected behavior and evidence.
