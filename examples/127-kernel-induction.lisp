;; 127-kernel-induction.lisp
;;
;; Surface -> Core -> Kernel, end to end, with NAMED variables.  Binders
;; introduce names and the converter resolves each name to its de Bruijn
;; index; the term is converted to a trusted KernelTerm and type-checked by
;; the kernel via (kernel-check term type), which returns #t only if the
;; trusted core accepts the term at that type.  (The raw (var N) form still
;; works as an escape hatch; named binders are just easier to read.)

(display "Dependent eliminators, checked end-to-end by the trusted kernel:")
(newline)

(display "  identity (lam (x Nat) x)        : ")
(display (kernel-check '(lam (x Nat) x) '(Pi (x Nat) Nat)))
(newline)

(display "  bool_rec (constant Nat motive)  : ")
(display (kernel-check
  '(boolrec (lam (b Bool) Nat) zero (succ zero) true) 'Nat))
(newline)

(display "  nat_rec  (with induction hyp.)  : ")
(display (kernel-check
  '(natrec (lam (n Nat) Nat) zero
           (lam (k Nat) (lam (ih Nat) (succ ih)))
           (succ (succ zero))) 'Nat))
(newline)

(display "  list_rec (length-shaped fold)   : ")
(display (kernel-check
  '(listrec (lam (xs (List Nat)) Nat) zero
            (lam (h Nat) (lam (t (List Nat)) (lam (ih Nat) (succ ih))))
            (cons zero (nil Nat))) 'Nat))
(newline)

(display "  sum_rec                         : ")
(display (kernel-check
  '(sumrec (lam (s (Sum Nat Bool)) Nat)
           (lam (a Nat) a) (lam (b Bool) zero)
           (inl zero Bool)) 'Nat))
(newline)

(display "  J (path induction, named)       : ")
(display (kernel-check
  '(J (lam (x Nat) (lam (y Nat) (lam (p (Id Nat x y)) Nat)))
      (lam (x Nat) zero) Nat zero zero (refl zero)) 'Nat))
(newline)

(display "  klet (let x = 0 in succ x)      : ")
(display (kernel-check '(klet (x zero) (succ x)) 'Nat))
(newline)

(display "  absurd (ex falso)               : ")
(display (kernel-check '(absurd Nat) '(Pi (e Empty) Nat)))
(newline)
(newline)

;; The headline: a genuine proof BY INDUCTION of a dependent statement,
;; readable thanks to named binders.
;;   motive C n := (Id Nat n n)     -- "for all n, n = n"
;;   base       := (refl zero)      : C zero
;;   step k ih  := (refl (succ k))  : C (succ k)
(display "Theorem (by induction): for all n, n = n.  Kernel verdict at n=2: ")
(display (kernel-check
  '(natrec (lam (n Nat) (Id Nat n n))
           (refl zero)
           (lam (k Nat) (lam (ih (Id Nat k k)) (refl (succ k))))
           (succ (succ zero)))
  '(Id Nat (succ (succ zero)) (succ (succ zero)))))
(newline)
