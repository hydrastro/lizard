; -*- lisp -*-
; ============================================================
;  EXAMPLE 78 — Complete kernel type theory showcase
; ============================================================

(display "╔══════════════════════════════════════════════════════╗") (newline)
(display "║  lizard kernel — Complete dependent type theory      ║") (newline)
(display "╚══════════════════════════════════════════════════════╝") (newline)
(newline)

; ═══════════════════════════════════════
; 1. BASE TYPES
; ═══════════════════════════════════════
(display "━━━ Base types ━━━") (newline)
(display "  Nat    : ") (display (kernel-infer 'Nat)) (newline)
(display "  Bool   : ") (display (kernel-infer 'Bool)) (newline)
(display "  Unit   : ") (display (kernel-infer 'Unit)) (newline)
(newline)

; ═══════════════════════════════════════
; 2. CONSTRUCTORS
; ═══════════════════════════════════════
(display "━━━ Constructors ━━━") (newline)
(display "  0         : ") (display (kernel-infer 0)) (newline)
(display "  (succ 0)  : ") (display (kernel-infer '(succ 0))) (newline)
(display "  true      : ") (display (kernel-infer 'true)) (newline)
(display "  false     : ") (display (kernel-infer 'false)) (newline)
(display "  *         : ") (display (kernel-infer '*)) (newline)
(newline)

; ═══════════════════════════════════════
; 3. TYPE FORMERS
; ═══════════════════════════════════════
(display "━━━ Type formers ━━━") (newline)
(display "  (List Nat)     : ") (display (kernel-infer '(List Nat))) (newline)
(display "  (Maybe Bool)   : ") (display (kernel-infer '(Maybe Bool))) (newline)
(display "  (Pi (x Nat) Nat)    : ") (display (kernel-infer '(Pi (x Nat) Nat))) (newline)
(display "  (Sigma (x Nat) Bool) : ") (display (kernel-infer '(Sigma (x Nat) Bool))) (newline)
(display "  (Id Nat 0 0)  : ") (display (kernel-infer '(Id Nat 0 0))) (newline)
(newline)

; ═══════════════════════════════════════
; 4. DATA CONSTRUCTORS
; ═══════════════════════════════════════
(display "━━━ Data constructors ━━━") (newline)
(display "  (cons 0 nil)       : ") (display (kernel-infer '(cons 0 nil))) (newline)
(display "  (just (succ 0))    : ") (display (kernel-infer '(just (succ 0)))) (newline)
(display "  (pair 0 true)      : ") (display (kernel-infer '(pair 0 true))) (newline)
(display "  (refl 0)           : ") (display (kernel-infer '(refl 0))) (newline)
(display "  (lam (x Nat) #0)   : ") (display (kernel-infer '(lam (x Nat) #0))) (newline)
(newline)

; ═══════════════════════════════════════
; 5. COMPUTATION (REDUCTION)
; ═══════════════════════════════════════
(display "━━━ Computation ━━━") (newline)
(display "  beta:  (app (lam (x Nat) (succ #0)) 0) → ")
(display (kernel-reduce '(app (lam (x Nat) (succ #0)) 0))) (newline)
(display "  fst:   (fst (pair 0 true)) → ")
(display (kernel-reduce '(fst (pair 0 true)))) (newline)
(display "  snd:   (snd (pair 0 true)) → ")
(display (kernel-reduce '(snd (pair 0 true)))) (newline)
(display "  if-t:  (if true 0 (succ 0)) → ")
(display (kernel-reduce '(if true 0 (succ 0)))) (newline)
(display "  if-f:  (if false 0 (succ 0)) → ")
(display (kernel-reduce '(if false 0 (succ 0)))) (newline)
(newline)

; ═══════════════════════════════════════
; 6. DEFINITIONAL EQUALITY
; ═══════════════════════════════════════
(display "━━━ Definitional equality ━━━") (newline)
(display "  (fst (pair 0 true)) ≡ 0?     ")
(display (kernel-equal? '(fst (pair 0 true)) 0)) (newline)
(display "  (if true 0 (succ 0)) ≡ 0?    ")
(display (kernel-equal? '(if true 0 (succ 0)) 0)) (newline)
(display "  (app (lam (x Nat) #0) 0) ≡ 0? ")
(display (kernel-equal? '(app (lam (x Nat) #0) 0) 0)) (newline)
(newline)

; ═══════════════════════════════════════
; 7. INTERACTIVE PROOF
; ═══════════════════════════════════════
(display "━━━ Interactive proof: 0 = 0 ━━━") (newline)
(begin-proof '(Id Nat 0 0))
(tactic-refl)
(qed)
(newline)

; ═══════════════════════════════════════
; 8. UNIFICATION
; ═══════════════════════════════════════
(display "━━━ Unification ━━━") (newline)
(define h0 (kernel-hole 'Nat))
(define h1 (kernel-hole 'Bool))
(kernel-unify '(pair ?0 ?1) '(pair (succ 0) true))
(newline)

; ═══════════════════════════════════════
; 9. UNIVERSE HIERARCHY
; ═══════════════════════════════════════
(display "━━━ Universe hierarchy ━━━") (newline)
(display "  Sort(0) : ") (display (kernel-infer '(Sort 0))) (newline)
(display "  Sort(1) : ") (display (kernel-infer '(Sort 1))) (newline)
(display "  Sort(2) : ") (display (kernel-infer '(Sort 2))) (newline)
(newline)

(display "╔══════════════════════════════════════════════════════╗") (newline)
(display "║  8 inductive types, 5 eliminators, unification,     ║") (newline)
(display "║  metavariables, tactics, and a universe hierarchy.   ║") (newline)
(display "║                                                      ║") (newline)
(display "║  This is Martin-Löf Type Theory in 900 lines of C.  ║") (newline)
(display "╚══════════════════════════════════════════════════════╝") (newline)
