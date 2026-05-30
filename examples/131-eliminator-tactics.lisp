;; 131-eliminator-tactics.lisp
;;
;; Case analysis and induction as tactics.  `tactic-cases` and
;; `tactic-induction` act on the innermost hypothesis: they synthesise the
;; motive automatically, generate one subgoal per constructor (plus an
;; induction hypothesis for the recursive Nat case), assemble the eliminator
;; (bool_rec / nat_rec), and -- as always -- the kernel re-checks the finished
;; term at `qed`.
;;
;; The headline is boolean involution, not (not b) = b.  It cannot be closed
;; by reflexivity alone: with b a variable, not (not b) is stuck and is NOT
;; definitionally b.  Only after casing on b do the two sides compute.
;;
;; Self-checking: `must` raises on a wrong verdict (regression guard).

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'eliminator-regression)))

(kernel-define 'not
  '(lam (b Bool) (boolrec (lam (x Bool) Bool) false true b))
  '(Pi (b Bool) Bool))

;; Sanity: reflexivity alone canNOT prove involution (the term is stuck).
(must "involution is NOT provable by refl alone"
  (not (kernel-check '(lam (b Bool) (refl (app not (app not b))))
                     '(Pi (b Bool) (Id Bool (app not (app not b)) b)))))

;; Theorem 1: forall b. not (not b) = b, by case analysis.
(begin-proof '(Pi (b Bool) (Id Bool (app not (app not b)) b)))
(tactic-intro 'b)
(tactic-cases)        ; -> two subgoals: not(not true)=true, not(not false)=false
(tactic-refl)         ; true  branch (now computes to true = true)
(tactic-refl)         ; false branch
(qed 'not-involution)

;; Theorem 2: forall n. n = n over Nat, by induction (exercises nat_rec,
;; the base subgoal and the step subgoal with its induction hypothesis).
(begin-proof '(Pi (n Nat) (Id Nat n n)))
(tactic-intro 'n)
(tactic-induction)    ; -> base: 0 = 0 ; step: Pi k. (k=k) -> (succ k = succ k)
(tactic-refl)         ; base
(tactic-intro 'k)
(tactic-intro 'ih)
(tactic-refl)         ; step
(qed 'nat-id-refl)

(newline)
(display "Reusing the eliminator-tactic theorems:") (newline)

(kernel-assume 'x 'Bool)
(must "not-involution instantiated at x"
  (kernel-check '(app not-involution x)
                '(Id Bool (app not (app not x)) x)))
(must "nat-id-refl instantiated at (succ zero)"
  (kernel-check '(app nat-id-refl (succ zero))
                '(Id Nat (succ zero) (succ zero))))

;; Soundness: the stored theorem has exactly its proved type.
(must "not-involution does NOT prove not(not b)=true (rejected)"
  (not (kernel-check 'not-involution
                     '(Pi (b Bool) (Id Bool (app not (app not b)) true)))))

(newline)
(display "Case analysis and induction verified by the kernel.") (newline)
