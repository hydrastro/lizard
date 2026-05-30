; -*- lisp -*-
; lib/concurrent.lisp — Concurrency patterns over atoms (Track C).
;
; Atoms provide atomic compare-and-swap semantics via swap!.
; These abstractions build higher-level coordination patterns
; on top of that primitive.

; ============================================================
;  COUNTER — atomic integer counter
; ============================================================
; Returns a counter object: an atom holding an integer.

(define (make-counter)
  (atom 0))

(define (counter-inc! c)
  (swap! c (lambda (n) (+ n 1))))

(define (counter-dec! c)
  (swap! c (lambda (n) (- n 1))))

(define (counter-add! c delta)
  (swap! c (lambda (n) (+ n delta))))

(define (counter-get c)
  (deref c))

(define (counter-reset! c)
  (reset! c 0))

; ============================================================
;  ACCUMULATOR — collect values into a list atomically
; ============================================================

(define (make-accumulator)
  (atom '()))

(define (accumulate! acc x)
  (swap! acc (lambda (lst) (cons x lst))))

(define (accumulator-values acc)
  (reverse (deref acc)))

(define (accumulator-count acc)
  (length (deref acc)))

; ============================================================
;  CELL — single-value reference with history
; ============================================================
; A cell tracks its current value and all past values.

(define (make-cell init)
  (atom (list init)))

(define (cell-set! cell val)
  (swap! cell (lambda (history) (cons val history))))

(define (cell-get cell)
  (car (deref cell)))

(define (cell-history cell)
  (deref cell))

(define (cell-rollback! cell)
  (swap! cell (lambda (history)
                (if (null? (cdr history)) history (cdr history)))))

; ============================================================
;  MEMOIZED REFERENCE — compute once, cache forever
; ============================================================
; Wraps a thunk; the first deref computes and caches the result.

(define (make-lazy-ref thunk)
  (atom (cons 'unforced thunk)))

(define (lazy-ref-get ref)
  (let ((state (deref ref)))
    (if (equal? (car state) 'forced)
        (cdr state)
        (let ((value ((cdr state))))
          (reset! ref (cons 'forced value))
          value))))

; ============================================================
;  REGISTRY — atomic key-value store (alist-backed)
; ============================================================

(define (make-registry)
  (atom '()))

(define (registry-put! reg key val)
  (swap! reg (lambda (alist)
               (cons (cons key val)
                     (filter (lambda (p) (not (equal? (car p) key))) alist)))))

(define (registry-get reg key)
  (let ((pair (assoc key (deref reg))))
    (if pair (cdr pair) #f)))

(define (registry-keys reg)
  (map car (deref reg)))

(define (registry-size reg)
  (length (deref reg)))

; ============================================================
;  SEMAPHORE — counting permit (cooperative)
; ============================================================
; Tracks available permits. acquire returns #t if a permit was
; available (and consumes it), #f otherwise.

(define (make-semaphore permits)
  (atom permits))

(define (semaphore-acquire! sem)
  (let ((available (deref sem)))
    (if (> available 0)
        (begin (swap! sem (lambda (n) (- n 1))) #t)
        #f)))

(define (semaphore-release! sem)
  (swap! sem (lambda (n) (+ n 1))))

(define (semaphore-available sem)
  (deref sem))
