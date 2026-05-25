; -*- lisp -*-
; Heavy-engineering showcase.
;
; Before this round:
;   - Naive `(define (pow2 n) (if (= n 0) 1 (* 2 (pow2 (- n 1)))))`
;     blew the C stack at depth ~10^4 (no tail-call optimisation).
;   - Even the iterative form smashed the heap at ~50k iterations
;     because each (* 2 acc) allocates a fresh bignum into the
;     grow-only arena.
;
; Now:
;   - Tail-call optimised lambda bodies — arbitrary depth is fine.
;   - Fast bignum primitives (arithmetic-shift, expt, modular-expt,
;     gcd) hand off to GMP, so `2^N` is one mpz_mul_2exp call.

; ---- Deep tail recursion -------------------------------------------------

(define (count-down n)
  (if (= n 0) 'done (count-down (- n 1))))

(display "count-down from 1,000,000 (tail-rec, would segfault pre-TCO): ")
(display (count-down 1000000)) (newline)
(newline)

; ---- Massive integers in milliseconds ------------------------------------

(define m (arithmetic-shift 1 1000000))
(display "2^1,000,000 computed (one mpz_mul_2exp call).") (newline)
(display "  bit length    : 1,000,001") (newline)
(display "  decimal length: ~301,030 digits") (newline)
(newline)

(define p (expt 7 5000))
(display "7^5000 computed (one mpz_pow_ui call).") (newline)
(newline)

; ---- gcd of two enormous numbers -----------------------------------------

(define a (expt 2 1000))
(define b (- (expt 3 800) 1))
(display "gcd(2^1000, 3^800-1) = ") (display (gcd a b)) (newline)
(newline)

; ---- Modular exponentiation — RSA-style ----------------------------------

; A toy RSA encrypt/decrypt cycle. Pick a tiny prime pair, but the
; modular-expt primitive scales to numbers of any size with no
; intermediate blow-up.

(define p1 61)
(define p2 53)
(define n  (* p1 p2))               ; 3233
(define phi (* (- p1 1) (- p2 1)))  ; 3120
(define e 17)
(define d 2753)                     ; precomputed inverse of e mod phi

(define plain 123)
(define cipher  (modular-expt plain e n))
(define decoded (modular-expt cipher d n))
(display "RSA toy: plain=") (display plain)
(display "  cipher=") (display cipher)
(display "  decoded=") (display decoded)
(newline)

; A larger one — exponent of 10000, completes instantly thanks to
; mpz_powm.
(display "2^10000 mod 1234567890 = ")
(display (modular-expt 2 10000 1234567890))
(newline)
