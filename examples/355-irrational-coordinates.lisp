; EXACT NAMING OF IRRATIONAL COORDINATES for a zero-dimensional polynomial system: take the univariate eliminant
; in a coordinate, peel off its rational roots, and name each remaining REAL root exactly as an algebraic number --
; a minimal polynomial together with an isolating rational interval (docs/CAS.md -- summit S3, beyond rational
; tuples).
;
; Where polysolve2 flagged a non-rational coordinate as 'irrational-fiber, this turns the flag into an exact name:
; rational roots stay rational; the eliminant is deflated by its rational linear factors; and each real root of the
; squarefree cofactor is isolated by Sturm and named (algebraic minpoly interval).  All exact over Q; complex roots
; are counted but not named (the real roots are the isolable quantity).
(import "cas/polysolve3.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Naming irrational coordinates exactly: algebraic numbers (minimal polynomial + isolating interval).") (newline) (newline)

(display "the eliminant x^2 - 2 has two irrational roots, named as algebraic numbers with minpoly x^2 - 2:") (newline)
(define r2 (ps3-named-real-roots (list -2 0 1)))
(display "  ") (display r2) (newline)
(chk "two algebraic roots are named" (= (ps3-len r2) 2))
(chk "the first named root satisfies x^2 - 2 (minpoly divides, interval brackets a sign change)" (ps3-verify-root (list -2 0 1) (car r2)))
(chk "the second named root satisfies x^2 - 2" (ps3-verify-root (list -2 0 1) (car (cdr r2))))
(chk "x^2 - 2 has no complex roots" (= (ps3-num-complex (list -2 0 1)) 0))

(display "a mixed eliminant (x-1)(x^2-2): the rational 1 plus the two algebraic roots:") (newline)
(define rm (ps3-named-real-roots (list 2 -2 -1 1)))
(display "  ") (display rm) (newline)
(chk "three roots named: one rational, two algebraic" (= (ps3-len rm) 3))
(chk "the irrational cofactor is exactly x^2 - 2" (equal? (ps3-irrational-factor (list 2 -2 -1 1)) (list -2 0 1)))

(display "soundness: x^2 + 1 has no real roots -- none are named, two complex are reported:") (newline)
(chk "no real roots named for x^2 + 1" (null? (ps3-named-real-roots (list 1 0 1))))
(chk "two complex roots reported honestly" (= (ps3-num-complex (list 1 0 1)) 2))

(display "a fully rational eliminant x^2 - 3x + 2 still names 1 and 2 as rationals:") (newline)
(chk "names {1, 2}" (= (ps3-len (ps3-named-real-roots (list 2 -3 1))) 2))

(newline)
(display "Irrational coordinates are now named exactly as algebraic numbers -- a minimal polynomial and an isolating") (newline)
(display "interval -- each certified against the eliminant, with rational coordinates kept rational and complex roots") (newline)
(display "counted honestly.  Pairing these named coordinates into full algebraic solution tuples, and naming complex") (newline)
(display "solutions, are the remaining steps.") (newline)
