; 232-logarithmic-polynomial.lisp -- integrating the primitive (logarithmic) polynomial part of a
; height-one tower: a polynomial sum_{k=0..n} a_k(x) (log x)^k with the a_k polynomials in x.  This
; is the logarithmic counterpart of example 231's exponential polynomial part.
;
; Since D sends log x to 1/x, the coefficient of (log x)^k in the derivative of sum b_k (log x)^k is
; b_k' + (k+1) b_{k+1}/x.  Writing b_k = x c_k makes the matching condition a_k = c_k + x c_k' +
; (k+1) c_{k+1}, and because (x c)' = c + x c', solving c + x c' = R for a polynomial c is just
; dividing the j-th coefficient of R by j+1.  Processing k from the top down gives every coefficient
; in closed form; this class is always elementary, and each answer is checked by differentiating in
; the tower.  `must` raises on failure.

(import "cas/logpoly.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'logpoly-check-failed)))
; a polynomial in log x is a list of x-polynomials (a_0 a_1 ... a_n), index = power of log x

(display "Logarithmic polynomial part of a tower: INT (sum_k a_k(x) (log x)^k) dx") (newline) (newline)

(display "1. INT (log x)^2 dx = x (log x)^2 - 2x log x + 2x") (newline)
(display "    coefficients (b_0 b_1 b_2) = ") (display (int-log-poly (list (list 0) (list 0) (list 1)))) (newline)
(display "    i.e. 2x, -2x, x  -- read low power of log x first") (newline)
(must "certified" (int-log-poly-verify (list (list 0) (list 0) (list 1))))
(newline)

(display "2. the pure powers INT (log x)^n dx") (newline)
(must "INT 1 dx = x"            (int-log-poly-verify (list (list 1))))
(must "INT log x dx = x log x - x" (int-log-poly-verify (list (list 0) (list 1))))
(must "INT (log x)^3 dx"        (int-log-poly-verify (list (list 0) (list 0) (list 0) (list 1))))
(must "INT (log x)^4 dx"        (int-log-poly-verify (list (list 0) (list 0) (list 0) (list 0) (list 1))))
(must "INT (log x)^5 dx"        (int-log-poly-verify (list (list 0) (list 0) (list 0) (list 0) (list 0) (list 1))))
(newline)

(display "3. polynomial coefficients in x") (newline)
(display "    INT x log x dx = (x^2/2) log x - x^2/4;  (b_0 b_1) = ") (display (int-log-poly (list (list 0) (list 0 1)))) (newline)
(must "INT x log x dx"                       (int-log-poly-verify (list (list 0) (list 0 1))))
(must "INT (3x^2+1)(log x)^3 dx"             (int-log-poly-verify (list (list 0) (list 0) (list 0) (list 1 0 3))))
(must "INT (x^2 (log x)^2 + x log x + 1) dx" (int-log-poly-verify (list (list 1) (list 0 1) (list 0 0 1))))
(must "INT (5x^3 - 2x)(log x)^2 dx"          (int-log-poly-verify (list (list 0) (list 0) (list 0 -2 0 5))))
(newline)

(display "all logarithmic-polynomial checks passed.") (newline)
