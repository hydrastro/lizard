; -*- lisp -*-
; lib/numtheory.lisp — Number theory (showcases lizard's bignums).
;
; These algorithms exercise the GMP-backed arbitrary-precision
; integers — the same fast multiply path used by power loops.

; ---- modulo that always returns a non-negative result ----
(define (mod a m)
  (let ((r (remainder a m)))
    (if (< r 0) (+ r m) r)))

; ---- gcd / lcm ----
(define (gcd2 a b)
  (if (= b 0) (nt-abs a) (gcd2 b (remainder a b))))

(define (nt-abs n) (if (< n 0) (- 0 n) n))

(define (lcm2 a b)
  (if (or (= a 0) (= b 0)) 0
    (quotient (nt-abs (* a b)) (gcd2 a b))))

; ---- extended Euclidean algorithm ----
; Returns (list g x y) such that a*x + b*y = g = gcd(a,b).
(define (ext-gcd a b)
  (if (= b 0)
      (list a 1 0)
      (let ((rec (ext-gcd b (remainder a b))))
        (let ((g (car rec))
              (x (car (cdr rec)))
              (y (car (cdr (cdr rec)))))
          (list g y (- x (* (quotient a b) y)))))))

; ---- fast modular exponentiation: base^exp mod m ----
; O(log exp) multiplications — handles huge exponents efficiently.
(define (mod-expt base exp m)
  (define (go b e acc)
    (if (= e 0) acc
      (go (mod (* b b) m)
          (quotient e 2)
          (if (= (remainder e 2) 1)
              (mod (* acc b) m)
              acc))))
  (go (mod base m) exp 1))

; ---- modular inverse (via extended gcd) ----
; Returns x such that (a*x) mod m = 1, or #f if none exists.
(define (mod-inverse a m)
  (let ((res (ext-gcd (mod a m) m)))
    (if (= (car res) 1)
        (mod (car (cdr res)) m)
        #f)))

; ---- primality (deterministic trial division) ----
(define (is-prime? n)
  (define (check d)
    (if (> (* d d) n) #t
      (if (= 0 (remainder n d)) #f
        (check (+ d 1)))))
  (if (< n 2) #f (check 2)))

; ---- factorization (trial division) ----
(define (factorize n)
  (define (go n d acc)
    (if (> (* d d) n)
        (if (> n 1) (reverse (cons n acc)) (reverse acc))
      (if (= 0 (remainder n d))
          (go (quotient n d) d (cons d acc))
          (go n (+ d 1) acc))))
  (if (< n 2) '() (go n 2 '())))

; ---- Euler's totient φ(n) ----
(define (totient n)
  (define (count k acc)
    (if (> k n) acc
      (count (+ k 1)
             (if (= (gcd2 n k) 1) (+ acc 1) acc))))
  (if (< n 1) 0 (count 1 0)))

; ---- big factorial (exercises fast bignum multiply) ----
(define (factorial n)
  (define (go k acc)
    (if (> k n) acc (go (+ k 1) (* acc k))))
  (go 1 1))

; ---- big power (the optimized path) ----
(define (power base exp)
  (define (go e acc)
    (if (= e 0) acc (go (- e 1) (* acc base))))
  (go exp 1))

; ---- Fibonacci (fast doubling, returns big values) ----
(define (fib-fast n)
  (define (go n)
    (if (= n 0) (list 0 1)
      (let ((pair (go (quotient n 2))))
        (let ((a (car pair)) (b (car (cdr pair))))
          (let ((c (* a (- (* 2 b) a)))
                (d (+ (* a a) (* b b))))
            (if (= (remainder n 2) 0)
                (list c d)
                (list d (+ c d))))))))
  (car (go n)))
