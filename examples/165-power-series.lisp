; 165-power-series.lisp — truncated power series over Q, with certificates, plus
; exact evaluation of indeterminate-form limits via series.
;
; A series to order N is (c_0 ... c_{N-1}) meaning c_0 + ... + c_{N-1} x^{N-1} + O(x^N).
; The results are checkable: a rational function's series satisfies q*S = p mod x^N;
; exp satisfies S'=S; log(1+x) satisfies (1+x)S'=1; sin/cos satisfy S''=-S;
; (1+x)^a satisfies (1+x)S'=a S.  Limits at 0 of a ratio are resolved exactly by
; comparing the orders of vanishing (an exact form of L'Hopital).  `must` raises.

(import "cas/series.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'series-check-failed)))
(define N 9)

(display "Power series over Q (with certificates) and limits") (newline) (newline)

(display "1. arithmetic and rational-function series") (newline)
(must "1/(1-x) = 1+x+x^2+..."        (equal? (ratfun->series (list 1) (list 1 -1) 6) (list 1 1 1 1 1 1)))
(must "1/(1-x-x^2) = Fibonacci"      (equal? (ratfun->series (list 1) (list 1 -1 -1) 8) (list 1 1 2 3 5 8 13 21)))
(must "certificate q*S = p mod x^N"  (equal? (ser-trunc (ser-mul (list 1 -1 -1) (ratfun->series (list 1) (list 1 -1 -1) N) N) N) (ser-trunc (list 1) N)))
(must "S * (1/S) = 1  (inverse)"     (equal? (ser-mul (list 1 2 3 4) (ser-inverse (list 1 2 3 4) N) N) (ser-trunc (list 1) N)))
(newline)

(display "2. elementary series, each verified by its defining ODE") (newline)
(must "exp: S' = S"                  (equal? (ser-trunc (ser-deriv (exp-series N)) (- N 1)) (ser-trunc (exp-series N) (- N 1))))
(must "log(1+x): (1+x) S' = 1"       (equal? (ser-trunc (ser-mul (list 1 1) (ser-deriv (log1p-series N)) (- N 1)) (- N 1)) (ser-trunc (list 1) (- N 1))))
(must "sin: S'' = -S"                (equal? (ser-trunc (ser-deriv (ser-deriv (sin-series N))) (- N 2)) (ser-trunc (ser-neg (sin-series N)) (- N 2))))
(must "cos: S'' = -S"                (equal? (ser-trunc (ser-deriv (ser-deriv (cos-series N))) (- N 2)) (ser-trunc (ser-neg (cos-series N)) (- N 2))))
(must "(1+x)^(1/2): (1+x) S' = (1/2) S" (equal? (ser-trunc (ser-mul (list 1 1) (ser-deriv (binom-series (/ 1 2) N)) (- N 1)) (- N 1)) (ser-trunc (ser-scale (/ 1 2) (binom-series (/ 1 2) N)) (- N 1))))
(must "sin^2 + cos^2 = 1"            (equal? (ser-trunc (ser-add (ser-mul (sin-series N) (sin-series N) N) (ser-mul (cos-series N) (cos-series N) N)) N) (ser-trunc (list 1) N)))
(newline)

(display "3. composition") (newline)
(must "exp(log(1+x)) = 1+x"          (equal? (exp-of (log1p-series N) N) (ser-trunc (list 1 1) N)))
(must "log(1+(e^x-1)) = x"           (equal? (log1p-of (ser-sub (exp-series N) (list 1)) N) (ser-trunc (list 0 1) N)))
(newline)

(display "4. limits at x->0 (exact, via series order)") (newline)
(must "lim sin(x)/x = 1"             (= (limit-ratio (sin-series N) (list 0 1)) 1))
(must "lim (1-cos x)/x^2 = 1/2"      (= (limit-ratio (ser-sub (list 1) (cos-series N)) (list 0 0 1)) (/ 1 2)))
(must "lim (e^x-1)/x = 1"            (= (limit-ratio (ser-sub (exp-series N) (list 1)) (list 0 1)) 1))
(must "lim (e^x-1-x)/x^2 = 1/2"      (= (limit-ratio (ser-sub (ser-sub (exp-series N) (list 1)) (list 0 1)) (list 0 0 1)) (/ 1 2)))
(must "lim sin(x)/x^3 = infinite"    (equal? (limit-ratio (sin-series N) (list 0 0 0 1)) 'infinite))
(newline)

(display "example: exp(x) = ") (display (ser->string (exp-series 6) "x")) (newline)
(display "all power-series checks passed.") (newline)
