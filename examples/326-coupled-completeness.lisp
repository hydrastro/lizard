; The COUPLED-RDE COMPLETENESS layer: the homogeneous-constant bookkeeping that lets SOLVABLE coupled cases --
; which the sound-but-incomplete recursive coupled solver (rischcrde) reports as inconclusive -- actually be
; SOLVED.  This recovers integrals like INT (e^x e^{e^x}) = e^{e^x} through the recursion, closing the gap left
; open by rischcrde while preserving soundness (docs/TRAGER_ROADMAP.md, the summit).
;
; In the exp-level banded recurrence, a degree whose RDE coefficient vanishes (the degree-0 homogeneous case
; D(y_0) = RHS) leaves the solution free up to an additive constant; the no-constant branch can force a spurious
; non-terminating tail even when a specific constant terminates it.  The tail depends linearly on that constant,
; so it is determined by two probe solves and a linear solve, then re-run and CERTIFIED -- a solution is returned
; only if it satisfies D y + F y = g, so soundness is preserved and the one-parameter fix falls back to an honest
; inconclusive when it does not certify.
(import "cas/rischcrdeh.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define t1 (list (list (quote exp) (rat-from-poly (list 0 1)))))
(define t2 (list (list (quote exp) (rat-from-poly (list 0 1))) (list (quote exp) (list (rat-zero) (rat-one)))))

(display "The coupled-RDE completeness layer: solvable coupled cases solved via homogeneous-constant bookkeeping.") (newline) (newline)

(display "the headline -- INT (e^x e^{e^x}) dx = e^{e^x}, now SOLVED through the recursion (was deferred):") (newline)
(define f2 (list (te-zero 1) (list (rat-zero) (rat-one))))
(define r (te-crdeh-integrate t2 2 f2))
(display "  result: ") (display (car r)) (display ",  y = ") (display (if (equal? (car r) (quote elementary)) (car (cdr r)) (quote NA))) (display "  (= e^{e^x})") (newline)
(chk "INT (e^x e^{e^x}) = e^{e^x} solved and certified" (if (equal? (car r) (quote elementary)) (te-crde-certify t2 2 (te-zero 2) f2 (car (cdr r))) #f))

(display "the height-1 subproblem the fix turns on -- D(c) + e^x c = e^x has the bounded solution c = 1:") (newline)
(define c (te-crdeh-solve t1 1 (list (rat-zero) (rat-one)) (list (rat-zero) (rat-one))))
(chk "D(c) + e^x c = e^x solved as c = 1, certified" (if (cond ((equal? c (quote no-solution)) #f) ((equal? c (quote inconclusive)) #f) (else #t)) (te-crde-certify t1 1 (list (rat-zero) (rat-one)) (list (rat-zero) (rat-one)) c) #f))

(display "soundness preserved -- INT e^{e^x} dx is STILL proven non-elementary (the genuine obstruction):") (newline)
(chk "INT e^{e^x} still non-elementary" (equal? (car (te-crdeh-integrate t2 2 (list (te-zero 1) (te-one 1)))) (quote non-elementary)))

(display "and the height-1 verdicts are unchanged:") (newline)
(define re1 (te-crdeh-integrate t1 1 (list (rat-zero) (rat-one))))
(chk "INT e^x = e^x, certified" (if (equal? (car re1) (quote elementary)) (te-crde-certify t1 1 (te-zero 1) (list (rat-zero) (rat-one)) (car (cdr re1))) #f))
(chk "INT e^x/x still non-elementary" (equal? (car (te-crdeh-integrate t1 1 (list (rat-zero) (rat-make (list 1) (list 0 1))))) (quote non-elementary)))

(newline)
(display "The completeness layer solves the degree-0 homogeneous freedom by a linear constant-solve, certified, so") (newline)
(display "solvable coupled integrals like INT (e^x e^{e^x}) = e^{e^x} are now found through the recursion -- while") (newline)
(display "the genuine non-elementary obstructions (INT e^{e^x}, INT e^x/x) are preserved exactly.") (newline)
