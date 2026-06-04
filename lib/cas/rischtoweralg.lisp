; -*- lisp -*-
; lib/cas/rischtoweralg.lisp -- the ALGEBRAIC LEVEL for the recursive tower: a level (alg n a) where theta is an
; algebraic element satisfying theta^n = a (a a tower element one level down), extending the recursive
; derivation and element algebra of rischtowern.lisp to algebraic extensions -- the first non-transcendental
; level type in the height-n Risch recursion (docs/TRAGER_ROADMAP.md, the summit, "the algebraic level").
;
; The derivation.  From theta^n = a, differentiating gives n theta^{n-1} theta' = a', so
;     theta' = a' / (n theta^{n-1}) = a' theta / (n theta^n) = (a' / (n a)) theta = w theta ,  w = a'/(n a).
; Hence D(theta^k) = (k w) theta^k, and the derivation is DIAGONAL exactly like the exponential level but with
; rate w = a'/(n a) in place of the exponent's derivative:
;     D(sum_{k<n} c_k theta^k) = sum_{k<n} (D(c_k) + k w c_k) theta^k .
; The structural difference from exp is the ALGEBRA: an element has theta-degree < n (since theta^n reduces to
; a), and multiplication must reduce theta^n -> a, theta^{n+1} -> a theta, etc.  Here we implement the quadratic
; case n = 2 (theta = sqrt(a)), the most important algebraic extension, where an element is c_0 + c_1 theta and
;     (c_0 + c_1 theta)(d_0 + d_1 theta) = (c_0 d_0 + c_1 d_1 a) + (c_0 d_1 + c_1 d_0) theta .
;
; Public:
;   tea-level-n tower h / tea-level-a tower h  -> the n and a of an (alg n a) level
;   tea-w tower h            -> the rate w = a'/(n a) at height h (a height-(h-1) element)
;   tea-deriv tower h e      -> D(e) for an algebraic level (diagonal with rate w), reusing height h-1 recursively
;   tea-mul tower h a b      -> multiply two height-h algebraic elements, reducing theta^2 = a (n = 2)
;   tea-sq tower h e         -> e^2 reduced
;
; These dispatch from a thin extension of te-deriv: a tower may now contain (alg n a) levels, and tea-deriv is
; called for them while exp/log keep their rischtowern behavior.
;
; Verified: D(sqrt(x)) = 1/(2 sqrt(x)); D(x + sqrt(x)) = 1 + 1/(2 sqrt(x)); the algebra (sqrt(x))^2 = x;
; D(theta^2) consistent with D(a) (the defining relation differentiated); a height-2 algebraic-over-rational case.
;
; Builds on rischtowern.lisp (the recursive element algebra + derivation) and tower.lisp / poly.lisp.

(import "cas/rischtowern.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

; ----- algebraic-level accessors: a level is (alg n a) -----
(define (tea-level-n tower h) (car (cdr (te-level tower h))))
(define (tea-level-a tower h) (car (cdr (cdr (te-level tower h)))))

; ----- the rate w = a' / (n a) at height h (a height-(h-1) element) -----
(define (tea-w tower h) (te-rat-div tower (- h 1) (te-deriv tower (- h 1) (tea-level-a tower h)) (te-scale-int (- h 1) (tea-level-n tower h) (tea-level-a tower h))))

; ----- the diagonal algebraic derivation: D(sum c_k theta^k) = sum (D(c_k) + k w c_k) theta^k -----
(define (tea-deriv tower h e) (tea-d-go tower h e 0 (tea-w tower h)))
(define (tea-d-go tower h e k w)
  (if (>= k (te-alen e)) (quote ())
      (cons (te-add tower (- h 1) (te-deriv tower (- h 1) (te-acoeff h e k)) (te-mul tower (- h 1) (te-scale-int (- h 1) k w) (te-acoeff h e k)))
            (tea-d-go tower h e (+ k 1) w))))
(define (te-alen l) (if (null? l) 0 (+ 1 (te-alen (cdr l)))))
(define (te-anth l k) (if (= k 0) (car l) (te-anth (cdr l) (- k 1))))
(define (te-acoeff h e k) (if (if (< k 0) #t (>= k (te-alen e))) (te-zero (- h 1)) (te-anth e k)))

; ----- multiplication for n = 2: (c_0 + c_1 t)(d_0 + d_1 t) = (c_0 d_0 + c_1 d_1 a) + (c_0 d_1 + c_1 d_0) t -----
(define (tea-mul tower h x y)
  (tea-mk (te-add tower (- h 1) (te-mul tower (- h 1) (te-acoeff h x 0) (te-acoeff h y 0)) (te-mul tower (- h 1) (te-mul tower (- h 1) (te-acoeff h x 1) (te-acoeff h y 1)) (tea-level-a tower h)))
          (te-add tower (- h 1) (te-mul tower (- h 1) (te-acoeff h x 0) (te-acoeff h y 1)) (te-mul tower (- h 1) (te-acoeff h x 1) (te-acoeff h y 0)))))
(define (tea-mk c0 c1) (list c0 c1))
(define (tea-sq tower h e) (tea-mul tower h e e))
