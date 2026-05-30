;; 136-term-mode-proving.lisp
;;
;; Term-mode proving (Phase 7, TT3): a theorem is STATED in surface type theory
;; and PROVED by giving a term -- no tactics.  `(def name type term)` and
;; `(theorem name type term)` elaborate the term against the type (inserting
;; implicit arguments and solving holes by unification), report any open goals
;; with their local context, and on success kernel-check the elaborated term
;; and store it as a reusable delta-transparent definition.  The trusted kernel
;; re-checks every result, so the (untrusted) elaborator cannot make an unsound
;; statement go through.

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'term-mode-regression)))

;; ---- a definition that uses implicit arguments, then is reused ----
(display "Definition with implicit type arguments:") (newline)
(must "def comp : {A B C} (B->C) -> (A->B) -> A -> C"
  (def 'comp
    '(IPi (A (Sort 0)) (IPi (B (Sort 0)) (IPi (C (Sort 0))
       (Pi (g (Pi (y B) C)) (Pi (f (Pi (x A) B)) (Pi (x A) C))))))
    '(lam (A (Sort 0)) (lam (B (Sort 0)) (lam (C (Sort 0))
       (lam (g (Pi (y B) C)) (lam (f (Pi (x A) B)) (lam (x A) (app g (app f x))))))))))

;; ---- a theorem whose proof is a one-line term (implicit A inferred) ----
(display "Theorem proved as a term:") (newline)
(must "theorem refl-thm : {A} (x:A) -> Id A x x  :=  \\x. refl x"
  (theorem 'refl-thm
    '(IPi (A (Sort 0)) (Pi (x A) (Id A x x)))
    '(lam (A (Sort 0)) (lam (x A) (refl x)))))

;; ---- double negation on Bool, proved by the eliminator (no tactics) ----
(display "Proof by the eliminator (case analysis as a term):") (newline)
(kernel-define 'neg
  '(lam (b Bool) (boolrec (lam (c Bool) Bool) false true b))
  '(Pi (b Bool) Bool))
(must "theorem neg-neg : (b:Bool) -> Id Bool (neg (neg b)) b"
  (theorem 'neg-neg
    '(Pi (b Bool) (Id Bool (app neg (app neg b)) b))
    '(lam (b Bool)
       (boolrec (lam (c Bool) (Id Bool (app neg (app neg c)) c))
                (refl true) (refl false) b))))
;; reuse the stored theorem as a lemma applied to an argument
(must "reuse: neg-neg true : Id Bool (neg (neg true)) true"
  (theorem 'nn-true
    '(Id Bool (app neg (app neg true)) true)
    '(app neg-neg true)))

;; ---- a genuine induction, as a term: forall n, add2 n z2 = n ----
;; Recursion is on the first argument, so add2 n z2 does not compute on its own;
;; we eliminate on n.  The successor step is congruence (proved once via J).
(display "Induction-as-eliminator + congruence, as a term:") (newline)
(data '(Nat2 () (Sort 0) (z2 ()) (s2 (Nat2))))
(kernel-define 'add2
  '(lam (m Nat2) (lam (n Nat2)
     (app (app (app (app Nat2-rec (lam (x Nat2) Nat2)) n)
                (lam (k Nat2) (lam (ih Nat2) (app s2 ih)))) m)))
  '(Pi (m Nat2) (Pi (n Nat2) Nat2)))
(kernel-define 'cong
  '(lam (A (Sort 0)) (lam (B (Sort 0)) (lam (f (Pi (z A) B))
     (lam (x A) (lam (y A) (lam (p (Id A x y))
        (J (lam (x2 A) (lam (y2 A) (lam (q (Id A x2 y2)) (Id B (app f x2) (app f y2)))))
           (lam (x2 A) (refl (app f x2))) A x y p)))))))
  '(IPi (A (Sort 0)) (IPi (B (Sort 0)) (Pi (f (Pi (z A) B))
     (IPi (x A) (IPi (y A) (Pi (p (Id A x y)) (Id B (app f x) (app f y)))))))))
(must "theorem add2-right-zero : (n:Nat2) -> Id Nat2 (add2 n z2) n"
  (theorem 'add2-right-zero
    '(Pi (n Nat2) (Id Nat2 (app (app add2 n) z2) n))
    '(lam (n Nat2)
       (app (app (app (app Nat2-rec (lam (k Nat2) (Id Nat2 (app (app add2 k) z2) k)))
                  (refl z2))
             (lam (k Nat2) (lam (ih (Id Nat2 (app (app add2 k) z2) k))
                              (app (app cong s2) ih))))
          n))))

;; ---- an incomplete proof reports its open goal instead of being accepted ----
(display "An open goal is reported, not accepted:") (newline)
(must "theorem with a hole leaves a goal (returns #f)"
  (not (theorem 'unfinished
         '(Pi (b Bool) (Id Bool (app neg (app neg b)) b))
         '(lam (b Bool) ?0))))

(newline)
(display "Theorems are stated in surface TT, proved by terms, and kernel-checked.")
(newline)
