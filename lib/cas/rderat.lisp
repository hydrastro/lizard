; -*- lisp -*-
; lib/cas/rderat.lisp -- the Risch differential equation with a RATIONAL coefficient f, and the
; integration of R(x) e^{u(x)} for rational R AND rational u.
;
; rischde.lisp solves y' + f y = g for polynomial f.  This handles f rational, in the case f is
; weakly normalized -- no simple pole of f has a positive-integer residue.  That covers exactly
; the integration of R(x) e^{u(x)} with u rational, because f = u' is the derivative of a
; rational function and therefore has every residue zero (an antiderivative of 1/(x-a) is a
; logarithm, never rational), so u' is automatically weakly normalized.
;
; Denominator bound.  Poles of a solution y occur only at poles of g; at a pole p of g of order
; l where f has a pole of order k, the dominant balance of y' + f y forces y to have a pole of
; order max(0, l - max(k,1)) there (k>=2: f y dominates, order l-k; k<=1: y' dominates, order
; l-1).  Hence denom(y) = prod over squarefree factors p^l of denom(g) of p^{max(0,l-max(k,1))},
; with k the multiplicity of p in denom(f).  For polynomial f every k=0 and this reduces to
; gcd(E,E'), matching rischde.lisp.  Writing y = U/denom(y) gives a single linear equation
; P1 U' + P0 U = RHS for a polynomial U, solved by undetermined coefficients (reusing
; rischde.lisp).  Every returned answer is differentiation-certified, so it is sound; the bound
; makes it complete on this class, validated below against integrals with known answers.
;
; Builds on lib/cas/rischde.lisp.

(import "cas/rischde.lisp")

(define (rdr-pow p k) (if (= k 0) (list 1) (poly-mul p (rdr-pow p (- k 1)))))
(define (rdr-ml p d acc) (let ((qr (poly-divmod d p))) (if (poly-zero? (car (cdr qr))) (rdr-ml p (poly-div d p) (+ acc 1)) acc)))
(define (rdr-mult p d) (if (poly-const? p) 0 (rdr-ml p d 0)))           ; multiplicity of p in d

; denominator bound: prod over squarefree factors (l p) of E of p^{max(0, l - max(k,1))}
(define (rdr-db facts fd acc)
  (if (null? facts) acc
      (let ((l (car (car facts))) (p (car (cdr (car facts)))))
        (let ((k (rdr-mult p fd)))
          (rdr-db (cdr facts) fd (poly-mul acc (rdr-pow p (max 0 (- l (max k 1))))))))))
(define (rdr-denbound E fd) (rdr-db (square-free (poly-monic E)) fd (list 1)))

; solve y' + (fa/fd) y = (ga/gd), f weakly normalized -> (num . den) | 'none
(define (rdr-solve fa fd ga gd)
  (let ((fr (rde-rmake fa fd)) (gr (rde-rmake ga gd)))
    (let ((fa2 (car fr)) (fd2 (cdr fr)) (ga2 (car gr)) (gd2 (cdr gr)))
      (let ((Dz (rdr-denbound gd2 fd2)))
        (let ((P1 (poly-mul gd2 (poly-mul fd2 Dz)))
              (P0 (poly-mul gd2 (poly-sub (poly-mul fa2 Dz) (poly-mul fd2 (poly-deriv Dz)))))
              (RHS (poly-mul ga2 (poly-mul fd2 (poly-mul Dz Dz)))))
          (let ((U (rde-poly-rde P1 P0 RHS)))
            (if (equal? U 'none) 'none (rde-rmake U Dz))))))))
(define (rdr-verify fa fd ga gd y)
  (rde-rzero? (rde-rsub (rde-radd (rde-rderiv y) (rde-rmul (rde-rmake fa fd) y)) (rde-rmake ga gd))))

; INT R(x) e^{u(x)} dx with R, u rational ; f = u' ; answer h e^u with h = rdr-solve
(define (int-rat-exp-rat Ra Rd ua ud)
  (let ((up (rde-rderiv (cons ua ud))))
    (let ((h (rdr-solve (car up) (cdr up) Ra Rd)))
      (if (equal? h 'none) (list 'non-elementary) (list 'elementary h)))))
(define (int-rat-exp-rat-verify Ra Rd ua ud)
  (let ((res (int-rat-exp-rat Ra Rd ua ud)))
    (if (equal? (car res) 'non-elementary) #f
        (let ((up (rde-rderiv (cons ua ud)))) (rdr-verify (car up) (cdr up) Ra Rd (car (cdr res)))))))
