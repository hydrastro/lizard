; The EXPONENTIAL single-logarithm recognizer that completes the proper-fraction branch of the height-two
; exponential integrator (docs/CAS.md -- the exponential second-monomial integrator, single-logarithm case).
;
; The unified driver integrates the proper part of an exponential height-two integrand by Hermite reduction and a
; power-sum step, but its power-sum wrapper only accepted a squarefree denominator that is a bare power
; c*theta2^j.  This recognizer -- the exponential mirror of the primitive single-log recognizer -- handles any
; squarefree denominator: As/Ds is the single logarithm c log(Ds) exactly when As = c * D2(Ds) for a tower
; constant c, found by one polynomial division over K1[theta2] (no resultant), and certified by differentiation.
(import "cas/tower2expfull.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define MEXP (list (quote exp) (list 0 1)))                          ; theta1 = e^x
(define TH1 (list (list (rat-zero) (rat-one)) (list (rat-one))))     ; theta1 = e^x (u' = theta1)
(define UP TH1)                                                       ; theta2 = exp(e^x)

(display "Exponential single logarithm: INT [c D2(v)]/v dx = c log(v), for a squarefree v that is not a power.") (newline) (newline)

(display "v = theta2 - e^x is squarefree but NOT a bare power -- the case the power-sum wrapper rejected:") (newline)
(define v (list (k1-neg TH1) (k1-one)))
(define dv (t2e-deriv v UP MEXP))
(define r1 (t2ef-newlog dv v UP MEXP))
(must "INT D2(v)/v dx is recognized as a logarithm" (equal? (car r1) (quote log)))
(must "the residue is 1" (equal? (car (cdr r1)) (k1-one)))
(must "the logarithm log(theta2 - e^x) is certified by differentiation" (t2ef-newlog-verify dv v UP MEXP))

(display "a scaled residue: INT [2 D2(v)]/v dx = 2 log(v):") (newline)
(define As2 (h2-cscale (k1-iscale 2 (k1-one)) dv))
(must "the residue is 2" (equal? (car (cdr (t2ef-newlog As2 v UP MEXP))) (k1-iscale 2 (k1-one))))
(must "the scaled logarithm is certified" (t2ef-newlog-verify As2 v UP MEXP))

(display "a different squarefree denominator v = theta2 + 2 e^x:") (newline)
(define v2 (list (k1-iscale 2 TH1) (k1-one)))
(must "log(theta2 + 2 e^x) is recognized and certified" (if (equal? (car (t2ef-newlog (t2e-deriv v2 UP MEXP) v2 UP MEXP)) (quote log)) (t2ef-newlog-verify (t2e-deriv v2 UP MEXP) v2 UP MEXP) #f))

(display "soundness: a remainder that is not a constant multiple of D2(v)/v is not misreported as a log:") (newline)
(define notlog (list (k1-zero) (k1-one)))                            ; theta2, not c D2(v)
(must "INT theta2/v dx returns 'none (not a single logarithm)" (equal? (t2ef-newlog notlog v UP MEXP) (quote none)))
(must "and the proper-part driver defers it rather than guessing" (equal? (t2ef-proper notlog v UP MEXP) (quote notrecognized)))

(newline)
(display "The exponential proper-fraction branch now resolves the single-logarithm case for any squarefree") (newline)
(display "denominator, not just bare powers -- cheaply, by one division, certified by differentiation.  The general") (newline)
(display "multi-residue x-dependent RootSum still routes through the dedicated fraction-free resultant integrator.") (newline)
