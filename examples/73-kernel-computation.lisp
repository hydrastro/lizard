; -*- lisp -*-
; ============================================================
;  EXAMPLE 73 — Kernel computation: natrec, reduction, proofs
; ============================================================
;
; The kernel can now COMPUTE with natural numbers using natrec
; and verify definitional equalities.

(display "=== Kernel reduction ===") (newline)
(newline)

; succ(succ(0)) reduces to itself (already normal)
(display "  (succ (succ 0)) reduces to: ")
(display (kernel-reduce '(succ (succ 0)))) (newline)

; fst(pair(0, succ(0))) reduces to 0
(display "  (fst (pair 0 (succ 0))) reduces to: ")
(display (kernel-reduce '(fst (pair 0 (succ 0))))) (newline)

; snd(pair(0, succ(0))) reduces to succ(0)
(display "  (snd (pair 0 (succ 0))) reduces to: ")
(display (kernel-reduce '(snd (pair 0 (succ 0))))) (newline)
(newline)

(display "=== Kernel definitional equality ===") (newline)
(newline)

; 0 = 0
(display "  0 ≡ 0? ") (display (kernel-equal? 0 0)) (newline)

; succ(0) = succ(0)
(display "  (succ 0) ≡ (succ 0)? ")
(display (kernel-equal? '(succ 0) '(succ 0))) (newline)

; succ(0) ≠ 0
(display "  (succ 0) ≡ 0? ")
(display (kernel-equal? '(succ 0) 0)) (newline)

; fst(pair(0, succ(0))) = 0
(display "  (fst (pair 0 (succ 0))) ≡ 0? ")
(display (kernel-equal? '(fst (pair 0 (succ 0))) 0)) (newline)

; Pair projections: fst and snd
(display "  (snd (pair 0 (succ 0))) ≡ (succ 0)? ")
(display (kernel-equal? '(snd (pair 0 (succ 0))) '(succ 0))) (newline)
(newline)

(display "=== Type inference (extended) ===") (newline)
(newline)

(display "  (pair 0 (succ 0)) : ")
(display (kernel-infer '(pair 0 (succ 0)))) (newline)

(display "  (fst (pair 0 (succ 0))) : ")
(display (kernel-infer '(fst (pair 0 (succ 0))))) (newline)

(display "  (lam (x Nat) #0) : ")
(display (kernel-infer '(lam (x Nat) #0))) (newline)

(display "  (app (lam (x Nat) #0) (succ 0)) : ")
(display (kernel-infer '(app (lam (x Nat) #0) (succ 0)))) (newline)
(newline)

(display "=== Beta reduction ===") (newline)
(newline)

; (λ x:Nat. x) applied to (succ 0) should reduce to (succ 0)
(display "  (app (lam (x Nat) #0) (succ 0)) reduces to: ")
(display (kernel-reduce '(app (lam (x Nat) #0) (succ 0)))) (newline)

; (λ x:Nat. succ(x)) applied to 0 should reduce to (succ 0)
(display "  (app (lam (x Nat) (succ #0)) 0) reduces to: ")
(display (kernel-reduce '(app (lam (x Nat) (succ #0)) 0))) (newline)
(newline)

(display "=== Interactive proof: 0 = 0 ===") (newline)
(begin-proof '(Id Nat 0 0))
(tactic-refl)
(qed)
(newline)

(display "=== Lazy evaluation ===") (newline)
(define expensive (delay '(begin (display "  Computing... ") (+ 40 2))))
(display "  Promise created, not yet forced.") (newline)
(display "  First force: ") (display (force expensive)) (newline)
(display "  Second force (cached): ") (display (force expensive)) (newline)
(display "  (promise? expensive): ") (display (promise? expensive)) (newline)
(newline)

(display "=== End of kernel computation ===") (newline)
