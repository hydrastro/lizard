; ALGEBRAIC-LEVEL INTEGRATION through the height-n recursion: with the algebraic level (alg n a), theta^n = a,
; now wired into the recursive coupled RDE / integrator, integrals over sqrt-towers are DECIDED through the same
; recursion that handles exp and log levels (docs/TRAGER_ROADMAP.md, the summit, "wire the algebraic level into
; the integrator").
;
; The algebraic level is diagonal with rate w = a'/(n a), so INT f = (D y = f) reduces per theta-degree k to the
; base-level problem D(y_k) + (k w) y_k = f_k, with theta-degree bounded by n-1 (theta^n reduces to a) so there
; is no non-terminating tail; the differentiation certificate is the arbiter.
(import "cas/rischintn.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define t1a (list (list (quote alg) 2 (rat-from-poly (list 0 1)))))   ; theta = sqrt(x)

(display "Integration over the algebraic level sqrt(x), through the same recursion as exp and log.") (newline) (newline)

(display "INT 1/(2 sqrt x) dx = sqrt x  (the integrand is (1/(2x)) sqrt x; the degree-1 RDE gives y_1 = 1):") (newline)
(define f1 (list (rat-zero) (rat-make (list 1) (list 0 2))))
(define r1 (te-integrate t1a 1 f1))
(chk "INT 1/(2 sqrt x) = sqrt x, certified" (if (equal? (car r1) (quote elementary)) (te-int-certify t1a 1 f1 (car (cdr r1))) #f))

(display "INT (1 + 1/(2 sqrt x)) dx = x + sqrt x  (degree 0 gives x, degree 1 gives sqrt x):") (newline)
(define f2 (list (rat-one) (rat-make (list 1) (list 0 2))))
(define r2 (te-integrate t1a 1 f2))
(chk "INT (1 + 1/(2 sqrt x)) = x + sqrt x, certified" (if (equal? (car r2) (quote elementary)) (te-int-certify t1a 1 f2 (car (cdr r2))) #f))

(display "INT x^(-3/2) dx = -2/sqrt x  (the integrand is (1/x^2) sqrt x; the degree-1 RDE gives y_1 = -2/x):") (newline)
(define f3 (list (rat-zero) (rat-make (list 1) (list 0 0 1))))
(define r3 (te-integrate t1a 1 f3))
(display "  y = ") (display (if (equal? (car r3) (quote elementary)) (car (cdr r3)) (quote NA))) (display "  (= (-2/x) sqrt x = -2/sqrt x)") (newline)
(chk "INT x^(-3/2) = -2/sqrt x, certified" (if (equal? (car r3) (quote elementary)) (te-int-certify t1a 1 f3 (car (cdr r3))) #f))

(display "soundness -- a wrong candidate does not certify:") (newline)
(chk "the candidate x (not the integral of 1/(2 sqrt x)) is rejected" (if (te-int-certify t1a 1 f1 (list (rat-from-poly (list 0 1)) (rat-zero))) #f #t))

(newline)
(display "The algebraic level integrates through the recursion just like exp and log: each theta-degree reduces to") (newline)
(display "a base-level RDE with rate a'/(n a), the degree bounded by n-1, every result certified by D y = f.") (newline)
