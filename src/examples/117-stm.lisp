; -*- lisp -*-
; ============================================================
;  EXAMPLE 117 — Track C: Software Transactional Memory
; ============================================================

(import "match.lisp")
(import "stm.lisp")

(display "=== VERSIONED REFS ===") (newline)
(define account-a (make-ref 100))
(define account-b (make-ref 50))
(display "  account-a: value=") (display (ref-value account-a))
(display " version=") (display (ref-version account-a)) (newline)
(display "  account-b: value=") (display (ref-value account-b))
(display " version=") (display (ref-version account-b)) (newline)
(newline)

(display "=== ATOMIC TRANSFER ===") (newline)
(display "  transfer 30 from a to b (atomically):") (newline)
(stm-transfer! account-a account-b 30)
(display "  account-a: ") (display (ref-value account-a))
(display " (v") (display (ref-version account-a)) (display ")") (newline)
(display "  account-b: ") (display (ref-value account-b))
(display " (v") (display (ref-version account-b)) (display ")") (newline)
(display "  total preserved: ")
(display (+ (ref-value account-a) (ref-value account-b))) (newline)
(newline)

(display "=== ATOMIC SINGLE-REF UPDATE ===") (newline)
(define counter (make-ref 0))
(stm-update! counter (lambda (n) (+ n 1)))
(stm-update! counter (lambda (n) (+ n 1)))
(stm-update! counter (lambda (n) (* n 10)))
(display "  counter after +1 +1 *10: ") (display (ref-value counter)) (newline)
(display "  version (3 commits): ") (display (ref-version counter)) (newline)
(newline)

(display "=== MULTI-REF TRANSACTION ===") (newline)
; Atomically: move money AND log the transaction count.
(define balance (make-ref 1000))
(define tx-count (make-ref 0))
(atomically
  (lambda (tx)
    (tx-write! tx balance (- (tx-read tx balance) 250))
    (tx-write! tx tx-count (+ (tx-read tx tx-count) 1))))
(display "  balance: ") (display (ref-value balance)) (newline)
(display "  tx-count: ") (display (ref-value tx-count)) (newline)
(display "  (both updated atomically in one transaction)") (newline)
(newline)

(display "=== READ-YOUR-WRITES within a transaction ===") (newline)
(define r (make-ref 5))
(define result
  (atomically
    (lambda (tx)
      (tx-write! tx r 100)
      ; reading r inside the same tx sees the pending write
      (tx-read tx r))))
(display "  wrote 100, then read inside same tx: ") (display result) (newline)
(display "  committed value: ") (display (ref-value r)) (newline)
(newline)

(display "=== TRANSACTION VALIDATION (optimistic concurrency) ===") (newline)
(display "  Each transaction validates that refs it read are") (newline)
(display "  unchanged before committing; otherwise it retries.") (newline)
(display "  This is Clojure-style STM: lock-free, composable,") (newline)
(display "  and atomic across any number of refs.") (newline)
(newline)

(display "=== Track C milestone: STM ✓ ===") (newline)
