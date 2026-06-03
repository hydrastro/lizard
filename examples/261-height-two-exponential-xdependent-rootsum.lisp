; The exponential analogue of example 260.  For theta2 = exp(u) the substitution integrator (tower2sub)
; only reaches logarithm arguments constant in x; when the arguments depend on x the residues must come
; from the Sylvester resultant over K1 = Q(x)(theta1).  Because the fraction-free apparatus of tower2ff is
; indifferent to the derivation -- it only consumes D, A and D2(D) -- tower2expff reuses it whole, feeding
; D2(D) computed with the EXPONENTIAL derivation D2(theta2) = u' theta2.  With theta2 = exp(e^x) (so
; u' = e^x = theta1) the degree-three integral
;     INT A/D dx = log(theta2 - e^x) + 2 log(theta2 - 2 e^x) + 3 log(theta2 + e^x)
; (all three logarithm arguments x-dependent and coprime to theta2, since theta2 = exp is a unit) is solved
; fraction-free: Bareiss resultant over Q(x)[theta1], gcd-free ratio test, primitive-PRS gcd for the
; arguments.  Residues 1, 2, 3 are recovered and the RootSum is certified by the matching EXPONENTIAL
; logarithmic derivative: sum c_i D2(v_i)/v_i equals A/D exactly in K1[theta2].  Computed once and certified.
(import "cas/tower2expff.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define MEXP (list 'exp (list 0 1)))
(define TH1 (list (list (rat-zero) (rat-one)) (list (rat-one))))     ; theta1 = e^x
(define UP TH1)                                                       ; theta2 = exp(e^x),  u' = e^x = theta1
(define w1 (list (k1-neg TH1) (k1-one)))                             ; theta2 - e^x
(define w2 (list (k1-neg (k1-iscale 2 TH1)) (k1-one)))              ; theta2 - 2 e^x
(define w3 (list TH1 (k1-one)))                                      ; theta2 + e^x
(define D (h2-mul (h2-mul w1 w2) w3))
(define A (h2-add (h2-add (h2-cscale (k1-from-int 1) (h2-mul (t2e-deriv w1 UP MEXP) (h2-mul w2 w3)))
                          (h2-cscale (k1-from-int 2) (h2-mul (t2e-deriv w2 UP MEXP) (h2-mul w1 w3))))
                  (h2-cscale (k1-from-int 3) (h2-mul (t2e-deriv w3 UP MEXP) (h2-mul w1 w2)))))
(display "INT A/D dx = log(exp(e^x)-e^x) + 2 log(exp(e^x)-2e^x) + 3 log(exp(e^x)+e^x)") (newline) (newline)
(define lg (h2rt-logpart-exp-ff A D UP MEXP))                        ; fraction-free, exponential, computed ONCE
(must "logarithmic part is a RootSum (exponential, x-dependent)" (equal? (car lg) 'rootsum))
(must "three distinct residues"                                  (= (length (car (cdr lg))) 3))
(must "the residues are 1, 2, 3"                                 (equal? (map car (car (cdr lg))) (list 1 2 3)))
(must "RootSum CERTIFIED: sum c D2(v)/v = A/D (exponential)"
      (h2tr-equal? (h2rt-logderiv-exp (car (cdr lg)) UP MEXP (h2tr-zero)) (list A D)))
(newline) (display "exponential x-dependent degree-3 RootSum certified, fraction-free over Q(x)[theta1].") (newline)
