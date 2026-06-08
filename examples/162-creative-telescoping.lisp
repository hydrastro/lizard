; 162-creative-telescoping.lisp — Wilf-Zeilberger creative telescoping:
; machine-checked proofs of binomial identities.
;
; To prove SUM_k summand(n,k) = rhs(n), set F = summand/rhs so the claim becomes
; SUM_k F(n,k) = 1, and exhibit a rational certificate R(n,k) with
;   F(n+1,k) - F(n,k) = G(n,k+1) - G(n,k),   G = R F.
; Summing over k telescopes the right side to 0, so SUM_k F is constant in n; one
; base value finishes the proof.  Dividing by F gives a purely rational identity
;   r1(n,k) - 1 = R(n,k+1) r2(n,k) - R(n,k),   r1=F(n+1,k)/F(n,k), r2=F(n,k+1)/F(n,k),
; which is THE certificate and is checked EXACTLY with bivariate arithmetic.  R is
; either discovered (a linear solve over Q for the coefficients of P, with R=P/D)
; or supplied and verified; either way the bivariate identity is the proof.
; `must` raises on failure.

(import "cas/wz.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'wz-check-failed)))
(define (fact n) (if (= n 0) 1 (* n (fact (- n 1)))))
(define (binom n k) (if (or (< k 0) (> k n)) 0 (/ (fact n) (* (fact k) (fact (- n k))))))
(define (sum-b n k acc) (if (> k n) acc (sum-b n (+ k 1) (+ acc (binom n k)))))
(define (sum-bsq n k acc) (if (> k n) acc (sum-bsq n (+ k 1) (+ acc (* (binom n k) (binom n k))))))

(display "Wilf-Zeilberger creative telescoping: proofs of binomial identities") (newline) (newline)

(display "1. SUM_k C(n,k) = 2^n  (certificate discovered automatically)") (newline)
(define R1 (wz-search (list (list (/ 1 2) (/ 1 2))) (list (list 1 1) (list -1))
                      (list (list 0 1) (list -1)) (list (list 1) (list 1))))
(must "certificate found" (pair? R1))
(must "R(n,k) = -k / (2(n+1-k))   (the classic WZ certificate)"
      (and (bp-equal? (car (cdr R1)) (list (list) (list (/ -1 2)))) (bp-equal? (car (cdr (cdr R1))) (list (list 1 1) (list -1)))))
(must "rational telescoping identity verified"
      (wz-verify (list (list (/ 1 2) (/ 1 2))) (list (list 1 1) (list -1)) (list (list 0 1) (list -1)) (list (list 1) (list 1)) R1))
(must "numeric check: SUM_k C(5,k) = 32 = 2^5" (= (sum-b 5 0 0) 32))
(newline)

(display "2. SUM_k k*C(n,k) = n*2^(n-1)  (certificate discovered automatically)") (newline)
(define R2 (wz-search (list (list 0 (/ 1 2))) (list (list 1 1) (list -1))
                      (list (list 0 1) (list -1)) (list (list) (list 1))))
(must "certificate found" (pair? R2))
(must "rational telescoping identity verified"
      (wz-verify (list (list 0 (/ 1 2))) (list (list 1 1) (list -1)) (list (list 0 1) (list -1)) (list (list) (list 1)) R2))
(newline)

(display "3. SUM_k C(n,k)^2 = C(2n,n)  (Vandermonde central binomial; certificate verified)") (newline)
; r1 = (n+1)^3 / (2(2n+1)(n+1-k)^2),  r2 = (n-k)^2 / (k+1)^2
(define s-a1n (list (poly-pow (list 1 1) 3)))
(define s-a1d (bp-scaleq 2 (bp-mul (list (list 1 2)) (bp-mul (list (list 1 1) (list -1)) (list (list 1 1) (list -1))))))
(define s-a2n (bp-mul (list (list 0 1) (list -1)) (list (list 0 1) (list -1))))
(define s-a2d (bp-mul (list (list 1) (list 1)) (list (list 1) (list 1))))
; supplied certificate R = -k^2(3n+3-2k) / (2(2n+1)(n+1-k)^2)
(define s-P (bp-add (bp-scalen (poly-neg (list 3 3)) (bp-monomial 0 2)) (bp-scaleq 2 (bp-monomial 0 3))))
(define R3 (list 'ok s-P s-a1d))
(must "supplied certificate verifies the telescoping identity" (wz-verify s-a1n s-a1d s-a2n s-a2d R3))
(must "numeric check: SUM_k C(5,k)^2 = 252 = C(10,5)" (= (sum-bsq 5 0 0) (binom 10 5)))
(newline)

(display "all creative-telescoping checks passed.") (newline)
