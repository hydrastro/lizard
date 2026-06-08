; 169-rt-tower.lisp — closing the algebraic-residue cases of integration over a
; primitive monomial (theta = log x) with in-tower Rothstein-Trager.
;
; INT (1/x) R(log x) dx becomes INT R(t) dt under t = log x.  The rational reducer
; handles rational residues, arctangents, and the polynomial/Hermite parts; it
; declines only when the residues are genuinely algebraic.  We then route the proper
; part through rt-log-part, whose answer is a RootSum over the algebraic residues and
; whose own certificate (a fully rational identity) verifies the logarithmic part
; exactly.  So the previously-deferred INT 1/(x(log^2 x - 2)) is now resolved -- and
; certified.  `must` raises on failure.

(import "cas/rt-tower.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'rt-tower-check-failed)))

(display "In-tower Rothstein-Trager: algebraic-residue integration") (newline) (newline)

(display "1. rational residues (already handled, still certified)") (newline)
(define r1 (int-tower-log (list 1) (list -1 0 1)))
(display "    INT 1/(x(log^2 x - 1)) = ") (display (tower-result->string r1)) (newline)
(must "resolved and certified" (and (equal? (car r1) 'ok) (tower-certified? r1)))
(define r2 (int-tower-log (list 1) (list 1 0 1)))
(display "    INT 1/(x(log^2 x + 1)) = ") (display (tower-result->string r2)) (newline)
(must "arctangent case certified" (and (equal? (car r2) 'ok) (tower-certified? r2)))
(newline)

(display "2. ALGEBRAIC residues -- the case the basic reducer declines") (newline)
(must "basic reducer declines INT 1/(x(log^2 x - 2))" (equal? (car (integrate-primitive-log (list 1) (list -2 0 1))) 'cannot))
(define r3 (int-tower-log (list 1) (list -2 0 1)))
(display "    INT 1/(x(log^2 x - 2)) = ") (display (tower-result->string r3)) (newline)
(must "now RESOLVED via Rothstein-Trager" (equal? (car r3) 'ok))
(must "and certified (exact rational identity)" (tower-certified? r3))
(must "the underlying log-part certificate holds" (rt-certificate (list 1) (list -2 0 1) (rt-log-part (list 1) (list -2 0 1))))
(newline)

(display "3. polynomial part + algebraic logarithm") (newline)
(define r4 (int-tower-log (list 0 0 1) (list -2 0 1)))
(display "    INT log^2 x/(x(log^2 x - 2)) = ") (display (tower-result->string r4)) (newline)
(must "resolved and certified" (and (equal? (car r4) 'ok) (tower-certified? r4)))
(newline)

(display "4. another algebraic case, INT 1/(x(log^2 x - 3))") (newline)
(define r5 (int-tower-log (list 1) (list -3 0 1)))
(display "    = ") (display (tower-result->string r5)) (newline)
(must "resolved and certified" (and (equal? (car r5) 'ok) (tower-certified? r5)))
(newline)

(display "all in-tower Rothstein-Trager checks passed.") (newline)
