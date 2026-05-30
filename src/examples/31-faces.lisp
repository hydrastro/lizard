; -*- lisp -*-
; ============================================================
;  FACES & PARTIAL ELEMENTS — Turn 7
; ============================================================
;
; A face formula is a boolean combination of equations on the
; interval. It describes a subset of the cube — the "where" of
; a partial cube. Faces are the machinery that lets us talk
; about partial elements, which are what Kan composition (Turn 8)
; will need.
;
; Syntax:
;   F0          — false: empty subset
;   F1          — true:  whole cube
;   (F-eq i j)  — atomic: i and j are equal as interval terms
;   (F-and φ ψ) — conjunction
;   (F-or  φ ψ) — disjunction
;
;   (Partial φ A) — type of A-elements defined only on face φ
;   (Sub A φ u)   — type of A-elements agreeing with partial u on φ
;
; What this turn adds beyond Turn 6's interval:
;   - Face simplification: F-and/F-or with units, F-eq on concrete
;   - The CCHM "connection-on-face" equations:
;       (F-eq (i ∧ j) i0)  →  (F-or  (F-eq i i0) (F-eq j i0))
;       (F-eq (i ∧ j) i1)  →  (F-and (F-eq i i1) (F-eq j i1))
;       (F-eq (i ∨ j) i0)  →  (F-and (F-eq i i0) (F-eq j i0))
;       (F-eq (i ∨ j) i1)  →  (F-or  (F-eq i i1) (F-eq j i1))
;       (F-eq (~ i) i0)    →  (F-eq i i1)
;       (F-eq (~ i) i1)    →  (F-eq i i0)
;   - face-entails? decision procedure (#t/#f/unknown)
;   - Partial and Sub type formers with typing rules

; ------------------------------------------------------------
; 1. Concrete face reduction
; ------------------------------------------------------------

(display "=== Concrete F-eq ===") (newline)
(display "(F-eq i0 i0) = ") (display (reduce (F-eq (i0) (i0)))) (newline)
(display "(F-eq i1 i1) = ") (display (reduce (F-eq (i1) (i1)))) (newline)
(display "(F-eq i0 i1) = ") (display (reduce (F-eq (i0) (i1)))) (newline)
(display "(F-eq i  i)  = ") (display (reduce (F-eq (I-var 'i) (I-var 'i)))) (newline)
(display "  (reflexivity: any i = i is F1)") (newline)
(newline)

; ------------------------------------------------------------
; 2. Boolean face algebra
; ------------------------------------------------------------

(display "=== F-and ===") (newline)
(display "F1 ∧ φ = ") (display (reduce (F-and (F1) (F-eq (I-var 'i) (i0))))) (newline)
(display "F0 ∧ φ = ") (display (reduce (F-and (F0) (F-eq (I-var 'i) (i0))))) (newline)
(display "φ ∧ φ  = ") (display (reduce (F-and (F-eq (I-var 'i) (i0)) (F-eq (I-var 'i) (i0))))) (newline)
(newline)

(display "=== F-or ===") (newline)
(display "F1 ∨ φ = ") (display (reduce (F-or (F1) (F-eq (I-var 'i) (i0))))) (newline)
(display "F0 ∨ φ = ") (display (reduce (F-or (F0) (F-eq (I-var 'i) (i0))))) (newline)
(newline)

; ------------------------------------------------------------
; 3. Connection-on-face equations (the De Morgan-style rules)
; ------------------------------------------------------------

(display "=== Connection-on-face ===") (newline)
(display "(F-eq (i ∧ j) i0) = ")
(display (reduce (F-eq (I-and (I-var 'i) (I-var 'j)) (i0)))) (newline)
(display "  (i ∧ j is 0 iff i is 0 OR j is 0)") (newline)

(display "(F-eq (i ∧ j) i1) = ")
(display (reduce (F-eq (I-and (I-var 'i) (I-var 'j)) (i1)))) (newline)
(display "  (i ∧ j is 1 iff BOTH i and j are 1)") (newline)

(display "(F-eq (i ∨ j) i0) = ")
(display (reduce (F-eq (I-or (I-var 'i) (I-var 'j)) (i0)))) (newline)
(display "  (i ∨ j is 0 iff BOTH i and j are 0)") (newline)

(display "(F-eq (i ∨ j) i1) = ")
(display (reduce (F-eq (I-or (I-var 'i) (I-var 'j)) (i1)))) (newline)

(display "(F-eq (~ i) i0) = ")
(display (reduce (F-eq (I-neg (I-var 'i)) (i0)))) (newline)
(display "  (~ i = 0 means i = 1)") (newline)

(display "(F-eq (~ i) i1) = ")
(display (reduce (F-eq (I-neg (I-var 'i)) (i1)))) (newline)
(newline)

; ------------------------------------------------------------
; 4. Composed: connection equation + boolean simplification
; ------------------------------------------------------------

(display "=== Composition ===") (newline)
(display "(F-and (F-eq (i ∧ j) i1) (F-eq k i1)) = ")
(display (reduce
          (F-and (F-eq (I-and (I-var 'i) (I-var 'j)) (i1))
                 (F-eq (I-var 'k) (i1))))) (newline)
(display "  (inner connection-eq expands into AND, then the outer AND")
(newline)
(display "   absorbs to give a flat 3-way conjunction)") (newline)
(newline)

; ------------------------------------------------------------
; 5. Face entailment — the decision procedure
; ------------------------------------------------------------

(display "=== Entailment (#t / #f / unknown) ===") (newline)
(display "F0 ⊨ anything:        ")
(display (face-entails? (F0) (F-eq (I-var 'i) (i0)))) (newline)
(display "anything ⊨ F1:        ")
(display (face-entails? (F-eq (I-var 'i) (i0)) (F1))) (newline)
(display "φ ⊨ φ:                ")
(display (face-entails? (F-eq (I-var 'i) (i0)) (F-eq (I-var 'i) (i0)))) (newline)
(display "(φ ∧ ψ) ⊨ φ:          ")
(display (face-entails?
          (F-and (F-eq (I-var 'i) (i0)) (F-eq (I-var 'j) (i1)))
          (F-eq (I-var 'i) (i0)))) (newline)
(display "φ ⊨ (φ ∨ ψ):          ")
(display (face-entails?
          (F-eq (I-var 'i) (i0))
          (F-or (F-eq (I-var 'i) (i0)) (F-eq (I-var 'j) (i1))))) (newline)
(display "φ ⊨ unrelated ψ:      ")
(display (face-entails? (F-eq (I-var 'i) (i0)) (F-eq (I-var 'k) (i1)))) (newline)
(display "  (last one is honestly undecidable without more info)") (newline)
(newline)

; ------------------------------------------------------------
; 6. Partial and Sub types
; ------------------------------------------------------------

(display "=== Partial and Sub ===") (newline)
(define ctx_AB (context (variable 'A (U 0)) (variable 'u 'A)))
(display "(infer ctx (Partial F1 A)) = ")
(display (infer ctx_AB (Partial (F1) 'A))) (newline)
(display "(infer ctx (Partial (F-eq i i0) A)) = ")
(display (infer ctx_AB (Partial (F-eq (I-var 'i) (i0)) 'A))) (newline)
(newline)

; ------------------------------------------------------------
; 7. Toggleability
; ------------------------------------------------------------

(display "=== Pluggability ===") (newline)
(flag-set! 'reduce-f-conn-eq #f)
(display "with conn-eq OFF: (F-eq (i ∧ j) i0) = ")
(display (reduce (F-eq (I-and (I-var 'i) (I-var 'j)) (i0)))) (newline)
(flag-set! 'reduce-f-conn-eq #t)
(newline)

(display "=== Scope ===") (newline)
(display "  Faces, boolean algebra, connection equations.") (newline)
(display "  Face entailment with honest undecidability.") (newline)
(display "  Partial and Sub typing.") (newline)
(display "  Next (Turn 8): Kan composition `comp` — the rule") (newline)
(display "  that makes everything compute up to homotopy.") (newline)
