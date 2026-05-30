; -*- lisp -*-
; lib/hit-compute.lisp — HIT path computation & canonicity (Track Q).
;
; Completes Track Q with the computational payoff of HITs:
;   * loop composition / inversion forming the integers
;   * the universal cover of S¹ (transport along loopⁿ adds n)
;   * π₁(S¹) ≅ ℤ realized computationally
;   * a canonicity check: closed terms of a base type reduce to a
;     canonical constructor

(import "match.lisp")
(import "cubical.lisp")
(import "hit-recursors.lisp")

; ============================================================
;  LOOPS AS INTEGERS  (the fundamental group of S¹)
; ============================================================
; A "loop word" is an integer n: the path loopⁿ (negative = inverse).

(define (loop-power n) n)                 ; loopⁿ represented by n
(define (loop-id) 0)                      ; refl = loop⁰

; Composition of loops adds exponents:  loopᵐ · loopⁿ = loopᵐ⁺ⁿ
(define (loop-compose m n) (+ m n))

; Inversion negates:  (loopⁿ)⁻¹ = loop⁻ⁿ
(define (loop-invert n) (- 0 n))

; The groupoid laws, computed:
(define (loop-law-identity n)
  (list (loop-compose (loop-id) n) n))     ; refl · p = p
(define (loop-law-inverse n)
  (list (loop-compose n (loop-invert n)) (loop-id)))  ; p · p⁻¹ = refl
(define (loop-law-assoc a b c)
  (list (loop-compose (loop-compose a b) c)
        (loop-compose a (loop-compose b c))))

; ============================================================
;  UNIVERSAL COVER  (S¹'s cover is the line; fibers are ℤ)
; ============================================================
; Transport in the universal cover along loopⁿ shifts an integer by n.
; This IS the equivalence π₁(S¹) ≅ ℤ, computed.

(define (cover-transport loop-n start)
  (+ start loop-n))

; winding: encode a loop word, decode it back — round-trips to ℤ.
(define (encode-loop n) (cover-transport n 0))
(define (decode-winding n) n)

; ============================================================
;  PATH COMPUTATION VIA THE CIRCLE RECURSOR
; ============================================================
; Map S¹ into ℤ-with-a-loop: base ↦ 0, loop ↦ "+1". Then the image
; of loopⁿ is n. We model the target loop as the successor path.

(define (s1-to-int-base) 0)

; Apply the recursor's loop image n times (compute loopⁿ's image).
(define (s1-rec-loop-power n)
  (define (go k acc) (if (= k 0) acc (go (- k 1) (+ acc 1))))
  (if (< n 0)
      (- 0 (go (- 0 n) 0))
      (go n 0)))

; ============================================================
;  CANONICITY
; ============================================================
; Canonicity: every CLOSED term of a base/inductive type reduces to
; a canonical constructor. We check this for Bool, Nat, and the
; circle's points using `reduce`.

(define (canonical-bool? v)
  (or (equal? v 'true) (equal? v 'false) (equal? v #t) (equal? v #f)))

(define (canonical-nat? v) (and (number? v) (>= v 0)))

; Check that reducing an expression yields a canonical value.
(define (check-canonicity expr pred)
  (let ((v (reduce expr)))
    (list 'reduced-to v 'canonical? (pred v))))

; The circle has two canonical "shapes": a point (base) or a path
; (a loop power). Classify a reduced S¹ expression.
(define (s1-canonical-form v)
  (if (equal? v 'base) 'point
    (if (loop-pt? v) 'on-loop 'other)))

; ============================================================
;  DEGREE OF A SELF-MAP OF S¹
; ============================================================
; A map S¹ → S¹ is classified by its degree (how many times it
; wraps). Composing degrees multiplies; this models [S¹,S¹] ≅ ℤ.

(define (map-degree d) d)
(define (compose-degrees d1 d2) (* d1 d2))
(define (degree-of-identity) 1)
(define (degree-of-constant) 0)

; ============================================================
;  REPORTING
; ============================================================

(define (q-show label v)
  (display "  ") (display label) (display " = ") (display v) (newline))
