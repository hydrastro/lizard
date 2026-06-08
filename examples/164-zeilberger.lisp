; 164-zeilberger.lisp — Zeilberger's algorithm (creative telescoping):
; automatically DISCOVER the linear recurrence a definite hypergeometric sum obeys.
;
; For S(n) = SUM_k F(n,k), Zeilberger finds polynomials a_0(n),...,a_J(n) and a
; rational certificate R(n,k) with SUM_j a_j(n) F(n+j,k) = G(n,k+1)-G(n,k), G=R F.
; Summing over k telescopes the right side to 0, so SUM_j a_j(n) S(n+j) = 0.  The
; coefficients are recovered as a nontrivial nullspace vector of an exact Q-matrix
; (built from the bivariate telescoping identity); the certificate then proves the
; recurrence exactly.  We exhibit a first-order case (sum of binomials) and a
; genuine SECOND-order case (central Delannoy numbers), each checked three ways:
; the bivariate certificate, and the discovered recurrence annihilating the actual
; integer sequence.  `must` raises on failure.

(import "cas/zeilberger.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'zb-check-failed)))
(define (fact n) (if (= n 0) 1 (* n (fact (- n 1)))))
(define (binom n k) (if (or (< k 0) (> k n)) 0 (/ (fact n) (* (fact k) (fact (- n k))))))
(define (rec3 as f n) (+ (* (poly-eval (nth as 0) n) (f n))
                         (+ (if (> (length as) 1) (* (poly-eval (nth as 1) n) (f (+ n 1))) 0)
                            (if (> (length as) 2) (* (poly-eval (nth as 2) n) (f (+ n 2))) 0))))
(define (pow2 n) (if (= n 0) 1 (* 2 (pow2 (- n 1)))))
(define (delannoy n) (dsum n 0 0))
(define (dsum n k acc) (if (> k n) acc (dsum n (+ k 1) (+ acc (* (binom n k) (binom (+ n k) k))))))

(display "Zeilberger: discovering recurrences for definite sums") (newline) (newline)

(display "1. S(n) = SUM_k C(n,k)   [ = 2^n ]   -- first-order recurrence discovered") (newline)
; F = C(n,k): r1 = (n+1)/(n+1-k), r2 = (n-k)/(k+1)
(define c1 (zb-try (list (list 1 1)) (list (list 1 1) (list -1))
                   (list (list 0 1) (list -1)) (list (list 1) (list 1)) (list (list 1 1) (list -1)) 1 0 0 1))
(must "recurrence found" (pair? c1))
(display "    recurrence: ") (display (zb-recurrence->string c1)) (newline)
(must "bivariate certificate verifies" (zb-verify (list (list 1 1)) (list (list 1 1) (list -1)) (list (list 0 1) (list -1)) (list (list 1) (list 1)) c1))
(must "discovered recurrence annihilates 2^n (n=1..4)"
      (and (= (rec3 (car (cdr c1)) pow2 1) 0) (= (rec3 (car (cdr c1)) pow2 2) 0)
           (= (rec3 (car (cdr c1)) pow2 3) 0) (= (rec3 (car (cdr c1)) pow2 4) 0)))
(newline)

(display "2. S(n) = SUM_k C(n,k) C(n+k,k)  (central Delannoy)  -- SECOND-order discovered") (newline)
; r1 = (n+k+1)/(n+1-k), r2 = (n-k)(n+k+1)/(k+1)^2 ; certificate denom (n+1-k)(n+2-k)
(define D-a1n (list (list 1 1) (list 1)))
(define D-a1d (list (list 1 1) (list -1)))
(define D-a2n (list (list 0 1 1) (list -1) (list -1)))
(define D-a2d (list (list 1) (list 2) (list 1)))
(define D-D (bp-mul D-a1d (bp-shiftn D-a1d)))
(define c2 (zb-try D-a1n D-a1d D-a2n D-a2d D-D 2 1 1 2))
(must "recurrence found" (pair? c2))
(display "    recurrence: ") (display (zb-recurrence->string c2)) (newline)
(must "order is 2 (three coefficients)" (= (length (car (cdr c2))) 3))
(must "bivariate certificate verifies" (zb-verify D-a1n D-a1d D-a2n D-a2d c2))
(must "Delannoy values 1,3,13,63,321" (and (= (delannoy 0) 1) (= (delannoy 1) 3) (= (delannoy 2) 13) (= (delannoy 3) 63) (= (delannoy 4) 321)))
(must "discovered recurrence annihilates Delannoy (n=0..2)"
      (and (= (rec3 (car (cdr c2)) delannoy 0) 0) (= (rec3 (car (cdr c2)) delannoy 1) 0) (= (rec3 (car (cdr c2)) delannoy 2) 0)))
(newline)

(display "all Zeilberger checks passed.") (newline)
