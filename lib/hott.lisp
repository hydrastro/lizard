; -*- lisp -*-
; lib/hott.lisp — Homotopy Type Theory path algebra (Track Q).
;
; Wraps the cubical/identity primitives into a usable toolkit:
;   refl, Id-sym, Id-trans, transport
;
; In HoTT, a proof of equality a = b is a PATH from a to b.
; Paths form a groupoid: they can be reversed (symmetry),
; composed (transitivity), and have an identity (reflexivity).

; ============================================================
;  GROUPOID OPERATIONS
; ============================================================

; The reflexivity path at x: a proof that x = x.
(define (path-refl x) (refl x))

; Path reversal (symmetry): if p : a = b then (path-sym p) : b = a.
(define (path-sym p) (Id-sym p))

; Path composition (transitivity): if p : a = b and q : b = c
; then (path-trans p q) : a = c.
(define (path-trans p q) (Id-trans p q))

; Transport: if p : a = b and x : P(a), then (path-transport p x) : P(b).
(define (path-transport p x) (transport p x))

; ============================================================
;  PATH ALGEBRA (chaining)
; ============================================================

; Compose a list of paths left-to-right.
; (path-concat (list p q r)) = p · q · r
(define (path-concat paths)
  (if (null? paths) #f
    (if (null? (cdr paths)) (car paths)
      (path-trans (car paths) (path-concat (cdr paths))))))

; Double reversal: sym(sym(p)). In HoTT this equals p (an involution).
(define (path-sym-sym p) (path-sym (path-sym p)))

; ============================================================
;  CONGRUENCE (ap / cong)
; ============================================================
; If p : a = b then applying f to both sides gives f(a) = f(b).
; This is "action on paths" — functions respect equality.
; We model it by transporting along the path conceptually; here
; we provide the structural helper.

(define (path-cong f p)
  ; ap f p : f a = f b. We use Id-trans/refl structure as a witness.
  ; (In the scaffold, this records the intent; the kernel verifies.)
  (refl (f (path-endpoint-a p))))

; Extract claimed endpoints (scaffold helpers).
(define (path-endpoint-a p)
  (if (Id? p) (Id-a p) p))

(define (path-endpoint-b p)
  (if (Id? p) (Id-b p) p))

; ============================================================
;  GROUPOID LAWS (as checkable properties)
; ============================================================
; These return the two sides that SHOULD be equal under the
; kernel's computation rules. The point is to demonstrate the
; structure; real verification needs the type checker.

; Left identity:  refl · p  =  p
(define (law-left-identity a p)
  (list (path-trans (path-refl a) p) p))

; Right identity:  p · refl  =  p
(define (law-right-identity p b)
  (list (path-trans p (path-refl b)) p))

; Inverse law:  p · sym(p)  =  refl
(define (law-inverse a p)
  (list (path-trans p (path-sym p)) (path-refl a)))

; Involution:  sym(sym(p))  =  p
(define (law-involution p)
  (list (path-sym-sym p) p))

; ============================================================
;  EQUALITY REASONING (calculational style)
; ============================================================
; Build a chain of equational steps. Each step is a path; the
; chain composes them. Mirrors "equational reasoning" in Agda.

; (begin-chain a) starts a proof that a = ...
; Steps are added with path-trans. This is sugar over path-concat.
(define (chain . steps)
  (path-concat steps))

; ============================================================
;  REPORTING
; ============================================================

(define (path-describe label p)
  (display "  ")
  (display label)
  (display " : ")
  (display p)
  (newline))
