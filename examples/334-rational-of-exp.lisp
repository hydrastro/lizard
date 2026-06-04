; Integration of ANY rational function of e^x, via the substitution t = e^x (dx = dt/t): INT R(e^x) dx =
; INT R(t)/t dt, a rational integral in t handled completely by the top-level rational integrator, then
; reinterpreted in e^x (log(t) = x, log(t-c) = log(e^x - c)).  This closes the "any rational function of e^x"
; capability row (docs/TRAGER_ROADMAP.md, the summit).
;
; The reduction is exact and the result certified: the t-integral is self-certified by the rational integrator,
; and INT R(e^x) dx = INT R(t)/t dt is the change-of-variables identity (t' = t).
(import "cas/rischratmono.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Rational functions of e^x, integrated by t = e^x and the complete rational integrator.") (newline) (newline)

(display "INT 1/(e^x + 1) dx = x - log(e^x + 1)  (in t: INT 1/(t(t+1)) dt = log t - log(t+1)):") (newline)
(define r1 (ratmono-exp-integrate (list 1) (list 1 1)))
(display "  t-logs = ") (display (car (cdr (cdr r1)))) (display "  (residue +1 at t, -1 at t+1)") (newline)
(chk "INT 1/(e^x+1) elementary, certified" (if (equal? (car r1) (quote elementary)) (ratmono-exp-certify (list 1) (list 1 1) r1) #f))

(display "INT e^x/(e^x + 1) dx = log(e^x + 1):") (newline)
(define r2 (ratmono-exp-integrate (list 0 1) (list 1 1)))
(chk "INT e^x/(e^x+1) elementary, certified" (if (equal? (car r2) (quote elementary)) (ratmono-exp-certify (list 0 1) (list 1 1) r2) #f))

(display "INT 1/(e^x - 1) dx = -x + log(e^x - 1):") (newline)
(define r3 (ratmono-exp-integrate (list 1) (list -1 1)))
(chk "INT 1/(e^x-1) elementary, certified" (if (equal? (car r3) (quote elementary)) (ratmono-exp-certify (list 1) (list -1 1) r3) #f))

(display "INT 1/(e^(2x) + 1) dx = x - (1/2) log(e^(2x) + 1)  (the t^2+1 factor gives a real logarithm here):") (newline)
(define r4 (ratmono-exp-integrate (list 1) (list 1 0 1)))
(chk "INT 1/(e^(2x)+1) elementary, certified" (if (equal? (car r4) (quote elementary)) (ratmono-exp-certify (list 1) (list 1 0 1) r4) #f))

(newline)
(display "Any rational function of e^x integrates through the substitution t = e^x: the resulting rational") (newline)
(display "integral in t is solved completely (rational part + Rothstein-Trager logarithms) and certified, then") (newline)
(display "read back in e^x -- closing the rational-function-of-e^x row of the capability map.") (newline)
