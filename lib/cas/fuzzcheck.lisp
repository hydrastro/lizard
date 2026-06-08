; -*- lisp -*-
; lib/cas/fuzzcheck.lisp -- a deterministic randomized validator that hardens the integration
; stack by checking its core INVARIANTS and its COMPLETENESS on constructed-solvable instances.
;
; A linear-congruential generator produces reproducible pseudo-random polynomials with small
; integer coefficients.  Three families of checks:
;   * rational integration (rischrat): for random A/D, the Hermite rational part must always be
;     exact (rat-integrate-rational-ok?), and when the residues are all rational the whole answer
;     must differentiate back to A/D.  These are invariants -- they must hold for every input.
;   * the Risch DE (rderat): for random polynomial f and rational h, set g = h' + f h; the solver
;     must then FIND a solution and that solution must certify.  A returned "none" here is a
;     completeness bug, which is exactly the failure mode a differentiation certificate cannot
;     catch on its own -- so we construct solvable instances on purpose.
;   * exponential integration (rischde): for random polynomial p and rational h, set
;     R = h' + p' h; INT R e^p must come back elementary and certify.
; Builds on rischrat.lisp and rderat.lisp.

(import "cas/ratfull.lisp")
(import "cas/rderat.lisp")

; ---- deterministic PRNG and small random polynomials ----
(define (fz-next s) (remainder (+ (* s 1103515245) 12345) 2147483648))
(define (fz-coeff s lo hi) (+ lo (remainder (floor (/ s 65536)) (+ 1 (- hi lo)))))
(define (fz-coeffs s n lo hi acc)
  (if (= n 0) (cons (poly-norm (reverse acc)) s)
      (let ((s2 (fz-next s))) (fz-coeffs s2 (- n 1) lo hi (cons (fz-coeff s2 lo hi) acc)))))
(define (fz-nonzero p) (if (poly-zero? p) (list 1) p))

; ---- per-instance checks (return #t if the invariant/solvability holds) ----
(define (fz-ratint-check A D)
  (if (poly-zero? D) #t
      (if (rat-integrate-full-rational-ok? A D)
          (if (rif-complete? (rat-integrate-full A D)) (rat-integrate-full-verify A D) #t) #f)))
(define (fz-rde-check fp hn hd)
  (let ((h (rde-rmake hn hd)))
    (let ((g (rde-radd (rde-rderiv h) (rde-rmul (cons fp (list 1)) h))))
      (let ((y (rdr-solve fp (list 1) (car g) (cdr g))))
        (if (equal? y 'none) #f (rdr-verify fp (list 1) (car g) (cdr g) y))))))
(define (fz-exp-check p hn hd)
  (let ((h (rde-rmake hn hd)))
    (let ((R (rde-radd (rde-rderiv h) (rde-rmul (cons (poly-deriv p) (list 1)) h))))
      (let ((res (int-rat-exp R p)))
        (if (equal? (car res) 'non-elementary) #f (int-rat-exp-verify R p))))))

; ---- loops: thread the PRNG state, count passes ----
(define (fz-ratint s n pass)
  (if (= n 0) pass
      (let ((ra (fz-coeffs s 3 -2 2 '())))
        (let ((rd (fz-coeffs (cdr ra) 3 -2 2 '())))
          (fz-ratint (cdr rd) (- n 1) (if (fz-ratint-check (car ra) (fz-nonzero (car rd))) (+ pass 1) pass))))))
(define (fz-rde s n pass)
  (if (= n 0) pass
      (let ((rf (fz-coeffs s 2 -2 2 '())))
        (let ((rn (fz-coeffs (cdr rf) 2 -2 2 '())))
          (let ((rd (fz-coeffs (cdr rn) 2 -1 2 '())))
            (fz-rde (cdr rd) (- n 1) (if (fz-rde-check (fz-nonzero (car rf)) (car rn) (fz-nonzero (car rd))) (+ pass 1) pass)))))))
(define (fz-exp s n pass)
  (if (= n 0) pass
      (let ((rp (fz-coeffs s 3 -2 2 '())))
        (let ((rn (fz-coeffs (cdr rp) 2 -2 2 '())))
          (let ((rd (fz-coeffs (cdr rn) 2 -1 2 '())))
            (fz-exp (cdr rd) (- n 1) (if (fz-exp-check (fz-nonzero (car rp)) (car rn) (fz-nonzero (car rd))) (+ pass 1) pass)))))))
