; -*- lisp -*-
; ============================================================
;  EXAMPLE 94 — Tactic combinators (Track K roadmap item)
; ============================================================
;
; Combinators let us build complex proof strategies from the
; primitive tactics — a roadmap milestone for Track K.

(import "tactics-ext.lisp")

(display "=== SEQUENCING (tseq) ===") (newline)
(display "  Prove (Id Nat (fst (pair 0 true)) 0) with simpl then refl:") (newline)
(begin-proof '(Id Nat (fst (pair 0 true)) 0))
(tseq (t-simpl) (t-refl))
(qed)
(newline)

(display "=== AUTO STRATEGY ===") (newline)
(display "  Prove 0 = 0 with t-auto (tries refl, then assumption):") (newline)
(prove-with '(Id Nat 0 0) (t-auto))
(newline)

(display "=== CRUSH STRATEGY (simpl + auto) ===") (newline)
(display "  Prove (Id Bool (if true true false) true) by computation:") (newline)
(prove-by-computation '(Id Bool (if true true false) true))
(newline)

(display "=== FIRST (try alternatives) ===") (newline)
(display "  Prove (Sum Nat Bool) trying left=0:") (newline)
(prove-sum '(Sum Nat Bool) 0)
(newline)

(display "=== STRATEGY COMPOSITION ===") (newline)
(display "  Combinators available:") (newline)
(display "    tseq    — run tactics in sequence (stop on failure)") (newline)
(display "    tfirst  — try alternatives until one succeeds") (newline)
(display "    ttry    — run a tactic, swallow failure") (newline)
(display "    trepeat — repeat a tactic until it fails") (newline)
(display "    t-auto  — refl or assumption") (newline)
(display "    t-crush — simpl then auto") (newline)
(newline)

(display "=== Roadmap milestone reached ===") (newline)
(display "  Track K: tactic combinators ✓") (newline)
(display "  Proof strategies can now be composed declaratively.") (newline)
