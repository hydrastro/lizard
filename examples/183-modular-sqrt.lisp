; 183-modular-sqrt.lisp -- modular square roots and quadratic residues.
;
; The Legendre symbol (a/p) comes from Euler's criterion, the Jacobi symbol (a/n) from the
; reciprocity recursion, and Tonelli-Shanks extracts a square root r with r^2 = a (mod p)
; whenever a is a residue.  Each square root is checked by squaring it back; the Legendre
; symbol is cross-checked against a direct count of roots for small primes; the Jacobi
; symbol against its multiplicative behaviour.  `must` raises on failure.

(import "cas/modsqrt.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'modsqrt-check-failed)))

(display "Modular square roots and quadratic residues") (newline) (newline)

(display "1. square roots, verified by squaring back") (newline)
(display "    sqrt(2) mod 7   = ") (display (sqrt-mod->string 2 7))   (newline)
(display "    sqrt(10) mod 13 = ") (display (sqrt-mod->string 10 13)) (newline)
(display "    sqrt(2) mod 113 = ") (display (sqrt-mod->string 2 113)) (newline)
(must "sqrt(2) mod 7 squares to 2"     (= (imod (* (sqrt-mod 2 7) (sqrt-mod 2 7)) 7) 2))
(must "sqrt(10) mod 13 squares to 10"  (= (imod (* (sqrt-mod 10 13) (sqrt-mod 10 13)) 13) 10))
(must "sqrt(2) mod 113 certified"      (sqrt-mod-ok? 2 113))
(must "3 is a non-residue mod 7"       (equal? (sqrt-mod 3 7) 'none))
(must "non-residue case certified"     (sqrt-mod-ok? 3 7))
(newline)

(display "2. a large prime p = 1 mod 4 (Tonelli-Shanks)") (newline)
(define p 1000033)
(define a (imod (* 314159 314159) p))
(define r (sqrt-mod a p))
(display "    a = 314159^2 mod p; recovered root r with r^2 = a: ") (display (= (imod (* r r) p) a)) (newline)
(must "Tonelli-Shanks root verified"   (= (imod (* r r) p) a))
(must "and certified"                  (sqrt-mod-ok? a p))
(newline)

(display "3. Legendre symbol vs direct root count (small primes)") (newline)
(must "(2/7) consistent"               (legendre-bruteforce-ok? 2 7))
(must "(3/7) consistent"               (legendre-bruteforce-ok? 3 7))
(must "(5/11) consistent"              (legendre-bruteforce-ok? 5 11))
(must "(6/11) consistent"              (legendre-bruteforce-ok? 6 11))
(must "(0/13) consistent"              (legendre-bruteforce-ok? 0 13))
(newline)

(display "4. Jacobi symbol") (newline)
(display "    (1001/9907) = ") (display (jacobi 1001 9907)) (newline)
(must "(1001/9907) = -1"               (= (jacobi 1001 9907) -1))
(must "Jacobi matches Legendre at a prime" (= (jacobi 7 13) (legendre 7 13)))
(must "multiplicative: (6/11)=(2/11)(3/11)" (= (jacobi 6 11) (* (jacobi 2 11) (jacobi 3 11))))
(newline)

(display "all modular-sqrt checks passed.") (newline)
