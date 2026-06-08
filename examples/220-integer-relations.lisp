; 220-integer-relations.lisp -- finding integer relations among rationals via LLL.
;
; An integer relation among x_1,...,x_n is a nonzero integer vector a with a.x = 0.  The
; problem is solved by lattice reduction: the rows (e_i | C*x_i) span a lattice whose short
; vectors (a, C*(a.x)) force a.x toward zero, so LLL surfaces the relation in the identity
; block.  The method is self-certifying -- only an a with a.x = 0 exactly is returned -- so
; every result here is checked against the exact rationals.  `must` raises on failure.

(import "cas/intrel.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'ir-check-failed)))

(display "integer relation detection") (newline) (newline)

(display "1. a relation among (1/2, 1/3, 1/6)") (newline)
(define x1 (list (/ 1 2) (/ 1 3) (/ 1 6)))
(define r1 (ir-relation x1))
(display "    relation a = ") (display r1) (display ", and a . x = ") (display (vdot r1 x1)) (newline)
(must "the relation is exact (a . x = 0) and nonzero" (ir-relation-ok? x1))
(newline)

(display "2. recognizing a rational: among (1, 3/7) the relation encodes 3/7") (newline)
(define x2 (list 1 (/ 3 7)))
(define r2 (ir-relation x2))
(display "    relation a = ") (display r2) (display "  (so a1*1 + a2*(3/7) = 0)") (newline)
(must "the rational is recovered exactly" (ir-relation-ok? x2))
(newline)

(display "3. a relation among the integers (3, 5, 7)") (newline)
(define r3 (ir-relation (list 3 5 7)))
(display "    relation a = ") (display r3) (display ", and a . (3,5,7) = ") (display (vdot r3 (list 3 5 7))) (newline)
(must "exact integer relation found" (ir-relation-ok? (list 3 5 7)))
(must "a longer relation among (2/3, 4/5, 6/7, 8/9)" (ir-relation-ok? (list (/ 2 3) (/ 4 5) (/ 6 7) (/ 8 9))))
(newline)

(display "4. the certificate rejects a non-relation") (newline)
(must "(2,-3) is a valid relation for (3,2)"      (ir-verify (list 2 -3) (list 3 2)))
(must "(1,1) is correctly rejected for (3,2)"     (not (ir-verify (list 1 1) (list 3 2))))
(newline)

(display "all integer-relation checks passed.") (newline)
