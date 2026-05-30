;; 130-tactic-proofs.lisp
;;
;; Interactive tactic proving, LCF-style: tactics (intro / assumption / refl /
;; apply ...) *assemble* a proof term against a goal state; `qed` then has the
;; trusted kernel re-check the assembled term against the original goal, and
;; (qed name) stores the verified theorem as a reusable library lemma.  So a
;; bug in any tactic cannot mint a false theorem -- the kernel is the anchor.
;;
;; Self-checking: `must` raises on a wrong verdict (regression guard).

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'tactic-regression)))

(kernel-assume 'P '(Sort 0))
(kernel-assume 'Q '(Sort 0))
(kernel-assume 'p 'P)

;; Theorem 1: P -> P, by (intro x; assumption).  Stored as id-P.
(begin-proof '(Pi (x P) P))
(tactic-intro 'x)
(tactic-assumption)
(qed 'id-P)

;; Theorem 2: P -> Q -> P, by (intro x; intro y; assumption).  Stored.
(begin-proof '(Pi (x P) (Pi (y Q) P)))
(tactic-intro 'x)
(tactic-intro 'y)
(tactic-assumption)
(qed 'const-PQ)

;; Theorem 3: Id Nat 0 0, by reflexivity.  Stored.
(begin-proof '(Id Nat zero zero))
(tactic-refl)
(qed 'refl0)

(newline)
(display "Reusing the tactic-proved lemmas as library theorems:") (newline)

;; Each stored theorem is now a reusable, kernel-checked constant.
(must "id-P applied to p proves P"
      (kernel-check '(app id-P p) 'P))
(must "const-PQ applied to p proves Q -> P"
      (kernel-check '(app const-PQ p) '(Pi (y Q) P)))
(must "refl0 inhabits Id Nat 0 0"
      (kernel-check 'refl0 '(Id Nat zero zero)))

;; Soundness: a stored theorem only has the type it was proved at.
(must "id-P does NOT have type P -> Q (rejected)"
      (not (kernel-check 'id-P '(Pi (x P) Q))))

(newline)
(display "Interactive proofs verified by the kernel and reusable.") (newline)
