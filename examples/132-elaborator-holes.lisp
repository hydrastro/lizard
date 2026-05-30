;; 132-elaborator-holes.lisp
;;
;; Term-first proving with holes (Phase 7 TT1/TT3).  You write a proof *term*
;; with holes (?0, ?1, ...) for the parts you have not filled in yet, and the
;; elaborator pushes the expected type inward so each hole learns its goal (the
;; type it must inhabit) and the hypotheses in scope.  Fill every hole and the
;; finished term is checked by the trusted kernel -- no tactics involved.
;;
;; `check-holes` prints the open goals and returns #t only when the term is
;; complete AND kernel-verified; it returns #f when holes remain or the term
;; does not fit its goal.  Self-checking via `must`.

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'elaborator-regression)))

(kernel-define 'not
  '(lam (b Bool) (boolrec (lam (x Bool) Bool) false true b))
  '(Pi (b Bool) Bool))

;; The proof skeleton for boolean involution, with the two branch proofs left
;; as holes.  The elaborator reports the goal of each hole (printed above).
(display "Proof skeleton for not (not b) = b -- open goals:") (newline)
(must "skeleton elaborates, but two holes remain"
  (not (check-holes
    '(lam (b Bool)
       (boolrec (lam (c Bool) (Id Bool (app not (app not c)) c)) ?0 ?1 b))
    '(Pi (b Bool) (Id Bool (app not (app not b)) b)))))

;; Fill both holes: the term is now complete and the kernel verifies it.
(display "Filled in:") (newline)
(must "filled proof is complete and kernel-verified"
  (check-holes
    '(lam (b Bool)
       (boolrec (lam (c Bool) (Id Bool (app not (app not c)) c))
                (refl true) (refl false) b))
    '(Pi (b Bool) (Id Bool (app not (app not b)) b))))

;; A wrong fill (false branch in the true slot) must be rejected.
(display "Wrong fill:") (newline)
(must "a wrong branch proof is rejected"
  (not (check-holes
    '(lam (b Bool)
       (boolrec (lam (c Bool) (Id Bool (app not (app not c)) c))
                (refl false) (refl false) b))
    '(Pi (b Bool) (Id Bool (app not (app not b)) b)))))

;; A hole in argument position learns its goal from the function's domain.
(kernel-assume 'A '(Sort 0))
(kernel-assume 'f '(Pi (x A) A))
(kernel-assume 'a 'A)
(display "Hole in argument position:") (newline)
(must "(f ?0) : A leaves a hole of goal A"
  (not (check-holes '(app f ?0) 'A)))
(must "(f a) : A is complete and verified"
  (check-holes '(app f a) 'A))

(newline)
(display "Holes report their goals; completed terms are kernel-checked.")
(newline)
