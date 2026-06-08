; ALGEBRAIC RESIDUES in Rothstein-Trager: the conjugate-root case.  When a proper rational function over an
; irreducible quadratic has IRRATIONAL residues (real algebraic), the integral is an algebraic-coefficient
; logarithm -- the first genuinely-algebraic-residue case above the rational Rothstein-Trager part
; (docs/TRAGER_ROADMAP.md, the frontier).
;
; For INT (Ax+B)/(x^2+px+q) dx with disc = p^2-4q > 0 and irrational roots, the answer is
;   (A/2) log(x^2+px+q)  +  ((B - Ap/2)/sqrt(disc)) log((x-r1)/(x-r2)),
; the second term carrying sqrt(d) (disc = s^2 d, d squarefree).  The soundness key: the derivative of the
; algebraic logarithm is RATIONAL (the sqrt cancels, since r1 - r2 = sqrt(disc)), so the whole result is
; CERTIFIED by an exact rational identity in Q(x) even though the antiderivative itself lives in Q(sqrt d).
(import "cas/algresq.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Algebraic residues: irrational (real algebraic) residues give an algebraic-coefficient logarithm,") (newline)
(display "certified over Q despite the sqrt(d) in the antiderivative.") (newline) (newline)

(display "INT 1/(x^2 - 2) dx = (1/(2 sqrt2)) log((x - sqrt2)/(x + sqrt2))  [disc = 8 = 2^2 * 2, radicand d = 2]:") (newline)
(define r1 (algresq-integrate 0 1 0 -2))
(display "  radicand d = ") (display (car (cdr (cdr (cdr (cdr r1)))))) (display ",  algebraic-log coefficient = (this)/sqrt(d)") (newline)
(chk "INT 1/(x^2-2): radicand is 2" (= (car (cdr (cdr (cdr (cdr r1))))) 2))
(chk "INT 1/(x^2-2) certified by the exact rational identity in Q(x)" (algresq-certify 0 1 0 -2 r1))

(display "INT x/(x^2 - 2) dx = (1/2) log(x^2 - 2)  [pure rational-coefficient log; no algebraic part]:") (newline)
(chk "INT x/(x^2-2) certified" (algresq-certify 1 0 0 -2 (algresq-integrate 1 0 0 -2)))

(display "INT (x+1)/(x^2 - 2) dx  [combines a rational-coeff log and an algebraic-coeff log]:") (newline)
(chk "INT (x+1)/(x^2-2) certified" (algresq-certify 1 1 0 -2 (algresq-integrate 1 1 0 -2)))

(display "INT (2x+3)/(x^2 - 3) dx  [disc = 12 = 2^2 * 3, radicand 3]:") (newline)
(chk "INT (2x+3)/(x^2-3) certified" (algresq-certify 2 3 0 -3 (algresq-integrate 2 3 0 -3)))

(display "INT 1/(x^2 + x - 1) dx  [disc = 5, squarefree, golden-ratio roots]:") (newline)
(chk "INT 1/(x^2+x-1) certified" (algresq-certify 0 1 1 -1 (algresq-integrate 0 1 1 -1)))

(display "the boundary cases are routed elsewhere, not faked:") (newline)
(chk "x^2 - 4 has rational roots -> not-applicable (the rational integrator handles it)" (equal? (car (algresq-integrate 0 1 0 -4)) (quote not-applicable)))
(chk "x^2 + 1 has negative discriminant -> not-applicable (the arctangent case)" (equal? (car (algresq-integrate 0 1 0 1)) (quote not-applicable)))

(newline)
(display "Algebraic residues in the conjugate-quadratic case are now integrated and certified: the antiderivative") (newline)
(display "carries sqrt(d), but its derivative collapses to a rational function, so the result is checked by an exact") (newline)
(display "identity over Q.  Higher-degree algebraic residues (cubic and beyond, RootSum over Q(alpha)) remain open.") (newline)
