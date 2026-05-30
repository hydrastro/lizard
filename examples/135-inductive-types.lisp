;; 135-inductive-types.lisp
;;
;; User-defined inductive types (Phase 7).  `(data ...)` declares a
;; parameterized inductive: the kernel strict-positivity-checks it and
;; registers the type former, the constructors, and an automatically generated
;; dependent eliminator `Name-rec` with a real iota-reduction rule.  No new
;; kernel term tags are involved -- the type former and constructors are
;; ordinary constants, the eliminator computes by iota -- so the whole feature
;; rests on just two trusted pieces: the positivity check and the iota rule.
;;
;; Self-checking via `must`.

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'inductive-regression)))

;; ---- a recursive inductive: Nat2 = z2 | s2 Nat2 ----
(display "Recursive inductive with a computing eliminator:") (newline)
(must "data Nat2 is accepted"
  (data '(Nat2 () (Sort 0) (z2 ()) (s2 (Nat2)))))

;; addition by recursion on the first argument
(kernel-define 'add2
  '(lam (m Nat2) (lam (n Nat2)
     (app (app (app (app Nat2-rec (lam (x Nat2) Nat2)) n)
                (lam (k Nat2) (lam (ih Nat2) (app s2 ih)))) m)))
  '(Pi (m Nat2) (Pi (n Nat2) Nat2)))
(must "the eliminator iota-reduces: add2 (s2 z2) (s2 z2) = s2 (s2 z2)"
  (kernel-check '(refl (app s2 (app s2 z2)))
    '(Id Nat2 (app (app add2 (app s2 z2)) (app s2 z2)) (app s2 (app s2 z2)))))
(must "base case computes: add2 z2 (s2 z2) = s2 z2"
  (kernel-check '(refl (app s2 z2))
    '(Id Nat2 (app (app add2 z2) (app s2 z2)) (app s2 z2))))

;; ---- an enum: no recursion, three nullary constructors ----
(display "Finite (enumeration) inductive:") (newline)
(must "data Color is accepted"
  (data '(Color () (Sort 0) (red ()) (green ()) (blue ()))))
(kernel-define 'isred
  '(lam (c Color)
     (app (app (app (app (app Color-rec (lam (x Color) Bool)) true) false) false) c))
  '(Pi (c Color) Bool))
(must "case analysis computes: isred green = false"
  (kernel-check '(refl false) '(Id Bool (app isred green) false)))

;; ---- a parameterized inductive with binary recursion: Bush A ----
(display "Parameterized inductive with two recursive subterms:") (newline)
(must "data Bush A is accepted"
  (data '(Bush ((A (Sort 0))) (Sort 0)
           (leaf ())
           (fork ((app Bush A) A (app Bush A))))))
(kernel-define 'natplus
  '(lam (m Nat) (lam (n Nat)
     (natrec (lam (x Nat) Nat) n (lam (k Nat) (lam (ih Nat) (succ ih))) m)))
  '(Pi (m Nat) (Pi (n Nat) Nat)))
;; size : (A : Type) -> Bush A -> Nat   (leaf = 1; fork l _ r = 1 + size l + size r)
(kernel-define 'bsize
  '(lam (A (Sort 0)) (lam (t (app Bush A))
     (app (app (app (app (app Bush-rec A) (lam (x (app Bush A)) Nat))
                (succ zero))
           (lam (l (app Bush A)) (lam (x A) (lam (r (app Bush A))
              (lam (ihl Nat) (lam (ihr Nat) (succ (app (app natplus ihl) ihr))))))))
        t)))
  '(Pi (A (Sort 0)) (Pi (t (app Bush A)) Nat)))
(must "binary recursion computes: size (fork leaf 0 leaf) = 3"
  (kernel-check '(refl (succ (succ (succ zero))))
    '(Id Nat (app (app bsize Nat)
                   (app (app (app (app fork Nat) (app leaf Nat)) zero) (app leaf Nat)))
             (succ (succ (succ zero))))))

;; ---- soundness: a negative (non-strictly-positive) occurrence is refused ----
(display "Strict positivity is enforced (soundness):") (newline)
(must "data Bad = mkBad (Bad -> Empty) is REJECTED"
  (not (data '(Bad () (Sort 0) (mkBad ((Pi (x Bad) Empty)))))))

(newline)
(display "Inductives are declared, their eliminators compute, positivity holds.")
(newline)
