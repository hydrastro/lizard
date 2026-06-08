; The LIOUVILLE STRUCTURE THEOREM made explicit for rational functions.  Liouville: INT f dx is elementary iff
; f = v' + sum_i c_i u_i'/u_i with v, u_i in the base field and c_i constants.  For a rational f this module
; returns that decomposition as an explicit witness and certifies it (docs/TRAGER_ROADMAP.md, the summit) -- the
; structure form is the certificate of HOW the integral is elementary.
;
; For f = N/D with D squarefree, deg N < deg D, and given simple roots a_i of D:
;   f = sum_i res_i/(x - a_i),  res_i = N(a_i)/D'(a_i),
; i.e. v = 0, c_i = res_i, u_i = x - a_i, with antiderivative sum res_i log(x - a_i).
(import "cas/liouvilleform.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The Liouville structure theorem for rational f: the explicit decomposition f = v' + sum c_i u_i'/u_i.") (newline) (newline)

(display "f = 1/(x^2 - 1): the residues at x = 1 and x = -1 give the log decomposition:") (newline)
(display "  Liouville form: ") (display (lf-form (list 1) (list -1 0 1) (list 1 -1))) (newline)
(display "  antiderivative: ") (display (lf-antiderivative-form (list 1) (list -1 0 1) (list 1 -1))) (display "  = (1/2)log(x-1) - (1/2)log(x+1)") (newline)
(chk "the residues are 1/2 and -1/2" (equal? (lf-residues (list 1) (list -1 0 1) (list 1 -1)) (list (cons 1 (/ 1 2)) (cons -1 (/ -1 2)))))
(chk "the structure form certifies: sum res_i/(x-a_i) = 1/(x^2-1)" (lf-certify (list 1) (list -1 0 1) (list 1 -1)))

(display "f = 2x/(x^2 - 1): residues 1 and 1, so INT = log(x-1) + log(x+1) = log(x^2-1):") (newline)
(chk "2x/(x^2-1): residues 1, 1, certified" (lf-certify (list 0 2) (list -1 0 1) (list 1 -1)))

(display "f = 1/((x-2)(x-3)) = 1/(x^2 - 5x + 6): residues at 2 and 3:") (newline)
(display "  ") (display (lf-residues (list 1) (list 6 -5 1) (list 2 3))) (display "  (-1 at x=2, +1 at x=3)") (newline)
(chk "1/((x-2)(x-3)): structure form certifies" (lf-certify (list 1) (list 6 -5 1) (list 2 3)))

(newline)
(display "The Liouville structure theorem made explicit: every rational integral is elementary, and the witness") (newline)
(display "f = sum (residue) (x - root)'/(x - root) is returned and certified -- the constructive content of HOW.") (newline)
