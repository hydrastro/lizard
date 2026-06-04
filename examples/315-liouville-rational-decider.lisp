; The rational-coefficient extension of the Liouville exponential decider: decides INT R(x) e^x dx for R a
; RATIONAL function (the polynomial case was liouville.lisp).  By Liouville's theorem this is elementary iff a
; RATIONAL S solves S' + S = R, with antiderivative S e^x; the decider returns S or PROVES non-elementarity
; (docs/TRAGER_ROADMAP.md, the summit).  R is given as a polynomial part plus a principal part at the origin
; (r_1 r_2 ...), r_j the coefficient of 1/x^j.
(import "cas/liouvillerat.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "INT R(x) e^x dx for rational R: elementary (with S, so INT = S e^x) or PROVEN non-elementary.") (newline) (newline)

(display "INT e^x/x dx -- the exponential integral Ei, PROVEN non-elementary (a simple pole, residue 1):") (newline)
(display "  ") (display (lr-decide (list) (list 1))) (newline)
(chk "INT e^x/x is PROVEN non-elementary (Ei)" (equal? (car (lr-decide (list) (list 1))) (quote non-elementary)))

(display "INT e^x/x^2 dx -- PROVEN non-elementary (reduces to Ei by parts):") (newline)
(chk "INT e^x/x^2 is PROVEN non-elementary" (equal? (car (lr-decide (list) (list 0 1))) (quote non-elementary)))

(display "INT (1/x - 1/x^2) e^x dx = e^x/x -- ELEMENTARY (the designed case, S = 1/x):") (newline)
(define d3 (lr-decide (list) (list 1 -1)))
(display "  ") (display d3) (newline)
(chk "INT (1/x - 1/x^2) e^x = e^x/x, S certified" (if (equal? (car d3) (quote elementary)) (lr-certify (list) (list 1 -1) (lr-nth d3 1) (lr-nth d3 2)) #f))

(display "INT x e^x dx = (x-1) e^x -- ELEMENTARY (polynomial part, S = x-1):") (newline)
(define d4 (lr-decide (list 0 1) (list)))
(chk "INT x e^x = (x-1) e^x, S certified" (if (equal? (car d4) (quote elementary)) (lr-certify (list 0 1) (list) (lr-nth d4 1) (lr-nth d4 2)) #f))

(display "INT (1/x^2 - 2/x^3) e^x dx = e^x/x^2 -- ELEMENTARY (S = 1/x^2):") (newline)
(define d5 (lr-decide (list) (list 0 1 -2)))
(chk "INT (1/x^2 - 2/x^3) e^x = e^x/x^2, S certified" (if (equal? (car d5) (quote elementary)) (lr-certify (list) (list 0 1 -2) (lr-nth d5 1) (lr-nth d5 2)) #f))

(newline)
(display "The decider solves S' + S = R over the rationals: a solution proves elementarity (INT = S e^x), an") (newline)
(display "inconsistent system proves non-elementarity -- the exponential integral Ei among the proven-impossible.") (newline)
