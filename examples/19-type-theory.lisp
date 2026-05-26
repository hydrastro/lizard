; -*- lisp -*-
; ============================================================
;  Couniverse-stratified type theory notation in lizard
; ============================================================
;
; *** READ THIS FIRST ***
;
; This file is a NOTATION PLAYGROUND for the type theory proposed in
; the thesis. None of these forms are checked. Lizard does not verify
; that a Pi's codomain is a type, that an inductive definition is
; strictly positive, that a substitution's mappings cover its source's
; domain, that a judgment is derivable, or anything else of substance.
; The forms exist so you can sketch the surface of the proposed system
; and pattern-match on the structure. Verification requires a separate
; type-checker kernel.
;
; What IS real here: the COUNIVERSE STRATIFICATION. Each context-layer
; form has a definite couniverse level that you can query at runtime:
;
;   Uco -2 — variables (binding sites)
;   Uco -1 — contexts (sequences of bindings)
;   Uco  0 — substitutions (context morphisms) and judgments
;
; ------------------------------------------------------------

; ---- The universe / couniverse hierarchies ------------------

(display "Universe levels (where TYPES of VALUES live):") (newline)
(display "  (U -1) primitive values     = ") (display (U -1)) (newline)
(display "  (U 0)  types of values      = ") (display (U 0))  (newline)
(display "  (U 1)  types of types       = ") (display (U 1))  (newline)
(newline)

(display "Couniverse levels (where BINDINGS live):") (newline)
(display "  (Uco -2) variables / binding sites  = ") (display (Uco -2)) (newline)
(display "  (Uco -1) contexts                   = ") (display (Uco -1)) (newline)
(display "  (Uco  0) substitutions / judgments  = ") (display (Uco 0))  (newline)
(newline)

; ---- Building the stratification with real values -----------

(display "Build a variable — lives at Uco -2:") (newline)
(define vx (variable 'x (U 0)))
(define vy (variable 'y (U 0)))
(define vn (variable 'n (Pi 'a 'Nat 'Nat)))
(display "  vx = ") (display vx) (newline)
(display "  uco-level vx = ") (display (uco-level vx)) (newline)
(newline)

(display "Build a context — lives at Uco -1:") (newline)
(define Gamma (context vx vy vn))
(display "  Gamma = ") (display Gamma) (newline)
(display "  uco-level Gamma = ") (display (uco-level Gamma)) (newline)
(display "  context-length  = ") (display (context-length Gamma)) (newline)
(newline)

(display "Context-extend (non-destructive):") (newline)
(define Gamma2 (context-extend Gamma (variable 'k (Id 'Nat 'a 'b))))
(display "  Gamma2 = ") (display Gamma2) (newline)
(display "  original Gamma still has length ") (display (context-length Gamma)) (newline)
(newline)

(display "Lookup by name (innermost wins):") (newline)
(display "  (context-lookup Gamma2 'x) = ") (display (context-lookup Gamma2 'x)) (newline)
(display "  (context-lookup Gamma2 'k) = ") (display (context-lookup Gamma2 'k)) (newline)
(display "  (context-lookup Gamma2 'missing) = ") (display (context-lookup Gamma2 'missing)) (newline)
(newline)

; ---- Substitutions: morphisms between contexts at Uco 0 -----

(display "Build a substitution — a morphism between contexts:") (newline)
(define s
  (substitution
    Gamma                 ; from this context...
    (empty-context)       ; ...to this context
    (list (cons 'x 0) (cons 'y 1) (cons 'n (refl 'a)))))
(display "  s = ") (display s) (newline)
(display "  uco-level s = ") (display (uco-level s)) (newline)
(newline)

; ---- Judgments: opaque conclusions of inference rules -------

(display "A judgment is (context |- term : type):") (newline)
(define j1 (judgment Gamma 'x (U 0)))
(define j2 (judgment Gamma (refl 'x) (Id (U 0) 'x 'x)))
(define j3 (judgment (empty-context) (Pi 'A (U 0) (Pi 'x 'A 'A))
                                     (U 0)))
(display "  j1 (var lookup)     : ") (display j1) (newline)
(display "  j2 (refl)           : ") (display j2) (newline)
(display "  j3 (polymorphic id) : ") (display j3) (newline)
(display "  uco-level j1 = ") (display (uco-level j1)) (newline)
(newline)

; ---- Reflection across the whole stratification -------------

(define (describe-uco x)
  (cond ((variable? x)
         (string-append "Uco -2 : variable "
                        (symbol->string (variable-name x))))
        ((context? x)
         (string-append "Uco -1 : context of "
                        (number->string (context-length x))
                        " binding(s)"))
        ((substitution? x)
         "Uco  0 : substitution (context morphism)")
        ((judgment? x)
         "Uco  0 : judgment")
        (else "not in the context layer")))

(display "Walk the stratification:") (newline)
(display "  vx      → ") (display (describe-uco vx))     (newline)
(display "  Gamma   → ") (display (describe-uco Gamma))  (newline)
(display "  Gamma2  → ") (display (describe-uco Gamma2)) (newline)
(display "  s       → ") (display (describe-uco s))      (newline)
(display "  j1      → ") (display (describe-uco j1))     (newline)
(display "  42      → ") (display (describe-uco 42))     (newline)
(display "  (U 0)   → ") (display (describe-uco (U 0)))  (newline)
(newline)

; ---- Sketching a tiny inference-rule SKELETON ---------------
;
; This is just a sketch — nothing here verifies anything. But it
; shows how you'd structure things if you were adding a checker.

(define (var-rule ctx name)
  ; Look up name in ctx; if found, produce a judgment for it.
  ; If not found, return #f (a real checker would raise an error).
  (let ((v (context-lookup ctx name)))
    (if v (judgment ctx name (variable-type v))
          #f)))

(display "Var rule applied:") (newline)
(display "  (var-rule Gamma 'x) = ") (display (var-rule Gamma 'x)) (newline)
(display "  (var-rule Gamma 'n) = ") (display (var-rule Gamma 'n)) (newline)
(display "  (var-rule Gamma 'missing) = ") (display (var-rule Gamma 'missing)) (newline)
(newline)

(define (pi-intro-rule ctx binder dom body-judgment)
  ; If body-judgment is (ctx,binder:dom |- body : codom),
  ; then we package (ctx |- (lambda (binder) body) : (Pi binder dom codom)).
  ; This is a SKELETON — no check happens.
  (if (judgment? body-judgment)
      (judgment ctx
                (list 'lambda (list binder) (judgment-term body-judgment))
                (Pi binder dom (judgment-type body-judgment)))
      #f))

(display "Pi-intro skeleton (purely structural):") (newline)
(define body-j (judgment (context-extend (empty-context) (variable 'x (U 0)))
                         'x (U 0)))
(display "  body  : ") (display body-j) (newline)
(display "  intro : ") (display (pi-intro-rule (empty-context) 'x (U 0) body-j)) (newline)
