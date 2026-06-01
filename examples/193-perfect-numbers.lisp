; 193-perfect-numbers.lisp -- perfect numbers, amicable pairs, and aliquot sums.
;
; The aliquot sum s(n) = sigma(n) - n adds the proper divisors of n.  n is perfect when
; sigma(n) = 2n, abundant when s(n) > n, deficient when s(n) < n; two numbers are amicable
; when each is the other's aliquot sum.  Euclid-Euler builds an even perfect number
; 2^(p-1)(2^p - 1) from each Mersenne prime.  Every classification is decided through the
; independent sigma function, which is the certificate.  `must` raises on failure.

(import "cas/perfect.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'perfect-check-failed)))

(display "Perfect numbers, amicable pairs, aliquot sums") (newline) (newline)

(display "1. classification by aliquot sum") (newline)
(display "    perfect numbers up to 1000: ") (display (perfects-upto 1000)) (newline)
(display "    12 is ") (display (classify 12)) (display ", 8 is ") (display (classify 8)) (display ", 6 is ") (display (classify 6)) (newline)
(must "perfects up to 1000 are 6, 28, 496" (equal? (perfects-upto 1000) (list 6 28 496)))
(must "6, 28, 496, 8128 are perfect" (and (perfect? 6) (perfect? 28) (perfect? 496) (perfect? 8128)))
(must "12 is abundant"  (abundant? 12))
(must "8 is deficient"  (deficient? 8))
(must "aliquot(12) = 16" (= (aliquot 12) 16))
(newline)

(display "2. amicable pairs") (newline)
(must "(220, 284) are amicable"   (amicable? 220 284))
(must "(1184, 1210) are amicable" (amicable? 1184 1210))
(must "(220, 285) are not"        (not (amicable? 220 285)))
(must "a number is not amicable with itself" (not (amicable? 6 6)))
(newline)

(display "3. Euclid-Euler: Mersenne primes give even perfect numbers") (newline)
(display "    p = 13 gives 2^12(2^13 - 1) = ") (display (euclid-euler 13)) (newline)
(must "p=2,3,5,7 are Mersenne-prime exponents" (and (mersenne-prime? 2) (mersenne-prime? 3) (mersenne-prime? 5) (mersenne-prime? 7)))
(must "2^11 - 1 = 2047 is not prime"  (not (mersenne-prime? 11)))
(must "Euclid-Euler at p=7 is 8128"   (= (euclid-euler 7) 8128))
(must "the 5th perfect number 33550336 (p=13) is perfect" (perfect? (euclid-euler 13)))
(must "Euclid-Euler certified for p=2,3,5,7,13" (and (euclid-euler-ok? 2) (euclid-euler-ok? 3) (euclid-euler-ok? 5) (euclid-euler-ok? 7) (euclid-euler-ok? 13)))
(newline)

(display "all perfect-number checks passed.") (newline)
