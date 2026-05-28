; -*- lisp -*-
; lib/stm.lisp — Software Transactional Memory (Track C).
;
; Refs are versioned mutable cells. A transaction reads and writes
; refs in a local log, then COMMITS atomically: it validates that
; every ref it read is unchanged, applies all writes, and bumps
; versions. If validation fails it retries. This is optimistic
; concurrency control — the heart of Clojure's STM.
;
; Each ref carries a unique id so it can be used as a log key
; (atoms compare by content, not identity).

(import "match.lisp")

; ============================================================
;  REFS  (versioned cells)
; ============================================================

(define ref-id-counter (atom 0))

(define (make-ref v)
  (swap! ref-id-counter (lambda (n) (+ n 1)))
  (list 'ref (deref ref-id-counter) (atom (cons 0 v))))

(define (ref? x) (and (pair? x) (equal? (car x) 'ref)))
(define (ref-id r) (car (cdr r)))
(define (ref-cell r) (car (cdr (cdr r))))

; Direct (non-transactional) access.
(define (ref-version r) (car (deref (ref-cell r))))
(define (ref-value r) (cdr (deref (ref-cell r))))
(define (ref-set-raw! r v)
  (reset! (ref-cell r) (cons (+ 1 (ref-version r)) v)))

; ============================================================
;  TRANSACTIONS
; ============================================================
; A transaction is an atom holding (cons reads writes):
;   reads  : alist (ref-id . (cons ref version-seen))
;   writes : alist (ref-id . (cons ref new-value))

(define (tx-begin) (atom (cons '() '())))
(define (tx-reads tx) (car (deref tx)))
(define (tx-writes tx) (cdr (deref tx)))

(define (tx-set-reads! tx reads)
  (reset! tx (cons reads (tx-writes tx))))
(define (tx-set-writes! tx writes)
  (reset! tx (cons (tx-reads tx) writes)))

; Read a ref inside a transaction. If written earlier in this tx,
; return that pending value; otherwise read the cell and log the
; version observed.
(define (tx-read tx r)
  (let ((w (assoc (ref-id r) (tx-writes tx))))
    (if w
        (cdr (cdr w))                       ; pending write value
        (begin
          (tx-set-reads! tx
            (alist-put (tx-reads tx) (ref-id r) (cons r (ref-version r))))
          (ref-value r)))))

; Write a ref inside a transaction (buffered until commit).
(define (tx-write! tx r v)
  (tx-set-writes! tx
    (alist-put (tx-writes tx) (ref-id r) (cons r v)))
  v)

; alist put (replace existing key).
(define (alist-put al key val)
  (cons (cons key val)
        (filter (lambda (p) (not (equal? (car p) key))) al)))

; ============================================================
;  COMMIT
; ============================================================
; Validate all read versions, then apply writes atomically.
; Returns #t on success, #f if a conflict was detected.

(define (tx-validate tx)
  (fold-left
    (lambda (ok entry)
      (and ok
           (let ((r (car (cdr entry)))
                 (seen (cdr (cdr entry))))
             (= (ref-version r) seen))))
    #t
    (tx-reads tx)))

(define (tx-apply-writes! tx)
  (for-each
    (lambda (entry)
      (let ((r (car (cdr entry)))
            (v (cdr (cdr entry))))
        (ref-set-raw! r v)))
    (tx-writes tx)))

(define (tx-commit! tx)
  (if (tx-validate tx)
      (begin (tx-apply-writes! tx) #t)
      #f))

; ============================================================
;  ATOMIC TRANSACTION RUNNER (with retry)
; ============================================================
; body-fn receives the transaction handle; do tx-read / tx-write!
; through it. Commits atomically; retries on conflict.

(define (atomically body-fn)
  (define (attempt n)
    (if (= n 0)
        (begin (display "  STM: transaction failed after retries") (newline) #f)
      (let ((tx (tx-begin)))
        (let ((result (body-fn tx)))
          (if (tx-commit! tx)
              result
              (attempt (- n 1)))))))
  (attempt 100))

; ============================================================
;  CONVENIENCE: common atomic operations
; ============================================================

; Atomically transfer `amount` from ref a to ref b.
(define (stm-transfer! a b amount)
  (atomically
    (lambda (tx)
      (tx-write! tx a (- (tx-read tx a) amount))
      (tx-write! tx b (+ (tx-read tx b) amount)))))

; Atomically update a single ref with a function.
(define (stm-update! r f)
  (atomically
    (lambda (tx)
      (tx-write! tx r (f (tx-read tx r))))))
