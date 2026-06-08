; 214-polynomial-crt.lisp -- the Chinese Remainder Theorem over F_p[x].
;
; For pairwise-coprime polynomial moduli and target residues there is a unique polynomial,
; of degree below the product of the moduli, that reduces to each residue.  It is built from
; the cofactors M_i = M/m_i and their inverses modulo m_i (extended Euclid over F_p).
; Certified by reducing the reconstruction modulo each modulus and by the degree bound; and
; since reduction modulo x - c is evaluation at c, CRT with linear moduli reproduces
; ordinary interpolation.  `must` raises on failure.

(import "cas/polycrt.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'polycrt-check-failed)))
(define p 5)

(display "Polynomial Chinese Remainder Theorem over F_p[x]") (newline) (newline)

(display "1. two coprime moduli over F_5: (x^2+2) and (x+1)") (newline)
(define f (poly-crt (list (list 1 1) (list 2)) (list (list 2 0 1) (list 1 1)) p))
(display "    residues (x+1) and 2 reconstruct to f = ") (display (poly->string f)) (newline)
(display "    f mod (x^2+2) = ") (display (poly->string (pmod f (list 2 0 1) p))) (display ",  f mod (x+1) = ") (display (poly->string (pmod f (list 1 1) p))) (newline)
(must "the moduli are coprime"          (= (pdeg (pgcd (list 2 0 1) (list 1 1) p)) 0))
(must "reconstruction matches every residue and bounds the degree" (poly-crt-ok? (list (list 1 1) (list 2)) (list (list 2 0 1) (list 1 1)) p))
(newline)

(display "2. three coprime moduli over F_5: (x^2+x+1), (x+2), (x)") (newline)
(display "    residues (x+1), 4, 2 reconstruct to ") (display (crt->string (list (list 1 1) (list 4) (list 2)) (list (list 1 1 1) (list 2 1) (list 0 1)) p)) (newline)
(must "three-modulus reconstruction is correct" (poly-crt-ok? (list (list 1 1) (list 4) (list 2)) (list (list 1 1 1) (list 2 1) (list 0 1)) p))
(must "a two-quadratic system over F_7 reconstructs" (poly-crt-ok? (list (list 1 2) (list 5 1)) (list (list 1 0 1) (list 3 1 1)) 7))
(newline)

(display "3. linear moduli recover polynomial interpolation") (newline)
(display "    through (0,1)(1,2)(2,3) over F_5: ") (display (crt->string (list (list 1) (list 2) (list 3)) (linmods (list 0 1 2) p) p)) (newline)
(display "    through x^2 sampled at 0..4: ") (display (crt->string (map list (list 0 1 4 4 1)) (linmods (list 0 1 2 3 4) p) p)) (newline)
(must "CRT interpolation agrees with the values over F_5" (interp-ok? (list 0 1 2 3 4) (list 2 3 0 1 4) p))
(must "CRT interpolation agrees with the values over F_7" (interp-ok? (list 1 2 3 4 5 6) (list 1 4 2 0 4 1) 7))
(newline)

(display "all polynomial-CRT checks passed.") (newline)
