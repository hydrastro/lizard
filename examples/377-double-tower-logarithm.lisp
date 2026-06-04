; The LOGARITHMIC INTEGRAL in the double algebraic tower Q(x)[y][z]/(z^2-y, y^2-x): the field inverse and the
; third-kind construction INT (e'/e) dx = log(e), certified (docs/TRAGER_ROADMAP.md -- RUNG 5, the third-kind
; logarithm over a stacked algebraic tower of x^(1/4)).
;
; The inverse uses the outer conjugate: e*conj(e) = a^2 - b^2 y lands in the inner field, so one inner inverse
; gives e^(-1).  Then e'/e = D(e)*e^(-1), and INT (e'/e) dx = log(e), certified two independent ways: the
; inverse-free cleared identity e*(e'/e) = D(e) in the tower, and the round trip.  A non-logarithmic differential
; is rejected, never assigned a spurious logarithm.
(import "cas/algtower2log.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Logarithms over the tower of x^(1/4): INT (e'/e) dx = log(e), with the field inverse, certified.") (newline) (newline)

(display "the field inverse is exact (one inner inverse via the outer conjugate):") (newline)
(define e1 (t2-add (t2-one) (t2-z)))                  ; 1 + z = 1 + x^(1/4)
(must "e * e^(-1) = 1 for e = 1 + x^(1/4)" (t2-equal? (t2-mul e1 (t2l-inv e1)) (t2-one)))
(define exz (t2-add (t2-from-rat (rat-from-poly (list 0 1))) (t2-z)))   ; x + z
(must "e * e^(-1) = 1 for e = x + x^(1/4)" (t2-equal? (t2-mul exz (t2l-inv exz)) (t2-one)))

(display "INT (e'/e) dx = log(1 + x^(1/4)), certified by the cleared identity e*(e'/e) = D(e):") (newline)
(must "the cleared certificate holds" (t2l-cleared-cert e1))
(must "the round trip d(log e) = e'/e holds" (t2l-verify-log e1))

(display "the pure generators integrate to their logarithms:") (newline)
(must "e'/e = 1/(4x) for e = x^(1/4), i.e. INT = log(x^(1/4))" (rat-equal? (af-u (t2-a (t2l-logderiv (t2-z)))) (rat-make (list 1) (list 0 4))))
(must "e'/e = 1/(2x) for e = sqrt(x), i.e. INT = log(sqrt x)" (rat-equal? (af-u (t2-a (t2l-logderiv (t2-y)))) (rat-make (list 1) (list 0 2))))
(must "the cleared certificate holds for e = x^(1/4)" (t2l-cleared-cert (t2-z)))

(display "soundness: a differential that is not d(log e) is rejected, not assigned a logarithm:") (newline)
(must "F = z is correctly rejected as d(log(1 + x^(1/4)))" (if (t2l-is-logderiv? e1 (t2-z)) #f #t))
(must "the true e'/e is accepted by the recognizer" (t2l-is-logderiv? e1 (t2l-logderiv e1)))

(newline)
(display "The third-kind logarithm now works over a tower of TWO stacked algebraic extensions: the inverse via the") (newline)
(display "outer conjugate, the logarithmic differential e'/e, and INT (e'/e) dx = log(e) certified by the inverse-free") (newline)
(display "cleared identity.  Deeper towers with several independent radicals remain the open summit.") (newline)
