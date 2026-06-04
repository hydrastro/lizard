; The THIRD-KIND ELEMENTARITY TEST via residues: for an integrand omega = a + b*y on y^2 = q, the rational part a
; must equal (1/2) N'/N for the norm N of g = A + B*sqrt(q), so the residues of 2a are the multiplicities of the
; zeros/poles of g and MUST be integers for the logarithmic part to be elementary with rational-coefficient g
; (docs/CAS.md -- summit S1, the decision half complementing the recognizer in elliptic3general).
;
; If INT omega = c*log(g) + algebraic, then a = (1/2) N'/N, so 2a = N'/N has integer residues (the orders of N).
; A non-integer residue of 2a is a sound certificate that no rational g exists; all-integer residues pass the
; necessary condition.  Residues at simple rational poles are num(r)/den'(r), exact over Q.
(import "cas/elliptic3residue.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Third-kind elementarity test: the residues of 2a must be integers for a rational-coefficient g.") (newline) (newline)

(display "a = x/(x^2-1): then 2a = 2x/(x^2-1) has residues 1, 1 at x = 1, -1 -- integers, so a third-kind log is") (newline)
(display "possible:") (newline)
(define a1 (rat-make (list 0 1) (list -1 0 1)))
(display "  residues of 2a: ") (display (e3r-residues-2a a1)) (newline)
(chk "the residues are integers" (e3r-all-integer-residues? a1))
(chk "the residue at x = 1 is exactly 1" (= (e3r-residue-at a1 1) 1))
(chk "a third-kind logarithmic part is possible" (e3r-third-kind-possible? (list a1 (rat-make (list) (list 1)))))

(display "a = 1/(x-2): then 2a = 2/(x-2) has residue 2 at x = 2 -- an integer:") (newline)
(define a3 (rat-make (list 1) (list -2 1)))
(chk "residue is the integer 2" (e3r-all-integer-residues? a3))

(display "soundness: a = (1/4)/(x-1) gives 2a = (1/2)/(x-1) with residue 1/2 -- NOT an integer, so NO rational g") (newline)
(display "exists and the test rejects it:") (newline)
(define a2 (rat-make (list (/ 1 4)) (list -1 1)))
(display "  residues of 2a: ") (display (e3r-residues-2a a2)) (newline)
(chk "the non-integer residue is rejected" (if (e3r-all-integer-residues? a2) #f #t))

(display "a = 0 has no poles and trivially passes:") (newline)
(chk "the zero rational part passes" (e3r-all-integer-residues? (rat-make (list) (list 1))))

(newline)
(display "The third-kind decision now has its residue half: the rational part of omega must have all-integer") (newline)
(display "residues in 2a for an elementary log with rational g, an exact necessary condition that complements the") (newline)
(display "recognizer.  Constructing g from a valid omega in full generality remains the Jacobian-torsion summit.") (newline)
