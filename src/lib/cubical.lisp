; -*- lisp -*-
; lib/cubical.lisp — Cubical type theory & Higher Inductive Types (Track Q).
;
; Wraps lizard's cubical primitives into a usable toolkit:
;   (i0) (i1)            interval endpoints
;   (I-var 'i)           interval variable
;   (I-and a b) (I-or a b) (I-neg a)   De Morgan interval algebra
;   (path-abs 'i body)   path abstraction   <i> body
;   (path-app p r)       path application   p @ r
;   (reduce e)           normalize an expression
;   transport, comp, hcomp
;
; In cubical type theory a PATH is a function out of the interval I.
; p : Path A a b means p is <i> ... with p@i0 ≡ a and p@i1 ≡ b.

; ============================================================
;  INTERVAL ALGEBRA
; ============================================================

(define (i-var name) (I-var name))
(define (i-meet a b) (I-and a b))     ; ∧  (greatest lower bound)
(define (i-join a b) (I-or a b))      ; ∨  (least upper bound)
(define (i-not a) (I-neg a))          ; ~  (De Morgan involution)

; Reduce an interval expression to normal form.
(define (i-reduce e) (reduce e))

; ============================================================
;  PATHS
; ============================================================

; Make a path <i> body  (body is an expression mentioning (I-var i)).
(define (path i body) (path-abs i body))

; Apply a path at an interval point: p @ r.
(define (path-at p r) (path-app p r))

; Endpoints.
(define (path-start p) (reduce (path-app p (i0))))
(define (path-end p)   (reduce (path-app p (i1))))

; The constant path at a:  <i> a  (a degenerate path, refl).
(define (path-const a) (path-abs 'i a))

; ============================================================
;  PATH OPERATIONS (the groupoid, cubically)
; ============================================================

; Path inversion:  p⁻¹ = <i> p @ (~ i)
; Reverses a path by negating the interval coordinate.
(define (path-invert p)
  (path-abs 'i (path-app p (I-neg (I-var 'i)))))

; ap / congruence:  ap f p = <i> f (p @ i)
; A function sends a path a=b to a path f(a)=f(b).
(define (path-ap f p)
  (path-abs 'i (f (path-app p (I-var 'i)))))

; Reduce a path to a normal-form abstraction.
(define (path-normalize p) (reduce p))

; ============================================================
;  CONNECTIONS (squares from a path + interval algebra)
; ============================================================

; The ∧-connection:  <i><j> p @ (i ∧ j)
; A square whose diagonal is p, with two constant sides.
(define (connection-and p)
  (path-abs 'i (path-abs 'j (path-app p (I-and (I-var 'i) (I-var 'j))))))

; The ∨-connection:  <i><j> p @ (i ∨ j)
(define (connection-or p)
  (path-abs 'i (path-abs 'j (path-app p (I-or (I-var 'i) (I-var 'j))))))

; ============================================================
;  TRANSPORT
; ============================================================

; Transport a value along a path (here a path of types / a refl).
(define (cub-transport p x) (transport p x))

; ============================================================
;  HIGHER INDUCTIVE TYPES (structural definitions)
; ============================================================
; HITs add PATH constructors to a type. We describe them as data
; (point constructors + path constructors) so their structure is
; explicit; full computation needs the kernel's HIT recursors.

; ---- The circle S¹ ----
; Constructors:  base : S¹    loop : base = base
(define s1-base 'base)
(define (s1-loop) (path-abs 'i (s1-loop-point (I-var 'i))))
; the loop traces from base back to base; as a scaffold its value
; at any interior point is the symbolic "on-loop" marker.
(define (s1-loop-point i) (list 'loop-at i))

; S¹ recursion principle: to map S¹ → X, give a point b:X and a
; loop l: b=b. Returns a description of the eliminator's action.
(define (s1-rec point loop-path)
  (list 's1-rec point loop-path))

; ---- The interval type 𝕀 (contractible HIT) ----
; Constructors: zero : 𝕀,  one : 𝕀,  seg : zero = one
(define interval-zero 'izero)
(define interval-one 'ione)
(define (interval-seg)
  (path-abs 'i (list 'seg-at (I-var 'i))))

; ---- Suspension ΣA ----
; Constructors: north, south : ΣA,  merid : A → (north = south)
(define susp-north 'north)
(define susp-south 'south)
(define (susp-merid a)
  (path-abs 'i (list 'merid a (I-var 'i))))

; ============================================================
;  REPORTING
; ============================================================

(define (cub-show label e)
  (display "  ")
  (display label)
  (display " = ")
  (display (reduce e))
  (newline))
