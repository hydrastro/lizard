; -*- lisp -*-
; lib/cas/rothstein.lisp -- the Rothstein-Trager logarithmic part of integration.
;
; For a proper rational function a/b with b squarefree, the integral is a sum of logarithms
;   INT a/b dx = sum_i c_i log(v_i),
; and Rothstein and Trager showed how to find it WITHOUT factoring b.  The constants c_i are
; the roots of the resultant  R(y) = res_x(a - y b', b)  -- exactly the residues of a/b at the
; roots of b -- and for each such c the corresponding argument is  v_c = gcd(a - c b', b).
; This module assembles that answer (over the rational roots of R, found by the rational root
; theorem) and certifies it by differentiation: d/dx sum c_i log v_i = sum c_i v_i'/v_i, which
; is checked to equal a/b as an exact identity of rational functions.  When R has only rational
; roots the logarithmic part is complete; otherwise the remaining residues are algebraic and
; the resultant is reported as the structural witness.  Builds on resultant.lisp (which brings
; poly.lisp and the parametric resultant rt-resultant).

(import "cas/resultant.lisp")

; ---------- squarefree test (b must be squarefree for the basic algorithm) ----------
(define (ros-squarefree? b) (poly-const? (poly-gcd b (poly-deriv b))))

; ---------- rational roots of R(y) over Q, via the rational root theorem ----------
(define (ros-denlcm p) (if (null? p) 1 (lcm2 (denominator (car p)) (ros-denlcm (cdr p)))))
(define (ros-int-poly p) (poly-scale (ros-denlcm p) p))           ; clear denominators -> integer coeffs
(define (ros-divisors n) (ros-div (if (< n 0) (- n) n) 1 '()))
(define (ros-div n d acc) (cond ((> d n) (reverse acc)) ((= (remainder n d) 0) (ros-div n (+ d 1) (cons d acc))) (else (ros-div n (+ d 1) acc))))
(define (ros-peel-zeros p acc) (if (and (not (poly-zero? p)) (= (poly-coeff p 0) 0)) (ros-peel-zeros (poly-div p (list 0 1)) #t) (cons p acc)))
(define (ros-mem? x l) (cond ((null? l) #f) ((= x (car l)) #t) (else (ros-mem? x (cdr l)))))
(define (ros-rational-roots R)
  (let ((R0 (poly-norm R)))
    (if (poly-zero? R0) '()
        (let ((pz (ros-peel-zeros R0 #f)))
          (let ((core (ros-int-poly (car pz))) (zero-root (cdr pz)))
            (ros-rrt core (if zero-root (list 0) '())))))))
(define (ros-rrt p found)
  (let ((a0 (poly-coeff p 0)) (an (poly-lead p)))
    (ros-cands p (ros-divisors a0) (ros-divisors an) found)))
(define (ros-cands p ps qs found) (if (null? ps) found (ros-cands p (cdr ps) qs (ros-cands-q p (car ps) qs found))))
(define (ros-cands-q p pp qs found)
  (if (null? qs) found
      (ros-cands-q p pp (cdr qs)
        (ros-try p (- pp) (car qs) (ros-try p pp (car qs) found)))))
(define (ros-try p pp q found)
  (let ((c (/ pp q)))
    (if (and (= (poly-eval p c) 0) (not (ros-mem? c found))) (cons c found) found)))

; ---------- the logarithmic part: list of (c v) meaning c * log(v) ----------
(define (ros-arg a b c) (poly-monic (poly-gcd (poly-sub a (poly-scale c (poly-deriv b))) b)))
(define (rt-log-part a b) (ros-terms (ros-rational-roots (rt-resultant a b)) a b))
(define (ros-terms roots a b) (if (null? roots) '() (cons (list (car roots) (ros-arg a b (car roots))) (ros-terms (cdr roots) a b))))

; ---------- certificate: d/dx sum c_i log v_i = a/b  (cross-multiplied identity) ----------
(define (ros-prod vs) (if (null? vs) (list 1) (poly-mul (car vs) (ros-prod (cdr vs)))))
(define (ros-args terms) (if (null? terms) '() (cons (car (cdr (car terms))) (ros-args (cdr terms)))))
; numerator of sum c_i v_i'/v_i over common denominator V = prod v_i
(define (ros-deriv-numer terms V) (rdn terms V))
(define (rdn terms V)
  (if (null? terms) '()
      (let ((c (car (car terms))) (v (car (cdr (car terms)))))
        (poly-add (poly-scale c (poly-mul (poly-deriv v) (poly-div V v))) (rdn (cdr terms) V)))))
(define (rt-verify a b)
  (let ((terms (rt-log-part a b)))
    (if (null? terms) #f
        (let ((V (ros-prod (ros-args terms))))
          (poly-zero? (poly-sub (poly-mul (ros-deriv-numer terms V) b) (poly-mul a V)))))))
; complete iff all residues are rational: total degree of the v_i equals deg b
(define (ros-degsum terms) (if (null? terms) 0 (+ (poly-deg (car (cdr (car terms)))) (ros-degsum (cdr terms)))))
(define (rt-complete? a b) (= (ros-degsum (rt-log-part a b)) (poly-deg b)))
(define (rt-resultant-of a b) (rt-resultant a b))
