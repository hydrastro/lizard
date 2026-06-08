; RECURSIVE Newton-Puiseux -- separating branches that share a leading term, completing the local-branch
; analysis for curves with tangent multiplicity (Rung 4 of the Trager-Bronstein climb, docs/TRAGER_ROADMAP.md).
;
; The Newton polygon (newton.lisp) gives each branch's leading exponent and coefficient, and puiseuxg.lisp
; expands a branch when its edge-polynomial root is SIMPLE.  When a root c has multiplicity m > 1, the m
; branches all begin c*x^mu and separate only at higher order -- the simple-root solver cannot tell them apart.
; The classical resolution is to SUBSTITUTE y = (c + y1) * x^mu, obtaining a new equation G(x, y1) = 0; after
; dividing out the common x-power, G's own Newton polygon resolves the next term, recursing until the roots are
; simple (the branches have separated).  When the constant-in-y coefficient vanishes, y is an exact factor and
; y = 0 is itself a (terminating) branch, peeled before recursing on F / y.
;
; Each branch is returned as a sequence of (mu . c) leading-term pairs, y = c0 x^{mu0} + c1 x^{mu0+mu1} + ...,
; which is exactly the data the integral-basis correction terms are built from.
(import "cas/puiseuxr.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))
(define (memb x l) (if (null? l) #f (if (equal? x (car l)) #t (memb x (cdr l)))))
(define (subset? a b) (if (null? a) #t (if (memb (car a) b) (subset? (cdr a) b) #f)))
(define (same-set? a b) (if (= (length a) (length b)) (subset? a b) #f))

(display "Recursive Newton-Puiseux: separating branches with a shared leading term, by substitute-and-recurse.") (newline) (newline)

(display "distinct simple tangents need no recursion -- the node F = y^2 - x^2 - x^3 (branches y ~ +-x):") (newline)
(define Fn (list (list 0 0 -1 -1) (list) (list 1)))
(display "  branch leads = ") (display (pr-branch-leads Fn 2)) (newline)
(chk "node separates into y ~ x and y ~ -x" (same-set? (pr-branch-leads Fn 2) (list (list (cons 1 1)) (list (cons 1 -1)))))

(display "three simple tangents, F = (y-x)(y-2x)(y-3x):") (newline)
(define F3 (list (list 0 0 0 -6) (list 0 0 11) (list 0 -6) (list 1)))
(display "  branch leads = ") (display (pr-branch-leads F3 2)) (newline)
(chk "three branches y ~ x, 2x, 3x" (same-set? (pr-branch-leads F3 2) (list (list (cons 1 1)) (list (cons 1 2)) (list (cons 1 3)))))

(display "a SHARED leading term that needs the recursion, F = (y - x^2)(y - x^2 - x^3):") (newline)
(define F (list (list 0 0 0 0 1 1) (list 0 0 -2 -1) (list 1)))
(display "  the edge polynomial is (c-1)^2 -- a double root c=1 at slope mu=2, branches both start x^2") (newline)
(display "  substitute y = (1+y1)x^2, deflate -> ") (display (pr-deflate (pr-subst F 1 2))) (display "  (= y1^2 - x y1, simple roots 0 and x)") (newline)
(chk "the deflated equation is y1^2 - x y1" (equal? (pr-deflate (pr-subst F 1 2)) (list (quote ()) (list 0 -1) (list 1))))
(display "  branch leads = ") (display (pr-branch-leads F 3)) (newline)
(chk "separates into y = x^2 and y = x^2 + x^3 (second term +x in the x^2-scaled coordinate)"
     (same-set? (pr-branch-leads F 3) (list (list (cons 2 1)) (list (cons 2 1) (cons 1 1)))))

(display "the edge-polynomial root multiplicity is read directly:") (newline)
(chk "roots of (c-1)^2 = (1 -2 1): c = 1 with multiplicity 2" (equal? (pr-roots-mult (list 1 -2 1) 4) (list (cons 1 2))))

(newline)
(display "Recursive Newton-Puiseux: branches sharing a tangent are separated by substitution and recursion,") (newline)
(display "giving each branch's distinct leading-term sequence -- the input to the integral-basis corrections.") (newline)
