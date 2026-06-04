; RADICAL MEMBERSHIP f in sqrt(I): does f vanish on the variety V(I)?  Decided by the Rabinowitsch trick and
; Hilbert's Nullstellensatz (docs/CAS.md -- summit S4, strictly stronger than ordinary ideal membership).
;
; By the Nullstellensatz, f vanishes on V(I) iff f is in sqrt(I), iff the ideal <I, 1 - t*f> in one extra variable
; is the whole ring, iff 1 reduces to 0 modulo a Groebner basis of {generators of I} union {1 - t*f}.  This is an
; exact two-sided decision, and strictly stronger than membership: x vanishes on V(<x^2>) without lying in <x^2>.
(import "cas/radideal.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Radical membership f in sqrt(I): geometric vanishing on V(I), via the Nullstellensatz.") (newline) (newline)

(display "the defining example: x vanishes where x^2 does, so x is in sqrt(<x^2>) -- but NOT in <x^2> itself:") (newline)
(define x2 (list (cons 1 (list 2))))
(define x (list (cons 1 (list 1))))
(chk "x is in the radical of <x^2>" (ri-in-radical? (list x2) x 1))
(chk "x is NOT in <x^2> as an ideal (normal form is nonzero)" (if (ri-in-ideal? (list x2) x 1) #f #t))
(chk "so the radical is strictly larger than the ideal" (if (ri-in-ideal? (list x2) x 1) #f (ri-in-radical? (list x2) x 1)))

(display "y^2 vanishes where y^3 does, so y^2 is in sqrt(<y^3>):") (newline)
(chk "y^2 is in the radical of <y^3>" (ri-in-radical? (list (list (cons 1 (list 3)))) (list (cons 1 (list 2))) 1))

(display "on the ideal <x^2 + y^2 - 1, x - y>, the generator x - y vanishes on V(I); x + y does not:") (newline)
(define f1 (list (cons 1 (list 2 0)) (cons 1 (list 0 2)) (cons -1 (list 0 0))))
(define f2 (list (cons 1 (list 1 0)) (cons -1 (list 0 1))))
(chk "x - y is in the radical" (ri-in-radical? (list f1 f2) (list (cons 1 (list 1 0)) (cons -1 (list 0 1))) 2))
(chk "x + y is NOT in the radical" (if (ri-in-radical? (list f1 f2) (list (cons 1 (list 1 0)) (cons 1 (list 0 1))) 2) #f #t))

(display "soundness: a polynomial nonzero on the variety is rejected -- x - 1 does not vanish at x = 0:") (newline)
(chk "x - 1 is NOT in the radical of <x^2>" (if (ri-in-radical? (list x2) (list (cons 1 (list 1)) (cons -1 (list 0))) 1) #f #t))

(display "an inconsistent system has empty variety, so 1 lies in its radical:") (newline)
(chk "1 is in sqrt(<x, x-1>)" (ri-in-radical? (list (list (cons 1 (list 1))) (list (cons 1 (list 1)) (cons -1 (list 0)))) (list (cons 1 (list 0))) 1))

(newline)
(display "Radical membership now decides geometric vanishing on a variety -- the Nullstellensatz test via the") (newline)
(display "Rabinowitsch trick -- strictly stronger than ideal membership and exact in both directions.  Computing a") (newline)
(display "full set of radical generators, and primary decomposition, remain beyond this point.") (newline)
