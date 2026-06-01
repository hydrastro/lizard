; 199-lucas-lehmer.lisp -- the Lucas-Lehmer test and Lucas sequences.
;
; Lucas-Lehmer is the exact, efficient primality proof for Mersenne numbers M_p = 2^p - 1:
; with s_0 = 4 and s_{i+1} = s_i^2 - 2 (mod M_p), M_p is prime iff s_{p-2} = 0.  Only p-2
; modular squarings are needed, so primes far beyond trial division -- like M_127, a
; 39-digit number -- are certified instantly.  These exponents are exactly the ones behind
; the even perfect numbers.  The module also gives the Lucas sequences U_n, V_n, with the
; identity V_n^2 - D U_n^2 = 4 Q^n as a certificate.  `must` raises on failure.

(import "cas/lucas.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'lucas-check-failed)))
(define (all? l) (cond ((null? l) #t) ((car l) (all? (cdr l))) (else #f)))

(display "The Lucas-Lehmer test and Lucas sequences") (newline) (newline)

(display "1. Mersenne primality by Lucas-Lehmer") (newline)
(display "    exponents in 2..40 giving Mersenne primes: ") (display (mersenne-prime-exponents 2 40)) (newline)
(must "M_3, M_5, M_7 are prime"   (and (mersenne-prime-ll? 3) (mersenne-prime-ll? 5) (mersenne-prime-ll? 7)))
(must "M_13, M_17, M_19 are prime" (and (mersenne-prime-ll? 13) (mersenne-prime-ll? 17) (mersenne-prime-ll? 19)))
(must "M_11 is composite (2047 = 23*89)" (not (mersenne-prime-ll? 11)))
(must "M_23 is composite"          (not (mersenne-prime-ll? 23)))
(must "Mersenne exponents in 2..40 are 2,3,5,7,13,17,19,31"
      (equal? (mersenne-prime-exponents 2 40) (list 2 3 5 7 13 17 19 31)))
(newline)

(display "2. primes far beyond trial division") (newline)
(display "    M_31  = ") (display (mersenne 31))  (newline)
(display "    M_127 = ") (display (mersenne 127)) (newline)
(must "M_31 is prime"  (mersenne-prime-ll? 31))
(must "M_61 is prime"  (mersenne-prime-ll? 61))
(must "M_107 is prime" (mersenne-prime-ll? 107))
(must "M_127 is prime" (mersenne-prime-ll? 127))
(newline)

(display "3. cross-check against the Miller-Rabin test") (newline)
(must "Lucas-Lehmer and Miller-Rabin agree on M_p for p in 3..31"
      (all? (map lucas-lehmer-agrees? (list 3 5 7 11 13 17 19 23 29 31))))
(newline)

(display "4. Lucas sequences and the companion identity") (newline)
(display "    F_10..F_15 = ") (display (map fib (range 10 15))) (newline)
(display "    L_0..L_8   = ") (display (map lucasnum (range 0 8))) (newline)
(must "Fibonacci F_10..F_15 are 55,89,144,233,377,610" (equal? (map fib (range 10 15)) (list 55 89 144 233 377 610)))
(must "Lucas L_0..L_6 are 2,1,3,4,7,11,18"             (equal? (map lucasnum (range 0 6)) (list 2 1 3 4 7 11 18)))
(must "identity holds for (1,-1), n=1..12" (all? (map (lambda (n) (lucas-identity-ok? 1 -1 n)) (range 1 12))))
(must "identity holds for (3,2), n=1..10"  (all? (map (lambda (n) (lucas-identity-ok? 3 2 n)) (range 1 10))))
(newline)

(display "all Lucas-Lehmer checks passed.") (newline)
