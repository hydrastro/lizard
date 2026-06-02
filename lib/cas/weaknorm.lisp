; -*- lisp -*-
; lib/cas/weaknorm.lisp -- weak normalization, completing the base-case Risch differential
; equation over Q(x) for an ARBITRARY rational coefficient f.
;
; rderat.lisp solves y' + f y = g when f is weakly normalized -- no simple pole of f has a
; positive-integer residue.  When f is not, this step removes the obstruction.  At a simple pole
; of f with residue n a solution may have a pole of order n (a "resonance" the denominator bound
; would otherwise miss), and that contributes a rational factor (x-a)^n to the homogeneous
; solution exp(-INT f).  WeakNormalizer builds q = prod (x-a)^n over those poles, so that
; y = z/q with z solving z' + (f - q'/q) z = q g, where f - q'/q IS weakly normalized.  The poles
; and their residues are found WITHOUT factoring: the residues at the simple poles (roots of d1,
; the multiplicity-one part of denom(f)) are the roots of R(y) = res_x(a - y d', d1); for each
; positive integer n among them the corresponding factor is gcd(d1, a - n d').
;
; This matters precisely when exp(-INT f) is NOT rational (so the forced pole cannot be shifted
; away), e.g. f = (5/2 x - 6)/(x^2-3x), g = 1/(2 x^2 (x-3)) has the unique rational solution
; y = 1/x^2, which the un-normalized solver cannot represent but rde-general finds.  Every
; returned solution is differentiation-certified.  Builds on rderat.lisp and rothstein.lisp.

(import "cas/rderat.lisp")
(import "cas/rothstein.lisp")

; --- residue resultant: Res_x(a - y*dp, modulus) as a polynomial in y (dp = full d') ---
(define (wn-badz a dp) (let ((dd (poly-deg dp))) (if (< (poly-deg a) dd) 0 (/ (poly-coeff a dd) (poly-lead dp)))))
(define (wn-pts a dp modulus y badz count acc)
  (if (= count 0) (reverse acc)
      (if (= y badz) (wn-pts a dp modulus (+ y 1) badz count acc)
          (wn-pts a dp modulus (+ y 1) badz (- count 1)
                  (cons (cons y (resultant (poly-sub a (poly-scale y dp)) modulus)) acc)))))
(define (wn-residue-poly a dp modulus) (lagrange (wn-pts a dp modulus 1 (wn-badz a dp) (+ (poly-deg modulus) 1) '())))

; --- positive-integer residues, and the q product ---
(define (wn-posint? x) (and (> x 0) (= (denominator x) 1)))
(define (wn-posints roots) (cond ((null? roots) '()) ((wn-posint? (car roots)) (cons (car roots) (wn-posints (cdr roots)))) (else (wn-posints (cdr roots)))))
(define (wn-mult1-part facts) (cond ((null? facts) (list 1)) ((= (car (car facts)) 1) (poly-mul (car (cdr (car facts))) (wn-mult1-part (cdr facts)))) (else (wn-mult1-part (cdr facts)))))
(define (wn-build a dp d1 ns acc)
  (if (null? ns) acc
      (let ((n (car ns)))
        (let ((g (poly-monic (poly-gcd d1 (poly-sub a (poly-scale n dp))))))
          (wn-build a dp d1 (cdr ns) (poly-mul acc (rdr-pow g n)))))))
(define (wn-q a d)
  (let ((d1 (wn-mult1-part (square-free (poly-monic d)))))
    (if (poly-const? d1) (list 1)
        (let ((dp (poly-deriv d)))
          (wn-build a dp d1 (wn-posints (ros-rational-roots (wn-residue-poly a dp d1))) (list 1))))))
(define (weak-normalizer fa fd) (let ((fr (rde-rmake fa fd))) (wn-q (car fr) (cdr fr))))

; --- the full base-case rational Risch DE: y' + (fa/fd) y = (ga/gd) -> (num . den) | 'none ---
(define (rde-general fa fd ga gd)
  (let ((q (weak-normalizer fa fd)))
    (if (poly-const? q)
        (rdr-solve fa fd ga gd)
        (let ((qpq (rde-rmake (poly-deriv q) q)))                       ; q'/q
          (let ((ftil (rde-rsub (rde-rmake fa fd) qpq))                 ; f - q'/q  (weakly normalized)
                (gtil (rde-rmul (rde-rmake ga gd) (cons q (list 1)))))  ; q g
            (let ((z (rdr-solve (car ftil) (cdr ftil) (car gtil) (cdr gtil))))
              (if (equal? z 'none) 'none (rde-rmul z (rde-rmake (list 1) q)))))))))   ; y = z/q
(define (rde-general-verify fa fd ga gd y)
  (rde-rzero? (rde-rsub (rde-radd (rde-rderiv y) (rde-rmul (rde-rmake fa fd) y)) (rde-rmake ga gd))))
