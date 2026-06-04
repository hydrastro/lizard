; RUNG 5, going deeper: the ARBITRARY-DEPTH iterated LOGARITHM tower, the dual of the iterated exponential
; (itexp.lisp).  Define L_0 = x, L_1 = log x, L_2 = log(log x), ..., L_n = log(L_{n-1}).  Where the iterated
; exponential was multiplicative, the iterated logarithm is its reciprocal mirror: the lower logarithms appear
; in the DENOMINATOR, so elements are Laurent monomials in L_1..L_n (docs/TRAGER_ROADMAP.md, Rung 5).
;
; The derivative law (induction from L_k = log(L_{k-1}), so L_k' = L_{k-1}'/L_{k-1}):
;   L_k' = 1/(L_0 L_1 ... L_{k-1}) = 1/(x L_1 L_2 ... L_{k-1}).
; In particular d/dx(L_n) = 1/(x L_1 ... L_{n-1}), so INT 1/(x L_1 ... L_{n-1}) dx = L_n at any depth.
(import "cas/itlog.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The arbitrary-depth iterated logarithm tower: L_k = log(L_{k-1}), with INT 1/(x L_1...L_{n-1}) = L_n.") (newline) (newline)

(display "the derivative law L_k' = 1/(x L_1 ... L_{k-1}):") (newline)
(display "  L_1' has exponent vector ") (display (il-mvec (car (il-Lk-deriv 3 1)))) (display " with coefficient 1/x  (= 1/x)") (newline)
(display "  L_2' has exponent vector ") (display (il-mvec (car (il-Lk-deriv 3 2)))) (display " with coefficient 1/x  (= 1/(x L_1))") (newline)
(display "  L_3' has exponent vector ") (display (il-mvec (car (il-Lk-deriv 3 3)))) (display " with coefficient 1/x  (= 1/(x L_1 L_2))") (newline)
(chk "L_1'=1/x, L_2'=1/(x L_1), L_3'=1/(x L_1 L_2)" (if (il-vec-eq? (il-mvec (car (il-Lk-deriv 3 1))) (list 0 0 0)) (if (il-vec-eq? (il-mvec (car (il-Lk-deriv 3 2))) (list -1 0 0)) (il-vec-eq? (il-mvec (car (il-Lk-deriv 3 3))) (list -1 -1 0)) #f) #f))

(display "depth 2: INT 1/(x log x) dx = log(log x):") (newline)
(chk "d/dx(L_2)=1/(x L_1), so INT 1/(x log x) = log log x" (il-certify 2 (il-top 2) (il-int-denom 2)))

(display "depth 3: INT 1/(x log x log(log x)) dx = log(log(log x)):") (newline)
(display "  d/dx(L_3) has exponent vector ") (display (il-mvec (car (il-deriv 3 (il-top 3))))) (display "  (= 1/(x L_1 L_2))") (newline)
(chk "INT 1/(x log x log log x) = log log log x" (il-certify 3 (il-top 3) (il-int-denom 3)))

(display "depth 4 and 5 (the same law at greater height):") (newline)
(chk "INT 1/(x L_1 L_2 L_3) = L_4" (il-certify 4 (il-top 4) (il-int-denom 4)))
(chk "INT 1/(x L_1 L_2 L_3 L_4) = L_5" (il-certify 5 (il-top 5) (il-int-denom 5)))

(display "soundness -- a wrong answer is rejected by the certificate:") (newline)
(chk "INT 1/(x L_1) is NOT L_3, so claiming so is rejected" (not (il-certify 3 (il-top 3) (il-int-denom 2))))

(newline)
(display "Iterated logarithms of arbitrary depth: the law L_k' = 1/(x L_1...L_{k-1}) gives") (newline)
(display "INT 1/(x L_1 ... L_{n-1}) dx = L_n at any height n, the reciprocal dual of the iterated exponential.") (newline)
