; -*- lisp -*-
; ============================================================
;  PHASE L.5 — Dimension-creating quantification, dual side
;  co-pi-fresh and co-sigma-fresh
; ============================================================
;
; L.3 introduced pi-fresh: "quantifying over a type generates
; a new dimension in the UNIVERSE lattice."
;
; L.5 introduces co-pi-fresh: "quantifying over a context/
; observation generates a new dimension in the COUNIVERSE
; lattice."
;
; Same term shape:
;   (co-pi-fresh 'x A B)    same binder/domain/codomain layout
;   (co-sigma-fresh 'x A B) same
;
; Different typing rule. Where pi-fresh's domain must be a
; universe (U-set, U, U-max...), co-pi-fresh's domain must be
; a COUNIVERSE (Co-set, Uco, Co-max...). The result lives in
; the couniverse lattice.
;
; The fresh-dim counter is SHARED. Every dimension-creating
; binding gets a globally unique nat; the AST kind tells you
; which lattice owns it.

; ============================================================
;  1. Couniverse value typing
; ============================================================

(display "=== Couniverse value typing ===") (newline)
(display "  (Uco 0) : ")
(display (infer (context) (Uco 0))) (newline)
(display "  (Co-set 0 1) : ")
(display (infer (context) (Co-set 0 1))) (newline)
(newline)

; ============================================================
;  2. co-pi-fresh introduces a Co-set dimension
; ============================================================

(display "=== co-pi-fresh ===") (newline)
(display "(co-pi-fresh 'x (Uco 0) (Uco 0)) : ") (newline)
(display "  ") (display (infer (context) (co-pi-fresh 'x (Uco 0) (Uco 0)))) (newline)
(display "  ↑ a fresh dim joins the result in the COUNIVERSE lattice") (newline)
(newline)

; ============================================================
;  3. Compare with pi-fresh side-by-side
; ============================================================

(display "=== U lattice vs Co lattice side by side ===") (newline)
(display "  pi-fresh:    ")
(display (infer (context) (pi-fresh 'x (U 0) (U 0)))) (newline)
(display "  co-pi-fresh: ")
(display (infer (context) (co-pi-fresh 'y (Uco 0) (Uco 0)))) (newline)
(display "  pi-fresh:    ")
(display (infer (context) (pi-fresh 'z (U 0) (U 0)))) (newline)
(display "  co-pi-fresh: ")
(display (infer (context) (co-pi-fresh 'w (Uco 0) (Uco 0)))) (newline)
(display "  ↑ fresh dims interleave; their *sort* says which lattice") (newline)
(newline)

; ============================================================
;  4. Sort discipline
; ============================================================

(display "=== Sort discipline ===") (newline)
(display "co-pi-fresh REJECTS a universe-typed domain:") (newline)
(display "  (co-pi-fresh 'x (U 0) (Uco 0)) → ")
(display (infer (context) (co-pi-fresh 'x (U 0) (Uco 0)))) (newline)
(newline)

(display "pi-fresh REJECTS a couniverse-typed domain:") (newline)
(display "  (pi-fresh 'x (Uco 0) (U 0)) → ")
(display (infer (context) (pi-fresh 'x (Uco 0) (U 0)))) (newline)
(newline)

(display "The two lattices stay separate even under quantification.") (newline)
(newline)

; ============================================================
;  5. Nested co-pi-fresh accumulates Co-set dims
; ============================================================

(display "=== Nested co-pi-fresh ===") (newline)
(display "(co-pi-fresh 'x (Uco 0) (co-pi-fresh 'y (Uco 0) (Uco 0))) : ") (newline)
(display "  ")
(display (infer (context)
                (co-pi-fresh 'x (Uco 0) (co-pi-fresh 'y (Uco 0) (Uco 0))))) (newline)
(display "  ↑ inner and outer each contribute a Co-set dim") (newline)
(newline)

; ============================================================
;  6. co-sigma-fresh
; ============================================================

(display "=== co-sigma-fresh ===") (newline)
(display "(co-sigma-fresh 'x (Uco 0) (Uco 0)) : ")
(display (infer (context) (co-sigma-fresh 'x (Uco 0) (Uco 0)))) (newline)
(newline)

; ============================================================
;  7. What this enables for the thesis
; ============================================================

(display "=== Phase L.5 complete ===") (newline)
(display "") (newline)
(display "With L.5 the universe/couniverse symmetry is complete in") (newline)
(display "the lattice-creation story:") (newline)
(display "") (newline)
(display "  pi-fresh    : quantify over a TYPE          → new U dim") (newline)
(display "  co-pi-fresh : quantify over an OBSERVATION  → new Co dim") (newline)
(display "") (newline)
(display "Both produce strictly larger lattice elements than their") (newline)
(display "parts. Both stay within their own lattice — no accidental") (newline)
(display "cross-contamination of sorts.") (newline)
(display "") (newline)
(display "What this scaffolds for further work:") (newline)
(display "  - HITs with separate universe and couniverse annotations") (newline)
(display "  - Judgments that explicitly track 'this proposition is") (newline)
(display "    being established AT THIS observation level'") (newline)
(display "  - The duality functor between U and Co (a later phase)") (newline)
