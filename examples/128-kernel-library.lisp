;; 128-kernel-library.lisp
;;
;; A kernel-checked proof *library*: define lemmas and constants by name with
;; (kernel-define name term type) — which type-checks the term against the
;; type in the trusted kernel before storing it — then *reference* those names
;; in later proofs.  The surface converter resolves a free name to its stored
;; (closed) definition, so a library of reusable, verified lemmas builds up.
;;
;; Self-checking: `must` raises (failing the example) if a verdict is wrong,
;; so this doubles as a regression guard.

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'proof-library-regression)))

(display "Building a kernel-checked proof library:") (newline)

;; A constant and a universally-quantified lemma, each verified on definition.
(kernel-define 'two '(succ (succ zero)) 'Nat)
(kernel-define 'refl-all '(lam (n Nat) (refl n)) '(Pi (n Nat) (Id Nat n n)))

;; Reference the library names 'refl-all' and 'two' in a fresh proof:
;; applying the lemma to the constant yields a proof of (Id Nat two two).
(must "apply lemma refl-all to two"
      (kernel-check '(app refl-all two) '(Id Nat two two)))

;; A new definition built *on top of* earlier ones, then reused downstream.
(kernel-define 'two-eq '(app refl-all two) '(Id Nat two two))
(must "downstream lemma two-eq"
      (kernel-check 'two-eq '(Id Nat two two)))

;; Soundness survives the library: the lemma cannot prove a false equation,
;; so the kernel must REJECT this (we assert the rejection).
(must "refl-all cannot prove 2 = 0 (rejected)"
      (not (kernel-check '(app refl-all two) '(Id Nat two zero))))

;; Definitions are *abstract constants* yet definitionally equal to their
;; bodies: proving (Id Nat two (succ (succ zero))) requires the kernel to
;; delta-unfold the constant 'two' to its definition.
(must "two delta-unfolds to (succ (succ zero))"
      (kernel-check '(refl two) '(Id Nat two (succ (succ zero)))))
(must "but two is not (succ zero) (rejected)"
      (not (kernel-check '(refl two) '(Id Nat two (succ zero)))))

(newline)
(display "Proof library verified by the trusted kernel.") (newline)
