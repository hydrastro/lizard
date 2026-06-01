; 216-reed-muller-codes.lisp -- first-order Reed-Muller codes RM(1,m).
;
; Codewords are truth tables of affine Boolean functions f = a_0 XOR a_1 x_1 XOR ... XOR
; a_m x_m over all 2^m inputs: a [2^m, m+1, 2^{m-1}] code correcting t = 2^{m-2}-1 errors.
; Decoding is maximum-likelihood via the fast Walsh-Hadamard transform -- mapping bits to
; signs and transforming, the largest-magnitude coefficient names the nearest affine
; function (its index gives a_1..a_m, its sign gives a_0).  Certified by the round trip.

(import "cas/reedmuller.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'rm-check-failed)))

(display "First-order Reed-Muller codes (Walsh-Hadamard decoding)") (newline) (newline)

(display "1. ") (display (rm-info 2)) (newline)
(display "    f = x_1 has coefficients (0 1 0); truth table = ") (display (rm-encode (list 0 1 0) 2)) (newline)
(display "    Walsh-Hadamard transform of its signs = ") (display (wht (signs (list 0 1 0 1)))) (display "  (peak at index 1)") (newline)
(must "decoding the truth table recovers (0 1 0)" (equal? (rm-decode (list 0 1 0 1) 2) (list 0 1 0)))
(newline)

(display "2. ") (display (rm-info 4)) (newline)
(define a (list 1 0 1 1 0))
(display "    message  = ") (display a) (newline)
(display "    codeword = ") (display (rm-encode a 4)) (newline)
(must "clean decode" (rm-clean-ok? a 4))
(must "1 error corrected"  (rm-roundtrip-ok? a 4 (list 5)))
(must "2 errors corrected" (rm-roundtrip-ok? a 4 (list 0 9)))
(must "3 errors corrected (= correction radius t)" (rm-roundtrip-ok? a 4 (list 2 7 13)))
(must "all-ones message survives 3 errors"  (rm-roundtrip-ok? (list 1 1 1 1 1) 4 (list 0 7 15)))
(newline)

(display "3. ") (display (rm-info 5)) (newline)
(must "clean decode" (rm-clean-ok? (list 1 0 1 0 1 1) 5))
(must "7 errors corrected (= t)" (rm-roundtrip-ok? (list 1 0 1 0 1 1) 5 (list 0 3 7 12 19 25 31)))
(newline)

(display "all Reed-Muller checks passed.") (newline)
