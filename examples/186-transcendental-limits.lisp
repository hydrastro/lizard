; 186-transcendental-limits.lisp -- limits of indeterminate forms via Taylor series.
;
; A 0/0 limit of f(x)/g(x) as x -> 0 is resolved exactly by expanding numerator and
; denominator as power series over Q and comparing leading orders -- L'Hopital's rule in
; series form, with no derivatives and no floating point.  This handles the elementary
; functions exp, sin, cos, log(1+x), and tan.  Each value is certified independently: for
; a finite limit L the series of f - L*g must vanish to strictly higher order than g.
; `must` raises on failure.

(import "cas/translimit.lisp")
(define N 12)
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'translimit-check-failed)))
(define (lim label g h v)
  (display "    ") (display label) (display " = ") (display (limit->string (slimit g h))) (newline)
  (must "    value" (equal? (slimit g h) v))
  (must "    certificate" (slimit-ok? g h)))

(display "Transcendental limits via Taylor series") (newline) (newline)

(display "1. the standard 0/0 limits") (newline)
(lim "sin(x)/x"          (sin-series N) (id-series N) 1)
(lim "(1 - cos x)/x^2"   (ser-sub (const-series 1 N) (cos-series N)) (xpow 2 N) (/ 1 2))
(lim "(e^x - 1)/x"       (ser-sub (exp-series N) (const-series 1 N)) (id-series N) 1)
(lim "log(1+x)/x"        (log1p-series N) (id-series N) 1)
(newline)

(display "2. higher-order vanishing") (newline)
(lim "(e^x - 1 - x)/x^2" (ser-sub (ser-sub (exp-series N) (const-series 1 N)) (id-series N)) (xpow 2 N) (/ 1 2))
(lim "(sin x - x)/x^3"   (ser-sub (sin-series N) (id-series N)) (xpow 3 N) (/ -1 6))
(newline)

(display "3. quotients of two transcendental functions") (newline)
(lim "tan(x)/x"               (tan-series N) (id-series N) 1)
(lim "x/(e^x - 1)"            (id-series N) (ser-sub (exp-series N) (const-series 1 N)) 1)
(lim "(1 - cos x)/(x sin x)"  (ser-sub (const-series 1 N) (cos-series N)) (ser-mul (id-series N) (sin-series N) N) (/ 1 2))
(newline)

(display "4. zero and infinite limits") (newline)
(lim "x^2/sin(x)"  (xpow 2 N) (sin-series N) 0)
(lim "sin(x)/x^2"  (sin-series N) (xpow 2 N) 'infinite)
(newline)

(display "all transcendental-limit checks passed.") (newline)
