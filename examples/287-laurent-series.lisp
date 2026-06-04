; LAURENT SERIES -- power series allowing finitely many negative-power terms, f(x) = sum_{k>=N} a_k x^k with N
; possibly negative.  This completes the series capability of the system (Taylor in series.lisp, Puiseux in the
; puiseux* modules, Laurent here) and is the analytic backbone for residues and principal parts.
;
; A Laurent series is (laurent N coeffs), meaning sum_i coeffs[i] x^(N+i).  The module provides the full Laurent
; algebra and -- most usefully -- the Laurent expansion of a rational function p(x)/q(x) at a point (writing
; q = x^v u(x) with u(0) != 0, so p/q = x^{-v}(p u^{-1})), from which the residue at any point follows.
(import "cas/laurent.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Laurent series: expansion of rational functions at a pole, residues, and the Laurent algebra.") (newline) (newline)

(display "expansion at a pole, 1/(x^2(1-x)) at x = 0:") (newline)
(define L (lr-expand-ratfun (list 1) (list 0 0 1 -1) 5))   ; q = x^2 - x^3
(display "  terms (exponent . coeff): ") (display (lr-terms L)) (newline)
(display "  = x^-2 + x^-1 + 1 + x + x^2 + ...,  lowest exponent ") (display (lr-lo L)) (display ", residue ") (display (lr-residue L)) (newline)
(chk "lowest exponent -2, residue 1, constant term 1" (if (= (lr-lo L) -2) (if (= (lr-residue L) 1) (= (lr-coeff L 0) 1) #f) #f))

(display "residues of a rational function at its poles, x/((x-1)(x-2)):") (newline)
(display "  residue at x=1 = ") (display (lr-residue-at (list 0 1) (list 2 -3 1) 1 4)) (display " ; residue at x=2 = ") (display (lr-residue-at (list 0 1) (list 2 -3 1) 2 4)) (newline)
(chk "Res_{x=1} = -1 and Res_{x=2} = 2 (sum to the limit at infinity)" (if (= (lr-residue-at (list 0 1) (list 2 -3 1) 1 4) -1) (= (lr-residue-at (list 0 1) (list 2 -3 1) 2 4) 2) #f))

(display "the Laurent algebra:") (newline)
(define A (lr-make -1 (list 1 1)))    ; x^-1 + 1
(define P (lr-mul A A))
(display "  (x^-1 + 1)^2 = ") (display (lr-terms P)) (display "  = x^-2 + 2x^-1 + 1") (newline)
(chk "(x^-1+1)^2 = x^-2 + 2x^-1 + 1" (if (= (lr-coeff P -2) 1) (if (= (lr-coeff P -1) 2) (= (lr-coeff P 0) 1) #f) #f))
(define Inv (lr-inverse A 5))
(display "  1/(x^-1 + 1) = x/(1+x) = ") (display (lr-terms Inv)) (display "  = x - x^2 + x^3 - ...") (newline)
(chk "inverse of a Laurent unit: 1/(x^-1+1) starts x - x^2 + ..." (if (= (lr-lo Inv) 1) (if (= (lr-coeff Inv 1) 1) (= (lr-coeff Inv 2) -1) #f) #f))

(display "integration with explicit logarithm detection:") (newline)
(define I (lr-integrate (lr-make -2 (list 1 1 1))))   ; integral of x^-2 + x^-1 + 1
(display "  integral of (x^-2 + x^-1 + 1) = -x^-1 + x  plus  ") (display (cdr I)) (display " * log(x)") (newline)
(chk "the x^-1 term integrates to a logarithm (coefficient 1); the rest to -x^-1 + x" (if (= (cdr I) 1) (= (lr-coeff (car I) -1) -1) #f))

(newline)
(display "Laurent series complete: rational-function expansion at any point, residues, full algebra, inverse,") (newline)
(display "and integration with the logarithmic term made explicit -- the negative-power analytic toolkit.") (newline)
