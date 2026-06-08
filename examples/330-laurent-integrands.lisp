; LAURENT INTEGRANDS over a logarithmic level: integrate f = sum_k c_k theta^k where the sum may include NEGATIVE
; powers of theta = log(b), extending the polynomial-in-theta recursion to the Laurent case and capturing the
; classic new-logarithm integrals (docs/TRAGER_ROADMAP.md, the summit).
;
; A Laurent integrand splits into the polynomial part (powers >= 0, integrated by the height-n integrator) and
; the theta^{-1} coefficient c_{-1}: by the Risch theory INT c_{-1} theta^{-1} is elementary as a NEW logarithm
; exactly when c_{-1}/theta' is a constant m, giving INT (m u)/theta = m log(theta).  Deeper negative powers or a
; non-constant residue (the li case) are deferred honestly; every elementary result is certified.
(import "cas/rischlaurent.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define t1log (list (list (quote log) (rat-from-poly (list 0 1)))))   ; theta = log x, u = 1/x

(display "Laurent integrands over Q(x)(log x): the theta^{-1} term can produce a new logarithm.") (newline) (newline)

(display "the canonical case INT 1/(x log x) dx = log log x  (integrand is u theta^{-1} with u = 1/x = theta'):") (newline)
(define r (laurent-int-x-logx))
(display "  result: ") (display (car r)) (display ",  new-log coefficient m = ") (display (car (cdr (cdr r)))) (display "  (so the integral is 1 * log(log x))") (newline)
(chk "INT 1/(x log x) elementary, new-log coefficient = 1" (if (equal? (car r) (quote elementary)) (rat-equal? (car (cdr (cdr r))) (rat-one)) #f))

(display "a combined integrand INT (log x + 1/(x log x)) dx = (x log x - x) + log log x:") (newline)
(define r2 (laurent-integrate t1log 1 (list (rat-make (list 1) (list 0 1))) (list (rat-zero) (rat-one))))
(chk "polynomial part integrates to x log x - x, certified" (if (equal? (car r2) (quote elementary)) (te-int-certify t1log 1 (list (rat-zero) (rat-one)) (car (cdr r2))) #f))
(chk "and the new-log coefficient is 1" (if (equal? (car r2) (quote elementary)) (rat-equal? (car (cdr (cdr r2))) (rat-one)) #f))

(display "the logarithmic-integral case INT 1/log x dx (li) defers -- its residue 1/u = x is not constant, so") (newline)
(display "no elementary new logarithm exists (li is genuinely non-elementary):") (newline)
(chk "INT 1/log x defers honestly" (equal? (car (laurent-integrate t1log 1 (list (rat-one)) (quote ()))) (quote deferred)))

(display "a deeper Laurent integrand (theta^{-2}) also defers:") (newline)
(chk "theta^{-2} defers" (equal? (car (laurent-integrate t1log 1 (list (rat-make (list 1) (list 0 1)) (rat-one)) (quote ()))) (quote deferred)))

(newline)
(display "Laurent integrands extend the recursion past polynomials in theta: the theta^{-1} residue gives a new") (newline)
(display "logarithm when it is a constant multiple of theta', and the genuinely non-elementary cases (li, deeper") (newline)
(display "negative powers) are deferred honestly -- soundness held by the differentiation certificate.") (newline)
