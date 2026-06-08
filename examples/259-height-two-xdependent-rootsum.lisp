; General height-two Rothstein-Trager with X-DEPENDENT logarithm arguments.  Example 246 integrates a
; height-two RootSum whose denominator has rational (x-independent) coefficients; here the logarithm
; arguments themselves depend on x, through theta1 = e^x:
;     INT A/D dx = log(theta2 - e^x) + 2 log(theta2 + e^x),   theta2 = log(e^x + 1),
; with D = (theta2 - e^x)(theta2 + e^x) = theta2^2 - e^(2x) (an x-dependent K1 coefficient).  The
; logarithmic part is the genuine Sylvester resultant over K1 = Q(x)(theta1) -- no substitution shortcut,
; because the arguments are not constant in x -- evaluated by Gaussian elimination over K1 with an
; explicit (gc) between evaluations so the transient K1-fraction garbage is reclaimed (this is what keeps
; the multi-evaluation resultant within memory).  The residues 1, 2 are recovered by rational Lagrange
; interpolation of the resultant ratios, and the RootSum is certified by differentiation: the sum of
; c_i D2(v_i)/v_i is checked to equal A/D exactly in K1[theta2].  Computed once and certified from the
; same result to stay within the interpreter's heap.
(import "cas/tower2rt.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(define MEXP (list 'exp (list 0 1)))
(define TH1 (list (list (rat-zero) (rat-one)) (list (rat-one))))             ; theta1 = e^x as K1 element
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))  ; D theta2 = e^x/(e^x+1), theta2 = log(e^x+1)
(define v1 (list (k1-neg TH1) (k1-one)))                                     ; theta2 - e^x   (x-dependent argument)
(define v3 (list TH1 (k1-one)))                                              ; theta2 + e^x   (x-dependent argument)
(define D (h2-mul v1 v3))                                                    ; theta2^2 - e^(2x)
(define A (h2-add (h2-cscale (k1-from-int 1) (h2-mul (t2-deriv v1 Dth2 MEXP) v3))
                  (h2-cscale (k1-from-int 2) (h2-mul (t2-deriv v3 Dth2 MEXP) v1))))   ; D2(v1) v3 + 2 D2(v3) v1
(display "INT A/D dx = log(theta2 - e^x) + 2 log(theta2 + e^x),  theta2 = log(e^x + 1)") (newline) (newline)
(define lg (h2rt-logpart A D Dth2 MEXP))                                     ; the K1 resultant, computed ONCE
(must "logarithmic part is a RootSum (x-dependent arguments)" (equal? (car lg) 'rootsum))
(must "two distinct residues"                                 (= (length (car (cdr lg))) 2))
(must "the residues are 1 and 2"                              (equal? (map car (car (cdr lg))) (list 1 2)))
(must "RootSum CERTIFIED: sum c D2(v)/v = A/D in K1[theta2]"
      (h2tr-equal? (h2rt-logderiv (car (cdr lg)) Dth2 MEXP (h2tr-zero)) (list A D)))
(newline) (display "x-dependent height-two RootSum certified (resultant over K1, gc-managed).") (newline)
