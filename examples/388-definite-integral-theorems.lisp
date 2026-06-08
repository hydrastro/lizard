; CERTIFIED DEFINITE INTEGRALS as THEOREMS by the Fundamental Theorem of Calculus: each definite integral of a
; polynomial is computed as F(b) - F(a), the single nontrivial premise "F is an antiderivative of f" is discharged
; by the differentiation arbiter, and a structured proof record is produced that a checker can re-verify
; independently (docs/CAS.md -- the proof-producing CAS: an integral as a theorem about a number).
(import "cas/defint.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Definite integrals as certified theorems: INT_a^b f dx = F(b) - F(a), with F' = f checked by the arbiter.") (newline) (newline)

(display "Theorem: INT_0^1 x^2 dx = 1/3.") (newline)
(display "  antiderivative F = x^3/3, and F' = ") (display (dint-antideriv (list 0 0 1))) (display " differentiates back to x^2:") (newline)
(must "the antiderivative is x^3/3" (equal? (dint-antideriv (list 0 0 1)) (list 0 0 0 (/ 1 3))))
(must "F' = f is certified by differentiation" (dint-certify (list 0 0 1)))
(must "so INT_0^1 x^2 dx = 1/3" (= (dint-value (list 0 0 1) 0 1) (/ 1 3)))
(display "  the proof record (re-checkable):") (newline)
(display "  ") (display (dint-prove (list 0 0 1) 0 1)) (newline)
(must "the proof record re-verifies independently" (dint-recheck (dint-prove (list 0 0 1) 0 1)))

(display "Theorem: INT_0^2 (3x^2 + 2x + 1) dx = 14.") (newline)
(display "  F = x^3 + x^2 + x, F(2) - F(0) = (8 + 4 + 2) - 0 = 14:") (newline)
(must "the value is 14" (= (dint-value (list 1 2 3) 0 2) 14))
(must "certified and re-checked" (dint-recheck (dint-prove (list 1 2 3) 0 2)))

(display "Theorem: INT_{-1}^1 x^2 dx = 2/3 (the symmetric interval).") (newline)
(must "the value is 2/3" (= (dint-value (list 0 0 1) -1 1) (/ 2 3)))

(display "Theorem: INT_a^a f dx = 0 for every f (degenerate interval).") (newline)
(must "INT_3^3 (x^3 + ... ) = 0" (= (dint-value (list 1 2 3 4) 3 3) 0))

(display "Soundness: a proof record with a tampered value does NOT re-check.") (newline)
(must "claiming INT_0^1 x^2 = 1/2 fails the re-check"
  (if (dint-recheck (list (quote theorem) (list (quote definite-integral) (list 0 0 1) 0 1) (quote =) (/ 1 2)
                          (list (quote by) (quote FTC) (list (quote antiderivative) (list 0 0 0 (/ 1 3))) (list (quote certificate) (quote F-prime=f) #t)))) #f #t))

(newline)
(display "Each definite integral is now a theorem: the value plus a proof record whose one nontrivial premise") (newline)
(display "(F is an antiderivative of f) is discharged by the differentiation arbiter and re-verifiable by a checker.") (newline)
(display "Non-elementary integrands (sinc, exp(-x^2)) need their own parameter-integral proofs -- see example 389.") (newline)
