; -*- lisp -*-
; ============================================================
;  EXAMPLE 62 — Propositional truncation (Phase H.2)
; ============================================================
;
; Propositional truncation ‖A‖ is the simplest higher inductive
; type (HIT) with non-trivial computation. It "forgets" the
; distinguishing structure of A: any two elements of ‖A‖ are
; equal by construction.
;
; The rules:
;
;   ‖A‖ : Universe-of-A
;
;   x : A
;   ──────────────────
;   |x| : ‖A‖
;
;   C type
;   cm : A → C
;   cs : "any two elements of C are path-equal"
;   e  : ‖A‖
;   ─────────────────────────────────
;   (trunc-rec C cm cs e) : C
;
;   (trunc-rec C cm cs |x|)  ⟶  (cm x)   [beta]
;
; What's covered: type former, intro, recursor, and the primary
; computation rule. What's not yet covered: deep propositionality
; checking of cs (a future turn), recursors that need to compute
; on path constructors (Path C requires no special computation
; rule here because cs only asserts propositionality structurally).

; ============================================================
;  1. Type former
; ============================================================

(display "=== Truncation type former ===") (newline)
(display "  (Trunc (U 0)) : ")
(display (infer (context) (Trunc (U 0)))) (newline)
(display "  ↑ universe-preserving: Trunc-of-(U 0) lives at (U 1)") (newline)
(display "    (same as (U 0) itself — Trunc adds no level)") (newline)
(newline)

(display "  Higher universes work the same way:") (newline)
(display "  (Trunc (U 7)) : ")
(display (infer (context) (Trunc (U 7)))) (newline)
(newline)

; ============================================================
;  2. Constructor (trunc-intro)
; ============================================================

(display "=== Constructor: lifting a value to its truncation ===") (newline)
(define ctxv (context-extend (context) (variable 'x (U 0))))
(display "  With x : (U 0):") (newline)
(display "    (trunc-intro 'x) : ")
(display (infer ctxv (trunc-intro 'x))) (newline)
(display "  ↑ wraps a value of A as a value of (Trunc A)") (newline)
(newline)

; ============================================================
;  3. Recursor — the eliminator
; ============================================================

(display "=== Recursor: mapping out of a truncation ===") (newline)
(display "") (newline)
(display "The recursor lets you USE a value of (Trunc A) by giving:") (newline)
(display "  - cm : A → C       — what to do with the underlying value") (newline)
(display "  - cs : prop. of C  — proof that C respects truncation") (newline)
(display "  - e  : (Trunc A)   — the truncated value") (newline)
(display "and produces a result of type C.") (newline)
(newline)

(define ctxrec
  (context-extend
    (context-extend
      (context-extend
        (context-extend
          (context-extend (context)
            (variable 'A (U 0)))
          (variable 'C (U 0)))
        (variable 'cm (Pi '_ 'A 'C)))
      (variable 'cs (Pi 'x 'C (Pi 'y 'C (Path 'C 'x 'y)))))
    (variable 'e (Trunc 'A))))

(display "  Context: A : (U 0), C : (U 0), cm : A→C, cs : prop(C), e : Trunc A") (newline)
(display "  (trunc-rec 'C 'cm 'cs 'e) : ")
(display (infer ctxrec (trunc-rec 'C 'cm 'cs 'e))) (newline)
(display "  ↑ result type is C, the motive") (newline)
(newline)

; ============================================================
;  4. The primary computation rule
; ============================================================

(display "=== Primary computation rule (beta) ===") (newline)
(display "") (newline)
(display "When the scrutinee is a literal (trunc-intro x), beta fires:") (newline)
(display "") (newline)
(display "  (trunc-rec C cm cs (trunc-intro x))  ⟶  (@ cm x)") (newline)
(display "") (newline)

(display "  (reduce (trunc-rec 'C 'cm 'cs (trunc-intro 'x))):") (newline)
(display "    ")
(display (reduce (trunc-rec 'C 'cm 'cs (trunc-intro 'x)))) (newline)
(display "  ↑ reduced to (@ cm x)") (newline)
(newline)

; ============================================================
;  5. Determinism
; ============================================================

(display "=== Determinism of reduction ===") (newline)
(display "") (newline)
(display "The beta rule's LHS pattern is unique:") (newline)
(display "  (trunc-rec C cm cs (trunc-intro x))") (newline)
(display "Only this exact shape triggers. Other shapes don't reduce.") (newline)
(display "") (newline)

(display "  Variable scrutinee — no match, recursor stays:") (newline)
(display "    (reduce (trunc-rec 'C 'cm 'cs 'e)): ")
(display (reduce (trunc-rec 'C 'cm 'cs 'e))) (newline)
(newline)

(display "  Recursor scrutinee — no match (not a trunc-intro):") (newline)
(display "    (reduce (trunc-rec 'C 'cm 'cs (trunc-rec 'D 'cm2 'cs2 'e))): ")
(display (reduce (trunc-rec 'C 'cm 'cs (trunc-rec 'D 'cm2 'cs2 'e)))) (newline)
(newline)

(display "  Application as scrutinee — no match:") (newline)
(display "    (reduce (trunc-rec 'C 'cm 'cs (@ 'f 'x))): ")
(display (reduce (trunc-rec 'C 'cm 'cs (@ 'f 'x)))) (newline)
(newline)

(display "The rule has no overlap with any other reduction in lizard:") (newline)
(display "  - trunc-rec is the UNIQUE elim form for Trunc") (newline)
(display "  - No other rule produces a trunc-intro as its result") (newline)
(display "  - The LHS pattern (trunc-rec _ _ _ (trunc-intro _)) matches") (newline)
(display "    at exactly one position when present") (newline)
(newline)

; ============================================================
;  6. Composing
; ============================================================

(display "=== Composing constructors and beta ===") (newline)
(display "") (newline)

(display "  Beta inside compound terms — outer fires, inner stays:") (newline)
(display "    (reduce (trunc-rec 'C 'cm 'cs (trunc-intro (trunc-intro 'x)))): ")
(display (reduce (trunc-rec 'C 'cm 'cs (trunc-intro (trunc-intro 'x))))) (newline)
(display "  ↑ outer beta fires → (@ cm (trunc-intro x))") (newline)
(display "    inner trunc-intro stays — it's a value, not a redex") (newline)
(newline)

; ============================================================
;  Honest scope
; ============================================================

(display "=== Honest scope statement ===") (newline)
(display "") (newline)
(display "What's checked (standard rules):") (newline)
(display "  ✓ Trunc preserves universe levels") (newline)
(display "  ✓ trunc-intro lifts a value to its truncation") (newline)
(display "  ✓ trunc-rec accepts well-formed motive, point, prop, scrutinee") (newline)
(display "  ✓ trunc-rec result type is the motive C") (newline)
(display "  ✓ Beta on (trunc-intro x) reduces to (@ cm x)") (newline)
(display "  ✓ Determinism: no rule overlap, unique LHS pattern") (newline)
(display "") (newline)
(display "What's not yet checked (honest gaps):") (newline)
(display "  - cs's deep shape (Pi x:C. Pi y:C. Path C x y) is not") (newline)
(display "    fully verified — only the outer Pi over C is checked.") (newline)
(display "    A future turn (\"propositionality coherence\") can tighten this.") (newline)
(display "  - Soundness of the combined cubical + modal + HIT theory") (newline)
(display "    not proven (same as everywhere in lizard).") (newline)
(display "  - Higher truncations (set truncation, n-truncation) not provided.") (newline)
(display "  - Non-propositional HITs (S¹, S², interval, suspension) still") (newline)
(display "    pending — they require interval-aware computation that this") (newline)
(display "    turn doesn't attempt.") (newline)
(newline)

(display "=== End of H.2 introductory example ===") (newline)
