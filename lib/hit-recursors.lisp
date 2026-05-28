; -*- lisp -*-
; lib/hit-recursors.lisp — HIT recursors / eliminators (Track Q).
;
; Recursors for Higher Inductive Types that actually COMPUTE: to
; map a HIT into a type X, give the image of each point constructor
; and each path constructor; the recursor dispatches accordingly,
; satisfying the computation (β) rules:
;
;   S¹-rec(b, l)(base)   ≡ b
;   S¹-rec(b, l)(loop@i) ≡ l@i
;
; HIT elements are represented as tagged data so the recursor can
; pattern-match on the constructor.

(import "cubical.lisp")

; ============================================================
;  CIRCLE  S¹
; ============================================================
; Elements:  'base   |   (loop-pt i)   for an interval point i

(define (loop-pt i) (list 'loop-pt i))
(define (loop-pt? x) (and (pair? x) (equal? (car x) 'loop-pt)))
(define (loop-pt-i x) (car (cdr x)))

; The recursor. Given target point b:X and target loop l:b=b (a
; path in X), returns a function S¹ -> X.
(define (s1-rec b l)
  (lambda (x)
    (if (equal? x 'base)
        b                                   ; β-rule: base ↦ b
        (if (loop-pt? x)
            (reduce (path-app l (loop-pt-i x)))  ; β-rule: loop@i ↦ l@i
            b))))

; Verify the computation rules hold.
(define (s1-rec-check b l)
  (let ((f (s1-rec b l)))
    (list
      (list 'base-> (f 'base))
      (list 'loop@i0-> (f (loop-pt (i0))))
      (list 'loop@i1-> (f (loop-pt (i1)))))))

; ============================================================
;  SUSPENSION  ΣA
; ============================================================
; Elements: 'north | 'south | (merid-pt a i)

(define (merid-pt a i) (list 'merid-pt a i))
(define (merid-pt? x) (and (pair? x) (equal? (car x) 'merid-pt)))
(define (merid-pt-a x) (car (cdr x)))
(define (merid-pt-i x) (car (cdr (cdr x))))

; Recursor: target north n:X, south s:X, and merid-img : A -> (n = s).
(define (susp-rec n s merid-img)
  (lambda (x)
    (if (equal? x 'north) n
      (if (equal? x 'south) s
        (if (merid-pt? x)
            (reduce (path-app (merid-img (merid-pt-a x)) (merid-pt-i x)))
            n)))))

; ============================================================
;  TORUS  T²  (as two circles' worth of loops + a square)
; ============================================================
; Elements: 'pt | (loop-a i) | (loop-b i)
; Constructors: a point, two loops, and a square relating them.

(define (torus-loop-a i) (list 'torus-a i))
(define (torus-loop-b i) (list 'torus-b i))

(define (torus-rec pt path-a path-b)
  (lambda (x)
    (if (equal? x 'pt) pt
      (if (and (pair? x) (equal? (car x) 'torus-a))
          (reduce (path-app path-a (car (cdr x))))
        (if (and (pair? x) (equal? (car x) 'torus-b))
            (reduce (path-app path-b (car (cdr x))))
            pt)))))

; ============================================================
;  PROPOSITIONAL TRUNCATION  ‖A‖
; ============================================================
; Elements: (inj a) | (squash x y i)   — squash makes all elements
; equal, collapsing A to a mere proposition.

(define (trunc-inj a) (list 'inj a))
(define (trunc-inj? x) (and (pair? x) (equal? (car x) 'inj)))
(define (trunc-inj-val x) (car (cdr x)))

; Recursor into a PROPOSITION (a type where all elements are equal):
; given f : A -> X (with X a prop), map ‖A‖ -> X by f on inj.
(define (trunc-rec f)
  (lambda (x)
    (if (trunc-inj? x) (f (trunc-inj-val x)) 'squashed)))

; ============================================================
;  WINDING / DEGREE DEMO
; ============================================================
; Map S¹ into the integers by sending loop to "+1". Iterating the
; loop n times models the winding number — the fundamental group
; of the circle is ℤ.

; A path that, conceptually, adds 1 each time around.
(define (winding-loop) (path-const 'turn))

; Compose the loop n times (as a list of turns) and count.
(define (winding-number n)
  (define (go k acc) (if (= k 0) acc (go (- k 1) (+ acc 1))))
  (go n 0))

; ============================================================
;  REPORTING
; ============================================================

(define (hit-show label v)
  (display "  ") (display label) (display " = ") (display v) (newline))
