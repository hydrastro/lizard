; 200-gaussian-integers.lisp -- the Gaussian integers Z[i].
;
; Z[i] is a Euclidean domain under the norm N(a+bi) = a^2 + b^2.  Division rounds
; z*conj(w)/N(w) to the nearest Gaussian integer, the remainder reduces the norm, and a
; Euclidean gcd follows.  A rational prime p = 1 (mod 4) splits as (a+bi)(a-bi) with
; a^2+b^2 = p -- exactly the sum-of-two-squares decomposition.  Certificates: the norm is
; multiplicative, division satisfies N(r) < N(w), and the gcd divides both inputs exactly.
; `must` raises on failure.

(import "cas/gaussint.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'gaussint-check-failed)))
(define (rng a b) (if (> a b) '() (cons a (rng (+ a 1) b))))
(define (all? l) (cond ((null? l) #t) ((car l) (all? (cdr l))) (else #f)))

(display "The Gaussian integers Z[i]") (newline) (newline)

(display "1. arithmetic and norm") (newline)
(display "    norm of 3+2i is ") (display (gi-norm (cons 3 2))) (newline)
(must "(2+i)(2-i) = 5"        (gi-equal? (gi-mul (cons 2 1) (cons 2 -1)) (cons 5 0)))
(must "norm of 3+2i is 13"    (= (gi-norm (cons 3 2)) 13))
(must "norm is multiplicative" (and (gi-norm-mult-ok? (cons 3 2) (cons 1 4)) (gi-norm-mult-ok? (cons 5 -1) (cons 2 7)) (gi-norm-mult-ok? (cons 0 3) (cons 6 1))))
(newline)

(display "2. splitting rational primes into Gaussian primes") (newline)
(display "    5  -> ") (display (gi->string (gi-split 5)))  (newline)
(display "    13 -> ") (display (gi->string (gi-split 13))) (newline)
(display "    97 -> ") (display (gi->string (gi-split 97))) (newline)
(must "5 splits as 2+i"   (gi-equal? (gi-split 5) (cons 2 1)))
(must "13 splits as 3+2i" (gi-equal? (gi-split 13) (cons 3 2)))
(must "2 ramifies as 1+i" (gi-equal? (gi-split 2) (cons 1 1)))
(must "every prime p=1 mod 4 in 5..200 splits correctly"
      (all? (map gi-split-ok? (filter (lambda (p) (and (prime? p) (= (imod p 4) 1))) (rng 5 200)))))
(newline)

(display "3. Euclidean division and gcd") (newline)
(must "division property z=qw+r with N(r)<N(w)" (and (gi-div-ok? (cons 17 5) (cons 3 2)) (gi-div-ok? (cons 100 1) (cons 7 4)) (gi-div-ok? (cons 5 3) (cons 2 1))))
(must "gcd(5, 2+i) divides both"        (gi-gcd-divides? (cons 5 0) (cons 2 1)))
(must "gcd(11+3i, 1+8i) divides both"   (gi-gcd-divides? (cons 11 3) (cons 1 8)))
(must "gcd(6+8i, 4+4i) divides both"    (gi-gcd-divides? (cons 6 8) (cons 4 4)))
(newline)

(display "4. Gaussian primality") (newline)
(must "2+i is a Gaussian prime (norm 5)"     (gi-prime? (cons 2 1)))
(must "3 is a Gaussian prime (inert)"        (gi-prime? (cons 3 0)))
(must "7 is a Gaussian prime (inert)"        (gi-prime? (cons 7 0)))
(must "5 is NOT a Gaussian prime (it splits)" (not (gi-prime? (cons 5 0))))
(must "1+i is a Gaussian prime (norm 2)"     (gi-prime? (cons 1 1)))
(newline)

(display "all Gaussian-integer checks passed.") (newline)
