; -*- lisp -*-
; ============================================================
;  PHASE M.4 — Structural rules
; ============================================================
;
; Structural rules govern HOW the context is used in derivations:
;
;   Weakening:   from Γ ⊢ J derive Γ, x:A ⊢ J
;                (you can add unused hypotheses)
;
;   Contraction: from Γ, x:A, y:A ⊢ J derive Γ, x:A ⊢ J[x/y]
;                (you can use the same hypothesis twice)
;
;   Exchange:    order of hypotheses doesn't matter
;
; Standard intuitionistic / classical logic admits all three.
; Removing them gives substructural logics:
;
;   without weakening:   RELEVANCE logic (every premise must be used)
;   without contraction: AFFINE / LINEAR logic (each premise once at most)
;   without exchange:    ORDERED logic (premises in fixed positions)
;
; ----- What M.4 actually wires -----
;
; M.4 registers all three as named toggles AND wires WEAKENING
; into the Pi typing rule: when weakening is disabled, a Pi
; whose binder doesn't appear in the codomain (the simple
; arrow) is rejected. This is the structural rule with the
; cleanest local check.
;
; Contraction and exchange are registered as toggles but their
; meaningful implementation requires a usage-tracking refactor
; (see scope notes below) that is deferred to a later phase.

; ============================================================
;  1. Weakening: the wired rule
; ============================================================

(display "=== Default: weakening allowed, simple arrow OK ===") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  (Pi 'x (U 0) (U 0)) [simple arrow]:") (newline)
(display "    ") (display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(newline)

(display "=== Disable weakening ===") (newline)
(logic-rule-disable 'weakening)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "    ↑ 'custom because weakening off doesn't match any") (newline)
(display "      cube-only bundle") (newline)
(display "  (Pi 'x (U 0) (U 0)) [REJECTED — no use of binder]:") (newline)
(display "    ") (display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  (Pi 'x (U 0) (Id (U 0) 'x 'x)) [allowed — binder used]:") (newline)
(display "    ") (display (infer (context) (Pi 'x (U 0) (Id (U 0) 'x 'x)))) (newline)
(newline)

(logic-rule-enable 'weakening)

; ============================================================
;  2. Substructural bundles
; ============================================================

(display "=== Linear, affine, relevant variants of STLC ===") (newline)
(display "") (newline)

(set-logic 'linear-STLC)
(display "linear-STLC (no weakening, no contraction):") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  Simple arrow: ")
(display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  ↑ rejected: linear-STLC forbids weakening") (newline)
(newline)

(set-logic 'affine-STLC)
(display "affine-STLC (weakening on, contraction off):") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  Simple arrow: ")
(display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  ↑ allowed: affine keeps weakening; uses bounded by") (newline)
(display "    contraction (not yet enforced — see scope notes)") (newline)
(newline)

(set-logic 'relevant-STLC)
(display "relevant-STLC (weakening off, contraction on):") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  Simple arrow: ")
(display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  ↑ rejected: relevant logic forbids unused hypotheses") (newline)
(newline)

; ============================================================
;  3. The full picture
; ============================================================

(set-logic 'CoC)
(display "=== All bundles ===") (newline)
(display "  ") (display (list-logics)) (newline)
(newline)

; ============================================================
;  4. Scope honesty
; ============================================================

(display "=== Scope of M.4 ===") (newline)
(display "") (newline)
(display "What's wired:") (newline)
(display "  ✓ weakening — disabling it rejects simple arrows in Pi") (newline)
(display "") (newline)
(display "What's registered but not yet wired:") (newline)
(display "  ✗ contraction — would require usage-tracking refactor:") (newline)
(display "    every typing derivation would need to count how many") (newline)
(display "    times each variable was looked up") (newline)
(display "  ✗ exchange — would require positional context model;") (newline)
(display "    lizard currently uses named binders, which makes") (newline)
(display "    exchange vacuous in the standard formulation") (newline)
(display "") (newline)
(display "The substructural bundles (linear / affine / relevant)") (newline)
(display "set the right toggles, but only weakening produces") (newline)
(display "different type-checking behavior. The bundles are still") (newline)
(display "useful — they declare INTENT, even where enforcement is") (newline)
(display "partial.") (newline)
(newline)

(logic-config-reset)
(display "=== After reset ===") (newline)
(display "  current-logic: ") (display (current-logic)) (newline)
(newline)

(display "=== Phase M.4 complete ===") (newline)
(newline)
(display "Next: M.5 (modalities), then M.6 (lattice/HIT toggles).") (newline)
