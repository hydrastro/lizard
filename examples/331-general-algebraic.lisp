; GENERAL-DEGREE algebraic extensions theta^n = a (n >= 2): the general-n multiplication with reduction
; theta^n -> a, completing the algebraic level beyond the quadratic case, and cube-root integration through the
; recursion (docs/TRAGER_ROADMAP.md, the summit).
;
; The recursive derivation and the integrator's algebraic case are already general in n (both use the diagonal
; rate w = a'/(n a) and bound theta-degree by n-1), so cube-root integrals are decided directly; this example
; also exercises the general-n multiplication that reduces theta^n = a, theta^{n+r} = a theta^r.
(import "cas/rischtoweralgn.lisp")
(import "cas/rischintn.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define t1c (list (list (quote alg) 3 (rat-from-poly (list 0 1)))))   ; theta = x^(1/3), n = 3, a = x

(display "General algebraic extensions: cube root x^(1/3), theta^3 = x.") (newline) (newline)

(display "the general-n algebra reduces theta^3 = x and higher powers:") (newline)
(define th (list (rat-zero) (rat-one) (rat-zero)))
(define th2 (list (rat-zero) (rat-zero) (rat-one)))
(chk "theta * theta^2 = x (theta^3 reduces to a)" (rat-equal? (tn2-coeff 1 (tean-mul t1c 1 3 th th2) 0) (rat-from-poly (list 0 1))))
(chk "theta^2 * theta^2 = x theta (theta^4 reduces to a theta)" (rat-equal? (tn2-coeff 1 (tean-mul t1c 1 3 th2 th2) 1) (rat-from-poly (list 0 1))))
(chk "theta^3 = x via repeated multiplication" (rat-equal? (tn2-coeff 1 (tean-pow t1c 1 3 th 3) 0) (rat-from-poly (list 0 1))))

(display "cube-root integration through the recursion: INT (1/3) x^(-2/3) dx = x^(1/3):") (newline)
(define f (list (rat-zero) (rat-make (list 1) (list 0 3)) (rat-zero)))
(define r (te-integrate t1c 1 f))
(display "  y = ") (display (if (equal? (car r) (quote elementary)) (car (cdr r)) (quote NA))) (display "  (= x^(1/3))") (newline)
(chk "INT (1/3) x^(-2/3) = x^(1/3), certified" (if (equal? (car r) (quote elementary)) (te-int-certify t1c 1 f (car (cdr r))) #f))

(display "the diagonal rate w = a'/(3a) = 1/(3x) drives the derivation for any n:") (newline)
(chk "D(x^(1/3)) has theta-coefficient 1/(3x)" (rat-equal? (te-coeff 1 (te-deriv t1c 1 th) 1) (rat-make (list 1) (list 0 3))))

(newline)
(display "General-degree algebraic extensions complete: the diagonal derivation and integrator already handle any") (newline)
(display "n via the rate a'/(n a), and the general-n multiplication reduces theta^n = a -- cube roots and beyond.") (newline)
