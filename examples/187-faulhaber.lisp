; 187-faulhaber.lisp -- Bernoulli numbers and Faulhaber's power-sum formula.
;
; The Bernoulli numbers come from sum_{k=0}^{m} C(m+1,k) B_k = 0 with B_0 = 1, and
; Faulhaber's formula turns the power sum into a degree k+1 polynomial in n:
;   S_k(n) = sum_{i=1}^{n} i^k = (1/(k+1)) sum_{j=0}^{k} C(k+1,j) B_j^{+} n^{k+1-j}.
; Each polynomial is certified exactly by matching the directly computed sum 1^k+...+m^k
; at m = 0, 1, ..., k+2; agreement past degree+1 points pins the unique polynomial.
; `must` raises on failure.

(import "cas/bernoulli.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'faulhaber-check-failed)))

(display "Bernoulli numbers and Faulhaber's formula") (newline) (newline)

(display "1. Bernoulli numbers") (newline)
(display "    B_0..B_10 = ") (display (map bernoulli (prange 0 10))) (newline)
(must "B_0 = 1"        (= (bernoulli 0) 1))
(must "B_1 = -1/2"     (= (bernoulli 1) (/ -1 2)))
(must "B_2 = 1/6"      (= (bernoulli 2) (/ 1 6)))
(must "B_4 = -1/30"    (= (bernoulli 4) (/ -1 30)))
(must "B_6 = 1/42"     (= (bernoulli 6) (/ 1 42)))
(must "B_10 = 5/66"    (= (bernoulli 10) (/ 5 66)))
(must "odd B_3,B_5 vanish" (and (= (bernoulli 3) 0) (= (bernoulli 5) 0)))
(newline)

(display "2. power-sum polynomials") (newline)
(display "    sum i   = ") (display (faulhaber->string 1)) (newline)
(display "    sum i^2 = ") (display (faulhaber->string 2)) (newline)
(display "    sum i^3 = ") (display (faulhaber->string 3)) (newline)
(must "sum i   = n^2/2 + n/2"           (equal? (faulhaber-poly 1) (list 0 (/ 1 2) (/ 1 2))))
(must "sum i^2 = (2n^3+3n^2+n)/6"       (equal? (faulhaber-poly 2) (list 0 (/ 1 6) (/ 1 2) (/ 1 3))))
(must "sum i^3 = (n^2(n+1)^2)/4"        (equal? (faulhaber-poly 3) (list 0 0 (/ 1 4) (/ 1 2) (/ 1 4))))
(newline)

(display "3. certificates: polynomial equals the direct sum") (newline)
(must "k=1 certified" (faulhaber-ok? 1))
(must "k=2 certified" (faulhaber-ok? 2))
(must "k=3 certified" (faulhaber-ok? 3))
(must "k=4 certified" (faulhaber-ok? 4))
(must "k=5 certified" (faulhaber-ok? 5))
(must "k=6 certified" (faulhaber-ok? 6))
(newline)

(display "4. evaluating the closed forms") (newline)
(display "    sum_{i=1}^{10} i^3 = ") (display (poly-eval (faulhaber-poly 3) 10)) (display " (= 55^2)") (newline)
(must "sum_{1..10} i^3 = 3025"       (= (poly-eval (faulhaber-poly 3) 10) 3025))
(must "sum_{1..100} i^2 = 338350"    (= (poly-eval (faulhaber-poly 2) 100) 338350))
(must "matches direct sum at 50, k=4" (= (poly-eval (faulhaber-poly 4) 50) (sumpow 50 4)))
(newline)

(display "all Faulhaber checks passed.") (newline)
