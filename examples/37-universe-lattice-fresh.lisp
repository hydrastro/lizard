; -*- lisp -*-
; ============================================================
;  UNIVERSE LATTICE — Phase L.3
;  Dimension-creating quantification
; ============================================================
;
; The thesis claim: "what generates a new universe is quantifying
; on a previous universe."
;
; This phase reifies that claim. Two new constructors:
;
;   (pi-fresh 'x A B)    — Pi that adds a fresh dimension
;   (sigma-fresh 'x A B) — Sigma that adds a fresh dimension
;
; They have the same TERM semantics as their non-fresh counterparts
; (binders, lambdas, applications all work identically). They differ
; only in the TYPING RULE:
;
;   A : U_S       B : U_T (under x:A)
;   ───────────────────────────────────────────────
;   (pi-fresh x A B) : (U-max U_S U_T) ∨ {fresh dim}
;
; Each call to pi-fresh or sigma-fresh allocates a NEW dimension from
; a per-session counter, so distinct quantification events produce
; universes with distinct dim sets.
;
; This is opt-in (alongside existing Pi/Sigma) so existing code keeps
; working. Whether to make it the default is a separate decision.

(display "============================================================") (newline)
(display "  Comparison: Pi vs pi-fresh") (newline)
(display "============================================================") (newline)
(newline)

(display "Old Pi — single level via max:") (newline)
(display "  (Pi 'x (U 0) (U 0)) : ")
(display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  (Pi 'x (U 2) (U 5)) : ")
(display (infer (context) (Pi 'x (U 2) (U 5)))) (newline)
(display "  ↑ no fresh dimension introduced") (newline)
(newline)

(display "New pi-fresh — adds a fresh dimension:") (newline)
(display "  (pi-fresh 'x (U 0) (U 0)) : ")
(display (infer (context) (pi-fresh 'x (U 0) (U 0)))) (newline)
(display "  ↑ note the fresh dim 1000 appears beside the max") (newline)
(newline)

(display "============================================================") (newline)
(display "  Each quantification gets its OWN dimension") (newline)
(display "============================================================") (newline)
(newline)

(display "Three consecutive pi-fresh calls:") (newline)
(display "  ") (display (infer (context) (pi-fresh 'a (U 0) (U 0)))) (newline)
(display "  ") (display (infer (context) (pi-fresh 'b (U 0) (U 0)))) (newline)
(display "  ") (display (infer (context) (pi-fresh 'c (U 0) (U 0)))) (newline)
(display "  ↑ each got a distinct fresh dim from the per-session counter") (newline)
(newline)

(display "These universes are now INCOMPARABLE:") (newline)
(display "  (U-set 1 1001) ≤ (U-set 1 1002): ")
(display (universe-leq? (U-set 1 1001) (U-set 1 1002))) (newline)
(display "  (U-set 1 1002) ≤ (U-set 1 1001): ")
(display (universe-leq? (U-set 1 1002) (U-set 1 1001))) (newline)
(display "  ↑ neither is below the other — they're parallel in the lattice") (newline)
(newline)

(display "============================================================") (newline)
(display "  Nested pi-fresh accumulates dimensions") (newline)
(display "============================================================") (newline)
(newline)

(display "(pi-fresh 'x (U 0) (pi-fresh 'y (U 0) (U 0))) :") (newline)
(display "  ")
(display (infer (context) (pi-fresh 'x (U 0) (pi-fresh 'y (U 0) (U 0))))) (newline)
(display "  ↑ outer adds one fresh dim, inner adds another, both flow up") (newline)
(newline)

(display "Three-level nesting:") (newline)
(display "  ")
(display (infer (context)
                (pi-fresh 'x (U 0)
                  (pi-fresh 'y (U 0)
                    (pi-fresh 'z (U 0) (U 0)))))) (newline)
(display "  ↑ three new dimensions, all in the result universe") (newline)
(newline)

(display "============================================================") (newline)
(display "  Sigma-fresh works the same way") (newline)
(display "============================================================") (newline)
(newline)

(display "(sigma-fresh 'x (U 0) (U 0)) :") (newline)
(display "  ")
(display (infer (context) (sigma-fresh 'x (U 0) (U 0)))) (newline)
(display "  ↑ pulls from the same counter as pi-fresh") (newline)
(newline)

(display "============================================================") (newline)
(display "  Mixing pi-fresh and ordinary Pi") (newline)
(display "============================================================") (newline)
(newline)

(display "Inner Pi (no new dim), outer pi-fresh:") (newline)
(display "  ")
(display (infer (context)
                (pi-fresh 'x (U 0) (Pi 'y (U 0) (U 0))))) (newline)
(display "  ↑ inner Pi contributes its max level; outer adds a fresh dim") (newline)
(newline)

(display "Inner pi-fresh, outer ordinary Pi:") (newline)
(display "  ")
(display (infer (context)
                (Pi 'x (U 0) (pi-fresh 'y (U 0) (U 0))))) (newline)
(display "  ↑ inner pi-fresh's dim flows up through outer Pi") (newline)
(newline)

(display "============================================================") (newline)
(display "  What this means for the thesis") (newline)
(display "============================================================") (newline)
(newline)
(display "Each dimension-creating quantification produces a universe") (newline)
(display "that is STRICTLY EXTENDS the universes of its parts via a") (newline)
(display "new lattice dimension. Two such quantifications produce") (newline)
(display "incomparable universes — neither extends the other.") (newline)
(newline)
(display "This is structurally impossible in a tower-based universe") (newline)
(display "system. It's the foundational machinery for an account of") (newline)
(display "type theory where the *act of binding* is what generates") (newline)
(display "size growth, rather than a pre-existing tower of levels.") (newline)
(newline)
(display "=== Phase L.3 complete ===") (newline)
