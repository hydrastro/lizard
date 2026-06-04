; The RAMIFIED-PLACE integral element on y^2 = f: building and certifying the local integral-closure generator at a
; place where the cover ramifies (a root of f of multiplicity >= 2, where the branch is a Puiseux series in a
; fractional power) -- the case intbasis.lisp and vanhoeij.lisp defer, harder than the multi-branch node because the
; local parameter is fractional (docs/CAS.md -- general integral closure: the ramified places).
;
; At x = a with (x-a)^m || f, the valuations at the place are v(x-a) = 2 and v(y) = m, so for m >= 2 the element
; w = y/(x-a)^{floor(m/2)} is integral (valuation m mod 2 >= 0) and generates the local integral closure.  Its
; integrality is certified by the monic minimal polynomial: w^2 = f/(x-a)^{2 floor(m/2)} is a polynomial.
(import "cas/ramplace.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Ramified-place integral element: w = y/(x-a)^floor(m/2) at a root a of multiplicity m of f.") (newline) (newline)

(display "the cusp y^2 = x^3 (multiplicity 3 at the origin -- the branch is y = x^{3/2}):") (newline)
(define fc (list 0 0 0 1))
(must "the multiplicity of x in x^3 is 3" (= (ram-mult fc 0) 3))
(must "the valuations are v(x) = 2 and v(y) = 3" (if (= (ram-val-x fc 0) 2) (= (ram-val-y fc 0) 3) #f))
(must "the place is ramified (m is odd)" (ram-is-ramified? fc 0))
(display "  the integral element is w = y/x^1, and w^2 = ") (display (ram-element-sq fc 0)) (display " (= x):") (newline)
(must "the element is y/x (k = 1)" (equal? (ram-element fc 0) (list (quote elt) 1)))
(must "its square is x, so w satisfies the monic w^2 - x = 0" (equal? (ram-element-sq fc 0) (list 0 1)))
(must "the integrality certificate holds" (ram-certify fc 0))
(must "the place is classified ramified" (equal? (ram-place-type fc 0) (quote ramified)))

(display "a higher cusp y^2 = x^5 (multiplicity 5): the element is y/x^2, w^2 = x:") (newline)
(define f5 (list 0 0 0 0 0 1))
(must "the element is y/x^2 (k = 2)" (equal? (ram-element f5 0) (list (quote elt) 2)))
(must "w^2 = x and it certifies" (if (equal? (ram-element-sq f5 0) (list 0 1)) (ram-certify f5 0) #f))

(display "a ramified place away from the origin: y^2 = (x-1)^3 at x = 1, element y/(x-1), w^2 = x-1:") (newline)
(define f31 (poly-mul (poly-mul (list -1 1) (list -1 1)) (list -1 1)))
(must "w^2 = x-1 and it certifies" (if (equal? (ram-element-sq f31 1) (list -1 1)) (ram-certify f31 1) #f))

(display "an even multiplicity y^2 = x^2 SPLITS the place (m even): w = y/x is a unit, w^2 = 1:") (newline)
(define f2 (list 0 0 1))
(must "the place is split, not ramified" (if (ram-is-ramified? f2 0) #f #t))
(must "it is classified split" (equal? (ram-place-type f2 0) (quote split)))
(must "w^2 = 1 (a unit) and it certifies" (if (equal? (ram-element-sq f2 0) (list 1)) (ram-certify f2 0) #f))

(display "soundness: a point that is not a root of f is unramified-regular with no new element:") (newline)
(must "x = 1 on y^2 = x^3 has multiplicity 0" (= (ram-mult fc 1) 0))
(must "it is unramified-regular" (equal? (ram-place-type fc 1) (quote unramified-regular)))
(must "and yields no new integral element" (equal? (ram-element fc 1) (quote no-new-element)))

(newline)
(display "The local integral closure at a ramified place of y^2 = f is now constructed and certified, with the exact") (newline)
(display "valuations v(x) = 2, v(y) = m and the place classified ramified or split.  The general-degree ramification") (newline)
(display "(a Puiseux cycle of length e | n in a degree-n cover) remains ahead.") (newline)
