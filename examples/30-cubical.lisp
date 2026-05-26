; -*- lisp -*-
; ============================================================
;  CUBICAL TYPE THEORY — Turn 6, foundation
; ============================================================
;
; This is the first cubical-type-theory layer in lizard. It coexists
; with the observational identity machinery — lizard is now bilingual.
;
; Proofs needing univalence will use Path (the cubical machinery).
; Proofs not needing univalence can use Id (observational) — that
; fragment is unchanged.
;
; What this turn adds:
;   * The interval I with endpoints i0 and i1
;   * Interval variables (I-var 'i)
;   * Connection operations: I-and, I-or, I-neg
;   * Path type (Path A a b)
;   * Path abstraction (path-abs 'i body), written informally (<i> body)
;   * Path application (path-app p i), written informally (p @ i)
;
; Reduction rules added:
;   * Interval algebra: i0/i1 absorbtion, idempotence, ~ involution
;   * Path-app beta: ((<i> body) @ j) → body[j/i]
;
; Typing rules added:
;   * I : (U 0); i0, i1, I-var : I; binops yield I
;   * Path A a b : universe of A (parallel to Id)
;   * path-abs check against Path: body must inhabit A in extended
;     interval context, AND body[i0/i] ≡ a, AND body[i1/i] ≡ b
;   * path-app yields the path's domain
;
; Future turns: connection-op equations, comp (Kan composition),
; Glue, ua. This turn is foundation only.

; ------------------------------------------------------------
; 1. Interval algebra
; ------------------------------------------------------------

(display "=== Interval algebra ===") (newline)
(display "i0 ∧ i = ") (display (reduce (I-and (i0) (I-var 'i)))) (newline)
(display "i1 ∧ i = ") (display (reduce (I-and (i1) (I-var 'i)))) (newline)
(display "i ∧ i  = ") (display (reduce (I-and (I-var 'i) (I-var 'i)))) (newline)
(display "i0 ∨ i = ") (display (reduce (I-or (i0) (I-var 'i)))) (newline)
(display "i1 ∨ i = ") (display (reduce (I-or (i1) (I-var 'i)))) (newline)
(display "~ i0   = ") (display (reduce (I-neg (i0)))) (newline)
(display "~ i1   = ") (display (reduce (I-neg (i1)))) (newline)
(display "~ ~ i  = ") (display (reduce (I-neg (I-neg (I-var 'i))))) (newline)
(newline)

; ------------------------------------------------------------
; 2. Path-app beta
; ------------------------------------------------------------

(display "=== Path-app beta ===") (newline)

; The simplest substitution
(display "(<i> i) @ i0 = ")
(display (reduce (path-app (path-abs 'i (I-var 'i)) (i0)))) (newline)
(display "(<i> i) @ i1 = ")
(display (reduce (path-app (path-abs 'i (I-var 'i)) (i1)))) (newline)
(display "(<i> a) @ j = ")
(display (reduce (path-app (path-abs 'i 'a) (I-var 'j)))) (newline)
(display "  (body doesn't mention i, so substitution is a no-op)") (newline)

; Compound: substitute then simplify by interval algebra
(display "(<i> ~i ∨ i) @ i0 = ")
(display (reduce (path-app
                  (path-abs 'i (I-or (I-neg (I-var 'i)) (I-var 'i)))
                  (i0)))) (newline)
(display "  (substitute: ~i0 ∨ i0 → i1 ∨ i0 → i1)") (newline)

(display "(<i> i ∧ ~i) @ i1 = ")
(display (reduce (path-app
                  (path-abs 'i (I-and (I-var 'i) (I-neg (I-var 'i))))
                  (i1)))) (newline)
(display "  (i1 ∧ ~i1 → i1 ∧ i0 → i0)") (newline)
(newline)

; ------------------------------------------------------------
; 3. Cubical types
; ------------------------------------------------------------

(display "=== Type checking ===") (newline)
(display "I : ") (display (infer (context) (I))) (newline)
(display "i0 : ") (display (infer (context) (i0))) (newline)
(display "i1 : ") (display (infer (context) (i1))) (newline)
(display "(I-and i0 i1) : ") (display (infer (context) (I-and (i0) (i1)))) (newline)
(display "(I-neg i0) : ") (display (infer (context) (I-neg (i0)))) (newline)
(newline)

; ------------------------------------------------------------
; 4. Path formation
; ------------------------------------------------------------

(display "=== Path type formation ===") (newline)

(define ctx_p (context (variable 'A (U 0)) (variable 'a 'A)))

(display "(Path A a a) : ") (display (infer ctx_p (Path 'A 'a 'a))) (newline)
(display "  (Path lives in the same universe as A — small like Id)") (newline)
(newline)

; ------------------------------------------------------------
; 5. Path abstraction and the endpoint condition
; ------------------------------------------------------------

(display "=== Path abstraction ===") (newline)

; The constant path (<i> a) : Path A a a — both endpoints reduce to a
(display "Constant path:") (newline)
(display "  (check (<i> a) (Path A a a)) = ")
(display (check ctx_p (path-abs 'i 'a) (Path 'A 'a 'a))) (newline)
(display "  (body is `a`, endpoints both reduce to `a`)") (newline)

; A non-constant path between distinct endpoints fails the endpoint check
(define ctx_ab (context (variable 'A (U 0)) (variable 'a 'A) (variable 'b 'A)))
(display "Endpoint mismatch:") (newline)
(display "  (check (<i> a) (Path A a b)) = ")
(display (check ctx_ab (path-abs 'i 'a) (Path 'A 'a 'b))) (newline)
(display "  (body is `a` constant, can't connect to `b`)") (newline)
(newline)

; ------------------------------------------------------------
; 6. Path application
; ------------------------------------------------------------

(display "=== Path application ===") (newline)
(define ctx_papp (context (variable 'A (U 0)) (variable 'a 'A)
                          (variable 'p (Path 'A 'a 'a))))
(display "Given p : Path A a a") (newline)
(display "  (p @ i0) : ") (display (infer ctx_papp (path-app 'p (i0)))) (newline)
(display "  (p @ i1) : ") (display (infer ctx_papp (path-app 'p (i1)))) (newline)
(display "  (path application returns the domain)") (newline)
(newline)

; ------------------------------------------------------------
; 7. Bilingual: Path coexists with Id
; ------------------------------------------------------------

(display "=== Path and Id coexist ===") (newline)
(display "Observational Id (still works):") (newline)
(display "  (refl a) : (Id A a a) = ")
(display (check ctx_p (refl 'a) (Id 'A 'a 'a))) (newline)

(display "Cubical Path (new):") (newline)
(display "  (<i> a) : (Path A a a) = ")
(display (check ctx_p (path-abs 'i 'a) (Path 'A 'a 'a))) (newline)
(display "  (Both express 'a is equal to itself' — different machinery)") (newline)
(newline)

; ------------------------------------------------------------
; 8. Scope
; ------------------------------------------------------------

(display "=== Scope ===") (newline)
(display "  This turn (Turn 6): foundation of cubical layer.") (newline)
(display "    Interval, endpoints, connection ops, Path, path-abs, path-app.") (newline)
(display "    Interval algebra and path-beta reductions.") (newline)
(display "    Endpoint-checking on path-abs.") (newline)
(display "  Turn 7: connection-op algebraic equations") (newline)
(display "    (i ∧ ~i not always i0; needs face semantics)") (newline)
(display "  Turn 8: comp (Kan composition) — the rule that makes") (newline)
(display "    everything actually compute up to homotopy") (newline)
(display "  Turn 9: Glue types") (newline)
(display "  Turn 10: ua derived from Glue — computational univalence") (newline)
