; -*- lisp -*-
; lib/cas/rischde.lisp -- the Risch differential equation with rational-function data, and the
; resulting integration of  R(x) * exp(p(x))  for rational R and polynomial exponent p.
;
; risch.lisp solves the Risch DE  y' + f y = g  only for polynomial f and g.  Here f is a
; polynomial but g may be any rational function in Q(x), which is exactly what is needed to
; integrate  R(x) e^{p(x)}  with R rational: that integral equals h(x) e^{p(x)} iff h solves
; h' + p' h = R, and p' is a polynomial while R is rational.  Because p' has no poles, a pole of
; R of order m forces a pole of h of order m-1, so the denominator of any solution divides
; gcd(E, E') where E = denominator(R).  Writing h = U/gcd(E,E') turns the problem into a single
; first-order linear equation  P1 U' + P0 U = RHS  for an unknown polynomial U, solved by
; undetermined coefficients (a linear system over Q).  Soundness is guaranteed by an exact
; certificate -- every returned h is differentiated and checked to satisfy h' + p' h = R -- and
; for this class the denominator bound makes it complete, so a reported "non-elementary" is a
; genuine proof that no elementary antiderivative of the given form exists (e.g. INT e^(x^2) dx,
; INT e^x/x dx).  Builds on lib/cas/poly.lisp.

(import "cas/poly.lisp")

; ===========================================================================
;  exact Gaussian elimination over Q
;  eqs : list of rows, each row = (c_0 ... c_{m-1} rhs)  (length m+1)
;  returns a length-m solution (free variables set to 0) or 'none if inconsistent
; ===========================================================================
(define (rde-nth r i) (if (= i 0) (car r) (rde-nth (cdr r) (- i 1))))
(define (rde-last r) (if (null? (cdr r)) (car r) (rde-last (cdr r))))
(define (rde-vscale c r) (map (lambda (x) (* c x)) r))
(define (rde-vsub r s) (if (null? r) '() (cons (- (car r) (car s)) (rde-vsub (cdr r) (cdr s)))))
(define (rde-append-rev seen rest) (if (null? seen) rest (rde-append-rev (cdr seen) (cons (car seen) rest))))
(define (rde-fp eqs col seen)
  (if (null? eqs) #f
      (if (= (rde-nth (car eqs) col) 0)
          (rde-fp (cdr eqs) col (cons (car eqs) seen))
          (cons (car eqs) (rde-append-rev seen (cdr eqs))))))
(define (rde-find-piv eqs col) (rde-fp eqs col '()))
(define (rde-elim-all eqs prow col)
  (if (null? eqs) '()
      (cons (rde-vsub (car eqs) (rde-vscale (rde-nth (car eqs) col) prow))
            (rde-elim-all (cdr eqs) prow col))))
(define (rde-all-rhs-zero? eqs)
  (cond ((null? eqs) #t) ((= (rde-last (car eqs)) 0) (rde-all-rhs-zero? (cdr eqs))) (else #f)))
(define (rde-lookup vals k) (cond ((null? vals) 0) ((= (car (car vals)) k) (cdr (car vals))) (else (rde-lookup (cdr vals) k))))
(define (rde-tail-dot eq vals k m) (if (>= k m) 0 (+ (* (rde-nth eq k) (rde-lookup vals k)) (rde-tail-dot eq vals (+ k 1) m))))
(define (rde-insert x l) (cond ((null? l) (list x)) ((>= (car x) (car (car l))) (cons x l)) (else (cons (car l) (rde-insert x (cdr l))))))
(define (rde-isort l) (if (null? l) '() (rde-insert (car l) (rde-isort (cdr l)))))
(define (rde-bs piv m vals)
  (if (null? piv) vals
      (let ((col (car (car piv))) (eq (cdr (car piv))))
        (let ((v (- (rde-last eq) (rde-tail-dot eq vals (+ col 1) m))))
          (rde-bs (cdr piv) m (cons (cons col v) vals))))))
(define (rde-mat vals m i) (if (>= i m) '() (cons (rde-lookup vals i) (rde-mat vals m (+ i 1)))))
(define (rde-backsub piv m) (rde-mat (rde-bs (rde-isort piv) m '()) m 0))
(define (rde-gj eqs col m piv)
  (if (= col m)
      (if (rde-all-rhs-zero? eqs) (rde-backsub piv m) 'none)
      (let ((p (rde-find-piv eqs col)))
        (if p
            (let ((prow (rde-vscale (/ 1 (rde-nth (car p) col)) (car p))))
              (rde-gj (rde-elim-all (cdr p) prow col) (+ col 1) m (cons (cons col prow) piv)))
            (rde-gj eqs (+ col 1) m piv)))))
(define (rde-gauss eqs m) (rde-gj eqs 0 m '()))

; ===========================================================================
;  solve  P1 U' + P0 U = RHS  for a polynomial U   (undetermined coefficients)
;  For our use f = p' is a nonzero polynomial, so deg P0 >= deg P1 and the leading
;  term never cancels: deg U = deg RHS - deg P0 exactly, so the bound is tight.
; ===========================================================================
(define (rde-monomial-deriv j) (if (= j 0) '() (poly-monomial j (- j 1))))   ; (x^j)' = j x^{j-1}
(define (rde-col P1 P0 j) (poly-add (poly-mul P1 (rde-monomial-deriv j)) (poly-mul P0 (poly-monomial 1 j))))
(define (rde-cols P1 P0 j N) (if (> j N) '() (cons (rde-col P1 P0 j) (rde-cols P1 P0 (+ j 1) N))))
(define (rde-maxdeg ps) (if (null? ps) -1 (max (poly-deg (car ps)) (rde-maxdeg (cdr ps)))))
(define (rde-coeffs-at cols k) (if (null? cols) '() (cons (poly-coeff (car cols) k) (rde-coeffs-at (cdr cols) k))))
(define (rde-append-list a b) (if (null? a) b (cons (car a) (rde-append-list (cdr a) b))))
(define (rde-row cols RHS k) (rde-append-list (rde-coeffs-at cols k) (list (poly-coeff RHS k))))
(define (rde-rows cols RHS k K) (if (> k K) '() (cons (rde-row cols RHS k) (rde-rows cols RHS (+ k 1) K))))
(define (rde-degbound P1 P0 RHS)
  (let ((dR (poly-deg RHS)) (d1 (poly-deg P1)) (d0 (poly-deg P0)))
    (+ 3 (max 0 (max (if (>= d0 0) (- dR d0) 0) (if (>= d1 0) (- dR (- d1 1)) 0))))))
(define (rde-poly-rde P1 P0 RHS)
  (if (poly-zero? RHS) '()
      (let ((N (rde-degbound P1 P0 RHS)))
        (let ((cols (rde-cols P1 P0 0 N)))
          (let ((K (max (rde-maxdeg cols) (poly-deg RHS))))
            (let ((sol (rde-gauss (rde-rows cols RHS 0 K) (+ N 1))))
              (if (equal? sol 'none) 'none (poly-norm sol))))))))

; ===========================================================================
;  rational-function helpers (num . den), den monic, reduced
; ===========================================================================
(define (rde-rmake n d)
  (if (poly-zero? n) (cons '() (list 1))
      (let ((g (poly-gcd n d)))
        (let ((n2 (poly-div n g)) (d2 (poly-div d g)))
          (let ((lc (poly-lead d2))) (cons (poly-scale (/ 1 lc) n2) (poly-scale (/ 1 lc) d2)))))))
(define (rde-rzero? r) (poly-zero? (car r)))
(define (rde-rderiv r) (rde-rmake (poly-sub (poly-mul (poly-deriv (car r)) (cdr r)) (poly-mul (car r) (poly-deriv (cdr r)))) (poly-mul (cdr r) (cdr r))))
(define (rde-radd a b) (rde-rmake (poly-add (poly-mul (car a) (cdr b)) (poly-mul (car b) (cdr a))) (poly-mul (cdr a) (cdr b))))
(define (rde-rsub a b) (rde-radd a (cons (poly-neg (car b)) (cdr b))))
(define (rde-rmul a b) (rde-rmake (poly-mul (car a) (car b)) (poly-mul (cdr a) (cdr b))))

; ===========================================================================
;  the Risch DE  y' + f y = g   (f polynomial, g = (B . E) rational) -> (U . Dy) | 'none
; ===========================================================================
(define (rischde f Brat)
  (let ((gr (rde-rmake (car Brat) (cdr Brat))))
    (let ((B (car gr)) (E (cdr gr)))
      (let ((Dy (poly-gcd E (poly-deriv E))))
        (let ((rad (poly-div E Dy)))
          (let ((P1 (poly-mul rad Dy))
                (P0 (poly-mul rad (poly-sub (poly-mul f Dy) (poly-deriv Dy))))
                (RHS (poly-mul B Dy)))
            (let ((U (rde-poly-rde P1 P0 RHS)))
              (if (equal? U 'none) 'none (cons (poly-norm U) (poly-norm Dy))))))))))
(define (rischde-verify f g y)
  (rde-rzero? (rde-rsub (rde-radd (rde-rderiv y) (rde-rmul (cons f (list 1)) y)) g)))

; ===========================================================================
;  INT R(x) e^{p(x)} dx  (R = (B . E) rational, p = polynomial exponent)
;  answer is h e^p with h solving h' + p' h = R; else non-elementary (proven)
; ===========================================================================
(define (int-rat-exp R p)
  (let ((h (rischde (poly-deriv p) R)))
    (if (equal? h 'none) (list 'non-elementary) (list 'elementary h))))
(define (int-rat-exp-verify R p)
  (let ((res (int-rat-exp R p)))
    (if (equal? (car res) 'non-elementary) #f (rischde-verify (poly-deriv p) R (car (cdr res))))))
