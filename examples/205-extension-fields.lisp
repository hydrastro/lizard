; 205-extension-fields.lisp -- arithmetic in the finite field GF(p^n).
;
; GF(p^n) is F_p[x]/(m) for a monic irreducible m of degree n, found with the
; irreducibility test from the factoriser.  Elements are polynomials of degree < n;
; addition is coefficientwise, multiplication reduces mod m, and inverses use Fermat
; (a^(p^n - 2)).  Four facts certify it: the modulus is irreducible, every nonzero element
; is invertible, the Frobenius identity a^(p^n) = a holds, and a primitive element's powers
; enumerate all p^n - 1 nonzero elements.  `must` raises on failure.

(import "cas/gfp.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'gfp-check-failed)))
(define (field-ok? p n) (and (gf-field-ok? p n) (gf-inverses-ok? p n) (gf-frobenius-ok? p n) (gf-primitive-generates? p n)))

(display "The finite fields GF(p^n)") (newline) (newline)

(display "1. construction: smallest monic irreducible modulus") (newline)
(display "    GF(8)  = F_2[x] / ") (display (gf-modulus->string 2 3)) (newline)
(display "    GF(16) = F_2[x] / ") (display (gf-modulus->string 2 4)) (newline)
(display "    GF(9)  = F_3[x] / ") (display (gf-modulus->string 3 2)) (newline)
(display "    GF(27) = F_3[x] / ") (display (gf-modulus->string 3 3)) (newline)
(must "GF(8) modulus is irreducible"  (gf-field-ok? 2 3))
(must "GF(16) modulus is irreducible" (gf-field-ok? 2 4))
(must "GF(9) modulus is irreducible"  (gf-field-ok? 3 2))
(must "GF(25) modulus is irreducible" (gf-field-ok? 5 2))
(newline)

(display "2. every nonzero element is invertible (field axiom)") (newline)
(must "GF(8): all nonzero invertible"  (gf-inverses-ok? 2 3))
(must "GF(16): all nonzero invertible" (gf-inverses-ok? 2 4))
(must "GF(9): all nonzero invertible"  (gf-inverses-ok? 3 2))
(must "GF(27): all nonzero invertible" (gf-inverses-ok? 3 3))
(newline)

(display "3. Frobenius a^(p^n) = a, and primitive elements generate the group") (newline)
(must "GF(8): Frobenius holds"   (gf-frobenius-ok? 2 3))
(must "GF(25): Frobenius holds"  (gf-frobenius-ok? 5 2))
(must "GF(8): primitive element generates all 7 nonzero elements"   (gf-primitive-generates? 2 3))
(must "GF(16): primitive element generates all 15 nonzero elements" (gf-primitive-generates? 2 4))
(must "GF(27): primitive element generates all 26 nonzero elements" (gf-primitive-generates? 3 3))
(must "all four field laws hold for GF(9) and GF(25)" (and (field-ok? 3 2) (field-ok? 5 2)))
(newline)

(display "4. explicit arithmetic in GF(8) = F_2[x]/(x^3+x+1)") (newline)
(define m (gf-modulus 2 3))
(must "x * (x+1) = x^2 + x"        (equal? (gf-mul (list 0 1) (list 1 1) m 2) (list 0 1 1)))
(must "x is primitive (order 7)"   (= (gf-order (list 0 1) m 2) 7))
(must "x * x^(-1) = 1"             (equal? (gf-mul (list 0 1) (gf-inv (list 0 1) m 2) m 2) (list 1)))
(newline)

(display "all extension-field checks passed.") (newline)
