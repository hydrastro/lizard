; 179-linear-recurrence.lisp -- closed forms for C-finite linear recurrences over Q.
;
; A recurrence a_n = c_1 a_{n-1} + ... + c_d a_{n-d} with initial values has the
; characteristic polynomial x^d - c_1 x^{d-1} - ... - c_d.  When it splits into linear
; factors over Q, the closed form is a sum of P_i(n)*r_i^n with deg P_i below the
; multiplicity of the root r_i; the coefficients solve a square rational system from the
; initial conditions.  Repeated roots contribute the factors n, n^2, ...  The closed form
; is verified by comparing it to the directly iterated sequence over many terms.  When a
; root is irrational the solver declines a rational closed form but still reports the
; characteristic polynomial and computes terms exactly.  This is the dual of Zeilberger,
; which discovers such recurrences.  `must` raises on failure.

(import "cas/linrec.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'linrec-check-failed)))

(display "Closed forms for linear recurrences over Q") (newline) (newline)

(display "1. distinct rational roots") (newline)
(define f1 (crec-solve (list 5 -6) (list 0 1)))
(display "    a_n=5a_{n-1}-6a_{n-2}, a0=0,a1=1  ->  a_n = ") (display (crec->string f1)) (newline)
(must "closed form certified"                 (crec-ok? (list 5 -6) (list 0 1) f1))
(must "predicts a_5 = 211"                     (= (eval-closed f1 5) 211))
(must "matches 3^n - 2^n at n=10"              (= (eval-closed f1 10) (- (expt 3 10) (expt 2 10))))
(newline)

(display "2. repeated root (root 1 with multiplicity 2)") (newline)
(define f2 (crec-solve (list 2 -1) (list 1 3)))
(display "    a_n=2a_{n-1}-a_{n-2}, a0=1,a1=3  ->  a_n = ") (display (crec->string f2)) (newline)
(must "closed form certified"                 (crec-ok? (list 2 -1) (list 1 3) f2))
(must "predicts a_100 = 201"                   (= (eval-closed f2 100) 201))
(newline)

(display "3. third order, roots 1,2,3") (newline)
(define f3 (crec-solve (list 6 -11 6) (list 0 1 5)))
(display "    char poly ") (display (crec-charpoly->string (list 6 -11 6))) (display "  ->  a_n = ") (display (crec->string f3)) (newline)
(must "closed form certified"                 (crec-ok? (list 6 -11 6) (list 0 1 5) f3))
(must "predicts a_6 = 665"                     (= (eval-closed f3 6) 665))
(newline)

(display "4. mixed signs: roots 2 and -1") (newline)
(define f4 (crec-solve (list 1 2) (list 2 1)))
(display "    a_n=a_{n-1}+2a_{n-2}, a0=2,a1=1  ->  a_n = ") (display (crec->string f4)) (newline)
(must "closed form certified"                 (crec-ok? (list 1 2) (list 2 1) f4))
(must "matches 2^n + (-1)^n at n=8"            (= (eval-closed f4 8) (+ (expt 2 8) (expt -1 8))))
(newline)

(display "5. Fibonacci: irrational roots, declined honestly") (newline)
(display "    char poly ") (display (crec-charpoly->string (list 1 1))) (display "  ->  ") (display (crec->string (crec-solve (list 1 1) (list 0 1)))) (newline)
(must "no rational closed form is claimed"     (equal? (crec-solve (list 1 1) (list 0 1)) 'not-rational))
(must "but terms are computed exactly"         (equal? (crec-terms (list 1 1) (list 0 1) 9) (list 0 1 1 2 3 5 8 13 21 34)))
(newline)

(display "all linear-recurrence checks passed.") (newline)
