; 196-quadratic-forms.lisp -- binary quadratic forms: reduction and class numbers.
;
; A form a x^2 + b xy + c y^2 is written (a b c) with discriminant b^2 - 4ac.  For D < 0
; and a > 0, Gauss reduction brings it to the unique equivalent reduced form, accumulating
; the SL2(Z) transformation M so the reduction carries its own proof.  The class number
; h(D) counts primitive reduced forms.  Reduction is certified four ways: discriminant
; invariance, the reduced predicate, det M = 1, and M actually mapping the form to its
; reduction.  `must` raises on failure.

(import "cas/quadforms.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'quadforms-check-failed)))

(display "Binary quadratic forms: reduction and class numbers") (newline) (newline)

(display "1. Gauss reduction, with a checked SL2 transformation") (newline)
(display "    3 4 5     -> ") (display (form->string (reduce-form (list 3 4 5)))) (newline)
(display "    10 34 29  -> ") (display (form->string (reduce-form (list 10 34 29)))) (display "  via M = ") (display (reduce-matrix (list 10 34 29))) (newline)
(display "    31 24 8   -> ") (display (form->string (reduce-form (list 31 24 8)))) (newline)
(must "3 4 5 reduces to 3 -2 4"     (equal? (reduce-form (list 3 4 5)) (list 3 -2 4)))
(must "discriminant is preserved"   (= (disc (reduce-form (list 3 4 5))) (disc (list 3 4 5))))
(must "the result is reduced"       (reduced? (reduce-form (list 3 4 5))))
(must "full reduction certificate for 3 4 5"    (reduce-ok? (list 3 4 5)))
(must "full reduction certificate for 10 34 29" (reduce-ok? (list 10 34 29)))
(must "full reduction certificate for 31 24 8"  (reduce-ok? (list 31 24 8)))
(newline)

(display "2. equivalent forms share a reduced representative") (newline)
(display "    1 0 1 and 10 34 29 both have discriminant ") (display (disc (list 1 0 1))) (newline)
(must "both reduce to the same form" (equal? (reduce-form (list 1 0 1)) (reduce-form (list 10 34 29))))
(must "a form already reduced is unchanged" (equal? (reduce-form (list 1 0 1)) (list 1 0 1)))
(newline)

(display "3. class numbers by enumeration") (newline)
(display "    h(-4) h(-23) h(-47) h(-163) = ")
(display (list (class-number -4) (class-number -23) (class-number -47) (class-number -163))) (newline)
(must "h(-3) = 1"   (= (class-number -3) 1))
(must "h(-4) = 1"   (= (class-number -4) 1))
(must "h(-15) = 2"  (= (class-number -15) 2))
(must "h(-23) = 3"  (= (class-number -23) 3))
(must "h(-47) = 5"  (= (class-number -47) 5))
(must "h(-71) = 7"  (= (class-number -71) 7))
(must "Heegner discriminant h(-163) = 1" (= (class-number -163) 1))
(newline)

(display "all quadratic-form checks passed.") (newline)
