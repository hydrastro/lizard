; 204-finite-field-factorization.lisp -- factoring polynomials over F_p (Cantor-Zassenhaus).
;
; Over a prime field F_p a polynomial factors uniquely into monic irreducibles.  This is
; computed by squarefree decomposition (with a p-th-root step in characteristic p),
; distinct-degree factorisation via the Frobenius map x -> x^p, and equal-degree splitting
; (the trace map for p = 2, the (p^d-1)/2 power for odd p).  The result is gated two
; independent ways: the product of the prime powers must reconstruct the monic input mod p,
; and every factor must pass a standalone irreducibility test.  `must` raises on failure.

(import "cas/ffactor.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'ffactor-check-failed)))
(define (nfac f p) (length (factor-mod f p)))

(display "Polynomial factorization over F_p") (newline) (newline)

(display "1. distinct factors and irreducibles") (newline)
(display "    x^2 - 1 over F_7  = ") (display (factor->string (factor-mod (list 6 0 1) 7))) (newline)
(display "    x^3 - x over F_7  = ") (display (factor->string (factor-mod (list 0 6 0 1) 7))) (newline)
(display "    x^4 + 5 over F_7  = ") (display (factor->string (factor-mod (list 5 0 0 0 1) 7))) (newline)
(must "x^2-1 over F_7 has two factors"     (= (nfac (list 6 0 1) 7) 2))
(must "x^3-x over F_7 has three factors"   (= (nfac (list 0 6 0 1) 7) 3))
(must "x^2+1 over F_7 is irreducible"      (irreducible? (list 1 0 1) 7))
(must "x^2+1 over F_5 is reducible"        (not (irreducible? (list 1 0 1) 5)))
(must "x^4+x^2+2 over F_5 is irreducible"  (irreducible? (list 2 0 1 0 1) 5))
(newline)

(display "2. repeated factors, including the characteristic-p p-th-root branch") (newline)
(display "    x^2 + 2x + 1 over F_3 = ") (display (factor->string (factor-mod (list 1 2 1) 3))) (newline)
(display "    x^3 + 1 over F_3     = ") (display (factor->string (factor-mod (list 1 0 0 1) 3))) (newline)
(display "    x^4 + 1 over F_2     = ") (display (factor->string (factor-mod (list 1 0 0 0 1) 2))) (newline)
(must "(x+1)^2 over F_3 is one factor with multiplicity 2" (and (= (nfac (list 1 2 1) 3) 1) (= (cdr (car (factor-mod (list 1 2 1) 3))) 2)))
(must "x^3+1 = (x+1)^3 over F_3 (p-th root branch)"        (and (= (nfac (list 1 0 0 1) 3) 1) (= (cdr (car (factor-mod (list 1 0 0 1) 3))) 3)))
(must "x^4+1 = (x+1)^4 over F_2"                           (= (cdr (car (factor-mod (list 1 0 0 0 1) 2))) 4))
(newline)

(display "3. the reconstruction + irreducibility certificate holds throughout") (newline)
(must "x^2-1 over F_7 certified"        (factor-mod-ok? (list 6 0 1) 7))
(must "x^3-x over F_7 certified"        (factor-mod-ok? (list 0 6 0 1) 7))
(must "x^4+5 over F_7 certified"        (factor-mod-ok? (list 5 0 0 0 1) 7))
(must "x^3+1 over F_3 certified"        (factor-mod-ok? (list 1 0 0 1) 3))
(must "x^4+1 over F_2 certified"        (factor-mod-ok? (list 1 0 0 0 1) 2))
(must "x^5+x^4+x+1 over F_2 certified"  (factor-mod-ok? (list 1 1 0 0 1 1) 2))
(must "x^6+4 over F_7 certified"        (factor-mod-ok? (list 4 0 0 0 0 0 1) 7))
(must "x^4+x^2+2 over F_5 certified"    (factor-mod-ok? (list 2 0 1 0 1) 5))
(newline)

(display "all finite-field factorization checks passed.") (newline)
