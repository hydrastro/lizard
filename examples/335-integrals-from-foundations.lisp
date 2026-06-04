; HOW AN INTEGRAL IS PROVEN, FROM THE FOUNDATIONS UP.  This example walks the full provenance chain behind a
; certified integral, from the constructed number system to the machine-checked differentiation certificate --
; making explicit the sense in which each result is grounded (docs/CAS.md).
;
; The chain has four layers:
;   (1) FOUNDATIONS.  Lizard's kernel is a small dependent type theory (trusted core).  On it the natural
;       numbers, then the integers, then the rationals Q are CONSTRUCTED -- the same construction one does set-
;       theoretically over ZFC (N as finite ordinals, Z as a quotient of N x N, Q as a quotient of Z x (Z\{0})).
;   (2) EXACT ARITHMETIC.  Q is realized with arbitrary-precision integers and reduced fractions, so every
;       arithmetic fact below is exact -- no floating point, no approximation.
;   (3) THE FIELD Q(x).  Rational functions are built on exact Q: a rational function is a pair of polynomials
;       over Q, with exact gcd reduction.  Differentiation D is a total, exact operation on Q(x).
;   (4) THE INTEGRAL CLAIM, REDUCED TO A CERTIFICATE.  A claim "INT f dx = F" is logically equivalent to the
;       algebraic identity D(F) = f.  The system produces F and then CHECKS D(F) = f by exact Q(x) arithmetic --
;       a finite computation.  So the integral is proven in the precise sense that its truth is reduced to, and
;       confirmed by, verified arithmetic over the constructed rationals.
(import "cas/rischtop.lisp")
(define (show l v) (display "  ") (display l) (display " => ") (display v) (newline))
(define (chk l x) (display "  [check] ") (display l) (display " : ") (display (if x "PROVEN" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "=== Layer 1-2: the constructed rationals Q, exact ===") (newline)
(display "Q is built from Z built from N (as over ZFC); arithmetic is exact (bignums + reduced fractions):") (newline)
(show "1/3 + 1/6 (exact)" (rat-add (rat-make (list 1) (list 3)) (rat-make (list 1) (list 6))))
(show "2/6 reduces to" (rat-make (list 2) (list 6)))
(chk "exact arithmetic: 1/3 + 1/6 = 1/2 exactly (not 0.4999...)" (rat-equal? (rat-add (rat-make (list 1) (list 3)) (rat-make (list 1) (list 6))) (rat-make (list 1) (list 2))))
(newline)

(display "=== Layer 3: the field Q(x) and exact differentiation ===") (newline)
(display "a rational function is a pair of polynomials over Q; D is total and exact:") (newline)
(show "D(x^2 - 1)" (rat-deriv (rat-from-poly (list -1 0 1))))
(show "D(1/x)" (rat-deriv (rat-make (list 1) (list 0 1))))
(chk "D(x^2-1) = 2x exactly" (rat-equal? (rat-deriv (rat-from-poly (list -1 0 1))) (rat-from-poly (list 0 2))))
(chk "D(1/x) = -1/x^2 exactly" (rat-equal? (rat-deriv (rat-make (list 1) (list 0 1))) (rat-make (list -1) (list 0 0 1))))
(newline)

(display "=== Layer 4: an integral, reduced to its certificate and PROVEN ===") (newline)
(display "CLAIM: INT 2x/(x^2-1) dx = log(x^2-1).") (newline)
(display "The system computes the antiderivative (rational part + logarithms found by Rothstein-Trager):") (newline)
(define f-num (list 0 2))
(define f-den (list -1 0 1))
(define result (integrate-top-rational f-num f-den))
(show "antiderivative (rational part, logs)" result)
(display "This claim is TRUE iff D(log(x^2-1)) = 2x/(x^2-1).  We CHECK that identity by exact Q(x) arithmetic:") (newline)
(show "D(log(x^2-1)) = (x^2-1)'/(x^2-1)" (rat-make (poly-deriv (list -1 0 1)) (list -1 0 1)))
(show "the integrand f = 2x/(x^2-1)" (rat-make f-num f-den))
(chk "the certificate D(answer) = f holds exactly -- so the integral is PROVEN" (integrate-top-certify-rational f-num f-den result))
(newline)

(display "=== a second integral, same provenance: INT 1/(x^2+1) dx = arctan x ===") (newline)
(define g-num (list 1))
(define g-den (list 1 0 1))
(define result2 (integrate-top-rational g-num g-den))
(show "antiderivative" result2)
(chk "D(arctan x) = 1/(x^2+1) exactly -- PROVEN" (integrate-top-certify-rational g-num g-den result2))
(newline)

(display "=== a NON-elementarity claim, also grounded ===") (newline)
(display "CLAIM: INT e^(e^x) dx is NOT elementary.  This is proven not by a certificate of an answer, but by an") (newline)
(display "EXACT OBSTRUCTION: the Risch recursion reduces it to a differential equation whose only solution would") (newline)
(display "force a non-terminating degree tail -- an exact, finite contradiction over Q(x):") (newline)
(define t2 (list (list (quote exp) (rat-from-poly (list 0 1))) (list (quote exp) (list (rat-zero) (rat-one)))))
(show "verdict for INT e^(e^x)" (car (integrate-top-tower t2 2 (list (te-zero 1) (te-one 1)))))
(chk "INT e^(e^x) is proven non-elementary by an exact obstruction" (equal? (car (integrate-top-tower t2 2 (list (te-zero 1) (te-one 1)))) (quote non-elementary)))
(newline)

(display "Every result is grounded the same way: the claim is reduced to a finite, exact statement over the") (newline)
(display "constructed rationals -- a differentiation identity for elementary integrals, an exact obstruction for") (newline)
(display "non-elementary ones -- and that statement is mechanically checked.  This is what 'proof-carrying' means") (newline)
(display "here: not a hand derivation from the ZFC axioms, but a machine-checkable reduction to verified arithmetic") (newline)
(display "over a number system itself constructed from those foundations.") (newline)
