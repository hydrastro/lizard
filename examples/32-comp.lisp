; -*- lisp -*-
; ============================================================
;  KAN COMPOSITION — Turn 8
; ============================================================
;
; Kan composition is the operation that makes cubical type theory
; *compute up to homotopy*. It fills in a missing face of a partial
; cube. Three flavors:
;
;   (comp  A φ u u0) — composition along an interval-indexed family
;   (hcomp A φ u u0) — homogeneous: type doesn't vary along interval
;   (fill  A φ u u0) — the whole filling line, not just the top
;
; Where:
;   A   — type family (path-abs over I for comp/fill, type for hcomp)
;   φ   — face formula saying *where* the partial element is defined
;   u   — partial element along the line (a path-abs i. value)
;   u0  — the bottom face (i.e. value at i=i0)
;
; The result lives in A @ i1 (for comp/hcomp) or is a path-abs covering
; the whole line (for fill).
;
; Reduction rules (this turn):
;
;   1. F1 boundary:     comp A [F1 ↦ u] u0  →  (u @ i1)
;                        (the partial is total — just take its top)
;   2. comp Unit:        always gives tt    (Unit is contractible)
;   3. comp Sigma:       comp pushes into Pair components
;   4. comp Pi (non-dep): comp on a Pi gives a Lambda over the
;                        codomain
;   5. hcomp F1:         same as comp F1
;
; Rules deferred to later turns (when Glue arrives):
;   - comp Path (needs multi-face partials)
;   - comp Sum
;   - comp U (the universe — this is where Glue lives)
;   - The fully dependent comp Pi (needs transport on argument)

; ------------------------------------------------------------
; 1. F1 boundary: total partial
; ------------------------------------------------------------

(display "=== F1 boundary ===") (newline)
(display "(comp <i>A F1 u u0)  = ")
(display (reduce (comp (path-abs 'i 'A) (F1) 'u 'u0))) (newline)
(display "  (φ=F1 means u is defined everywhere — top is (u @ i1))") (newline)

(display "(hcomp A F1 u u0)    = ")
(display (reduce (hcomp 'A (F1) 'u 'u0))) (newline)
(newline)

; ------------------------------------------------------------
; 2. F1 + path-app beta cascade
; ------------------------------------------------------------

(display "=== Cascade: F1 reduces, then path-app beta fires ===") (newline)
(display "(hcomp A F1 (<i> v) u0) = ")
(display (reduce (hcomp 'A (F1) (path-abs 'i 'v) 'u0))) (newline)
(display "  (F1 → (u @ i1) → path-app beta → v)") (newline)
(newline)

; ------------------------------------------------------------
; 3. comp Unit always gives tt
; ------------------------------------------------------------

(display "=== Contractibility of Unit ===") (newline)
(display "(comp <i>Unit φ u u0) = ")
(display (reduce (comp (path-abs 'i (Unit)) (F-eq (I-var 'k) (i0)) 'u 'u0)))
(newline)
(display "  (Unit is contractible — comp returns tt regardless of φ)") (newline)
(newline)

; ------------------------------------------------------------
; 4. comp Sigma decomposes
; ------------------------------------------------------------

(display "=== Sigma decomposition ===") (newline)
(display "(comp <i>(Sigma _ A B) φ u (Pair a0 b0)) = ") (newline)
(display "  ")
(display (reduce (comp (path-abs 'i (Sigma 'y 'A 'B))
                       (F-eq (I-var 'k) (i0))
                       'u
                       (Pair 'a0 'b0)))) (newline)
(display "  (comp on a Pair becomes a Pair of comps with fst/snd partials)")
(newline)
(newline)

; ------------------------------------------------------------
; 5. comp Pi becomes Lambda
; ------------------------------------------------------------

(display "=== Pi rule ===") (newline)
(display "(comp <i>(Pi y A B) φ u u0) where B doesn't mention y = ")
(newline)
(display "  ")
(display (reduce (comp (path-abs 'i (Pi 'y 'A 'B))
                       (F-eq (I-var 'k) (i0))
                       'u
                       'u0))) (newline)
(display "  (result is Lambda y. comp B [u@i applied to y] (u0 y))") (newline)
(newline)

; ------------------------------------------------------------
; 6. Typing rules
; ------------------------------------------------------------

(display "=== Typing ===") (newline)
(define ctx_c (context (variable 'A (U 0))
                       (variable 'u (Pi 'i (I) 'A))
                       (variable 'u0 'A)))

(display "(infer (comp <i>A F0 u u0))  = ")
(display (infer ctx_c (comp (path-abs 'i 'A) (F0) 'u 'u0))) (newline)
(display "  (result lives in A @ i1; for constant family that's A)") (newline)

(display "(infer (hcomp A F0 u u0))    = ")
(display (infer ctx_c (hcomp 'A (F0) 'u 'u0))) (newline)

(display "(infer (fill <i>A F0 u u0))  = ")
(display (infer ctx_c (fill (path-abs 'i 'A) (F0) 'u 'u0))) (newline)
(display "  (fill gives the whole line as Pi i:I. A @ i)") (newline)
(newline)

; ------------------------------------------------------------
; 7. What this enables
; ------------------------------------------------------------

(display "=== What Kan composition enables ===") (newline)
(display "  comp is THE rule that makes paths compose.") (newline)
(display "  Path transitivity: trans p q = <i> hcomp B [...] (p @ i)") (newline)
(display "  Path inverse:     sym p     = <i> p @ ~i (already worked)") (newline)
(display "  Transport:        transport A p x = comp A [F0 ↦ ⊥] x") (newline)
(display "  These all become DEFINABLE in terms of comp.") (newline)
(newline)

; ------------------------------------------------------------
; 8. Scope
; ------------------------------------------------------------

(display "=== Scope of Turn 8 ===") (newline)
(display "  DONE: comp/hcomp/fill AST + plumbing") (newline)
(display "        F1 boundary rule (both comp and hcomp)") (newline)
(display "        comp Unit, comp Sigma (non-dep), comp Pi (non-dep)") (newline)
(display "        Typing for all three operations") (newline)
(display "  DEFERRED:") (newline)
(display "    comp Path  — needs multi-face partial machinery") (newline)
(display "    comp Sum   — straightforward but not done yet") (newline)
(display "    comp U     — where Glue lives (Turn 9)") (newline)
(display "    fully dependent comp Pi (transport on argument)") (newline)
(display "  NEXT (Turn 9): Glue types") (newline)
(display "  AFTER (Turn 10): ua as derived term → computational univalence")
(newline)
