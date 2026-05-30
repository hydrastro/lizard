;; 129-kernel-axioms.lisp
;;
;; Axioms (postulates): (kernel-assume name type) introduces a named opaque
;; constant of the given type with NO defining value -- an assumption the
;; kernel will trust.  References to the name elaborate to an opaque constant
;; the kernel types by its postulated type, so you can prove *from* axioms
;; (classical logic, function extensionality, domain hypotheses, ...).
;;
;; Self-checking: `must` raises on a wrong verdict, so this is a regression
;; guard as well as a demonstration.

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'axiom-regression)))

(display "Proving from postulates:") (newline)

;; Three opaque propositions and two implications between them.
(kernel-assume 'P '(Sort 0))
(kernel-assume 'Q '(Sort 0))
(kernel-assume 'R '(Sort 0))
(kernel-assume 'imp '(Pi (h P) Q))   ; axiom: P implies Q
(kernel-assume 'qr  '(Pi (h Q) R))   ; axiom: Q implies R
(kernel-assume 'p   'P)              ; axiom: a proof of P

;; Modus ponens: from imp : P -> Q and p : P, derive a proof of Q.
(must "modus ponens (imp p : Q)"
      (kernel-check '(app imp p) 'Q))

;; Transitivity of implication: compose imp and qr into a proof of P -> R.
(must "compose imp,qr : P -> R"
      (kernel-check '(lam (x P) (app qr (app imp x))) '(Pi (x P) R)))

;; Soundness survives postulation:
;;  - p proves P, not Q;
;;  - the composition only type-checks in the correct order.
(must "p does NOT prove Q (rejected)"
      (not (kernel-check 'p 'Q)))
(must "mis-ordered composition (rejected)"
      (not (kernel-check '(lam (x P) (app imp (app qr x))) '(Pi (x P) R))))

(newline)
(display "Axioms usable, soundness intact.") (newline)
