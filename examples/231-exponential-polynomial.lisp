; 231-exponential-polynomial.lisp -- integrating the exponential polynomial part of a height-one
; tower: a Laurent polynomial sum_k a_k(x) theta^k in theta = e^p, the piece the tower Hermite
; reduction cannot reach.  Since D(b theta^k) = (b' + k p' b) theta^k, integrating a_k theta^k is
; the Risch differential equation b' + k p' b = a_k over Q(x) (that is, INT a_k e^{k p}), solved by
; rischde.lisp.  The exponentials e^{k p} for distinct k are independent, so by Liouville the whole
; sum is elementary iff every term is; the k = 0 term is an ordinary base-field integral.  Each
; coefficient is checked by re-deriving its defining equation.  `must` raises on failure.

(import "cas/expoly.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'expoly-check-failed)))
(define X (list 0 1)) (define X2 (list 0 0 1)) (define ONE (list 1))   ; p = x, p = x^2, constant 1

(display "Exponential polynomial part of a tower: INT (sum_k a_k(x) e^{k p}) dx") (newline) (newline)

(display "1. INT x e^x dx = (x-1) e^x  (solving b' + b = x)") (newline)
(define r (int-exp-poly (list (list 1 X ONE)) X))
(display "    coefficient of e^x in the answer = ") (display (cdr (cdr (car (car (cdr r)))))) (display "  (= x-1)") (newline)
(must "certified" (int-exp-poly-verify (list (list 1 X ONE)) X))
(newline)

(display "2. more exponential polynomials") (newline)
(must "INT e^(2x) dx = (1/2) e^(2x)"        (int-exp-poly-verify (list (list 2 ONE ONE)) X))
(must "INT (x^2 + x) e^x dx certified"      (int-exp-poly-verify (list (list 1 (list 0 1 1) ONE)) X))
(must "INT (e^x + e^(-x)) dx certified"     (int-exp-poly-verify (list (list 1 ONE ONE) (list -1 ONE ONE)) X))
(must "INT (3 e^x - 2 e^(3x)) dx certified" (int-exp-poly-verify (list (list 1 (list 3) ONE) (list 3 (list -2) ONE)) X))
(must "INT 2x e^(x^2) dx = e^(x^2)"         (int-exp-poly-verify (list (list 1 (list 0 2) ONE)) X2))
(newline)

(display "3. with a k=0 base-field term") (newline)
(must "INT (x e^x + 1/x) dx = (x-1) e^x + log x" (int-exp-poly-verify (list (list 1 X ONE) (list 0 ONE X)) X))
(newline)

(display "4. non-elementary integrals, proved impossible") (newline)
(must "INT e^x/x dx is non-elementary"            (not (int-exp-poly-elementary? (list (list 1 ONE X)) X)))
(must "INT e^(x^2) dx is non-elementary"          (not (int-exp-poly-elementary? (list (list 1 ONE ONE)) X2)))
(must "INT (x e^x + e^x/x) dx is non-elementary"  (not (int-exp-poly-elementary? (list (list 1 X ONE) (list 1 ONE X)) X)))
(newline)

(display "all exponential-polynomial checks passed.") (newline)
