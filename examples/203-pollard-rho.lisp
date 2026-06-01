; 203-pollard-rho.lisp -- Pollard's rho integer factorization.
;
; Trial division only reaches small factors; Pollard's rho iterates f(x) = x^2 + c (mod n)
; with Floyd cycle detection so that gcd(|x - y|, n) exposes a nontrivial divisor of a
; composite n, bumping c if a run collapses.  Full factorization strips small primes then
; recurses, testing primality with the deterministic Miller-Rabin from numbertheory.  The
; result is checked the only way that matters: the factors multiply back to n and every one
; is prime.  `must` raises on failure.

(import "cas/pollard.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'pollard-check-failed)))

(display "Pollard's rho integer factorization") (newline) (newline)

(display "1. semiprimes beyond trial division") (newline)
(display "    8051          = ") (display (pollard->string 8051)) (newline)
(display "    1234567       = ") (display (pollard->string 1234567)) (newline)
(must "8051 = 83 * 97"          (equal? (pollard-factorize 8051) (list 83 97)))
(must "10403 = 101 * 103"       (equal? (pollard-factorize 10403) (list 101 103)))
(must "1234567 = 127 * 9721"    (equal? (pollard-factorize 1234567) (list 127 9721)))
(newline)

(display "2. a 12-digit number and Fermat's fifth number") (newline)
(display "    600851475143 = ") (display (pollard->string 600851475143)) (newline)
(display "    4294967297   = ") (display (pollard->string 4294967297)) (newline)
(must "600851475143 = 71*839*1471*6857" (equal? (pollard-factorize 600851475143) (list 71 839 1471 6857)))
(must "Fermat F5 = 641 * 6700417"       (equal? (pollard-factorize 4294967297) (list 641 6700417)))
(newline)

(display "3. multiplicity and primes") (newline)
(display "    123456789     = ") (display (pollard->string 123456789)) (newline)
(must "123456789 = 3^2 * 3607 * 3803" (equal? (pollard-factor-rle 123456789) (list (cons 3 2) (cons 3607 1) (cons 3803 1))))
(must "a large prime stays whole" (equal? (pollard-factorize 1000000007) (list 1000000007)))
(newline)

(display "4. every factorization certified (product = n, all factors prime)") (newline)
(must "8051 certified"          (pollard-ok? 8051))
(must "600851475143 certified"  (pollard-ok? 600851475143))
(must "4294967297 certified"    (pollard-ok? 4294967297))
(must "123456789 certified"     (pollard-ok? 123456789))
(must "1000000007 certified"    (pollard-ok? 1000000007))
(newline)

(display "all Pollard-rho checks passed.") (newline)
