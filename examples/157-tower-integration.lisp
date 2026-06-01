; 157-tower-integration.lisp — integrating rational functions of a single
; transcendental monomial theta (= exp(u) or log x) over Q(x), with the EXACT
; differential-field derivation.
;
; Two genuinely-new certified families:
;   * new logarithms:   INT f = log(g)  when f = D(g)/g   (1/(x log x) = log log x,
;                        e^x/(e^x+1) = log(e^x+1), 2x/(x^2+1) = log(x^2+1))
;   * Hermite rational part of a primitive monomial (negative powers):
;       INT (c/x)(log x)^(-k) = -c/(k-1) (log x)^(1-k)  (1/(x (log x)^2) = -1/log x)
;
; Coefficients now live in Q(x), so D(log x) = 1/x is exact.  Rational answers
; are certified by differentiating back (tr-equal? with the integrand) and new
; logs by the defining identity D(g)/g = f.  Non-elementary inputs are declined,
; never faked.  `must` raises on failure.

(import "cas/tower.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'tower-check-failed)))

(define LOG (list 'log)) (define EXP1 (list 'exp (list 0 1)))
(define (xinv) (rat-make (list 1) (list 0 1)))            ; 1/x

(define (tower-ok? f mono)
  (let ((r (integrate-tower f mono)))
    (cond ((equal? (car r) 'log) (newlog-check? f (car (cdr r)) mono))
          ((equal? (car r) 'rat) (tr-equal? (tr-deriv (car (cdr r)) mono) f))
          (else #f))))
(define (gives-log? f mono g) (let ((r (integrate-tower f mono)))
  (and (equal? (car r) 'log) (rfpoly-equal? (car (cdr r)) g))))
(define (declines? f mono) (equal? (car (integrate-tower f mono)) 'failed))

(display "tower integration: exact derivation, new logs, Hermite rational part") (newline) (newline)

(display "1. the differential-field derivation D is exact (handles 1/x)") (newline)
(must "D(log x) = 1/x"         (rfpoly-equal? (Drf (rf-theta) LOG) (rf-const (xinv))))
(must "D((log x)^2) = (2/x)L"  (rfpoly-equal? (Drf (list (rat-zero) (rat-zero) (rat-one)) LOG)
                                              (list (rat-zero) (rat-scale 2 (xinv)))))
(must "D((e^x)^2) = 2 (e^x)^2" (rfpoly-equal? (Drf (list (rat-zero) (rat-zero) (rat-one)) EXP1)
                                              (list (rat-zero) (rat-zero) (rat-scale 2 (rat-one)))))
(must "D(-1/log x) = 1/(x(log x)^2)"
      (tr-equal? (tr-deriv (tr-make (rf-const (rat-neg (rat-one))) (rf-theta)) LOG)
                 (tr-make (rf-const (xinv)) (rfpoly-monomial (rat-one) 2))))
(newline)

(display "2. new logarithms (INT D(g)/g = log g), each re-certified") (newline)
(must "INT 1/(x log x) dx       = log(log x)"   (tower-ok? (tr-make (rf-const (xinv)) (rf-theta)) LOG))
(must "  ... and the answer is log(theta)"      (gives-log? (tr-make (rf-const (xinv)) (rf-theta)) LOG (rf-theta)))
(must "INT e^x/(e^x+1) dx        = log(e^x+1)"   (tower-ok? (tr-make (rf-theta) (list (rat-one) (rat-one))) EXP1))
(must "INT 2x/(x^2+1) dx         = log(x^2+1)"   (tower-ok? (tr-make (rf-const (rat-from-poly (list 0 2))) (rf-const (rat-from-poly (list 1 0 1)))) LOG))
(must "  ... answer is log(x^2+1)"               (gives-log? (tr-make (rf-const (rat-from-poly (list 0 2))) (rf-const (rat-from-poly (list 1 0 1)))) LOG (rf-const (rat-from-poly (list 1 0 1)))))
(newline)

(display "3. Hermite rational part: negative powers of a primitive monomial") (newline)
(must "INT 1/(x (log x)^2) dx    = -1/log x"      (tower-ok? (tr-make (rf-const (xinv)) (rfpoly-monomial (rat-one) 2)) LOG))
(must "INT 1/(x (log x)^3) dx    = -1/(2(log x)^2)" (tower-ok? (tr-make (rf-const (xinv)) (rfpoly-monomial (rat-one) 3)) LOG))
(must "INT 1/(x (log x)^5) dx    certified"        (tower-ok? (tr-make (rf-const (xinv)) (rfpoly-monomial (rat-one) 5)) LOG))
(newline)

(display "4. declines the non-elementary (never fakes an answer)") (newline)
(must "INT 1/log x dx   declined (this is li(x))"  (declines? (tr-make (rf-const (rat-one)) (rf-theta)) LOG))
(must "INT 1/(log x)^2 dx declined"                (declines? (tr-make (rf-const (rat-one)) (rfpoly-monomial (rat-one) 2)) LOG))
(newline)

(display "all tower-integration checks passed.") (newline)
