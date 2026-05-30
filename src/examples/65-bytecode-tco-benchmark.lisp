; -*- lisp -*-
; ============================================================
;  EXAMPLE 65 — Bytecode VM: TCO + benchmarks (Phase E.2)
; ============================================================
;
; The bytecode VM now has tail-call optimization. When a function
; call is in tail position, the VM replaces the current frame
; instead of pushing a new one — zero C stack growth.
;
;   (vm-eval expr)       — compile + execute on the VM
;   (vm-time expr)       — compile + execute, print elapsed time
;   (time-eval expr)     — tree-walk evaluate, print elapsed time
;   (disassemble expr)   — print bytecode

; ============================================================
;  1. TCO in action — deep recursion without stack overflow
; ============================================================

(display "=== Tail-call optimization ===") (newline)

;; sum-to via tail recursion: (sum-to 100000 0)
;; Without TCO this would overflow the C stack.
;; With TCO the VM reuses the same frame.
(display "  (vm-eval '(sum-to 10000)): ")
(display (vm-eval
  '((lambda (sum-to)
      (sum-to sum-to 10000 0))
    (lambda (self n acc)
      (if (= n 0) acc
        (self self (- n 1) (+ acc n)))))))
(newline)
(display "  ↑ 50005000 — computed via tail-recursive loop") (newline)
(newline)

;; Show the bytecode for the inner loop:
(display "  Bytecode for the accumulator loop:") (newline)
(disassemble '(lambda (self n acc)
  (if (= n 0) acc
    (self self (- n 1) (+ acc n)))))
(newline)

; ============================================================
;  2. Performance comparison: VM vs tree-walker
; ============================================================

(display "=== Benchmark: VM vs tree-walker ===") (newline)
(display "") (newline)

;; Fibonacci — deliberately NOT tail-recursive to stress both paths.
(display "  fib(25) via tree-walker:") (newline)
(time-eval '((lambda (fib)
  (fib fib 25))
  (lambda (self n)
    (if (< n 2) n
      (+ (self self (- n 1)) (self self (- n 2)))))))
(newline)

(display "  fib(25) via bytecode VM:") (newline)
(vm-time '((lambda (fib)
  (fib fib 25))
  (lambda (self n)
    (if (< n 2) n
      (+ (self self (- n 1)) (self self (- n 2)))))))
(newline)

; ============================================================
;  3. Tail-recursive benchmark
; ============================================================

(display "  count-down(100000) via tree-walker:") (newline)
(time-eval '((lambda (count)
  (count count 100000))
  (lambda (self n)
    (if (= n 0) 'done
      (self self (- n 1))))))
(newline)

(display "  count-down(100000) via bytecode VM (with TCO):") (newline)
(vm-time '((lambda (count)
  (count count 100000))
  (lambda (self n)
    (if (= n 0) 'done
      (self self (- n 1))))))
(newline)

(display "=== End of E.2 benchmarks ===") (newline)
