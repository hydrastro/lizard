; 245-height-two-integral.lisp -- a height-two integral with a logarithmic term (single-log case).
;
; After Hermite, a squarefree remainder A*/D* integrates to c log(D*) when A* = c D2(D*) for a constant
; c; the integrator returns the rational part plus that logarithm, certified by D2.  Here theta1 = e^x,
; theta2 = log(e^x + 1).  (A complete integral combining a rational part with a logarithm is in the
; cas_tower2int golden; the general several-residue logarithmic part is in example 246.)  `must` raises
; on failure.

(import "cas/tower2int.lisp")
(define EXP1 (list 'exp (list 0 1)))
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'h2-int-check-failed)))
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))

(display "Height-two logarithm: INT (D theta2)/theta2 dx = log(theta2) = log(log(e^x + 1))") (newline) (newline)
(define A (list Dth2))                                  ; D theta2
(define D (list (tr-zero) (t2-trone)))                  ; theta2
(define res (int-h2 A D Dth2 EXP1))
(must "the integral is elementary"               (equal? (car res) 'ok))
(must "the answer is a single logarithm"         (equal? (car (car (cdr (cdr res)))) 'log))
(must "the residue is the constant 1"            (tr-equal? (car (cdr (car (cdr (cdr res))))) (t2-trone)))
(must "logarithm certified: D2 = A/D"            (h2tr-equal? (int-h2-deriv res Dth2 EXP1) (list A D)))
(newline)
(display "height-two-integral checks passed.") (newline)
