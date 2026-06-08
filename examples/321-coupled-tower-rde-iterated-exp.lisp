; The COUPLED tower-field Risch differential equation: solves/decides c' + (m theta_1') c = target over
; K_1 = Q(x)(theta_1), theta_1 = exp(x), where the coefficient m theta_1' has POSITIVE theta_1-degree (since
; theta_1' = theta_1 = e^x).  A positive-degree coefficient COUPLES the theta_1-degrees into a banded recurrence;
; this module solves it and DETECTS the non-terminating degree tail that proves non-elementarity of the iterated
; exponential -- deriving INT exp(exp x) non-elementary through the actual RDE recursion, not asserting it
; (docs/TRAGER_ROADMAP.md, the summit, "beyond").
;
; Writing c = sum c_k s^k (s = e^x), the equation c' + (m s) c = target reads, per degree, c_0' = t_0 and
; c_k' + k c_k = t_k - m c_{k-1}; a nonzero c_{k-1} forces a nonzero right-hand side at degree k, so the solution
; can spill to ever-higher degrees.  Solving up to a bound: if the tail vanishes the solution is genuine;
; if the forced tail persists, the equation has no bounded-degree solution -- the obstruction, hence non-elementary.
(import "cas/rischtfrde2.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The coupled tower-field RDE: a positive-degree coefficient couples theta-degrees into a banded recurrence.") (newline) (newline)

(display "INT exp(exp x) dx reduces to c' + (s) c = 1 over Q(x)(e^x) -- and that has a NON-TERMINATING tail:") (newline)
(display "  c_0 = x (from c_0' = 1):           ") (display (ctf-step (rat-one) (rat-one) (rat-zero) 0)) (newline)
(display "  c_1 = 1 - x (from c_1' + c_1 = -c_0): ") (display (ctf-step (rat-one) (rat-zero) (rat-from-poly (list 0 1)) 1)) (newline)
(display "  ... each c_k forces a nonzero c_{k+1}, so no bounded-degree solution exists.") (newline)
(define v1 (ctf-solve (rat-one) (list (rat-one)) 4))
(display "  verdict: ") (display v1) (newline)
(chk "c' + (s) c = 1 has a non-terminating tail" (equal? (car v1) (quote non-elementary)))

(display "so INT exp(exp x) dx is PROVEN non-elementary -- derived through the RDE recursion:") (newline)
(display "  ") (display (ctf-decide-int-E2) ) (newline)
(chk "INT exp(exp x) dx PROVEN non-elementary through the tower-field RDE machinery" (equal? (car (ctf-decide-int-E2)) (quote non-elementary)))

(display "a terminating control -- c' + (s) c = 0 has the genuine bounded solution c = 0:") (newline)
(define v0 (ctf-solve (rat-one) (list (rat-zero)) 4))
(chk "c' + (s) c = 0 terminates: solvable (c = 0)" (equal? (car v0) (quote solvable)))

(newline)
(display "The coupled tower-field RDE solves the banded recurrence across theta-degrees and detects the") (newline)
(display "non-terminating tail -- so INT exp(exp x) is proven non-elementary by the recursion itself, the same") (newline)
(display "verdict the tower decider records, now derived from the differential-equation machinery underneath.") (newline)
