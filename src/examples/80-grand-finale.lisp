; -*- lisp -*-
; ============================================================
;  EXAMPLE 80 — Grand finale: complete type theory
; ============================================================

(display "╔═════════════════════════════════════════════════════╗") (newline)
(display "║  LIZARD — Complete Dependent Type Theory            ║") (newline)
(display "║  Martin-Löf TT + Bool + List + Maybe + Sum + Empty  ║") (newline)
(display "╚═════════════════════════════════════════════════════╝") (newline)
(newline)

; ═══════════════════════════════════════
; 1. ALL TYPES
; ═══════════════════════════════════════
(display "━━━ Complete type inventory ━━━") (newline)
(display "  Nat           : ") (display (kernel-infer 'Nat)) (newline)
(display "  Bool          : ") (display (kernel-infer 'Bool)) (newline)
(display "  Unit          : ") (display (kernel-infer 'Unit)) (newline)
(display "  Empty         : ") (display (kernel-infer 'Empty)) (newline)
(display "  (List Nat)    : ") (display (kernel-infer '(List Nat))) (newline)
(display "  (Maybe Bool)  : ") (display (kernel-infer '(Maybe Bool))) (newline)
(display "  (Sum Nat Bool): ") (display (kernel-infer '(Sum Nat Bool))) (newline)
(display "  (Pi (x Nat) Bool)    : ") (display (kernel-infer '(Pi (x Nat) Bool))) (newline)
(display "  (Sigma (x Nat) Bool) : ") (display (kernel-infer '(Sigma (x Nat) Bool))) (newline)
(display "  (Id Nat 0 0)  : ") (display (kernel-infer '(Id Nat 0 0))) (newline)
(newline)

; ═══════════════════════════════════════
; 2. ALL CONSTRUCTORS
; ═══════════════════════════════════════
(display "━━━ Constructors ━━━") (newline)
(display "  0             : ") (display (kernel-infer 0)) (newline)
(display "  (succ 0)      : ") (display (kernel-infer '(succ 0))) (newline)
(display "  true          : ") (display (kernel-infer 'true)) (newline)
(display "  false         : ") (display (kernel-infer 'false)) (newline)
(display "  *             : ") (display (kernel-infer '*)) (newline)
(display "  nil           : ") (display (kernel-infer '(cons 0 nil))) (newline)
(display "  (just 0)      : ") (display (kernel-infer '(just 0))) (newline)
(display "  (inl 0 Bool)  : ") (display (kernel-infer '(inl 0 Bool))) (newline)
(display "  (inr true Nat): ") (display (kernel-infer '(inr true Nat))) (newline)
(display "  (pair 0 true) : ") (display (kernel-infer '(pair 0 true))) (newline)
(display "  (refl 0)      : ") (display (kernel-infer '(refl 0))) (newline)
(display "  (lam (x Nat) #0) : ") (display (kernel-infer '(lam (x Nat) #0))) (newline)
(newline)

; ═══════════════════════════════════════
; 3. COMPUTATION
; ═══════════════════════════════════════
(display "━━━ Computation rules ━━━") (newline)
(display "  β:  (app (lam (x Nat) (succ #0)) 0)  → ")
(display (kernel-reduce '(app (lam (x Nat) (succ #0)) 0))) (newline)
(display "  π₁: (fst (pair 0 true))              → ")
(display (kernel-reduce '(fst (pair 0 true)))) (newline)
(display "  π₂: (snd (pair 0 true))              → ")
(display (kernel-reduce '(snd (pair 0 true)))) (newline)
(display "  if: (if true 0 (succ 0))             → ")
(display (kernel-reduce '(if true 0 (succ 0)))) (newline)
(newline)

; ═══════════════════════════════════════
; 4. KERNEL DEFINITIONS
; ═══════════════════════════════════════
(display "━━━ Kernel definitions ━━━") (newline)

; Define the identity function on Nat
(kernel-define 'id-nat
  '(lam (x Nat) #0)
  '(Pi (x Nat) Nat))

; Define a constant function
(kernel-define 'const-zero
  '(lam (x Nat) 0)
  '(Pi (x Nat) Nat))

(display "  (kernel-lookup 'id-nat): ") (display (kernel-lookup 'id-nat)) (newline)
(display "  (kernel-lookup 'const-zero): ") (display (kernel-lookup 'const-zero)) (newline)
(newline)

; ═══════════════════════════════════════
; 5. PROOFS
; ═══════════════════════════════════════
(display "━━━ Proofs ━━━") (newline)
(newline)

(display "  Proof: 0 = 0") (newline)
(begin-proof '(Id Nat 0 0))
(tactic-refl)
(qed)
(newline)

(display "  Proof: true = true") (newline)
(begin-proof '(Id Bool true true))
(tactic-refl)
(qed)
(newline)

; ═══════════════════════════════════════
; 6. UNIFICATION
; ═══════════════════════════════════════
(display "━━━ Unification ━━━") (newline)
(define h0 (kernel-hole '(Sort 0)))
(define h1 (kernel-hole '(Sort 0)))
(display "  Unify (Sum ?0 ?1) with (Sum Nat Bool):") (newline)
(kernel-unify '(Sum ?0 ?1) '(Sum Nat Bool))
(newline)

; ═══════════════════════════════════════
; 7. NESTED TYPES
; ═══════════════════════════════════════
(display "━━━ Type composition ━━━") (newline)
(display "  (List (Maybe (Sum Nat Bool))) : ")
(display (kernel-infer '(List (Maybe (Sum Nat Bool))))) (newline)
(display "  (Pi (x Nat) (Maybe (List Bool))) : ")
(display (kernel-infer '(Pi (x Nat) (Maybe (List Bool))))) (newline)
(display "  (Sigma (x (List Nat)) (Id Nat 0 0)) : ")
(display (kernel-infer '(Sigma (x (List Nat)) (Id Nat 0 0)))) (newline)
(newline)

(display "╔═════════════════════════════════════════════════════╗") (newline)
(display "║  10 type formers, 12 constructors, 6 eliminators,  ║") (newline)
(display "║  metavariables, unification, tactics, definitions. ║") (newline)
(display "║                                                     ║") (newline)
(display "║  Martin-Löf Type Theory — complete.                 ║") (newline)
(display "╚═════════════════════════════════════════════════════╝") (newline)
