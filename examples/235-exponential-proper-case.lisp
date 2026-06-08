; 235-exponential-proper-case.lisp -- the exponential proper case: integrating a proper rational
; function of theta = e^x (denominator coprime to theta), the mirror of example 234's logarithmic
; case.  The residue criterion is identical -- INT a/d elementary iff R(z) = Res_theta(d, a - z Dd)
; has constant roots, with INT a/d = sum_i c_i log gcd_theta(d, a - c_i Dd) -- but the exponential
; derivation D(theta^i) = i theta^i makes Dd have the same theta-degree as d, and each log argument
; v_i ~ theta^{deg v_i} behaves like (deg v_i) x at infinity (since log theta = x).  So the honest
; answer carries a base-field correction: the logarithmic part sum_i c_i log(v_i) is accompanied by
; - (sum_i c_i deg_theta v_i) x.  Hermite reduction gives the rational part, the residue reduction
; (shared with towerrt.lisp) the logarithms.  Every answer is certified by differentiating in the
; tower.  `must` raises on failure.

(import "cas/expnrt.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'expnrt-check-failed)))
(define (residues r) (map car (car (cdr (cdr r)))))
(define (correction r) (car (cdr (cdr (cdr r)))))

(display "Rothstein-Trager in an exponential tower: the proper case of INT R(e^x) dx") (newline) (newline)

(display "1. two rational residues with a base-field correction") (newline)
(display "    INT (-5 e^x - 6)/(e^(2x) + 3 e^x + 2) dx = log(e^x+1) + 2 log(e^x+2) - 3x") (newline)
(define a1 (list (rat-from-poly (list -6)) (rat-from-poly (list -5))))
(define d1 (list (rat-from-poly (list 2)) (rat-from-poly (list 3)) (rat-one)))
(define r1 (int-exp-rational a1 d1))
(display "    residues = ") (display (residues r1)) (display ",  correction coefficient of -x = ") (display (correction r1)) (newline)
(must "elementary"  (int-exp-rational-elementary? a1 d1))
(must "certified by differentiation in the tower" (int-exp-rational-verify a1 d1))
(newline)

(display "2. the proper part of e^x/(e^x+1)") (newline)
(display "    INT -1/(e^x+1) dx = log(e^x+1) - x") (newline)
(define a2 (list (rat-from-poly (list -1)))) (define d2 (list (rat-one) (rat-one)))
(display "    residue = ") (display (residues (int-exp-rational a2 d2))) (display ",  correction = ") (display (correction (int-exp-rational a2 d2))) (newline)
(must "certified" (int-exp-rational-verify a2 d2))
(newline)

(display "3. fractional residues and a fractional correction") (newline)
(define a3 (list (rat-make (list -9) (list 2)) (rat-make (list -1) (list 2))))
(define d3 (list (rat-from-poly (list 3)) (rat-from-poly (list 4)) (rat-one)))   ; theta^2+4theta+3
(display "    residues = ") (display (residues (int-exp-rational a3 d3))) (display ",  correction = ") (display (correction (int-exp-rational a3 d3))) (display "  (expect {2,-1/2}, 3/2)") (newline)
(must "certified" (int-exp-rational-verify a3 d3))
(newline)

(display "4. a balanced quotient of logarithms (zero correction)") (newline)
(define v1 (list (rat-one) (rat-one))) (define v2 (list (rat-from-poly (list 3)) (rat-one)))
(define E (list 'exp (list 0 1)))
(define a4 (rfpoly-sub (rfpoly-mul (Drf v1 E) v2) (rfpoly-mul (Drf v2 E) v1)))
(define d4 (rfpoly-mul v1 v2))
(must "INT for log((e^x+1)/(e^x+3)) certified, correction 0" (int-exp-rational-verify a4 d4))
(must "correction is indeed 0" (= (correction (int-exp-rational a4 d4)) 0))
(newline)

(display "5. non-elementarity decided by the residue criterion") (newline)
(define a5 (list (rat-one))) (define d5 (list (rat-from-poly (list 0 1)) (rat-one)))   ; theta + x
(must "INT 1/(e^x + x) dx reported non-elementary (residue depends on x)" (not (int-exp-rational-elementary? a5 d5)))
(newline)

(display "all exponential proper-case checks passed.") (newline)
