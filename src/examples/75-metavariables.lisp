; -*- lisp -*-
; ============================================================
;  EXAMPLE 75 — Metavariables / Holes (K.4)
; ============================================================
;
; The kernel now supports metavariables — placeholder terms
; that represent unknowns. They are the foundation for
; elaboration and type inference.
;
; Workflow:
;   1. Create holes with (kernel-hole type)
;   2. Solve them with (kernel-solve id term)
;   3. Substitute solutions with (kernel-zonk term)

(display "=== Metavariables / Holes ===") (newline)
(newline)

; Create a hole of type Nat
(display "  Create hole ?0 : Nat") (newline)
(define h0 (kernel-hole 'Nat))
(newline)

; Create another hole of type Bool
(display "  Create hole ?1 : Bool") (newline)
(define h1 (kernel-hole 'Bool))
(newline)

; View the metavar state
(display "  Current meta state:") (newline)
(kernel-meta-state)
(newline)

; Solve ?0 = (succ (succ 0))  (i.e., 2)
(display "  Solving ?0 = 2:") (newline)
(kernel-solve h0 '(succ (succ 0)))
(newline)

; Solve ?1 = true
(display "  Solving ?1 = true:") (newline)
(kernel-solve h1 'true)
(newline)

; View the meta state again
(display "  Meta state after solving:") (newline)
(kernel-meta-state)
(newline)

(display "=== Bool computation ===") (newline)
(newline)
(display "  (if true 0 (succ 0)) → ")
(display (kernel-reduce '(if true 0 (succ 0)))) (newline)
(display "  (if false 0 (succ 0)) → ")
(display (kernel-reduce '(if false 0 (succ 0)))) (newline)
(display "  true : ") (display (kernel-infer 'true)) (newline)
(display "  false : ") (display (kernel-infer 'false)) (newline)
(display "  (kernel-check true Bool): ")
(display (kernel-check 'true 'Bool)) (newline)
(newline)

(display "=== End of metavariables ===") (newline)
