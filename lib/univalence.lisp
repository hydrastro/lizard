; -*- lisp -*-
; lib/univalence.lisp — Glue, univalence & Kan composition (Track Q).
;
; The deepest part of cubical type theory: equivalences become
; paths (univalence), Glue types attach equivalent types along a
; face, and Kan operations (comp/hcomp/fill) make every type fibrant.
;
; Wraps the primitives:
;   (id-equiv A) (equiv-fun e) (equiv-inv e)
;   (Glue A φ T e) (glue-intro φ t a) (unglue e g)
;   (ua e)                          equivalence → path in the universe
;   (comp <i>A φ u u0) (hcomp A φ u u0) (fill ...)
;   (F0) (F1) (F-eq r s)            face values
;   (@ f x)                         application

; ============================================================
;  EQUIVALENCES
; ============================================================

(define (equiv-id A) (id-equiv A))
(define (equiv-forward e) (equiv-fun e))
(define (equiv-backward e) (equiv-inv e))

; Apply the forward / backward maps of an equivalence.
(define (equiv-app-fwd e x) (reduce (@ (equiv-fun e) x)))
(define (equiv-app-bwd e y) (reduce (@ (equiv-inv e) y)))

; Identity equivalence computes to the identity function.
(define (equiv-id-fwd A x) (reduce (@ (equiv-fun (id-equiv A)) x)))
(define (equiv-id-bwd A x) (reduce (@ (equiv-inv (id-equiv A)) x)))

; ============================================================
;  GLUE TYPES
; ============================================================
; Glue A φ T e is a type equal to A away from φ and to T (≃ A) on φ.

(define (glue A phi T e) (Glue A phi T e))

; Reduce a Glue at the trivial faces.
(define (glue-on-true A T e)  (reduce (Glue A (F1) T e)))   ; → T
(define (glue-on-false A T e) (reduce (Glue A (F0) T e)))   ; → A

; Introduce / eliminate Glue elements.
(define (glue-make phi t a) (glue-intro phi t a))
(define (glue-unglue e g) (reduce (unglue e g)))

; ============================================================
;  UNIVALENCE
; ============================================================
; ua turns an equivalence A ≃ B into a path A = B in the universe.

(define (univalence e) (ua e))

; ua of the identity equivalence is (essentially) refl.
(define (ua-id A) (reduce (ua (id-equiv A))))

; ============================================================
;  KAN COMPOSITION
; ============================================================
; comp : given a line of types <i>A, a face φ, a partial element u
; defined on φ, and a base u0 at i0, produce the composite at i1.

(define (kan-comp type-line phi partial base)
  (reduce (comp type-line phi partial base)))

; hcomp : homogeneous composition (the type doesn't vary).
(define (kan-hcomp A phi partial base)
  (reduce (hcomp A phi partial base)))

; fill : the filler — the whole composition square, not just its lid.
(define (kan-fill type-line phi partial base)
  (reduce (fill type-line phi partial base)))

; A constant composition (empty face F0) just returns the base.
(define (comp-const A base)
  (reduce (hcomp A (F0) (path-abs 'i base) base)))

; ============================================================
;  TRANSPORT VIA A TYPE LINE
; ============================================================
; transp <i>A u0 = comp <i>A F0 (empty) u0 — move a value along a
; line of types with no constraints.
(define (transp type-line base)
  (reduce (comp type-line (F0) (path-abs 'i base) base)))

; ============================================================
;  FACE LATTICE HELPERS
; ============================================================

(define (face-true) (F1))
(define (face-false) (F0))
(define (face-eq r s) (F-eq r s))

; ============================================================
;  REPORTING
; ============================================================

(define (uni-show label e)
  (display "  ") (display label) (display " = ") (display (reduce e)) (newline))
