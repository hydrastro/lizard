; 246-height-two-rootsum.lisp -- the general height-two Rothstein-Trager logarithmic part: several
; constant residues, not just a single logarithm.
;
; After Hermite reduces A/D to a rational part and a remainder A*/D* squarefree in theta2, the
; logarithmic part is a sum over residues c of c log(v_c), where v_c = gcd_theta2(D*, A* - c D2(D*))
; over K1 = Q(x)(theta1) and the residues are the roots of the Rothstein-Trager resultant
; R(z) = Res_theta2(D*, A* - z D2(D*)).  That resultant, an element of K1[z], is built by evaluating
; the Sylvester resultant over K1 at numeric z and interpolating; dividing by a fixed nonzero value
; cancels the K1 content, so the residues are rational exactly when the ratios are constant.  Here
; theta1 = e^x and theta2 = log(e^x + 1), and the integrand is constructed by superposing two single
; logarithms with residues 1 and 2:
;
;     INT (D theta2)(3 theta2 - 1) / (theta2^2 - theta2) dx = log(theta2) + 2 log(theta2 - 1)
;        = log(log(e^x + 1)) + 2 log(log(e^x + 1) - 1)
;
; The answer is certified by differentiating the RootSum with the two-level derivation D2 and checking
; equality with A/D over K1[theta2].  `must` raises on failure.

(import "cas/tower2rt.lisp")
(define EXP1 (list 'exp (list 0 1)))
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'h2-rootsum-check-failed)))
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))   ; D theta2 = e^x/(e^x+1)
(define A (list (k1-neg Dth2) (k1-mul (k1-from-int 3) Dth2)))                  ; (D theta2)(3 theta2 - 1)
(define D (list (k1-zero) (k1-neg (k1-one)) (k1-one)))                         ; theta2^2 - theta2 (squarefree)

(display "General height-two Rothstein-Trager logarithmic part (two rational residues)") (newline) (newline)
(define lg (h2rt-logpart A D Dth2 EXP1))
(must "the logarithmic part is a RootSum"             (equal? (car lg) 'rootsum))
(must "there are two distinct residues"               (= (length (car (cdr lg))) 2))
(must "RootSum certified: sum of c D2(v)/v equals A/D"
      (h2tr-equal? (h2rt-logderiv (car (cdr lg)) Dth2 EXP1 (h2tr-zero)) (list A D)))
(newline)
(display "all height-two-rootsum checks passed.") (newline)
