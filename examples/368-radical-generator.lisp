; The explicit RADICAL GENERATOR of a univariate principal ideal, with its squarefree (primary-like) decomposition
; and a constructive radical-membership test -- the CONSTRUCTIVE complement to radideal, which decided membership
; but produced no generator (docs/CAS.md -- summit S4, radical generators).
;
; For <f> over Q[x] the radical is again principal: sqrt(<f>) = <rad(f)> with rad(f) = f / gcd(f, f'), the product
; of the distinct factors.  Yun's decomposition f = prod g_k^k exposes the primary-like structure, and radical
; membership is the divisibility test h in sqrt(<f>) iff rad(f) | h -- exact and constructive.
(import "cas/radgen.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The radical generator of a principal ideal, its squarefree decomposition, and constructive membership.") (newline) (newline)

(define f (list -2 5 -4 1))   ; (x-1)^2 (x-2)

(display "for f = (x-1)^2 (x-2), the radical generator is (x-1)(x-2) = x^2 - 3x + 2:") (newline)
(display "  rad(f) = ") (display (rg-radical f)) (newline)
(chk "the radical generator is x^2 - 3x + 2" (equal? (rg-radical f) (list 2 -3 1)))

(display "Yun's squarefree decomposition exposes the multiplicities, and reconstructs f:") (newline)
(display "  decomposition (multiplicity . factor): ") (display (rg-decomposition f)) (newline)
(chk "the decomposition reconstructs f exactly" (rg-decomposition-ok? f))

(display "whether the ideal is already radical:") (newline)
(chk "(x-1)(x-2) is squarefree, so <(x-1)(x-2)> is a radical ideal" (rg-is-radical-ideal? (list 2 -3 1)))
(chk "but <(x-1)^2(x-2)> is NOT radical" (if (rg-is-radical-ideal? f) #f #t))

(display "constructive radical membership: h is in sqrt(<f>) exactly when rad(f) divides h:") (newline)
(chk "(x-1)(x-2) is in sqrt(<f>)" (rg-in-radical? f (list 2 -3 1)))
(chk "(x-1)(x-2)(x-5) is in sqrt(<f>)" (rg-in-radical? f (poly-mul (list 2 -3 1) (list -5 1))))
(chk "(x-1) alone is NOT in sqrt(<f>)" (if (rg-in-radical? f (list -1 1)) #f #t))
(chk "(x-3) is NOT in sqrt(<f>)" (if (rg-in-radical? f (list -3 1)) #f #t))

(display "a triple root: rad((x-1)^3) = (x-1):") (newline)
(define f2 (poly-mul (list -1 1) (poly-mul (list -1 1) (list -1 1))))
(chk "the radical of (x-1)^3 is (x-1)" (equal? (rg-radical f2) (list -1 1)))
(chk "and its decomposition reconstructs (x-1)^3" (rg-decomposition-ok? f2))

(newline)
(display "The radical of a principal ideal is now produced as an explicit generator, with the squarefree") (newline)
(display "decomposition certified by reconstruction and membership decided constructively by divisibility -- the") (newline)
(display "generator that the general Nullstellensatz test could not give.  Full primary decomposition is still ahead.") (newline)
