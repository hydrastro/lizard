; 145-object-gc.lisp — Phase 5: per-object, non-moving, conservative GC.
;
; Demonstrates that (gc) reclaims individual dead objects while every kind of
; live value survives collection AND the subsequent reuse of freed chunks.
; Self-checking: each `must` raises on failure, so the example exits non-zero
; (and the runner marks it failed) if the collector ever loses live data.

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'gc-check-failed)))

; ---- diverse live data, all reachable from the top level ----
(define live-list  (list 1 2 3 5 8 13 21))
(define live-deep  (list (list "a" 1) (list "b" 2) (list (list "c" 3) 4)))
(define live-map   (phash-map "one" 1 "two" 2 "three" 3))
(define live-set   (pset 11 22 33 44))
(define live-rat   (/ 355 113))
(define live-clos  (let ((base 1000)) (lambda (n) (+ base n))))
(define live-str   "a-string-that-must-not-be-clobbered")

; ---- a garbage generator whose allocations are genuinely dropped ----
(define (burn n)
  (if (> n 0)
      (begin
        (cons n (list n (* n n) "scratch"))   ; allocated and immediately dropped
        (burn (- n 1)))
      'ok))

(define (all-live-intact?)
  (and (equal? live-list (list 1 2 3 5 8 13 21))
       (equal? (car (cdr (cdr live-deep))) (list (list "c" 3) 4))
       (= (phash-get live-map "two") 2)
       (= (phash-get live-map "three") 3)
       (pset-contains? live-set 33)
       (not (pset-contains? live-set 99))
       (= live-rat (/ 355 113))
       (= (live-clos 7) 1007)
       (equal? live-str "a-string-that-must-not-be-clobbered")))

(display "object-gc: reclamation + survival across reuse") (newline)

(must "initial state" (all-live-intact?))

; Cycle: make garbage, collect, allocate again (reusing freed chunks), verify.
(define (cycle k)
  (if (> k 0)
      (begin
        (burn 1200)
        (gc)
        (burn 1200)
        (must "live data survived gc+reuse" (all-live-intact?))
        (cycle (- k 1)))
      'done))

(cycle 6)

; The collector actually reclaims individual objects: after making a pile of
; garbage, a collection reports a positive number of reclaimed bytes.
(burn 1500)
(define freed (cdr (assoc 'freed-bytes (gc))))
(must "gc reclaimed dead objects (freed > 0)" (> freed 0))

; Live data is still intact after measuring.
(must "final state" (all-live-intact?))

(display "object-gc: all checks passed") (newline)
