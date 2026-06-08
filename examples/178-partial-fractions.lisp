; 178-partial-fractions.lisp -- partial fraction decomposition over Q.
;
; For p/q the polynomial part is divided out, q is factored over Q into irreducible
; powers, and all numerators are found at once by matching the deg q coefficients of the
; cleared identity r = sum A_ij*(q/qi^j) -- a square rational linear system.  Irreducible
; quadratics are kept intact, so the result is a genuine real partial fraction with no
; complex numbers.  Each decomposition is checked by recombination: s*q + sum of the
; partial fractions must equal p exactly.  Denominators are shown in [ ] brackets.
; `must` raises on failure.

(import "cas/pfd.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'pfd-check-failed)))
(define (check label p q)
  (let ((ans (partial-fractions p q)))
    (display "  ") (display label) (display " = ") (display (pfd->string ans "x")) (newline)
    (must "    reconstruction certificate" (pfd-ok? p q ans))
    ans))

(display "Partial fraction decomposition over Q") (newline) (newline)

(display "1. distinct linear factors") (newline)
(check "1 / [x^2-1]"   (list 1) (list -1 0 1))
(check "x / [x^2-1]"   (list 0 1) (list -1 0 1))
(newline)

(display "2. an irreducible quadratic factor stays intact") (newline)
(define a3 (check "1 / [x^3+x]" (list 1) (list 0 1 0 1)))
(must "    decomposition has 2 terms"        (= (length (car (cdr a3))) 2))
(newline)

(display "3. improper fraction: a polynomial part appears") (newline)
(define a4 (check "x^3 / [x^2-1]" (list 0 0 0 1) (list -1 0 1)))
(must "    polynomial part is x"             (equal? (car a4) (list 0 1)))
(newline)

(display "4. a repeated factor expands to every power") (newline)
(define a5 (check "1 / [ (x-1)^2 (x+1) ]" (list 1) (list 1 -1 -1 1)))
(must "    decomposition has 3 terms"        (= (length (car (cdr a5))) 3))
(newline)

(display "all partial-fraction checks passed.") (newline)
