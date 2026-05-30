;; 134-implicit-arguments.lisp
;;
;; Implicit arguments (Phase 7, TT1).  A function type may mark leading binders
;; implicit with `IPi` ({A : Type} -> ...).  When such a function is applied,
;; the elaborator inserts a fresh metavariable for each implicit binder and
;; solves it by unification against the explicit argument (and the result
;; type).  `(elaborate term type)` shows the resulting core term -- with the
;; implicit arguments filled in -- and confirms it with the trusted kernel.
;;
;; Self-checking via `must`: `elaborate` returns #t only when the elaborated
;; term kernel-checks.

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'implicit-regression)))

;; id : {A : Type} -> A -> A
(kernel-define 'id '(lam (A (Sort 0)) (lam (x A) x))
                   '(IPi (A (Sort 0)) (Pi (x A) A)))

(display "Implicit type argument inferred from the value argument:") (newline)
(must "(id (succ zero)) : Nat   inserts A := Nat"
  (elaborate '(app id (succ zero)) 'Nat))
(must "(id true) : Bool         inserts A := Bool"
  (elaborate '(app id true) 'Bool))

;; const : {A B : Type} -> A -> B -> A   (two implicits)
(kernel-define 'const2
  '(lam (A (Sort 0)) (lam (B (Sort 0)) (lam (x A) (lam (y B) x))))
  '(IPi (A (Sort 0)) (IPi (B (Sort 0)) (Pi (x A) (Pi (y B) A)))))

(display "Both implicits inferred:") (newline)
(must "(const2 true (succ zero)) : Bool  inserts A := Bool, B := Nat"
  (elaborate '(app (app const2 true) (succ zero)) 'Bool))

;; Soundness: the inferred implicit must agree with the goal.  Here A is forced
;; to Bool by the argument `true`, so the result cannot be Nat.
(display "An inconsistent goal is rejected, not forced:") (newline)
(must "(id true) : Nat is rejected (A := Bool, Bool != Nat)"
  (not (elaborate '(app id true) 'Nat)))

;; Implicit insertion composes with a hole: the explicit argument is left open
;; while the implicit is still inferred from the goal.
(display "Implicit inferred, explicit left as a hole:") (newline)
(must "(id ?0) : Nat leaves ?0 open (and infers A := Nat)"
  (not (check-holes '(app id ?0) 'Nat)))

(newline)
(display "Implicit arguments are inferred and the terms kernel-checked.")
(newline)
