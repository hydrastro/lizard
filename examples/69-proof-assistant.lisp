; -*- lisp -*-
; ============================================================
;  EXAMPLE 69 — Proof assistant: tactics + kernel
; ============================================================
;
; lizard now has a trusted type-checking kernel and a tactic system.
; The kernel verifies proofs independently. Tactics help construct
; proof terms interactively.

(display "=== Kernel type checking ===") (newline)
(newline)

(display "  0 : ") (display (kernel-infer 0)) (newline)
(display "  3 : ") (display (kernel-infer 3)) (newline)
(display "  Nat : ") (display (kernel-infer 'Nat)) (newline)
(display "  (succ 0) : ") (display (kernel-infer '(succ 0))) (newline)
(display "  (refl 0) : ") (display (kernel-infer '(refl 0))) (newline)
(newline)

(display "  0 : Nat? ") (display (kernel-check 0 'Nat)) (newline)
(display "  Nat : (Sort 0)? ") (display (kernel-check 'Nat '(Sort 0))) (newline)
(display "  (Sort 0) : (Sort 1)? ") (display (kernel-check '(Sort 0) '(Sort 1))) (newline)
(newline)

(display "=== Interactive proof: 0 = 0 ===") (newline)
(newline)

;; Prove: Id Nat 0 0 (that zero equals zero)
(begin-proof '(Id Nat 0 0))
;; The goal is Id Nat 0 0 — we can solve it with reflexivity.
(tactic-refl)
(qed)
(newline)

(display "=== End of proof assistant example ===") (newline)
