; -*- lisp -*-
; ============================================================
;  EXAMPLE 62 — Propositional truncation (Phase H.2)
; ============================================================
;
; This example walks the truncation layer, now promoted from
; scaffold-only to checked typing rules + a primary computation
; rule. The AST shape is the one introduced by the
; lizard-diagnostics-proof-scaffolds upload:
;
;   (Trunc level A)           — type former
;   (trunc value)             — constructor
;   (trunc-elim C h e)        — eliminator
;
; The rules:
;
;   ‖A‖ : Universe-of-A
;
;   x : A
;   ─────────────────────────
;   (trunc x) : (Trunc ? A)
;
;   C type
;   h : A → C
;   e : (Trunc level A)
;   ─────────────────────────────
;   (trunc-elim C h e) : C
;
;   (trunc-elim C h (trunc x))  ⟶  (@ h x)   [beta]
;
; What's covered: type former, intro, eliminator, and the primary
; computation rule. What's not yet covered: deep propositionality
; obligation on C (motive must be a proposition), non-propositional
; HITs (S¹ remains scaffold-only).
;
; This example assumes (set-logic 'truncations) — the constructors
; are gated on the `truncations-enabled` rule.

(set-logic 'truncations)

; ============================================================
;  1. Type former
; ============================================================

(display "=== Truncation type former ===") (newline)
(display "  (Trunc 0 (U 0)) : ")
(display (infer (context) (Trunc 0 (U 0)))) (newline)
(display "  ↑ universe-preserving: result is (U 1)") (newline)
(display "    (level 0 = set truncation, but level is structural here)") (newline)
(newline)

(display "  Higher universes work the same way:") (newline)
(display "  (Trunc -1 (U 7)) : ")
(display (infer (context) (Trunc -1 (U 7)))) (newline)
(display "  ↑ level -1 = propositional truncation") (newline)
(newline)

; ============================================================
;  2. Constructor (trunc)
; ============================================================

(display "=== Constructor: lifting a value to its truncation ===") (newline)
(define ctxv (context-extend (context) (variable 'x (U 0))))
(display "  With x : (U 0):") (newline)
(display "    (trunc 'x) : ")
(display (infer ctxv (trunc 'x))) (newline)
(display "  ↑ result is (Trunc A) — level left unspecified from") (newline)
(display "    bare (trunc x); the value of A is inferred from x's type.") (newline)
(newline)

; ============================================================
;  3. Eliminator (trunc-elim)
; ============================================================

(display "=== Eliminator: mapping out of a truncation ===") (newline)
(display "") (newline)
(display "trunc-elim C h e takes:") (newline)
(display "  - C : type    — the motive (target type)") (newline)
(display "  - h : A → C   — the handler (point case)") (newline)
(display "  - e : Trunc A — the scrutinee") (newline)
(display "and produces a result of type C.") (newline)
(newline)

(define ctxe
  (context-extend
    (context-extend
      (context-extend
        (context-extend (context)
          (variable 'A (U 0)))
        (variable 'C (U 0)))
      (variable 'h (Pi '_ 'A 'C)))
    (variable 'v (Trunc 0 'A))))

(display "  Context: A : (U 0), C : (U 0), h : A→C, v : Trunc 0 A") (newline)
(display "  (trunc-elim 'C 'h 'v) : ")
(display (infer ctxe (trunc-elim 'C 'h 'v))) (newline)
(display "  ↑ result type is C, the motive") (newline)
(newline)

; ============================================================
;  4. The primary computation rule
; ============================================================

(display "=== Primary computation rule (beta) ===") (newline)
(display "") (newline)
(display "When the scrutinee is a literal (trunc x), beta fires:") (newline)
(display "") (newline)
(display "  (trunc-elim C h (trunc x))  ⟶  (@ h x)") (newline)
(display "") (newline)

(display "  (reduce (trunc-elim 'C 'h (trunc 'x))):") (newline)
(display "    ")
(display (reduce (trunc-elim 'C 'h (trunc 'x)))) (newline)
(display "  ↑ reduced to (@ h x)") (newline)
(newline)

; ============================================================
;  5. Determinism
; ============================================================

(display "=== Determinism of reduction ===") (newline)
(display "") (newline)
(display "The beta rule's LHS pattern is unique:") (newline)
(display "  (trunc-elim C h (trunc x))") (newline)
(display "Only this exact shape triggers. Other shapes don't reduce.") (newline)
(display "") (newline)

(display "  Variable scrutinee — no match, eliminator stays:") (newline)
(display "    (reduce (trunc-elim 'C 'h 'v)): ")
(display (reduce (trunc-elim 'C 'h 'v))) (newline)
(newline)

(display "  Eliminator scrutinee (not a trunc) — no match:") (newline)
(display "    (reduce (trunc-elim 'C 'h (trunc-elim 'D 'h2 'v))): ")
(display (reduce (trunc-elim 'C 'h (trunc-elim 'D 'h2 'v)))) (newline)
(newline)

(display "  Application as scrutinee — no match:") (newline)
(display "    (reduce (trunc-elim 'C 'h (@ 'f 'x))): ")
(display (reduce (trunc-elim 'C 'h (@ 'f 'x)))) (newline)
(newline)

(display "The rule has no overlap with any other reduction in lizard:") (newline)
(display "  - trunc-elim is the UNIQUE elim form for Trunc") (newline)
(display "  - No other rule produces a trunc as its result") (newline)
(display "  - The LHS pattern (trunc-elim _ _ (trunc _)) matches") (newline)
(display "    at exactly one position when present") (newline)
(newline)

; ============================================================
;  6. Composing
; ============================================================

(display "=== Composing constructors and beta ===") (newline)
(display "") (newline)

(display "  Beta inside compound terms — outer fires, inner stays:") (newline)
(display "    (reduce (trunc-elim 'C 'h (trunc (trunc 'x)))): ")
(display (reduce (trunc-elim 'C 'h (trunc (trunc 'x))))) (newline)
(display "  ↑ outer beta fires → (@ h (trunc x))") (newline)
(display "    inner trunc stays — it's a value, not a redex") (newline)
(newline)

; ============================================================
;  Honest scope (consistent with CLAIMS_MATRIX.md)
; ============================================================

(display "=== Honest scope statement ===") (newline)
(display "") (newline)
(display "What's checked (standard rules):") (newline)
(display "  ✓ Trunc preserves universe levels") (newline)
(display "  ✓ trunc lifts a value to its truncation") (newline)
(display "  ✓ trunc-elim accepts well-formed motive, handler, scrutinee") (newline)
(display "  ✓ trunc-elim result type is the motive C") (newline)
(display "  ✓ Beta on (trunc x) reduces to (@ h x)") (newline)
(display "  ✓ Determinism: no rule overlap, unique LHS pattern") (newline)
(display "  ✓ Gated on truncations-enabled rule (scaffold discipline)") (newline)
(display "") (newline)
(display "What's not yet checked (honest gaps in CLAIMS_MATRIX.md):") (newline)
(display "  - Propositionality obligation on motive C: not enforced.") (newline)
(display "    The standard rule requires C to be a proposition (any two") (newline)
(display "    elements path-equal). lizard accepts elims that may not be") (newline)
(display "    justified by the propositional-truncation principle.") (newline)
(display "  - Soundness of the combined cubical + modal + HIT theory") (newline)
(display "    is not proven (same as everywhere in lizard).") (newline)
(display "  - Non-propositional HITs (S¹, S², interval, suspension)") (newline)
(display "    remain scaffold-only — they need interval-aware") (newline)
(display "    computation that this turn doesn't attempt.") (newline)
(newline)

(display "=== End of H.2 walkthrough ===") (newline)

(logic-config-reset)
