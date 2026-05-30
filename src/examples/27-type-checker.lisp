; -*- lisp -*-
; ============================================================
;  BIDIRECTIONAL TYPE CHECKER for λΠ + Pi + one universe
; ============================================================
;
; lizard now has a real, working type checker for the smallest
; interesting dependent type theory:
;
;   * Variables looked up in context
;   * Universes (U n) : (U n+1)
;   * Pi types (Pi x A B) : (U-max univ(A) univ(B))
;   * Lambdas checked against Pi
;   * Applications using Pi-type substitution
;   * Annotations (annot t T) for type ascription
;   * Cumulativity: t : (U n) and n ≤ m implies t : (U m)
;
; Two primitives:
;   (infer Γ t)    → returns the type of t in context Γ
;   (check Γ t T)  → returns #t if t checks against T in Γ
;
; This is the smallest dependent type theory that's actually
; interesting. From here, typing rules for the rest of the HOTT
; fragment (Id, refl, J, xport, Pair, etc.) extend incrementally.

; ------------------------------------------------------------
; 1. Universe inference
; ------------------------------------------------------------

(display "=== Universe inference ===") (newline)
(display "(U 0) : ") (display (infer (context) (U 0))) (newline)
(display "(U 1) : ") (display (infer (context) (U 1))) (newline)
(display "(U 42) : ") (display (infer (context) (U 42))) (newline)
(display "  (Each universe lives one level up)") (newline)
(newline)

; ------------------------------------------------------------
; 2. Cumulativity
; ------------------------------------------------------------

(display "=== Cumulativity ===") (newline)
(display "(U 0) checks against (U 1):  ")
(display (check (context) (U 0) (U 1))) (newline)
(display "(U 0) checks against (U 5):  ")
(display (check (context) (U 0) (U 5))) (newline)
(display "(U 0) checks against (U 0):  ")
(display (check (context) (U 0) (U 0))) (newline)
(display "(U 5) checks against (U 0):  ")
(display (check (context) (U 5) (U 0))) (newline)
(display "  (smaller universe ≤ larger; reverse rejected)") (newline)
(newline)

; ------------------------------------------------------------
; 3. Variables and contexts
; ------------------------------------------------------------

(display "=== Variables ===") (newline)
(define ctx (context (variable 'A (U 0))
                     (variable 'x 'A)
                     (variable 'B (Pi 'a 'A (U 0)))))
(display "ctx = ") (newline)
(display "  ") (display ctx) (newline)
(display "(infer ctx 'A) = ") (display (infer ctx 'A)) (newline)
(display "(infer ctx 'x) = ") (display (infer ctx 'x)) (newline)
(display "(infer ctx 'B) = ") (display (infer ctx 'B)) (newline)
(newline)

; ------------------------------------------------------------
; 4. Pi formation
; ------------------------------------------------------------

(display "=== Pi formation ===") (newline)
(display "(Pi x (U 0) (U 0)) : ")
(display (infer (context) (Pi 'x (U 0) (U 0)))) (newline)
(display "  (codomain at U 0 lives in U 1)") (newline)

(display "(Pi x (U 0) (U 3)) : ")
(display (infer (context) (Pi 'x (U 0) (U 3)))) (newline)
(display "  (universe is max of domain's and codomain's universes)") (newline)

(display "(Pi x (U 2) (U 5)) : ")
(display (infer (context) (Pi 'x (U 2) (U 5)))) (newline)
(newline)

; ------------------------------------------------------------
; 5. Lambda checking — the identity function
; ------------------------------------------------------------

(display "=== Lambda checks against Pi ===") (newline)
(define id_type (Pi 'x (U 0) (U 0)))
(display "id : (Pi x (U 0) (U 0)) -- ")
(display (check (context) (Lambda 'x 'x) id_type)) (newline)

; alpha-equivalence: binder names don't matter
(display "with renamed binder -- ")
(display (check (context) (Lambda 'y 'y) id_type)) (newline)

; A wrong body: returns something not in the codomain type
(display "(Lambda x (U 0)) : id-type ? ")
(display (check (context) (Lambda 'x (U 0)) id_type)) (newline)
(display "  (returns (U 0) which has type (U 1), not (U 0))") (newline)
(newline)

; ------------------------------------------------------------
; 6. Application
; ------------------------------------------------------------

(display "=== Application ===") (newline)
(define app_ctx (context (variable 'A (U 0))
                         (variable 'f (Pi 'x 'A 'A))
                         (variable 'a 'A)))
(display "ctx: f : (Pi x A A), a : A") (newline)
(display "(infer (@ f a)) = ")
(display (infer app_ctx (@ 'f 'a))) (newline)
(display "  (application returns codomain after substitution)") (newline)

; A more complex case: applying the polymorphic identity (in spirit)
; Since we don't have universe polymorphism yet, we use annotations.
(define id_id
  (annot (Lambda 'x 'x) (Pi 'x (U 0) (U 0))))
(display "id_id : (Pi x (U 0) (U 0)) -- ")
(display (infer (context) id_id)) (newline)

; Apply id_id to a type-level variable
(define type_ctx (context (variable 'T (U 0))))
(display "(infer ctx (@ id_id 'T)) where T : (U 0) -- ")
(display (infer type_ctx (@ id_id 'T))) (newline)
(display "  (substitution: codomain (U 0) doesn't mention x, so result is (U 0))")
(newline)
(newline)

; ------------------------------------------------------------
; 7. Failure cases
; ------------------------------------------------------------

(display "=== Type errors (caught) ===") (newline)
(display "unbound 'q : (U 0) -- ")
(display (check (context) 'q (U 0))) (newline)

(display "Lambda checked against (U 0) -- ")
(display (check (context) (Lambda 'x 'x) (U 0))) (newline)
(display "  (Lambda must check against a Pi, not a universe)") (newline)
(newline)

; ------------------------------------------------------------
; 8. Annotation as the bridge between check and infer
; ------------------------------------------------------------

(display "=== Annotations ===") (newline)
(display "(infer (annot (Lambda x x) (Pi x (U 0) (U 0)))) = ") (newline)
(display "  ")
(display (infer (context)
                (annot (Lambda 'x 'x)
                       (Pi 'x (U 0) (U 0))))) (newline)
(display "  (annotation lets us give a Lambda an explicit type") (newline)
(display "   so it can be used in inference contexts like applications)")
(newline)
(newline)

; ------------------------------------------------------------
; 9. What this covers vs. what's still missing
; ------------------------------------------------------------

(display "=== Scope ===") (newline)
(display "  Implemented:") (newline)
(display "    Variables, universes with cumulativity,") (newline)
(display "    Pi formation, Lambda checking, application,") (newline)
(display "    annotations, conversion via reduce + alpha-equality") (newline)
(display "  This is λΠ + Pi + one universe with cumulativity.") (newline)
(display "  Not yet checked (each is incremental future work):") (newline)
(display "    Id, refl, J, xport, ap typing rules") (newline)
(display "    Pair, fst, snd, inl, inr, Case typing rules") (newline)
(display "    Unit, tt, Bot typing rules") (newline)
(display "    Universe binders (Λ at universe level)") (newline)
(display "    Implicit arguments, elaboration") (newline)
(display "    Inductive type schemas (each inductive is hardcoded)") (newline)
