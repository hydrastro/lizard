; 244-height-two-hermite.lisp -- Hermite reduction at height two: the rational part of integrating a
; rational function of a second monomial theta2 over the height-one field K1 = Q(x)(theta1).
;
; Mirrors the height-one Hermite reduction one level up: arithmetic on Q(x)[theta1] becomes arithmetic
; on K1[theta2], and the height-one derivation becomes the two-level derivation D2.  The setting is
; theta2 primitive over K1, here theta1 = e^x and theta2 = log(e^x + 1), so D theta2 = e^x/(e^x + 1)
; lies in K1.  Given a proper rational function A/D of theta2 over K1, Hermite returns a rational part
; g and a remainder A*/D* with D* squarefree, certified by D2(g) + A*/D* = A/D.  `must` raises on
; failure.  (A complete height-two integral with a logarithmic term is in example 245.)

(import "cas/tower2herm.lisp")
(define EXP1 (list 'exp (list 0 1)))
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'h2-hermite-check-failed)))
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))   ; D theta2 = e^x/(e^x+1) in K1

(display "Height-two Hermite reduction: INT (D theta2)/theta2^2 dx = -1/theta2") (newline) (newline)
(define A1 (list Dth2))                                   ; numerator D theta2 (degree 0 in theta2)
(define D1 (list (tr-zero) (tr-zero) (t2-trone)))         ; theta2^2
(define H1 (h2-hermite A1 D1 Dth2 EXP1))
(must "rational part is -1/theta2 (numerator -1)" (h2-equal? (car (car H1)) (list (k1-neg (k1-one)))))
(must "no squarefree remainder remains"          (h2-zero? (car (cdr H1))))
(must "Hermite identity D2(g) = A/D certified"
      (h2tr-equal? (h2tr-add (h2tr-deriv (car (car H1)) (car (cdr (car H1))) Dth2 EXP1)
                             (list (car (cdr H1)) (car (cdr (cdr H1))))) (list A1 D1)))
(newline)
(display "height-two-hermite checks passed.") (newline)
