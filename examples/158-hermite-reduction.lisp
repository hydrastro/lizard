; 158-hermite-reduction.lisp — general Hermite reduction for proper rational
; functions of a primitive monomial theta = log(x), the heart of the proper-
; rational case of transcendental Risch.
;
; Hermite reduction takes a/d (deg_theta a < deg_theta d) and, using the
; squarefree factorization of d and extended Euclid in Q(x)[theta], extracts an
; exact rational part g and leaves a remainder with SQUAREFREE denominator:
;     INT a/d = g + INT (squarefree remainder),
; which is then handed to the new-logarithm finisher.
;
; The pipeline is CERTIFIED: the reported antiderivative is differentiated back
; through the field's derivation and compared to the integrand (proper-verify).
; We test it the strongest way -- take a known answer, differentiate it to get
; the integrand, and check the integrator recovers an antiderivative whose
; derivative is exactly that integrand.  `must` raises on failure.

(import "cas/tower.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'hermite-check-failed)))

(define LOG (list 'log))
(define (R1) (rat-one)) (define (R0) (rat-zero)) (define (negone) (rat-neg (rat-one)))

(define (recover-rat? g)
  (let ((f (tr-deriv g LOG)))
    (let ((r (integrate-proper (car f) (car (cdr f)) LOG)))
      (and (equal? (car r) 'ok) (equal? (car (cdr (cdr r))) 'none)
           (proper-verify (car f) (car (cdr f)) LOG r) (tr-equal? (car (cdr r)) g)))))
(define (recover-mix? g G)
  (let ((f (tr-add (tr-deriv g LOG) (list (Drf G LOG) G))))
    (let ((r (integrate-proper (car f) (car (cdr f)) LOG)))
      (and (equal? (car r) 'ok) (proper-verify (car f) (car (cdr f)) LOG r)))))
(define (remainder-squarefree? a d)
  (<= (car (rf-max-mult (rf-yun (car (cdr (cdr (hermite a d LOG))))) (cons 0 '()))) 1))

(display "general Hermite reduction over a primitive monomial (theta = log x)") (newline) (newline)

(display "1. purely-rational antiderivatives recovered (Hermite), certified") (newline)
(must "1/(L^2+1)"   (recover-rat? (tr-make (rf-const (R1)) (list (R1) (R0) (R1)))))
(must "1/(L-1)^2"   (recover-rat? (tr-make (rf-const (R1)) (rfpoly-pow (list (negone) (R1)) 2))))
(must "1/(L^2 + x)" (recover-rat? (tr-make (rf-const (R1)) (list (rat-from-poly (list 0 1)) (R0) (R1)))))
(newline)

(display "2. Hermite leaves a SQUAREFREE remainder denominator") (newline)
(must "rem of 1/(L-1)^3 squarefree"     (remainder-squarefree? (rf-const (R1)) (rfpoly-pow (list (negone) (R1)) 3)))
(must "rem of (2L+3)/(L^2+1)^2 squarefree"
      (remainder-squarefree? (list (rat-from-poly (list 3)) (rat-from-poly (list 2))) (rfpoly-pow (list (R1) (R0) (R1)) 2)))
(newline)

(display "3. rational part + a new logarithm, combined and certified") (newline)
(must "1/(L-1)^2 + log(log x)"  (recover-mix? (tr-make (rf-const (R1)) (rfpoly-pow (list (negone) (R1)) 2)) (rf-theta)))
(newline)

(display "4. squarefree base case folds in: INT 2x/(x^2+1) = log(x^2+1)") (newline)
(must "INT 2x/(x^2+1) = log(x^2+1)"
      (let ((r (integrate-proper (rf-const (rat-from-poly (list 0 2))) (rf-const (rat-from-poly (list 1 0 1))) LOG)))
        (and (equal? (car r) 'ok)
             (proper-verify (rf-const (rat-from-poly (list 0 2))) (rf-const (rat-from-poly (list 1 0 1))) LOG r))))
(newline)

(display "all Hermite-reduction checks passed.") (newline)
