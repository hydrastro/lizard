; The COMPLETE third-kind solver: given an integrand omega on the curve y^2 = q(x), DIRECTLY solve for
; g = u(x) + sqrt(q) with omega = g'/g (u a polynomial of ANY degree), no brute-force search, no degree bound --
; closing frontier (a), unbounded third-kind g (docs/CAS.md).
;
; The matching equations are linear: with omega = a + b*y in K = Q(x)[y]/(y^2-q), the y-component of omega = g'/g
; gives u DIRECTLY as u = (q'/(2q) - a)/b.  Accept iff u is a genuine polynomial and the differentiation
; certificate confirms d/dx log(u + sqrt q) = omega.  This replaces the earlier bounded search with a closed-form
; solve that handles u of arbitrary degree, while staying sound (every hit certified, honest no-solution else).
(import "cas/elliptic3complete.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Complete third-kind solver: recover g = u + sqrt(q) of ANY degree directly from omega, certified.") (newline) (newline)

(display "the basic case, solved directly (no search): omega = d/dx log(x + sqrt(x^3+1)):") (newline)
(define q3 (rat-from-poly (list 1 0 0 1)))
(define om3 (e3-logderiv q3 (af-make (rat-from-poly (list 0 1)) (rat-one))))
(define r3 (e3c-solve q3 om3))
(chk "g = x + sqrt(x^3+1) found directly and certified" (if (equal? (car r3) (quote found)) (e3-certify q3 (car (cdr r3)) om3) #f))

(display "a HIGHER-degree u, beyond a small bounded search: g = (x^2 - 1) + sqrt(x^5+1):") (newline)
(define q5 (rat-from-poly (list 1 0 0 0 0 1)))
(define ghi (af-make (rat-from-poly (list -1 0 1)) (rat-one)))
(define omhi (e3-logderiv q5 ghi))
(chk "g = (x^2 - 1) + sqrt(x^5+1) recovered directly, certified" (if (equal? (car (e3c-solve q5 omhi)) (quote found)) (e3-certify q5 (car (cdr (e3c-solve q5 omhi))) omhi) #f))

(display "and a degree-3 u over a genus-3 curve: g = (x^3 + 2x) + sqrt(x^7+1):") (newline)
(define q7 (rat-from-poly (list 1 0 0 0 0 0 0 1)))
(define g7 (af-make (rat-from-poly (list 0 2 0 1)) (rat-one)))
(define om7 (e3-logderiv q7 g7))
(chk "g = (x^3 + 2x) + sqrt(x^7+1) found directly, certified" (if (equal? (car (e3c-solve q7 om7)) (quote found)) (e3-certify q7 (car (cdr (e3c-solve q7 om7))) om7) #f))

(display "soundness: the first-kind 1/sqrt(x^3+1) yields a non-polynomial u -> honest no-solution:") (newline)
(chk "1/sqrt(x^3+1) returns no-solution, not a false verdict" (equal? (car (e3c-solve q3 (af-make (rat-zero) (rat-make (list 1) (list 1 0 0 1))))) (quote no-solution)))

(newline)
(display "The third-kind solver is now COMPLETE for the g = u + sqrt(q) shape: a direct closed-form solve for u of") (newline)
(display "arbitrary degree, certified by differentiation, with honest no-solution when no such g exists.  The fully") (newline)
(display "general third kind (g = A + B sqrt q with B a nonconstant rational, and pole/residue analysis) remains.") (newline)
