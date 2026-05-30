; -*- lisp -*-
; ============================================================
;  EXAMPLE 90 — Track K: propositions as types
; ============================================================

(import "proof.lisp")

(display "=== CURRY-HOWARD: propositions as types ===") (newline)
(newline)
(display "  Proposition      Type") (newline)
(display "  ───────────      ────") (newline)
(display "  True             Unit") (newline)
(display "  False            Empty") (newline)
(display "  A and B          (Sigma (x A) B)") (newline)
(display "  A or B           (Sum A B)") (newline)
(display "  A implies B      (Pi (x A) B)") (newline)
(display "  not A            (Pi (x A) Empty)") (newline)
(newline)

(display "=== Type-checking propositions ===") (newline)
(report-type "implies-self (A=>A)" implies-self)
(report-type "nat-and-bool (A and B)" nat-and-bool)
(report-type "nat-or-bool (A or B)" nat-or-bool)
(newline)

(display "=== Proofs (inhabitants) ===") (newline)
(display "  proof of A=>A is identity:") (newline)
(report-type "    id" proof-implies-self)
(display "  Does it prove A=>A? ")
(display (has-type? proof-implies-self implies-self)) (newline)
(newline)
(display "  proof of (A and B) is a pair:") (newline)
(report-type "    pair" proof-and)
(display "  Does it prove A and B? ")
(display (has-type? proof-and nat-and-bool)) (newline)
(newline)

(display "=== Verified combinators ===") (newline)
(install-combinators!)
(display "  idNat: ") (display (kernel-lookup 'idNat)) (newline)
(display "  mySucc: ") (display (kernel-lookup 'mySucc)) (newline)
(display "  idBool: ") (display (kernel-lookup 'idBool)) (newline)
(newline)

(display "=== Proof scripts ===") (newline)
(display "1. prove 0 = 0 by reflexivity:") (newline)
(prove-refl '(Id Nat 0 0))
(newline)

(display "2. prove Nat by exact witness 0:") (newline)
(prove-exact 'Nat 0)
(newline)

(display "3. prove (Sum Nat Bool) by left witness 0:") (newline)
(prove-left '(Sum Nat Bool) 0)
(newline)

(display "4. prove (Sum Nat Bool) by right witness true:") (newline)
(prove-right '(Sum Nat Bool) 'true)
(newline)

(display "=== Computation as proof ===") (newline)
(report-reduce "(if true 0 (succ 0))" '(if true 0 (succ 0)))
(report-reduce "(fst (pair 0 true))" '(fst (pair 0 true)))
(display "  Are they equal? ")
(display (defeq? '(if true 0 (succ 0)) '(fst (pair 0 true)))) (newline)
(newline)

(display "=== End of propositions-as-types ===") (newline)
