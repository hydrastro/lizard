;; 133-elaborator-unification.lisp
;;
;; Unification-driven elaboration (Phase 7, TT1).  The elaborator now threads a
;; metavariable context and unifies the inferred type against the expected type
;; while checking.  A hole whose value is *determined* by its surroundings is
;; solved automatically; a hole that represents a genuine free choice (a proof
;; obligation) stays open and is reported.  Either way the finished, zonked
;; term is handed to the trusted kernel.
;;
;; `check-holes` returns #t only when no open holes remain AND the kernel
;; verifies the elaborated term.  Self-checking via `must`.

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'unification-regression)))

;; A determined hole: (refl ?0) : Id Nat 2 2 forces ?0 := 2.
(display "Inference fills determined holes:") (newline)
(must "(refl ?0) : Id Nat 2 2  solves ?0 and verifies"
  (check-holes '(refl ?0)
               '(Id Nat (succ (succ zero)) (succ (succ zero)))))

;; The same hole appears twice (refl x : Id _ x x) and must be solved
;; consistently.
(must "(refl ?0) : Id Bool true true  solves ?0 := true"
  (check-holes '(refl ?0) '(Id Bool true true)))

;; An argument inferred from the result type.  refl-all : Pi n. Id Nat n n;
;; (refl-all ?0) : Id Nat 2 2 forces ?0 := 2 through the return type.
(kernel-define 'refl-all '(lam (n Nat) (refl n)) '(Pi (n Nat) (Id Nat n n)))
(must "(refl-all ?0) : Id Nat 2 2  infers the argument ?0 := 2"
  (check-holes '(app refl-all ?0)
               '(Id Nat (succ (succ zero)) (succ (succ zero)))))

;; A genuine proof obligation is NOT invented: the two branches of the
;; involution proof stay open (the elaborator reports them, returns #f).
(kernel-define 'not '(lam (b Bool) (boolrec (lam (x Bool) Bool) false true b))
                    '(Pi (b Bool) Bool))
(display "Genuine obligations stay open (not invented):") (newline)
(must "involution skeleton leaves its two branches open"
  (not (check-holes
    '(lam (b Bool)
       (boolrec (lam (c Bool) (Id Bool (app not (app not c)) c)) ?0 ?1 b))
    '(Pi (b Bool) (Id Bool (app not (app not b)) b)))))

;; Inconsistent constraints are rejected, not papered over.
(must "(refl ?0) : Id Nat 1 2  is rejected (no consistent ?0)"
  (not (check-holes '(refl ?0)
                    '(Id Nat (succ zero) (succ (succ zero))))))

(newline)
(display "Determined holes are inferred; free obligations are reported.")
(newline)
