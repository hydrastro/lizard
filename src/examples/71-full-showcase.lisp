; -*- lisp -*-
; ============================================================
;  EXAMPLE 71 — Full showcase: everything lizard can do
; ============================================================

(display "╔══════════════════════════════════════════╗") (newline)
(display "║  lizard v5 — complete feature showcase   ║") (newline)
(display "╚══════════════════════════════════════════╝") (newline)
(newline)

; ---- 1. Practical Scheme ----
(display "1. PRACTICAL SCHEME") (newline)
(display "   Arithmetic: (+ 1 (* 2 3) (- 10 4)) = ")
(display (+ 1 (* 2 3) (- 10 4))) (newline)
(display "   Closures: ")
(define (make-adder n) (lambda (x) (+ x n)))
(define add5 (make-adder 5))
(display (add5 10)) (newline)
(newline)

; ---- 2. List operations ----
(display "2. LIST OPERATIONS") (newline)
(display "   map:     ") (display (map (lambda (x) (* x x)) '(1 2 3 4 5))) (newline)
(display "   filter:  ") (display (filter (lambda (x) (> x 3)) '(1 2 3 4 5))) (newline)
(display "   fold:    ") (display (fold-left + 0 '(1 2 3 4 5))) (newline)
(display "   reverse: ") (display (reverse '(a b c d e))) (newline)
(display "   apply:   ") (display (apply + '(10 20 30))) (newline)
(display "   assoc:   ") (display (assoc 'b '((a 1) (b 2) (c 3)))) (newline)
(display "   iota:    ") (display (iota 8)) (newline)
(newline)

; ---- 3. Persistent vectors ----
(display "3. PERSISTENT VECTORS") (newline)
(define v (pvec 'a 'b 'c 'd 'e))
(display "   v = ") (display v) (newline)
(define v2 (pvec-set v 2 'Z))
(display "   (pvec-set v 2 'Z) = ") (display v2) (newline)
(display "   v still = ") (display v) (newline)
(newline)

; ---- 4. Persistent hash maps ----
(display "4. PERSISTENT HASH MAPS") (newline)
(define m (phash-map 'x 10 'y 20 'z 30))
(display "   m = ") (display m) (newline)
(display "   (phash-get m 'y) = ") (display (phash-get m 'y)) (newline)
(define m2 (phash-set m 'y 999))
(display "   (phash-get m2 'y) = ") (display (phash-get m2 'y)) (newline)
(display "   (phash-get m 'y) = ") (display (phash-get m 'y))
(display "  ← immutable!") (newline)
(newline)

; ---- 5. Bytecode VM ----
(display "5. BYTECODE VM + TCO") (newline)
(display "   (vm-eval '(+ (* 6 7) 1)) = ")
(display (vm-eval '(+ (* 6 7) 1))) (newline)
(display "   TCO sum-to 10000: ")
(display (vm-eval
  '((lambda (f) (f f 10000 0))
    (lambda (self n acc)
      (if (= n 0) acc (self self (- n 1) (+ acc n)))))))
(newline)
(newline)

; ---- 6. Profiler ----
(display "6. PROFILER") (newline)
(profile '((lambda (f) (f f 15))
  (lambda (self n)
    (if (< n 2) n (+ (self self (- n 1)) (self self (- n 2)))))))
(newline)

; ---- 7. Garbage collector ----
(display "7. GARBAGE COLLECTOR") (newline)
(display "   ") (display (gc-stats)) (newline)
(newline)

; ---- 8. Module system ----
(display "8. MODULE SYSTEM") (newline)
(import "math-utils.lisp")
(display "   Imported math-utils: (square 9) = ") (display (square 9)) (newline)
(display "   (module-loaded? \"math-utils.lisp\") = ")
(display (module-loaded? "math-utils.lisp")) (newline)
(newline)

; ---- 9. Syntax objects ----
(display "9. SYNTAX OBJECTS") (newline)
(define stx (datum->syntax #f '(define x 42)))
(display "   (syntax? stx) = ") (display (syntax? stx)) (newline)
(display "   (syntax-e stx) = ") (display (syntax-e stx)) (newline)
(newline)

; ---- 10. Kernel type checker ----
(display "10. KERNEL TYPE CHECKER") (newline)
(display "    0 : ") (display (kernel-infer 0)) (newline)
(display "    Nat : ") (display (kernel-infer 'Nat)) (newline)
(display "    (succ (succ 0)) : ") (display (kernel-infer '(succ (succ 0)))) (newline)
(display "    (refl 0) : ") (display (kernel-infer '(refl 0))) (newline)
(display "    (pair 0 (succ 0)) : ") (display (kernel-infer '(pair 0 (succ 0)))) (newline)
(display "    (fst (pair 0 (succ 0))) : ") (display (kernel-infer '(fst (pair 0 (succ 0))))) (newline)
(newline)

; ---- 11. Interactive proof ----
(display "11. INTERACTIVE PROOF: 0 = 0") (newline)
(begin-proof '(Id Nat 0 0))
(tactic-refl)
(qed)
(newline)

; ---- 12. Structured diagnostics ----
(display "12. STRUCTURED DIAGNOSTICS") (newline)
(display "    Errors carry source locations:") (newline)
(display "    ") (display (error-location (eval 'undefined-xyz))) (newline)
(newline)

(display "╔══════════════════════════════════════════╗") (newline)
(display "║  All systems operational.                ║") (newline)
(display "╚══════════════════════════════════════════╝") (newline)
