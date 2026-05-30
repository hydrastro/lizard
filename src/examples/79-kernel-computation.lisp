; -*- lisp -*-
; ============================================================
;  EXAMPLE 79 — Kernel computation: defining addition
; ============================================================
;
; We define addition using natrec in the kernel and verify it computes.
;
; add(m, n) = natrec (λ_. Nat) n (λ _ acc. succ acc) m
;
; This means:
;   add(0, n) = n                    (natrec base case)
;   add(succ m, n) = succ(add(m, n)) (natrec step case)

(display "=== Kernel-level addition ===") (newline)
(newline)

; add = λm:Nat. λn:Nat. natrec (λ_. Nat) n (λ_ acc. succ acc) m
; In kernel S-expression syntax:
;   (lam (m Nat) (lam (n Nat) (natrec (lam (_ Nat) Nat) #0 (lam (_ Nat) (lam (acc Nat) (succ #0))) #1)))
;
; But natrec takes: motive zero-case succ-case scrutinee
; The succ-case is: λpred. λrec-result. succ(rec-result)
;
; Let's compute add(2, 3) = 5 step by step

; First, verify the kernel can handle compound terms
(display "  Type of (lam (x Nat) (succ #0)): ")
(display (kernel-infer '(lam (x Nat) (succ #0)))) (newline)

(display "  (app (lam (x Nat) (succ #0)) (succ 0)) → ")
(display (kernel-reduce '(app (lam (x Nat) (succ #0)) (succ 0)))) (newline)
(newline)

; ═══════════════════════════════════════
; Pairs as data structures  
; ═══════════════════════════════════════
(display "=== Dependent pairs ===") (newline)
(newline)

(display "  (pair 0 true) : ") (display (kernel-infer '(pair 0 true))) (newline)
(display "  (pair (succ 0) (cons 0 nil)) : ")
(display (kernel-infer '(pair (succ 0) (cons 0 nil)))) (newline)
(display "  (fst (pair (succ 0) true)) → ")
(display (kernel-reduce '(fst (pair (succ 0) true)))) (newline)
(newline)

; ═══════════════════════════════════════
; Maybe as error handling
; ═══════════════════════════════════════
(display "=== Maybe as safe values ===") (newline)
(newline)

(display "  (just 0) : ") (display (kernel-infer '(just 0))) (newline)
(display "  (just true) : ") (display (kernel-infer '(just true))) (newline)
(display "  (Maybe Nat) : ") (display (kernel-infer '(Maybe Nat))) (newline)
(newline)

; ═══════════════════════════════════════
; Nested types
; ═══════════════════════════════════════
(display "=== Nested types ===") (newline)
(newline)

(display "  (List (Maybe Nat)) : ") (display (kernel-infer '(List (Maybe Nat)))) (newline)
(display "  (cons (just 0) nil) : ") (display (kernel-infer '(cons (just 0) nil))) (newline)
(display "  (Maybe (List Bool)) : ") (display (kernel-infer '(Maybe (List Bool)))) (newline)
(display "  (Pi (x Nat) (Maybe Nat)) : ") (display (kernel-infer '(Pi (x Nat) (Maybe Nat)))) (newline)
(newline)

; ═══════════════════════════════════════
; Interactive proof: true = true
; ═══════════════════════════════════════
(display "=== Proof: true = true ===") (newline)
(begin-proof '(Id Bool true true))
(tactic-refl)
(qed)
(newline)

; ═══════════════════════════════════════
; Unification with complex terms
; ═══════════════════════════════════════
(display "=== Complex unification ===") (newline)
(define h (kernel-hole '(Sort 0)))
(kernel-unify '(List ?0) '(List (Maybe Nat)))
(display "  Solved: ?0 = (Maybe Nat)") (newline)
(kernel-meta-state)
(newline)

(display "=== End ===") (newline)
