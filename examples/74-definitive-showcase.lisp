; -*- lisp -*-
; ============================================================
;  EXAMPLE 74 — lizard v5 definitive showcase
; ============================================================

(display "╔══════════════════════════════════════════════╗") (newline)
(display "║  lizard v5 — definitive feature showcase     ║") (newline)
(display "╚══════════════════════════════════════════════╝") (newline)
(newline)

; ═══════════════════════════════════════
; 1. KERNEL TYPE THEORY
; ═══════════════════════════════════════
(display "━━━ KERNEL TYPE THEORY ━━━") (newline)
(newline)

(display "  Type inference:") (newline)
(display "    0 : ") (display (kernel-infer 0)) (newline)
(display "    Nat : ") (display (kernel-infer 'Nat)) (newline)
(display "    Bool : ") (display (kernel-infer 'Bool)) (newline)
(display "    true : ") (display (kernel-infer 'true)) (newline)
(display "    Unit : ") (display (kernel-infer 'Unit)) (newline)
(display "    * : ") (display (kernel-infer '*)) (newline)
(display "    (refl 0) : ") (display (kernel-infer '(refl 0))) (newline)
(display "    (pair 0 true) : ") (display (kernel-infer '(pair 0 true))) (newline)
(newline)

(display "  Reduction:") (newline)
(display "    (fst (pair 0 true)) → ")
(display (kernel-reduce '(fst (pair 0 true)))) (newline)
(display "    (snd (pair 0 true)) → ")
(display (kernel-reduce '(snd (pair 0 true)))) (newline)
(display "    (app (lam (x Nat) (succ #0)) 0) → ")
(display (kernel-reduce '(app (lam (x Nat) (succ #0)) 0))) (newline)
(display "    (if true 0 (succ 0)) → ")
(display (kernel-reduce '(if true 0 (succ 0)))) (newline)
(display "    (if false 0 (succ 0)) → ")
(display (kernel-reduce '(if false 0 (succ 0)))) (newline)
(newline)

(display "  Definitional equality:") (newline)
(display "    (fst (pair 0 true)) ≡ 0? ")
(display (kernel-equal? '(fst (pair 0 true)) 0)) (newline)
(display "    (if true 0 (succ 0)) ≡ 0? ")
(display (kernel-equal? '(if true 0 (succ 0)) 0)) (newline)
(newline)

; ═══════════════════════════════════════
; 2. INTERACTIVE PROOFS
; ═══════════════════════════════════════
(display "━━━ INTERACTIVE PROOFS ━━━") (newline)
(newline)

(display "  Proof 1: 0 = 0") (newline)
(begin-proof '(Id Nat 0 0))
(tactic-refl)
(qed)
(newline)

(display "  Proof 2: true = true") (newline)
(begin-proof '(Id Bool true true))
(tactic-refl)
(qed)
(newline)

; ═══════════════════════════════════════
; 3. BYTECODE VM + PROFILER
; ═══════════════════════════════════════
(display "━━━ BYTECODE VM + TCO ━━━") (newline)
(newline)

(display "  VM eval: (+ (* 6 7) 1) = ")
(display (vm-eval '(+ (* 6 7) 1))) (newline)

(display "  TCO sum 1..50000: ")
(display (vm-eval
  '((lambda (f) (f f 50000 0))
    (lambda (self n acc)
      (if (= n 0) acc (self self (- n 1) (+ acc n)))))))
(newline)
(newline)

(display "  Profile fib(18):") (newline)
(profile '((lambda (f) (f f 18))
  (lambda (self n)
    (if (< n 2) n (+ (self self (- n 1)) (self self (- n 2)))))))
(newline)

; ═══════════════════════════════════════
; 4. PERSISTENT DATA STRUCTURES
; ═══════════════════════════════════════
(display "━━━ PERSISTENT DATA ━━━") (newline)
(newline)

(define v (pvec 10 20 30 40 50))
(display "  pvec: ") (display v) (newline)
(display "  pvec-set v 2 99: ") (display (pvec-set v 2 99)) (newline)
(display "  original: ") (display v) (newline)

(define m (phash-map 'name "lizard" 'version 5 'lang "scheme"))
(display "  phash-map: ") (display m) (newline)
(display "  phash-entries: ") (display (phash-entries m)) (newline)
(display "  phash-values: ") (display (phash-values m)) (newline)
(newline)

; ═══════════════════════════════════════
; 5. ATOMS + FUNCTIONAL PROGRAMMING
; ═══════════════════════════════════════
(display "━━━ ATOMS + FP ━━━") (newline)
(newline)

(define counter (atom 0))
(for-each (lambda (_) (swap! counter (lambda (n) (+ n 1))))
          (iota 100))
(display "  100 atomic increments: ") (display (deref counter)) (newline)

(display "  fold-left sum: ") (display (fold-left + 0 (iota 10 1))) (newline)
(display "  map square: ") (display (map (lambda (x) (* x x)) '(1 2 3 4 5))) (newline)
(display "  filter even: ") (display (filter (lambda (x) (= 0 (% x 2))) (iota 10 1))) (newline)
(newline)

; ═══════════════════════════════════════
; 6. STRINGS
; ═══════════════════════════════════════
(display "━━━ STRINGS ━━━") (newline)
(newline)
(display "  split: ") (display (string-split "hello,world,foo" ",")) (newline)
(display "  join: ") (display (string-join '("a" "b" "c") "-")) (newline)
(display "  upcase: ") (display (string-upcase "lizard")) (newline)
(newline)

; ═══════════════════════════════════════
; 7. MODULES + GC + DIAGNOSTICS
; ═══════════════════════════════════════
(display "━━━ INFRASTRUCTURE ━━━") (newline)
(newline)
(import "math-utils.lisp")
(display "  Module: (cube 4) = ") (display (cube 4)) (newline)
(display "  GC: ") (display (gc-stats)) (newline)
(newline)

; ═══════════════════════════════════════
; 8. SYNTAX OBJECTS
; ═══════════════════════════════════════
(display "━━━ SYNTAX OBJECTS ━━━") (newline)
(newline)
(define stx (datum->syntax #f '(lambda (x) (+ x 1))))
(display "  syntax?: ") (display (syntax? stx)) (newline)
(display "  datum: ") (display (syntax-e stx)) (newline)
(newline)

(display "╔══════════════════════════════════════════════╗") (newline)
(display "║  All 8 subsystems operational.               ║") (newline)
(display "║                                              ║") (newline)
(display "║  Kernel: Nat, Bool, Unit, Pi, Sigma, Id, J   ║") (newline)
(display "║  Tactics: intro, exact, refl, assumption     ║") (newline)
(display "║  VM: 30 opcodes, TCO, profiler               ║") (newline)
(display "║  Data: pvec, phash-map, atoms                 ║") (newline)
(display "║  Stdlib: 30+ functions                        ║") (newline)
(display "║  Infra: modules, GC, diagnostics              ║") (newline)
(display "╚══════════════════════════════════════════════╝") (newline)
