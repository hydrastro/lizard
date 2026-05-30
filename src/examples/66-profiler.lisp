; -*- lisp -*-
; ============================================================
;  EXAMPLE 66 — Bytecode profiler (Phase E.2+)
; ============================================================
;
; (profile expr) compiles to bytecode, executes with instruction
; counting, and prints a detailed report.

(display "=== Profile: fibonacci ===") (newline)
(profile '((lambda (fib)
  (fib fib 20))
  (lambda (self n)
    (if (< n 2) n
      (+ (self self (- n 1)) (self self (- n 2)))))))
(newline)

(display "=== Profile: tail-recursive sum ===") (newline)
(profile '((lambda (sum)
  (sum sum 100000 0))
  (lambda (self n acc)
    (if (= n 0) acc
      (self self (- n 1) (+ acc n))))))
(newline)

(display "=== Profile: list building ===") (newline)
(profile '((lambda (build)
  (build build 1000 '()))
  (lambda (self n lst)
    (if (= n 0) lst
      (self self (- n 1) (cons n lst))))))
(newline)

(display "=== End of profiler example ===") (newline)
