; -*- lisp -*-
; ============================================================
;  EXAMPLE 103 — Number theory & big integers
; ============================================================
;
; These exercise lizard's GMP-backed bignums — the same fast
; multiply path optimized for power loops.

(import "numtheory.lisp")

(display "=== MODULAR ARITHMETIC ===") (newline)
(display "  mod -7 3 = ") (display (mod -7 3)) (display " (always non-negative)") (newline)
(display "  gcd(48, 36) = ") (display (gcd2 48 36)) (newline)
(display "  lcm(4, 6) = ") (display (lcm2 4 6)) (newline)
(newline)

(display "=== EXTENDED GCD ===") (newline)
(display "  ext-gcd(240, 46) = ") (display (ext-gcd 240 46)) (newline)
(display "  (g x y) where 240x + 46y = g") (newline)
(newline)

(display "=== FAST MODULAR EXPONENTIATION ===") (newline)
(display "  3^644 mod 645 = ") (display (mod-expt 3 644 645)) (newline)
(display "  2^1000 mod 1000000007 = ") (display (mod-expt 2 1000 1000000007)) (newline)
(newline)

(display "=== MODULAR INVERSE ===") (newline)
(display "  inverse of 3 mod 11 = ") (display (mod-inverse 3 11)) (newline)
(display "  check: 3 * 4 mod 11 = ") (display (mod (* 3 4) 11)) (newline)
(newline)

(display "=== PRIMALITY & FACTORIZATION ===") (newline)
(display "  is-prime? 97: ") (display (is-prime? 97)) (newline)
(display "  is-prime? 100: ") (display (is-prime? 100)) (newline)
(display "  factorize 360: ") (display (factorize 360)) (newline)
(display "  factorize 1001: ") (display (factorize 1001)) (newline)
(newline)

(display "=== EULER TOTIENT ===") (newline)
(display "  φ(12) = ") (display (totient 12)) (newline)
(display "  φ(13) = ") (display (totient 13)) (display " (prime → n-1)") (newline)
(newline)

(display "=== BIG INTEGERS (GMP-backed) ===") (newline)
(display "  20! = ") (display (factorial 20)) (newline)
(display "  30! = ") (display (factorial 30)) (newline)
(display "  2^64 = ") (display (power 2 64)) (newline)
(display "  fib(100) = ") (display (fib-fast 100)) (newline)
(display "  fib(200) = ") (display (fib-fast 200)) (newline)
(newline)

(display "=== THE OPTIMIZED POWER LOOP ===") (newline)
(display "  (power 123 100) — first 60 digits of a 210-digit number:") (newline)
(define big (power 123 100))
(display "    ") (display (number->string big)) (newline)
(newline)

(display "=== End — all exact, arbitrary precision ===") (newline)
