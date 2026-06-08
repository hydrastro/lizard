; The RECURSIVE COUPLED Risch differential equation: solves D y + F y = g at ARBITRARY height where F is an
; arbitrary tower element, by nesting the coupled banded recurrence recursively.  This is the piece that lets the
; height-n integrator SOLVE the exp-over-exp tower (and deeper) rather than deferring it (docs/TRAGER_ROADMAP.md,
; the summit).
;
; At an exponential level, D y + F y = g at theta-degree n is D(y_n) + (n Db + F_0) y_n = g_n - sum_{j>=1} F_j
; y_{n-j}, an RDE at height h-1 whose coefficient may itself be coupled -- solved recursively, bottoming at the
; rational RDE.  Certificate-gated for soundness: a returned y always satisfies D y + F y = g, a proven
; non-terminating tail yields 'no-solution, and an inconclusive bottom-up solve (a missed homogeneous freedom)
; is reported honestly rather than as a false verdict.
(import "cas/rischcrde.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define t1 (list (list (quote exp) (rat-from-poly (list 0 1)))))
(define t2 (list (list (quote exp) (rat-from-poly (list 0 1))) (list (quote exp) (list (rat-zero) (rat-one)))))

(display "The recursive coupled RDE D y + F y = g, F an arbitrary tower element, certificate-gated for soundness.") (newline) (newline)

(display "the coupled height-1 RDE c' + e^x c = 1 (the exp-over-exp top-degree subproblem) has no solution") (newline)
(display "  (a proven non-terminating tail), so it returns 'no-solution:") (newline)
(display "  ") (display (te-crde-solve t1 1 (list (rat-zero) (rat-one)) (list (rat-one)))) (newline)
(chk "c' + e^x c = 1 -> no-solution (proven non-terminating tail)" (equal? (te-crde-solve t1 1 (list (rat-zero) (rat-one)) (list (rat-one))) (quote no-solution)))

(display "a solvable coupled case D y + e^x y = e^x + e^{2x} recovers y = e^x, certified:") (newline)
(define ytest (list (rat-zero) (rat-one)))
(define gtest (te-add t1 1 (te-deriv t1 1 ytest) (te-mul t1 1 (list (rat-zero) (rat-one)) ytest)))
(define v2 (te-crde-solve t1 1 (list (rat-zero) (rat-one)) gtest))
(chk "solvable coupled case recovers y = e^x, certified" (if (cond ((equal? v2 (quote no-solution)) #f) ((equal? v2 (quote inconclusive)) #f) (else #t)) (te-crde-certify t1 1 (list (rat-zero) (rat-one)) gtest v2) #f))

(display "height 2: solving D y = e^{e^x} hits the coupled subproblem and proves INT e^{e^x} non-elementary:") (newline)
(chk "D y = e^{e^x} has no solution (INT e^{e^x} non-elementary, derived through the recursion)" (equal? (te-crde-solve t2 2 (te-zero 2) (list (te-zero 1) (te-one 1))) (quote no-solution)))

(display "height 0 reduces to the rational RDE (rischrde): D y + y = x gives y = x - 1:") (newline)
(chk "height 0 reduces to rischrde (y' + y = x -> x - 1)" (rat-equal? (te-crde-solve t1 0 (rat-from-poly (list 1)) (rat-from-poly (list 0 1))) (rat-from-poly (list -1 1))))

(newline)
(display "The recursive coupled RDE nests the banded recurrence at every height, bottoming at the rational RDE,") (newline)
(display "and is certificate-gated so a returned solution is always genuine -- the exp-over-exp tower now solved.") (newline)
