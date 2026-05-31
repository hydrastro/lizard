; 147-verified-integration.lisp — proof-carrying INTEGRATION.
;
; A computer algebra system normally asks you to TRUST that ∫f dx = F.  Here the
; answer comes with a machine-checkable proof.  The trick is the Fundamental
; Theorem of Calculus used backwards: "F is an antiderivative of f" is exactly
; the kernel's derivative judgment  Der F f.  So the (untrusted) integrator just
; has to FIND F; the SAME trusted differentiation rules that certify derivatives
; (der_sin, der_exp, der_add, ...) then certify the integral — with no new
; axioms.  Crucially, a WRONG antiderivative inhabits no such type, so the
; kernel rejects it (shown at the end).
;
; Self-checking: each `must` raises on failure (non-zero exit).

(import "cas/integral-cert.lisp" :as ic)

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'integral-check-failed)))

(display "verified integration (each result carries a kernel proof)") (newline)
(newline)
(display "Integrals the kernel ACCEPTS (the proof inhabits Der F f):") (newline)

; ---- the base table, every result kernel-checked ----
(must "/ 1 dx       = x       [der_id]"  (ic:verify 'one))
(must "/ cos x dx   = sin x   [der_sin]" (ic:verify 'cos))
(must "/ exp x dx   = exp x   [der_exp]" (ic:verify 'exp))
(must "/ (1/x) dx   = ln x    [der_ln]"  (ic:verify 'recip))
(must "/ (-sin x)dx = cos x   [der_cos]" (ic:verify 'negsin))

(newline)
(display "Linearity — sums integrate term by term (der_add glues proofs):") (newline)

(must "/ (cos + exp)            verified" (ic:verify '(+ cos exp)))
(must "/ (cos + exp + 1/x)      verified" (ic:verify '(+ cos (+ exp recip))))
(must "/ (1 + cos + exp)        verified" (ic:verify '(+ one (+ cos exp))))
(must "/ ((-sin) + (1/x))       verified" (ic:verify '(+ negsin recip)))

(newline)
(display "Non-triviality — the kernel REJECTS wrong antiderivatives:") (newline)

; Claim ∫cos = exp, offering exp's own derivative proof: type is Der exp exp,
; not Der exp cos, so it does not inhabit Der (claimed F) (integrand).
(must "wrong: / cos dx = exp    rejected" (not (ic:claim-holds? 'cos 'exp 'der_exp)))
(must "wrong: / exp dx = sin    rejected" (not (ic:claim-holds? 'exp 'sin 'der_sin)))
(must "wrong: / (1/x) dx = sin  rejected" (not (ic:claim-holds? 'recip 'sin 'der_sin)))

(newline)
(display "every accepted integral carried a proof the kernel checked;") (newline)
(display "verified-integration: all checks passed") (newline)
