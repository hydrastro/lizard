; -*- lisp -*-
; examples/139-cas-certificates.lisp
;
; Phase 8 — proof-CARRYING differentiation.  The symbolic differentiator in
; lib/cas/diff-cert.lisp emits, alongside each derivative, a CERTIFICATE that
; the trusted kernel type-checks.  The differentiation rules are postulated as
; constructors of a judgment  Der f g  ("g is the derivative of f"); a
; derivative's proof is a nested application of those rules, and the kernel
; verifies it inhabits  Der (\x. f) (\x. f').  Because the proof must literally
; have that type, a WRONG derivative cannot be certified — the kernel rejects it.
;
; Self-checking: each `must` raises if its claim is false, so a clean exit means
; every certificate the differentiator produced was accepted (and every bogus
; one refused).

(import "cas/diff-cert.lisp" :as dc)

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'cas-certificate-regression)))

(newline)
(display "Derivatives whose certificate the kernel ACCEPTS:") (newline)

;; d/dx x = 1                                          (der_id)
(must "d/dx(x) = 1                certified" (dc:certify (dc:cas 'x)))
;; d/dx a = 0   — a is a constant (no free x)          (der_const)
(must "d/dx(a) = 0                certified" (dc:certify (dc:cas 'a)))
;; d/dx (x*x) = 1*x + x*1                              (der_mul)
(must "d/dx(x*x) = 1*x + x*1      certified" (dc:certify (dc:cas '(* x x))))
;; d/dx (x*x*x)                                        (der_mul, nested)
(must "d/dx(x*x*x)                certified" (dc:certify (dc:cas '(* (* x x) x))))
;; d/dx (x*x + a)                                      (der_add + der_mul + der_const)
(must "d/dx(x*x + a)              certified" (dc:certify (dc:cas '(+ (* x x) a))))
;; d/dx ((x + a) * x)                                  (der_mul over der_add)
(must "d/dx((x + a)*x)            certified" (dc:certify (dc:cas '(* (+ x a) x))))

(newline)
(display "Elementary functions and the chain rule:") (newline)

;; base rules
(must "d/dx(sin x) = cos x        certified" (dc:certify (dc:cas '(sin x))))
(must "d/dx(cos x) = -sin x       certified" (dc:certify (dc:cas '(cos x))))
(must "d/dx(exp x) = exp x        certified" (dc:certify (dc:cas '(exp x))))
(must "d/dx(ln x)  = 1/x          certified" (dc:certify (dc:cas '(ln x))))
;; chain rule:  (f∘g)' = f'(g)·g'
(must "d/dx(sin(x*x))   [chain]   certified" (dc:certify (dc:cas '(sin (* x x)))))
(must "d/dx(exp(x*x+a)) [chain]   certified" (dc:certify (dc:cas '(exp (+ (* x x) a)))))
(must "d/dx(ln(sin x))  [chain^2] certified" (dc:certify (dc:cas '(ln (sin x)))))
(must "d/dx(x*sin x)    [prod+ch] certified" (dc:certify (dc:cas '(* x (sin x)))))

(newline)
(display "Non-triviality — the kernel REJECTS wrong derivatives:") (newline)

;; Claim d/dx(x*x) = x.  The (correct) proof term does not inhabit that type.
(must "wrong: d/dx(x*x) = x       rejected"
      (not (kernel-check (car (cdr (dc:diff (dc:cas '(* x x)))))
                         (dc:Der (dc:fn (dc:cas '(* x x))) (dc:fn 'x)))))
;; Claim d/dx(x) = 0  (it is 1).
(must "wrong: d/dx(x) = 0         rejected"
      (not (kernel-check (car (cdr (dc:diff 'x)))
                         (dc:Der (dc:fn 'x) (dc:fn 'zeroR)))))

(newline)
(display "every derivative carried a proof the kernel accepted") (newline)
