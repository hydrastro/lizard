; EXACT RATIONAL SOLUTION TUPLES for a zero-dimensional polynomial system, by triangular back-substitution on the
; lexicographic Groebner basis (docs/CAS.md -- summit S3: full coordinate back-substitution).  Where polyroots
; counted and isolated the real values of one coordinate, this recovers the complete solution POINTS whose
; coordinates are rational, exactly, and reports honestly ('irrational-fiber) when a coordinate is not rational so
; that nothing is invented or silently dropped.
;
; The lex basis is triangular (the elimination property): one element is univariate in the leading variable; its
; rational roots are found by the rational-root theorem (p/q with p | constant, q | leading -- exact and finite);
; each value is substituted into the system, reducing the variable count, and the recursion assembles the tuples.
(import "cas/polysolve2.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Solving polynomial systems for exact rational solution tuples by triangular back-substitution.") (newline) (newline)

(display "the rational-root theorem, exact: roots of 2x^2 - 3x + 1 = (2x-1)(x-1) are 1 and 1/2:") (newline)
(chk "rational roots of 2x^2-3x+1 are {1, 1/2}" (if (ps2-memq (/ 1 2) (ps2-rational-roots (list 1 -3 2))) (ps2-memq 1 (ps2-rational-roots (list 1 -3 2))) #f))
(chk "x^2 - 2 has no rational root (sqrt2 is irrational)" (null? (ps2-rational-roots (list -2 0 1))))

(display "a triangular system: x^2 - 3x + 2 = 0, y = x  ->  the points (1,1) and (2,2):") (newline)
(define f1 (list (cons 1 (list 2 0)) (cons -3 (list 1 0)) (cons 2 (list 0 0))))
(define f2 (list (cons 1 (list 0 1)) (cons -1 (list 1 0))))
(define S1 (list f1 f2))
(display "  ") (display (ps2-rational-solutions S1 2)) (newline)
(chk "exactly two solution tuples" (= (ps2-len (ps2-rational-solutions S1 2)) 2))
(chk "(1,1) makes every generator vanish" (ps2-verify S1 2 (list 1 1)))
(chk "(2,2) makes every generator vanish" (ps2-verify S1 2 (list 2 2)))

(display "a product system: x^2 = 1, y^2 = 4  ->  the four points (+-1, +-2):") (newline)
(define g1 (list (cons 1 (list 2 0)) (cons -1 (list 0 0))))
(define g2 (list (cons 1 (list 0 2)) (cons -4 (list 0 0))))
(display "  ") (display (ps2-rational-solutions (list g1 g2) 2)) (newline)
(chk "all four points recovered" (= (ps2-len (ps2-rational-solutions (list g1 g2) 2)) 4))

(display "a three-variable branching system: x^2 = 1, y = x, z = x*y  ->  (1,1,1) and (-1,-1,1):") (newline)
(define b1 (list (cons 1 (list 2 0 0)) (cons -1 (list 0 0 0))))
(define b2 (list (cons 1 (list 0 1 0)) (cons -1 (list 1 0 0))))
(define b3 (list (cons 1 (list 0 0 1)) (cons -1 (list 1 1 0))))
(display "  ") (display (ps2-rational-solutions (list b1 b2 b3) 3)) (newline)
(chk "(1,1,1) verifies" (ps2-verify (list b1 b2 b3) 3 (list 1 1 1)))
(chk "(-1,-1,1) verifies" (ps2-verify (list b1 b2 b3) 3 (list -1 -1 1)))

(display "soundness: x^2 = 2, y = x has only irrational solutions -- reported, not fabricated:") (newline)
(define h1 (list (cons 1 (list 2 0)) (cons -2 (list 0 0))))
(define h2 (list (cons 1 (list 0 1)) (cons -1 (list 1 0))))
(chk "no rational solutions returned for <x^2-2, y-x>" (null? (ps2-rational-solutions (list h1 h2) 2)))

(newline)
(display "The solver now recovers complete solution TUPLES, not just per-coordinate values: exact rational points") (newline)
(display "by back-substitution on the triangular lex basis, each certified to make every generator vanish, with an") (newline)
(display "explicit 'irrational-fiber report when a coordinate leaves Q.  Naming irrational coordinates exactly (via") (newline)
(display "algebraic numbers) and complex solutions are the remaining steps toward the full variety.") (newline)
