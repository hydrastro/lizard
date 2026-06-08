; 210-number-theoretic-transform.lisp -- the NTT over F_p and fast polynomial multiplication.
;
; The NTT is the finite-field discrete Fourier transform: with a primitive n-th root of
; unity it maps a vector to X_k = sum_j a_j w^{jk}, computed by recursive radix-2
; Cooley-Tukey in O(n log n).  It turns convolution into pointwise multiplication, so it
; multiplies polynomials fast.  Certified by the round trip, the convolution theorem, and
; agreement with the schoolbook product.  `must` raises on failure.

(import "cas/ntt.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'ntt-check-failed)))

(display "The Number-Theoretic Transform over F_p") (newline) (newline)

(display "1. transform and inverse over F_17 (n = 8, root w = 9)") (newline)
(define w (nth-root 17 8))
(display "    a        = ") (display (vec->string (list 1 2 3 4 0 0 0 0))) (newline)
(display "    NTT(a)   = ") (display (vec->string (ntt (list 1 2 3 4 0 0 0 0) w 17))) (newline)
(display "    INTT back = ") (display (vec->string (intt (ntt (list 1 2 3 4 0 0 0 0) w 17) w 17))) (newline)
(must "primitive 8th root mod 17 is 9"  (= (nth-root 17 8) 9))
(must "inverse recovers the input"      (ntt-roundtrip-ok? (list 1 2 3 4 0 0 0 0) 17 8))
(must "round trip for another vector"   (ntt-roundtrip-ok? (list 5 0 9 2 7 1 3 8) 17 8))
(newline)

(display "2. convolution theorem and polynomial multiplication") (newline)
(display "    (1+2x+3x^2)(4+5x+6x^2) via NTT = ") (display (vec->string (ntt-polymul (list 1 2 3) (list 4 5 6) w 17 8))) (newline)
(display "    schoolbook product (mod 17)     = ") (display (vec->string (school (list 1 2 3) (list 4 5 6) 17))) (newline)
(must "cyclic convolution matches the direct sum" (ntt-convolution-ok? (list 1 2 3 4) (list 5 6 7 8) 17 8))
(must "NTT product equals schoolbook product"     (ntt-polymul-ok? (list 1 2 3) (list 4 5 6) 17 8))
(must "another product agrees"                     (ntt-polymul-ok? (list 7 0 2 5) (list 1 9 3 0) 17 8))
(newline)

(display "3. a larger modulus: F_97, n = 16") (newline)
(must "primitive 16th root mod 97 has order 16" (= (nth-root 97 16) 8))
(must "round trip over F_97"            (ntt-roundtrip-ok? (list 1 2 3 4 5 6 7) 97 16))
(must "polynomial product over F_97"    (ntt-polymul-ok? (list 10 20 30 40) (list 5 6 7 8) 97 16))
(must "product of degree-7 polynomials over F_97" (ntt-polymul-ok? (list 1 2 3 4 5 6 7 8) (list 8 7 6 5 4 3 2 1) 97 16))
(newline)

(display "all NTT checks passed.") (newline)
