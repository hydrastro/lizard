; 160-gosper-summation.lisp — Gosper's algorithm: indefinite hypergeometric
; summation, the discrete analogue of the Risch decision procedure.
;
; A term t(n) is hypergeometric when r(n) = t(n+1)/t(n) is rational.  Gosper
; decides whether t has a hypergeometric antidifference S (S(n+1)-S(n)=t(n)) and,
; if so, returns a rational R(n) with S(n) = R(n) t(n) (so the sum telescopes);
; otherwise it PROVES none exists.  The input is r = rnum/rden.
;
; CERTIFICATE (purely rational): S(n+1)-S(n)=t(n)  <=>  R(n+1) r(n) - R(n) = 1,
; checked exactly as rational functions, so a wrong antidifference cannot pass.
; `must` raises on failure.

(import "cas/gosper.lisp")

(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'gosper-check-failed)))
(define (certified? rnum rden)
  (let ((r (gosper-sum rnum rden)))
    (and (equal? (car r) 'summable) (gosper-certificate rnum rden (car (cdr r)) (car (cdr (cdr r)))))))
(define (not-summable? rnum rden) (equal? (car (gosper-sum rnum rden)) 'not-summable))

(display "Gosper's algorithm: indefinite hypergeometric summation") (newline) (newline)

(display "1. summable terms -- antidifference found and certified") (newline)
(must "SUM k          (t=n,        r=(n+1)/n)"      (certified? (list 1 1) (list 0 1)))
(must "SUM k^2        (t=n^2)"                      (certified? (list 1 2 1) (list 0 0 1)))
(must "SUM k^3        (t=n^3)"                      (certified? (poly-pow (list 1 1) 3) (list 0 0 0 1)))
(must "SUM k^4        (t=n^4)"                      (certified? (poly-pow (list 1 1) 4) (list 0 0 0 0 1)))
(must "SUM k*k!       (t=n*n!,    r=(n+1)^2/n)"     (certified? (list 1 2 1) (list 0 1)))
(must "SUM n*2^n      (t=n*2^n,   r=2(n+1)/n)"      (certified? (list 2 2) (list 0 1)))
(must "SUM n^2*2^n    (t=n^2*2^n, r=2(n+1)^2/n^2)"  (certified? (list 2 4 2) (list 0 0 1)))
(must "SUM 1/(n(n+1)) (telescoping)"               (certified? (list 0 1) (list 2 1)))
(newline)

(display "2. exact antidifference for SUM k*k!  (expect S = (1/n) t(n) = n!)") (newline)
(define rkk (gosper-sum (list 1 2 1) (list 0 1)))
(must "R(n) = 1/n   (numerator 1, denominator n)"
      (and (equal? (poly-norm (car (cdr rkk))) (list 1)) (equal? (poly-norm (car (cdr (cdr rkk)))) (list 0 1))))
(newline)

(display "3. PROVED to have no hypergeometric antidifference (the decision)") (newline)
(must "SUM 1/n        (harmonic numbers)"           (not-summable? (list 0 1) (list 1 1)))
(must "SUM 1/n^2"                                   (not-summable? (list 0 0 1) (list 1 2 1)))
(must "SUM n!         (t=n!,       r=n+1)"          (not-summable? (list 1 1) (list 1)))
(must "SUM n^2*n!     (t=n^2*n!,   r=(n+1)^3/n^2)"  (not-summable? (poly-pow (list 1 1) 3) (list 0 0 1)))
(must "SUM C(2n,n)    (central binomial)"           (not-summable? (list 2 4) (list 1 1)))
(newline)

(display "all Gosper-summation checks passed.") (newline)
