; 244-height-two-hermite.lisp -- Hermite reduction at height two: the rational part of integrating a
; rational function of a second monomial theta2 over the height-one field K1 = Q(x)(theta1).
;
; This is the first substantive rung of full height-two integration.  It mirrors the height-one
; Hermite reduction one level up: arithmetic on Q(x)[theta1] becomes arithmetic on K1[theta2], and the
; height-one derivation becomes the two-level derivation D2.  The setting is theta2 primitive over K1,
; here theta1 = e^x and theta2 = log(e^x + 1), so D theta2 = e^x/(e^x + 1) lies in K1.  Given a proper
; rational function A/D of theta2 over K1, Hermite returns a rational part g and a remainder A*/D*
; with D* squarefree, certified by D2(g) + A*/D* = A/D.  `must` raises on failure.

(import "cas/tower2herm.lisp")
(define EXP1 (list 'exp (list 0 1)))
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'h2-hermite-check-failed)))
(define Dth2 (list (list (rat-zero) (rat-one)) (list (rat-one) (rat-one))))   ; D theta2 = e^x/(e^x+1) in K1
(define (cert H A D) (h2tr-equal? (h2tr-add (h2tr-deriv (car (car H)) (car (cdr (car H))) Dth2 EXP1)
                                            (list (car (cdr H)) (car (cdr (cdr H))))) (list A D)))

(display "Height-two Hermite reduction (theta2 = log(e^x + 1) over K1 = Q(x, e^x))") (newline) (newline)

(display "1. a purely rational height-two antiderivative") (newline)
(display "    INT (D theta2)/theta2^2 dx = -1/theta2   (= -1/log(e^x + 1))") (newline)
(define A1 (list Dth2))                                   ; numerator D theta2 (degree 0 in theta2)
(define D1 (list (tr-zero) (tr-zero) (t2-trone)))         ; theta2^2
(define H1 (h2-hermite A1 D1 Dth2 EXP1))
(must "rational part is -1/theta2 (numerator -1)" (h2-equal? (car (car H1)) (list (k1-neg (k1-one)))))
(must "no squarefree remainder remains"          (h2-zero? (car (cdr H1))))
(must "Hermite identity D2(g) = A/D certified"   (cert H1 A1 D1))
(newline)

(display "2. Hermite separates a rational part from a squarefree (logarithmic) remainder") (newline)
(display "    INT (D theta2)(theta2 + 1)/theta2^2 dx : rational part -1/theta2 plus INT (D theta2)/theta2") (newline)
(define A2 (list Dth2 Dth2))                              ; (D theta2)(1 + theta2)
(define D2p (list (tr-zero) (tr-zero) (t2-trone)))        ; theta2^2
(define H2 (h2-hermite A2 D2p Dth2 EXP1))
(must "a squarefree remainder of degree 1 in theta2 is left" (= (h2-deg (car (cdr H2))) 0))
(must "Hermite identity D2(g) + A*/D* = A/D certified"       (cert H2 A2 D2p))
(newline)

(display "all height-two-hermite checks passed.") (newline)
