; 197-higher-derivative-certificates.lisp -- kernel-certified higher-order derivatives.
;
; The proof-carrying differentiator emits, for d/dx e, a term inhabiting the trusted
; kernel judgment Der (\x. e) (\x. e').  Since e' is again a ring term, the differentiator
; iterates, and the whole chain f -> f' -> ... -> f^(k) is certified by having the kernel
; type-check every single-step proof.  The same machinery is a soundness witness: the
; kernel accepts exactly the term the differentiator produced and rejects every other
; claimed derivative -- the certificate has teeth.  `must` raises on failure.

(import "cas/diffn-cert.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'diffn-check-failed)))

(display "Kernel-certified higher-order derivatives") (newline) (newline)

(display "1. every step of the derivative chain is type-checked by the kernel") (newline)
(must "exp certified through order 3"        (nth-derivative-ok? (cas '(exp x)) 3))
(must "sin certified through order 3"        (certify-chain (cas '(sin x)) 3))
(must "cos certified through order 2"        (certify-chain (cas '(cos x)) 2))
(must "x*x certified through order 3"        (certify-chain (cas '(* x x)) 3))
(must "the chain rule sin(x*x) through order 2" (certify-chain (cas '(sin (* x x))) 2))
(newline)

(display "2. the kernel accepts the differentiator's own derivative term") (newline)
(must "exp: produced derivative accepted"   (true-derivative-accepted? (cas '(exp x))))
(must "x*x: produced derivative accepted"   (true-derivative-accepted? (cas '(* x x))))
(must "sin: produced derivative accepted"   (true-derivative-accepted? (cas '(sin x))))
(newline)

(display "3. soundness -- a WRONG derivative cannot be certified") (newline)
(must "x*x' = x is rejected"        (wrong-derivative-rejected? (cas '(* x x)) 'x))
(must "x*x' = oneR is rejected"     (wrong-derivative-rejected? (cas '(* x x)) 'oneR))
(must "x*x' = zeroR is rejected"    (wrong-derivative-rejected? (cas '(* x x)) 'zeroR))
(must "sin' = sin is rejected"      (wrong-derivative-rejected? (cas '(sin x)) (kapp 'sin 'x)))
(newline)

(display "4. rule-level soundness: the kernel distinguishes right from wrong rules") (newline)
(must "der_sin inhabits Der sin cos"        (kernel-check 'der_sin (Der 'sin 'cos)))
(must "der_sin does NOT inhabit Der sin sin" (not (kernel-check 'der_sin (Der 'sin 'sin))))
(must "der_exp inhabits Der exp exp"        (kernel-check 'der_exp (Der 'exp 'exp)))
(must "der_exp does NOT inhabit Der exp sin" (not (kernel-check 'der_exp (Der 'exp 'sin))))
(newline)

(display "all higher-derivative certificate checks passed.") (newline)
