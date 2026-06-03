; The general height-two logarithmic part needs R(z) = Res_theta2(D, A - z D2(D)) over K1 = Q(x)(theta1).
; towerrt evaluates it by Gaussian elimination over the FIELD K1, whose per-step k1-mul performs two
; Euclidean rfpoly-gcd cross-cancellations; for a degree-three denominator with x-dependent coefficients
; (four evaluations, coefficients like e^(2x)) that exhausts memory even with (gc).  This example uses the
; FRACTION-FREE path of tower2ff.lisp throughout:
;   * the resultant is taken over the integral domain Q(x)[theta1] by the Bareiss one-step elimination
;     (only rfpoly multiply / subtract / EXACT divide -- no rfpoly-gcd), after clearing coefficient
;     denominators by their minimal common multiple so degrees do not blow up;
;   * the residues are read from the ratios R(z_k)/R(z_0), in which the common clearing factor cancels,
;     by a gcd-free constancy test (a leading-coefficient ratio plus an rfpoly equality), avoiding any
;     gcd on the high-degree resultant values;
;   * each logarithm argument v_c = gcd(D, A - c D2(D)) is extracted by a fraction-free primitive PRS
;     over Q(x)[theta1] (pseudo-remainders with the rfpoly content divided out), avoiding the K1
;     Euclidean fraction blow-up; v_c is then made monic over K1.
; The integral is
;     INT A/D dx = log(theta2 - e^x) + 2 log(theta2) + 3 log(theta2 + e^x),   theta2 = log(e^x + 1),
; with D = theta2^3 - e^(2x) theta2 (an x-dependent K1 denominator) and residues 1, 2, 3.  The fraction-
; free machinery is first checked against towerrt's field resultant on a 5x5 case (they must agree), and
; the RootSum is certified by differentiation: sum of c_i D2(v_i)/v_i equals A/D exactly in K1[theta2].
; Computed once and certified from the same result.
(import "cas/tower2ff.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define MEXP (list 'exp (list 0 1)))
(define TH1 (list (list (rat-zero) (rat-one)) (list (rat-one))))             ; theta1 = e^x
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))  ; D theta2 = e^x/(e^x+1), theta2 = log(e^x+1)
; -- first: the fraction-free Bareiss resultant agrees with towerrt's field resultant on a 5x5 case --
(define (clear h2) (ff-clear-poly h2 (ff-denlcm h2 (rf-one))))
(define fa (list (k1-zero) (k1-neg (k1-mul TH1 TH1)) (k1-zero) (k1-one)))    ; theta2^3 - theta1^2 theta2
(define ga (list TH1 (k1-zero) (k1-one)))                                    ; theta2^2 + theta1
(must "fraction-free resultant == field resultant (5x5)"
      (tr-equal? (tr-reduce (h2-resultant fa ga)) (tr-reduce (h2-resultant-ff (clear fa) (clear ga)))))
; -- the degree-three x-dependent integrand --
(define v1 (list (k1-neg TH1) (k1-one))) (define v2 (list (k1-zero) (k1-one))) (define v3 (list TH1 (k1-one)))
(define D (h2-mul (h2-mul v1 v2) v3))                                        ; theta2^3 - e^(2x) theta2
(define A (h2-add (h2-add
            (h2-cscale (k1-from-int 1) (h2-mul (t2-deriv v1 Dth2 MEXP) (h2-mul v2 v3)))
            (h2-cscale (k1-from-int 2) (h2-mul (t2-deriv v2 Dth2 MEXP) (h2-mul v1 v3))))
            (h2-cscale (k1-from-int 3) (h2-mul (t2-deriv v3 Dth2 MEXP) (h2-mul v1 v2)))))
(display "INT A/D dx = log(theta2 - e^x) + 2 log(theta2) + 3 log(theta2 + e^x),  theta2 = log(e^x + 1)") (newline) (newline)
(define lg (h2rt-logpart-ff A D Dth2 MEXP))                                  ; fraction-free, computed ONCE
(must "logarithmic part is a RootSum (degree-3, x-dependent)" (equal? (car lg) 'rootsum))
(must "three distinct residues"                               (= (length (car (cdr lg))) 3))
(must "the residues are 1, 2, 3"                              (equal? (map car (car (cdr lg))) (list 1 2 3)))
(must "RootSum CERTIFIED: sum c D2(v)/v = A/D in K1[theta2]"
      (h2tr-equal? (h2rt-logderiv (car (cdr lg)) Dth2 MEXP (h2tr-zero)) (list A D)))
(newline) (display "degree-3 x-dependent height-two RootSum certified, fraction-free over Q(x)[theta1].") (newline)
