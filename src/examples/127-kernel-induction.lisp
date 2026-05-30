;; 127-kernel-induction.lisp
;;
;; Surface -> Core -> Kernel, end to end.  A term is written in concrete
;; surface syntax, converted to a trusted KernelTerm, and type-checked by the
;; kernel via (kernel-check term type), which returns #t only if the trusted
;; core accepts the term at that type.  De Bruijn variables are written
;; (var N) so they survive the reader.

(display "Dependent eliminators, checked end-to-end by the trusted kernel:")
(newline)

(display "  bool_rec (constant Nat motive)  : ")
(display (kernel-check
  '(boolrec (lam (b Bool) Nat) zero (succ zero) true)
  'Nat))
(newline)

(display "  nat_rec  (with induction hyp.)  : ")
(display (kernel-check
  '(natrec (lam (n Nat) Nat) zero
           (lam (k Nat) (lam (ih Nat) (succ (var 0))))
           (succ (succ zero)))
  'Nat))
(newline)

(display "  list_rec (length-shaped fold)   : ")
(display (kernel-check
  '(listrec (lam (xs (List Nat)) Nat) zero
            (lam (h Nat) (lam (t (List Nat)) (lam (ih Nat) (succ (var 0)))))
            (cons zero (nil Nat)))
  'Nat))
(newline)

(display "  sum_rec                         : ")
(display (kernel-check
  '(sumrec (lam (s (Sum Nat Bool)) Nat)
           (lam (a Nat) (var 0)) (lam (b Bool) zero)
           (inl zero Bool))
  'Nat))
(newline)

(display "  J (path induction)              : ")
(display (kernel-check
  '(J (lam (x Nat) (lam (y Nat) (lam (p (Id Nat (var 1) (var 0))) Nat)))
      (lam (x Nat) zero) Nat zero zero (refl zero))
  'Nat))
(newline)

(display "  absurd (ex falso)               : ")
(display (kernel-check '(absurd Nat) '(Pi (e Empty) Nat)))
(newline)
(newline)

;; The headline: a genuine proof BY INDUCTION of a dependent statement.
;;   motive C n := (Id Nat n n)         -- "for all n, n = n"
;;   base       := (refl zero)          : C zero
;;   step k ih  := (refl (succ k))      : C (succ k)
;; The nat_rec term inhabits C 2, and the kernel verifies it against that
;; dependent type: reflexivity proven by induction, checked by the core.
(display "Theorem (by induction): for all n, n = n.  Kernel verdict at n=2: ")
(display (kernel-check
  '(natrec (lam (n Nat) (Id Nat (var 0) (var 0)))
           (refl zero)
           (lam (k Nat) (lam (ih (Id Nat (var 0) (var 0))) (refl (succ (var 1)))))
           (succ (succ zero)))
  '(Id Nat (succ (succ zero)) (succ (succ zero)))))
(newline)
