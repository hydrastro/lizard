; The INTEGRAL BASIS at the finite places of a superelliptic curve y^n = f(x) -- the degree>2-in-y companion to
; the quadratic finite integral basis (docs/CAS.md -- summit S2, degree > 2 integral closure).
;
; For y^n = f with f squarefree, the integral closure of Q[x] in Q(x)[y]/(y^n - f) is the free Q[x]-module
; {1, y, ..., y^(n-1)}: each y^k is integral because it satisfies the monic t^n - f^k (since (y^k)^n = f^k), so the
; power basis is an integral basis and the order is maximal at the finite places.  When f is not squarefree the
; power basis is non-maximal, reported with the repeated factor rather than passed off as integrally closed.
(import "cas/superintbasis.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Integral basis of y^n = f at the finite places: the power basis {1, y, ..., y^(n-1)}, certified.") (newline) (newline)

(display "for y^3 = x the basis is {1, y, y^2}:") (newline)
(must "the basis exponents are (0 1 2)" (equal? (sib-power-basis 3) (list 0 1 2)))
(must "y^2 satisfies the monic t^3 - x^2 (so it is integral over Q[x])" (equal? (sib-element-minpoly 3 (list 0 1) 2) (list (quote monic) 3 (list 0 0 1))))
(must "y is integral" (sib-element-integral? 3 (list 0 1) 1))
(must "y^2 is integral" (sib-element-integral? 3 (list 0 1) 2))
(must "the discriminant of the order is 27 x^2 = 3^3 f^2" (equal? (sib-discriminant 3 (list 0 1)) (list 0 0 27)))
(must "the order is maximal, since x is squarefree" (sib-is-maximal? 3 (list 0 1)))

(display "for y^2 = x^2 - 1 the order is maximal (x^2 - 1 is squarefree):") (newline)
(must "x^2 - 1 is squarefree" (sib-squarefree? (list -1 0 1)))
(must "the order is maximal" (sib-is-maximal? 2 (list -1 0 1)))

(display "for n = 4 the basis is {1, y, y^2, y^3}:") (newline)
(must "the basis exponents are (0 1 2 3)" (equal? (sib-power-basis 4) (list 0 1 2 3)))

(display "soundness: y^2 = x^2 is NOT maximal, since x^2 is not squarefree -- reported with the repeated factor:") (newline)
(must "x^2 is not squarefree" (if (sib-squarefree? (list 0 0 1)) #f #t))
(must "the order is reported non-maximal" (if (sib-is-maximal? 2 (list 0 0 1)) #f #t))
(must "the repeated factor x is identified (degree 1)" (= (poly-deg (sib-repeated-factor 2 (list 0 0 1))) 1))

(newline)
(display "The integral closure of Q[x] in a superelliptic field y^n = f is now the certified power basis at the") (newline)
(display "finite places for squarefree f, with maximality decided by the squarefree test and non-maximal orders") (newline)
(display "reported honestly.  The van-Hoeij correction at repeated places, and the places over infinity, remain ahead.") (newline)
