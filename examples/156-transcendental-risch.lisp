; 156-transcendental-risch.lisp — the first rung of the Risch decision procedure:
; integration over a single transcendental monomial theta = exp(u) or log(x),
; with polynomial coefficients.
;
; A "tower polynomial" is a polynomial in theta with polynomial-in-x
; coefficients.  A derivation D acts on it (knowing D(e^u)=u' e^u, D(log x)=1/x).
; Exponential integration solves the Risch differential equation b' + i u' b = a_i
; for each power (a degree bound + linear solve that, with polynomial data,
; DECIDES elementarity).  Logarithmic integration is a triangular recurrence.
;
; Every elementary answer is CERTIFIED by differentiating it back with D and
; comparing to the integrand, and non-elementarity is a genuine decision -- so
; INT e^(x^2) dx is *proved* to have no elementary antiderivative.  `must` raises
; on failure.

(import "cas/risch.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'risch-check-failed)))

(define (exp-ok? A u)
  (let ((r (risch-exp A u))) (and (equal? (car r) 'elementary) (tpoly-equal? (D-exp (car (cdr r)) u) A))))
(define (exp-nonelem? A u) (equal? (car (risch-exp A u)) 'non-elementary))
(define (log-ok? A)
  (let ((r (risch-log A))) (and (equal? (car r) 'elementary) (tpoly-equal? (D-log (car (cdr r))) A))))

(define ex  (list 0 1))      ; theta = e^x
(define ex2 (list 0 0 1))    ; theta = e^(x^2)

(display "transcendental Risch: differential field, derivation, decision") (newline) (newline)

(display "1. the derivation D is correct") (newline)
(must "D(e^x) = e^x"               (tpoly-equal? (D-exp (list '() (list 1)) ex)  (list '() (list 1))))
(must "D(e^(x^2)) = 2x e^(x^2)"    (tpoly-equal? (D-exp (list '() (list 1)) ex2) (list '() (list 0 2))))
(must "D(x e^x) = (1+x) e^x"       (tpoly-equal? (D-exp (list '() (list 0 1)) ex) (list '() (list 1 1))))
(must "D(x log x - x) = log x"     (tpoly-equal? (D-log (list (list 0 -1) (list 0 1))) (list '() (list 1))))
(newline)

(display "2. exponential integration (elementary; each answer differentiates back)") (newline)
(must "INT x e^x dx"               (exp-ok? (list '() (list 0 1)) ex))
(must "INT (x^2+1) e^x dx"         (exp-ok? (list '() (list 1 0 1)) ex))
(must "INT e^x dx"                 (exp-ok? (list '() (list 1)) ex))
(must "INT (1+x) e^x dx"           (exp-ok? (list '() (list 1 1)) ex))
(must "INT 2x e^(x^2) dx"          (exp-ok? (list '() (list 0 2)) ex2))
(must "INT x e^(x^2) dx"           (exp-ok? (list '() (list 0 1)) ex2))
(must "INT (x + 3 e^x) dx"         (exp-ok? (list (list 0 1) (list 3)) ex))
; exact answers
(must "INT x e^x = (x-1)e^x"       (tpoly-equal? (car (cdr (risch-exp (list '() (list 0 1)) ex))) (list '() (list -1 1))))
(must "INT 2x e^(x^2) = e^(x^2)"   (tpoly-equal? (car (cdr (risch-exp (list '() (list 0 2)) ex2))) (list '() (list 1))))
(newline)

(display "3. PROOF of non-elementarity (the decision procedure)") (newline)
(must "INT e^(x^2) dx is non-elementary"     (exp-nonelem? (list '() (list 1)) ex2))
(must "INT x^2 e^(x^2) dx is non-elementary" (exp-nonelem? (list '() (list 0 0 1)) ex2))
(must "INT x^4 e^(x^2) dx is non-elementary" (exp-nonelem? (list '() (list 0 0 0 0 1)) ex2))
(newline)

(display "4. logarithmic integration (elementary; each answer differentiates back)") (newline)
(must "INT log x dx"               (log-ok? (list '() (list 1))))
(must "INT (log x)^2 dx"           (log-ok? (list '() '() (list 1))))
(must "INT (1 + log x) dx"         (log-ok? (list (list 1) (list 1))))
(must "INT x log x dx"             (log-ok? (list '() (list 0 1))))
(must "INT log x = x log x - x"    (tpoly-equal? (car (cdr (risch-log (list '() (list 1))))) (list (list 0 -1) (list 0 1))))
(newline)

(display "all transcendental-Risch checks passed.") (newline)
