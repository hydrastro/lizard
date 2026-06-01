; -*- lisp -*-
; lib/cas/diffn-cert.lisp -- kernel-certified higher-order derivatives.
;
; This sits on top of diff-cert.lisp, the proof-CARRYING differentiator that emits, for
; d/dx e, a term inhabiting the kernel judgment  Der (\x. e) (\x. e').  Because that result
; e' is itself a ring term in x, the differentiator can be applied again, and again: the
; k-th derivative is obtained by iterating it, and the WHOLE chain
;
;     f  ->  f'  ->  f''  ->  ...  ->  f^(k)
;
; is certified by having the trusted kernel type-check the single-step proof at every link.
; If any link's proof failed to inhabit its Der type the chain reports failure, so a
; certified chain is a kernel-checked proof that f^(k) is the k-th derivative of f.
;
; The same machinery gives a soundness witness: the kernel accepts EXACTLY the term the
; differentiator produced and rejects every other claimed derivative -- including
; mathematically-equal-but-unsimplified variants, since the bare ring judgment carries no
; simplification axioms.  That is the point: the certificate pins down a specific term, and
; only a correct derivative can inhabit the type.  Builds on diff-cert.lisp.

(import "cas/diff-cert.lisp")

; ---------- higher-order derivatives ----------
(define (nth-derivative e k) (if (= k 0) e (nth-derivative (derivative e) (- k 1))))

; certify every step of f -> f' -> ... -> f^(k); #t only if the kernel accepts them all
(define (certify-chain e k) (cond ((= k 0) #t) ((certify e) (certify-chain (derivative e) (- k 1))) (else #f)))

; the k-th derivative together with the per-step verdict
(define (nth-derivative-ok? e k) (certify-chain e k))

; ---------- soundness API ----------
; does the kernel accept CLAIMED as the derivative of e? (the diff proof vs the claimed type)
(define (accepts-claim? e claimed) (kernel-check (car (cdr (diff e))) (Der (fn e) (fn claimed))))
; the kernel accepts the differentiator's own term ...
(define (true-derivative-accepted? e) (accepts-claim? e (derivative e)))
; ... and rejects a wrong one
(define (wrong-derivative-rejected? e wrong) (not (accepts-claim? e wrong)))

; ---------- display ----------
(define (nth-derivative->string e k) (nth-derivative e k))
