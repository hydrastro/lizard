; -*- lisp -*-
; lib/cas/tower2exprde.lisp -- the exponential Risch differential equation at height two.
;
; This is the next rung of the exponential second-monomial branch.  Integrating a power sum
; Sum a_k theta2^k against the exponential derivation (theta2 = exp(u), D theta2 = u' theta2) reduces, in
; each degree k, to solving the coefficient equation b' + k u' b = a_k for b in K1 = Q(x)(theta1); the
; antiderivative coefficient is then b and the integral is Sum b theta2^k.  The exact-power case of
; tower2exp.lisp solved only the sub-case where b is a constant of the tower; this module solves the full
; equation for b a polynomial in theta1.  With theta1 = e^x (so D theta1 = theta1) the two-level operator
; acts on a coefficient b = Sum b_j theta1^j as (b' + k u' b)_j = b_j' + j b_j + k (u' * b)_j, where the
; term j b_j comes from differentiating theta1^j and (u' * b) is the convolution of the theta1-coefficient
; sequences.  Writing P for the theta1-degree of u', a polynomial solution has degree deg(a) - P and its
; coefficients are determined from the top down: the coefficient of theta1^j in the equation expresses
; b_{j-P} in terms of strictly-higher b's already found, so b_{deg(a)-P}, ..., b_0 fall out by successive
; division by k u'_P, with the lowest P equations becoming consistency conditions.  Rather than test those
; conditions directly, the solver assembles the candidate b and certifies it by differentiating with the
; exponential derivation and checking b' + k u' b = a in K1; a failure (including a genuine theta1
; denominator, which this polynomial solver does not treat) is reported as no solution.  Builds on
; tower2exp.lisp.

(import "cas/tower2exp.lisp")

; ----- coefficient access over Q(x)[theta1] -----
(define (k1-poly-coeffs x)                     ; theta1-coefficients of a polynomial K1 element; 'notpoly otherwise
  (let ((xn (k1-normalize x)))
    (if (> (rfpoly-deg (car (cdr xn))) 0) 'notpoly (car xn))))
(define (xrde-nth lst i) (if (= i 0) (car lst) (xrde-nth (cdr lst) (- i 1))))
(define (xrde-int-assoc lst key) (if (null? lst) #f (if (= (car (car lst)) key) (car lst) (xrde-int-assoc (cdr lst) key))))
(define (xrde-lookup solved i) (let ((p (xrde-int-assoc solved i))) (if p (cdr p) (rat-zero))))
(define (xrde-up-nth uc p) (if (< p 0) (rat-zero) (if (>= p (length uc)) (rat-zero) (xrde-nth uc p))))
(define (xrde-a-nth ac j) (if (< j 0) (rat-zero) (if (>= j (length ac)) (rat-zero) (xrde-nth ac j))))

; ----- the top-down recursion -----
(define (xrde-lowconv uc solved j cap p acc)    ; Sum_{p=0}^{cap-1} u'_p b_{j-p}
  (if (>= p cap) acc
      (xrde-lowconv uc solved j cap (+ p 1)
                   (rat-add acc (rat-mul (xrde-up-nth uc p) (xrde-lookup solved (- j p)))))))
(define (xrde-bm ac uc k P j solved)            ; b_{j-P} = [ a_j - b_j' - j b_j - k Sum_{p<P} u'_p b_{j-p} ] / (k u'_P)
  (let ((bj (xrde-lookup solved j)))
    (let ((rhs (rat-sub (rat-sub (rat-sub (xrde-a-nth ac j) (rat-deriv bj)) (rat-scale j bj))
                        (rat-scale k (xrde-lowconv uc solved j P 0 (rat-zero))))))
      (rat-div rhs (rat-scale k (xrde-up-nth uc P))))))
(define (xrde-solve ac uc k P m solved)         ; solve b_m for m = E down to 0
  (if (< m 0) solved
      (xrde-solve ac uc k P (- m 1) (cons (cons m (xrde-bm ac uc k P (+ m P) solved)) solved))))
(define (xrde-assemble solved m E) (if (> m E) '() (cons (xrde-lookup solved m) (xrde-assemble solved (+ m 1) E))))

; ----- the exponential RDE solver (b' + k u' b = a, b in Q(x)[theta1]) -----
(define (exp-rde-check a k uprime b mono1)
  (tr-equal? (tr-reduce (k1-add (tr-deriv b mono1) (k1-iscale k (k1-mul uprime b)))) (tr-reduce a)))
(define (exp-rde a k uprime mono1)
  (let ((ac (k1-poly-coeffs a)) (uc (k1-poly-coeffs uprime)))
    (if (equal? ac 'notpoly) 'nosolution
        (if (equal? uc 'notpoly) 'nosolution
            (let ((D (rfpoly-deg ac)) (P (rfpoly-deg uc)))
              (if (< D 0) (k1-zero)
                  (if (< (- D P) 0) 'nosolution
                      (let ((b (list (xrde-assemble (xrde-solve ac uc k P (- D P) '()) 0 (- D P)) (rf-const (rat-one)))))
                        (if (exp-rde-check a k uprime b mono1) b 'nosolution)))))))))
(define (exp-rde-solvable? a k uprime mono1) (if (equal? (exp-rde a k uprime mono1) 'nosolution) #f #t))

; ----- Laurent extension: b may carry a theta1 denominator (b = bbar * theta1^(-l)) -----
; A theta1^(-l) denominator reduces b' + k u' b = a to bbar' + (k u' - l) bbar = num, the same recursion
; with the diagonal term j shifted to j - l.  This finds solutions like b = e^{-x} that the polynomial
; solver misses.  Only pure-power denominators theta1^l are treated (the special part for an exponential
; monomial); any other denominator is reported as no solution.
(define (xrde-zeros n) (if (= n 0) (quote ()) (cons (rat-zero) (xrde-zeros (- n 1)))))
(define (rf-theta1-pow l) (if (= l 0) (list (rat-one)) (append (xrde-zeros l) (list (rat-one)))))
(define (rf-zeros-upto P j i) (if (>= j i) #t (if (rat-zero? (xrde-a-nth P j)) (rf-zeros-upto P (+ j 1) i) #f)))
(define (xrde-bm-l ac uc k P j l solved)
  (let ((bj (xrde-lookup solved j)))
    (let ((rhs (rat-sub (rat-sub (rat-sub (xrde-a-nth ac j) (rat-deriv bj)) (rat-scale (- j l) bj))
                        (rat-scale k (xrde-lowconv uc solved j P 0 (rat-zero))))))
      (rat-div rhs (rat-scale k (xrde-up-nth uc P))))))
(define (xrde-solve-l ac uc k P l m solved)
  (if (< m 0) solved
      (xrde-solve-l ac uc k P l (- m 1) (cons (cons m (xrde-bm-l ac uc k P (+ m P) l solved)) solved))))
(define (exp-rde-laurent a k uprime mono1)
  (let ((an (k1-normalize a)))
    (let ((den (car (cdr an))) (num (car an)) (uc (k1-poly-coeffs uprime)))
      (let ((l (rfpoly-deg den)))
        (if (equal? uc (quote notpoly)) (quote nosolution)
            (if (not (rf-zeros-upto den 0 l)) (quote nosolution)
                (let ((D (rfpoly-deg num)) (P (rfpoly-deg uc)))
                  (if (< D 0) (k1-zero)
                      (if (< (- D P) 0) (quote nosolution)
                          (let ((bbar (xrde-assemble (xrde-solve-l num uc k P l (- D P) (quote ())) 0 (- D P))))
                            (let ((b (k1-normalize (list bbar (rf-theta1-pow l)))))
                              (if (exp-rde-check a k uprime b mono1) b (quote nosolution))))))))))))) 
(define (exp-rde-laurent-solvable? a k uprime mono1) (if (equal? (exp-rde-laurent a k uprime mono1) (quote nosolution)) #f #t))

; ----- height-two exponential integrator using the RDE solver (generalizes the exact-power case) -----
(define (t2e-int-rde-go p uprime mono1 k)
  (if (null? p) '()
      (if (= k 0)
          (if (k1-zero? (car p)) (t2e-cons (k1-zero) (t2e-int-rde-go (cdr p) uprime mono1 1)) 'notexact)
          (let ((bk (exp-rde (car p) k uprime mono1)))
            (if (equal? bk 'nosolution) 'notexact (t2e-cons bk (t2e-int-rde-go (cdr p) uprime mono1 (+ k 1))))))))
(define (t2e-int-rde p uprime mono1) (t2e-int-rde-go p uprime mono1 0))
(define (t2e-int-rde-verify p uprime mono1)
  (let ((q (t2e-int-rde p uprime mono1)))
    (if (equal? q 'notexact) #f (h2-equal? (h2-norm (t2e-deriv q uprime mono1)) (h2-norm p)))))
(define (t2e-rde-integrable? p uprime mono1) (if (equal? (t2e-int-rde p uprime mono1) 'notexact) #f #t))
