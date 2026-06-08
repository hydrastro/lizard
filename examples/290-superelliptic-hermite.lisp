; SUPERELLIPTIC HERMITE REDUCTION -- integration of differentials P(x) y^j / g(x) dx on the curve y^n = g(x),
; generalizing the hyperelliptic (n = 2) Hermite reduction to arbitrary degree n.  This is part of the Rung-4
; integration payoff (docs/TRAGER_ROADMAP.md): turning the analysis of a general algebraic curve into actual
; integration on it.
;
; On y^n = g the derivation gives y' = g' y / (n g), so D(x^k y^j) = [k x^{k-1} g + (j/n) x^k g'] y^j / g -- the
; power y^j is preserved, so each y^j sector reduces independently, exactly the hyperelliptic mechanism with p
; replaced by g and the constant 1/2 replaced by j/n.  Subtracting these numerators descending in degree reduces
; INT (P y^j / g) dx to Q y^j + INT (S y^j / g) dx with deg S < deg g - 1.  If S = 0 the integral is elementary,
; equal to Q y^j, certified by the polynomial identity Q' g + (j/n) Q g' = P; if S != 0 the holomorphic
; first-kind remainder is reported (the integral is not elementary by this reduction).
(import "cas/sehermite.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Superelliptic Hermite reduction: INT P(x) y^j / g dx on y^n = g, elementary results differentiate-certified.") (newline) (newline)

(define g (list 1 0 0 1))   ; g = x^3 + 1

(display "a cube-root curve y^3 = x^3 + 1, integrating in the y^1 sector:") (newline)
(define P (list 0 2 0 0 3))   ; 3x^4 + 2x
(display "  INT (3x^4 + 2x) y / (x^3+1) dx  ->  ") (display (se-integrate P g 1 3)) (display "  = x^2 y") (newline)
(chk "elementary, Q = x^2" (equal? (se-integrate P g 1 3) (list (quote elementary) (list 0 0 1))))
(chk "certified: Q' g + (1/3) Q g' = P" (se-certify P g 1 3))

(display "the y^2 sector of the same curve (a constructed exact derivative D((x^3+x) y^2)):") (newline)
(define Q2 (list 0 1 0 1))     ; x^3 + x
(define P2 (se-dnum Q2 g 2 3))
(display "  the differential's numerator is ") (display P2) (display " ; INT (... y^2 / g) dx  ->  ") (display (se-integrate P2 g 2 3)) (newline)
(chk "recovers Q = x^3 + x in the y^2 sector, certified" (if (equal? (se-integrate P2 g 2 3) (list (quote elementary) Q2)) (se-certify P2 g 2 3) #f))

(display "the n = 2 specialization reproduces the hyperelliptic case (y^2 = x^3 + 1):") (newline)
(define P3 (list 0 0 (/ 3 2)))   ; 3x^2/2
(display "  INT (3x^2/2) y / (x^3+1) dx  ->  ") (display (se-integrate P3 g 1 2)) (display "  = y = sqrt(x^3+1)") (newline)
(chk "n=2 elementary with Q = 1 (matches d/dx sqrt(x^3+1))" (equal? (se-integrate P3 g 1 2) (list (quote elementary) (list 1))))

(display "an honest non-elementary verdict -- the holomorphic differential y / (x^3+1):") (newline)
(display "  INT y / (x^3+1) dx (numerator P = 1)  ->  ") (display (se-integrate (list 1) g 1 3)) (newline)
(chk "P = 1 is first-kind (holomorphic): non-elementary, reported not guessed" (equal? (car (se-integrate (list 1) g 1 3)) (quote non-elementary-first-kind)))

(newline)
(display "Superelliptic Hermite: each y^j sector of y^n = g reduces like the hyperelliptic case; elementary") (newline)
(display "antiderivatives Q y^j are differentiate-certified, and the first-kind obstruction is reported honestly.") (newline)
