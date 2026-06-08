; LIMITS at an ARBITRARY point, including indeterminate 0/0 forms with transcendental numerators, by local
; series -- generalizing translimit.lisp (limits at x = 0) to any point a and adding the order comparison that
; resolves 0/0, faster-vanishing numerator (limit 0), and faster-vanishing denominator (divergence).
;
; The numerator and denominator are given as their local series in t = x - a (the standard expansions of
; log(1+t), sin, cos, ... and, for rational functions, a Taylor shift provide these).  If both have the same
; order the limit is the ratio of the unit series after dividing out the common t^k -- L'Hopital by series.
(import "cas/slimit2.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Limits at an arbitrary point and indeterminate 0/0 forms, resolved by local series.") (newline) (newline)

(display "transcendental 0/0 forms:") (newline)
(display "  lim_{x->1} (log x)/(x-1): expand log x = (x-1) - (x-1)^2/2 + ... -> ") (display (sl-limit (list 0 1 (/ -1 2) (/ 1 3) (/ -1 4)) (list 0 1) 4)) (newline)
(chk "lim_{x->1} (log x)/(x-1) = 1" (= (sl-limit (list 0 1 (/ -1 2) (/ 1 3) (/ -1 4)) (list 0 1) 4) 1))
(chk "lim_{x->0} (1 - cos x)/x^2 = 1/2" (= (sl-limit (list 0 0 (/ 1 2) 0 (/ -1 24)) (list 0 0 1) 3) (/ 1 2)))
(chk "lim_{x->0} sin(x)/x = 1" (= (sl-limit (list 0 1 0 (/ -1 6)) (list 0 1) 3) 1))

(display "order comparison (vanishing and diverging cases):") (newline)
(chk "lim_{x->0} sin(x)/x^2 diverges (numerator order < denominator order)" (equal? (sl-limit (list 0 1 0 (/ -1 6)) (list 0 0 1) 3) (quote infinite)))
(chk "lim_{x->0} x^2/sin(x) = 0 (numerator vanishes faster)" (= (sl-limit (list 0 0 1) (list 0 1 0 (/ -1 6)) 3) 0))

(display "rational limits at any point via Taylor shift of the polynomials:") (newline)
(display "  shift x^2 - 4 to t = x - 2 gives ") (display (sl-shift-poly (list -4 0 1) 2)) (display " (= 4t + t^2)") (newline)
(chk "lim_{x->2} (x^2-4)/(x-2) = 4" (= (sl-limit (sl-shift-poly (list -4 0 1) 2) (sl-shift-poly (list -2 1) 2) 3) 4))
(chk "lim_{x->3} (x^2-9)/(x-3) = 6" (= (sl-limit (sl-shift-poly (list -9 0 1) 3) (sl-shift-poly (list -3 1) 3) 3) 6))

(newline)
(display "Limits at arbitrary points: indeterminate forms resolved by series order comparison, the quotient of") (newline)
(display "unit series giving the value -- L'Hopital carried out exactly on the local expansions.") (newline)
