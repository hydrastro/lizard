; 245-height-two-integral.lisp -- a COMPLETE height-two integral: rational part plus a logarithm.
;
; Building on the height-two Hermite reduction (example 244), this adds a logarithmic term: after
; Hermite reduces A/D to a rational part g and a squarefree remainder A*/D*, the remainder integrates
; to c log(D*) when A* = c D2(D*) for a constant c of the tower.  The integrator returns g + c log(D*),
; certified by differentiating with the two-level derivation D2 and checking equality with A/D over
; K1[theta2].  The tower is theta1 = e^x, theta2 = log(e^x + 1).  `must` raises on failure.

(import "cas/tower2int.lisp")
(define EXP1 (list 'exp (list 0 1)))
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'h2-int-check-failed)))
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))   ; D theta2 = e^x/(e^x+1) in K1

(display "Complete height-two integral (theta2 = log(e^x + 1) over K1 = Q(x, e^x))") (newline) (newline)

(display "    INT (D theta2)(theta2 + 1)/theta2^2 dx = -1/theta2 + log(theta2)") (newline)
(display "      = -1/log(e^x + 1) + log(log(e^x + 1))") (newline)
(define A (list Dth2 Dth2))                            ; (D theta2)(1 + theta2)
(define D (list (tr-zero) (tr-zero) (t2-trone)))       ; theta2^2
(define res (int-h2 A D Dth2 EXP1))
(must "the integral is found to be elementary"      (equal? (car res) 'ok))
(must "a logarithmic term is present"               (equal? (car (car (cdr (cdr res)))) 'log))
(must "the residue is the constant 1"               (tr-equal? (car (cdr (car (cdr (cdr res))))) (t2-trone)))
(must "rational part + log certified: D2 = A/D"     (h2tr-equal? (int-h2-deriv res Dth2 EXP1) (list A D)))
(newline)
(display "all height-two-integral checks passed.") (newline)
