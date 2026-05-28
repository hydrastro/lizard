; -*- lisp -*-
; ============================================================
;  EXAMPLE 96 — Control-flow forms (Phase 4)
; ============================================================

(import "match.lisp")
(import "control.lisp")

(display "=== CASE ===") (newline)
(define (day-type n)
  (case-of n
    (list
      (list '(0 6) (lambda () "weekend"))
      (list '(1 2 3 4 5) (lambda () "weekday"))
      (list 'else (lambda () "invalid")))))
(display "  day 0: ") (display (day-type 0)) (newline)
(display "  day 3: ") (display (day-type 3)) (newline)
(display "  day 9: ") (display (day-type 9)) (newline)
(newline)

(display "=== WHEN / UNLESS ===") (newline)
(display "  when #t: ")
(display (when-do #t (lambda () "ran"))) (newline)
(display "  when #f: ")
(display (when-do #f (lambda () "ran"))) (newline)
(display "  unless #f: ")
(display (unless-do #f (lambda () "ran"))) (newline)
(newline)

(display "=== COND ===") (newline)
(define (sign n)
  (cond-of
    (list
      (list (lambda () (> n 0)) (lambda () "positive"))
      (list (lambda () (< n 0)) (lambda () "negative"))
      (list 'else (lambda () "zero")))))
(display "  sign 5: ") (display (sign 5)) (newline)
(display "  sign -3: ") (display (sign -3)) (newline)
(display "  sign 0: ") (display (sign 0)) (newline)
(newline)

(display "=== DOTIMES ===") (newline)
(define counter (atom 0))
(define results
  (dotimes 5 (lambda ()
                (swap! counter (lambda (n) (+ n 1)))
                (deref counter))))
(display "  dotimes 5 collecting counter: ") (display results) (newline)
(newline)

(display "=== WHILE ===") (newline)
(define n (atom 10))
(define iters
  (while-do
    (lambda () (> (deref n) 0))
    (lambda () (reset! n (- (deref n) 1)))))
(display "  while (n>0) decrementing from 10: ") (display iters) (display " iterations") (newline)
(newline)

(display "=== THREADING ===") (newline)
(display "  thread 5 through [*2, +1, *10]: ")
(display (thread-through 5 (list (lambda (x) (* x 2))
                                 (lambda (x) (+ x 1))
                                 (lambda (x) (* x 10))))) (newline)
(newline)

(display "=== APPLY-WHEN ===") (newline)
(display "  double 5 if even? (no): ")
(display (apply-when even? (lambda (x) (* x 2)) 5)) (newline)
(display "  double 4 if even? (yes): ")
(display (apply-when even? (lambda (x) (* x 2)) 4)) (newline)
(newline)

(display "=== Phase 4: control forms ✓ ===") (newline)
