; The MULTI-BRANCH combined-correction integral element on y^2 = f: building and certifying an integral-basis
; element w = (A + B y)/d that is integral at a place where SEVERAL branches of the curve meet at once -- the case
; the single-branch van Hoeij correction defers as 'needs-place-combination (docs/CAS.md -- general integral
; closure: the combined correction across branches).
;
; At the node y^2 = x^2(x+1) the two branches y = +-(x + x^2/2 - ...) meet at the origin; no single-branch
; correction is integral at both.  The combined element is found by the exact integral-closure test in
; K = Q(x)[y]/(y^2-f): w = (A + B y)/d is integral over Q[x] iff its minimal polynomial w^2 - (2A/d)w + (A^2-B^2f)/d^2
; is monic with POLYNOMIAL coefficients -- the trace and norm both divide out.  The norm sees all branches at once,
; so this certifies integrality everywhere simultaneously.
(import "cas/vanhoeijmb.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Multi-branch integral element: w = (A + B y)/d integral at a place where several branches meet.") (newline) (newline)

(display "the node y^2 = x^2(x+1) = x^3 + x^2 has two branches y = +-(x + ...) meeting at the origin:") (newline)
(define fn (list 0 0 1 1))
(display "  w = y/x has trace 2A/d = ") (display (car (mbc-trace fn (list 0) (list 1) (list 0 1)))) (display " and norm (A^2-B^2 f)/d^2 = ") (display (car (mbc-norm fn (list 0) (list 1) (list 0 1)))) (newline)
(must "y/x is integral at the node (both trace and norm are polynomials)" (mbc-is-integral? fn (list 0) (list 1) (list 0 1)))
(must "its minimal polynomial is w^2 - (x+1) = 0, monic over Q[x]" (equal? (mbc-minpoly fn (list 0) (list 1) (list 0 1)) (list (quote monic) (list) (list -1 -1))))
(must "the full certificate holds (trace and norm reproduce 2A and A^2-B^2 f)" (mbc-certify fn (list 0) (list 1) (list 0 1)))
(must "so y/x is the combined-branch basis element the single-branch construction could not produce" (equal? (car (mbc-element fn (list 0) (list 1) (list 0 1))) (quote integral)))

(display "two nodes at once: y^2 = x^2 (x-1)^2 (x+1) -- the element y/(x(x-1)) is integral at both:") (newline)
(define fn2 (poly-mul (poly-mul (list 0 1) (list 0 1)) (poly-mul (poly-mul (list -1 1) (list -1 1)) (list 1 1))))
(define dd (poly-mul (list 0 1) (list -1 1)))
(must "y/(x(x-1)) is integral" (mbc-is-integral? fn2 (list 0) (list 1) dd))
(must "and certifies" (mbc-certify fn2 (list 0) (list 1) dd))

(display "soundness: candidates that are NOT integral are rejected, never forced:") (newline)
(must "y/x^2 is rejected (norm -(x+1)/x^2 is not a polynomial)" (if (mbc-is-integral? fn (list 0) (list 1) (list 0 0 1)) #f #t))
(must "on the smooth cubic y^2 = x(x-1)(x-2), y is integral" (mbc-is-integral? (list 0 2 -3 1) (list 0) (list 1) (list 1)))
(must "but y/(x-1) is rejected there (x=1 is a smooth branch point, no combined element)" (if (mbc-is-integral? (list 0 2 -3 1) (list 0) (list 1) (list -1 1)) #f #t))

(newline)
(display "The combined correction across several branches at a place is now constructed and certified for the") (newline)
(display "quadratic case, by the monic-minimal-polynomial integral-closure test -- the case the single-branch van") (newline)
(display "Hoeij correction deferred.  The general-degree combined correction (n branches over a degree-n field)") (newline)
(display "remains ahead, with the soundness boundary explicit.") (newline)
