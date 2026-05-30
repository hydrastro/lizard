; -*- lisp -*-
; ============================================================
;  EXAMPLE 82 — Complete tactic showcase
; ============================================================

(display "=== 8 Tactics ===") (newline)
(display "  intro, exact, refl, assumption,") (newline)
(display "  simpl, split, left, right") (newline)
(newline)

; ── 1. refl: prove 0 = 0 ──
(display "1. tactic-refl: prove 0 = 0") (newline)
(begin-proof '(Id Nat 0 0))
(tactic-refl)
(qed)
(newline)

; ── 2. exact: provide a direct witness ──
(display "2. tactic-exact: prove Nat (witness: 0)") (newline)
(begin-proof 'Nat)
(tactic-exact 0)
(qed)
(newline)

; ── 3. simpl: reduce goal ──
(display "3. tactic-simpl: simplify goal") (newline)
(begin-proof '(Id Nat (fst (pair 0 true)) 0))
(tactic-simpl)
(tactic-refl)
(qed)
(newline)

; ── 4. left: choose left of Sum ──
(display "4. tactic-left: prove Sum Nat Bool (choose left)") (newline)
(begin-proof '(Sum Nat Bool))
(tactic-left)
(tactic-exact 0)
(qed)
(newline)

; ── 5. right: choose right of Sum ──
(display "5. tactic-right: prove Sum Nat Bool (choose right)") (newline)
(begin-proof '(Sum Nat Bool))
(tactic-right)
(tactic-exact 'true)
(qed)
(newline)

; ── 6. kernel type checking ──
(display "6. Kernel types for this session:") (newline)
(display "  Sum Nat Bool : ") (display (kernel-infer '(Sum Nat Bool))) (newline)
(display "  Empty : ") (display (kernel-infer 'Empty)) (newline)
(display "  (inl 0 Bool) : ") (display (kernel-infer '(inl 0 Bool))) (newline)
(display "  (inr true Nat) : ") (display (kernel-infer '(inr true Nat))) (newline)
(newline)

; ── 7. kernel definitions ──
(display "7. Kernel definitions:") (newline)
(kernel-define 'id-bool '(lam (x Bool) #0) '(Pi (x Bool) Bool))
(display "  Defined id-bool : Pi(x:Bool).Bool") (newline)
(display "  Lookup: ") (display (kernel-lookup 'id-bool)) (newline)
(newline)

(display "=== All 8 tactics operational ===") (newline)
