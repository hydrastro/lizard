; Integration of rational functions of log x -- the dual of the e^x case, and a genuine DECIDABILITY result.
; Unlike e^x, the logarithm has no rational derivative-relation: under t = log x (x = e^t, dx = e^t dt) one gets
; INT R(log x) dx = INT R(t) e^t dt, an EXPONENTIAL integrand, not a rational one.  So the log case does NOT
; collapse to the rational integrator the way INT R(e^x) did; it is decided by the exponential Liouville
; machinery (docs/TRAGER_ROADMAP.md, the summit -- the asymmetry that makes this a decision, not a closed form).
;
; Polynomial in log x: elementary, integrated through the log tower and certified.  Proper rational in log x:
; the exponential-integral situation -- a nonzero residue is the Ei obstruction, so INT 1/log x (the logarithmic
; integral li) is PROVEN non-elementary, exactly as INT e^t/t dt = Ei(t) is.
(import "cas/rischratmonolog.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Rational functions of log x: the polynomial part is elementary, the proper-rational part is Ei-type.") (newline) (newline)

(display "the asymmetry: t = log x gives INT R(log x) dx = INT R(t) e^t dt, an exponential integral (t' = 1/x,") (newline)
(display "not rational), so this is the exponential Liouville case, not the rational integrator.") (newline) (newline)

(display "polynomial in log x is elementary, integrated through the tower and certified:") (newline)
(define r2 (ratmonolog-poly-integrate (list 0 0 1)))
(chk "INT (log x)^2 dx = x(log x)^2 - 2x log x + 2x, certified" (if (equal? (car r2) (quote elementary)) (ratmonolog-poly-certify (list 0 0 1) (car (cdr r2))) #f))
(define r3 (ratmonolog-poly-integrate (list 0 0 0 1)))
(chk "INT (log x)^3 dx elementary, certified" (if (equal? (car r3) (quote elementary)) (ratmonolog-poly-certify (list 0 0 0 1) (car (cdr r3))) #f))

(display "the logarithmic integral INT 1/log x dx (li) is PROVEN non-elementary -- the Ei obstruction:") (newline)
(chk "INT 1/log x (li) is non-elementary" (equal? (car (ratmonolog-decide (quote ()) (list 1))) (quote non-elementary)))
(chk "INT 1/(log x)^2 dx is non-elementary" (equal? (car (ratmonolog-decide (quote ()) (list 0 1))) (quote non-elementary)))

(display "and the substituted exponential decider agrees on the polynomial case (INT t e^t = (t-1)e^t):") (newline)
(chk "polynomial R decided elementary by the exponential decider" (equal? (car (ratmonolog-decide (list 0 1) (quote ()))) (quote elementary)))

(newline)
(display "Rational functions of log x are decided: the polynomial part integrates elementarily through the tower,") (newline)
(display "and the proper-rational part is the exponential-integral case -- li proven non-elementary by the exact Ei") (newline)
(display "obstruction.  Not a failure to integrate, but a proof of which integrals have no elementary form.") (newline)
