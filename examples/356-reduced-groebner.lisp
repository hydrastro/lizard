; The REDUCED Groebner basis: the canonical, UNIQUE basis of an ideal for a fixed monomial order, obtained by
; fully inter-reducing a Groebner basis (docs/CAS.md -- summit S4, the canonical normal form atop the stronger
; engines).  Because the reduced basis is unique, equality of reduced bases is an exact ideal-equality test.
;
; From any Groebner basis: minimalize (drop elements whose leading term divides another's), then inter-reduce
; (replace each element by its normal form with respect to the others) and make monic.  Both steps reuse the
; trusted normal-form reduction, so the result generates the same ideal and is canonical.
(import "cas/groebner4.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The reduced Groebner basis: the canonical normal form of an ideal, and an exact ideal-equality test.") (newline) (newline)

(display "the circle meets the line: <x^2+y^2-1, x-y> reduces to the canonical {x - y, y^2 - 1/2}:") (newline)
(define f1 (list (cons 1 (list 2 0)) (cons 1 (list 0 2)) (cons -1 (list 0 0))))
(define f2 (list (cons 1 (list 1 0)) (cons -1 (list 0 1))))
(define R (gb4-reduced (list f1 f2)))
(display "  ") (display R) (newline)
(chk "the reduced basis is genuinely reduced (monic, fully inter-reduced)" (gb4-is-reduced? R))
(chk "it is still a valid Groebner basis" (groebner-ok? R))
(chk "re-reducing it changes nothing -- it is canonical" (gb4-set-equal? (gb4-reduce R) R))

(display "the raw Buchberger output is NOT reduced -- reduction is doing real work:") (newline)
(chk "the unreduced basis fails the reduced test" (if (gb4-is-reduced? (groebner (list f1 f2))) #f #t))

(display "ideal equality by canonical form: <x-y, x^2+y^2-1> and <x-y, 2x^2-1> are the SAME ideal:") (newline)
(define h1 (list (cons 1 (list 1 0)) (cons -1 (list 0 1))))     ; x - y
(define h2 (list (cons 2 (list 2 0)) (cons -1 (list 0 0))))     ; 2x^2 - 1  (circle with y=x substituted)
(chk "<x-y, x^2+y^2-1> = <x-y, 2x^2-1>" (gb4-ideal-equal? (list f2 f1) (list h1 h2)))

(display "and a genuinely different ideal is distinguished:") (newline)
(define h3 (list (cons 1 (list 2 0)) (cons 1 (list 0 2)) (cons -4 (list 0 0))))   ; circle radius 2
(chk "<x-y, x^2+y^2-1> is NOT <x-y, x^2+y^2-4>" (if (gb4-ideal-equal? (list f2 f1) (list h1 h3)) #f #t))

(newline)
(display "The reduced Groebner basis gives every ideal a canonical normal form: minimalized, inter-reduced, monic.") (newline)
(display "Two ideals are equal exactly when their reduced bases match, turning ideal equality into a finite check.") (newline)
(display "A true F4-style linear-algebra engine for the heaviest systems remains the open direction.") (newline)
