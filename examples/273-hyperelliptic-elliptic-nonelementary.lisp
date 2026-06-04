; INT P(x)/sqrt(p) dx for SQUAREFREE p of arbitrary degree: the algebraic (elliptic / hyperelliptic) case
; (BOUNDARY 2) -- the genuine deepest summit, where the integrand itself contains an algebraic function and the
; non-elementarity is the classical elliptic obstruction.
;
; algfunc/algfuncint handled sqrt of a quadratic (genus 0, always elementary).  hyperell opens the real
; algebraic summit: p squarefree of degree m defines the hyperelliptic curve y^2 = p of genus g =
; floor((m-1)/2).  A Hermite-style reduction on the curve removes the polynomial part of the numerator --
; D(x^k y) = (k x^{k-1} p + x^k p'/2)/y -- writing INT P/sqrt(p) = Q sqrt(p) + INT S/sqrt(p) with deg S < m-1.
; The remaining INT S/sqrt(p) spans the g FIRST-KIND (holomorphic) differentials x^i/sqrt(p), i < g, which are
; NON-ELEMENTARY for g >= 1.  So the integral is elementary iff S = 0 after reduction.
;
; The decisive results:
;   * INT dx/sqrt(x^3+1)  -- genus 1 (elliptic): PROVEN non-elementary.  This is the canonical elliptic integral.
;   * INT dx/sqrt(x^5+1)  -- genus 2 (hyperelliptic): PROVEN non-elementary.
;   * INT (3x^2/2)/sqrt(x^3+1) dx = sqrt(x^3+1)        -- elementary, found and certified.
;   * INT ((5/2)x^3+1)/sqrt(x^3+1) dx = x sqrt(x^3+1)  -- elementary, found and certified.
; Every elementary answer is checked by differentiation inside the function field K = Q(x)[y]/(y^2 - p), the
; same certificate arbiter used everywhere; the non-elementary verdicts are decisions (the genus and the
; nonzero first-kind remainder are reported), not failures to find an answer.
(import "cas/hyperell.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))

(define p3 (list 1 0 0 1))        ; x^3 + 1   (genus 1)
(define p5 (list 1 0 0 0 0 1))    ; x^5 + 1   (genus 2)

(display "Hyperelliptic reduction on y^2 = p; first-kind differentials are non-elementary.") (newline) (newline)

(display "elliptic (genus 1), p = x^3 + 1:") (newline)
(define r1 (he-integrate (list 1) p3))
(display "  INT dx/sqrt(x^3+1) -> ") (display (car r1)) (display " (genus ") (display (car (cdr r1))) (display ")") (newline)
(must "INT dx/sqrt(x^3+1) PROVEN non-elementary (the canonical elliptic integral)" (equal? (car r1) (quote non-elementary)))
(must "INT (3x^2/2)/sqrt(x^3+1) dx = sqrt(x^3+1)         elementary, certified" (he-integrate-certify (list 0 0 (/ 3 2)) p3))
(must "INT ((5/2)x^3+1)/sqrt(x^3+1) dx = x sqrt(x^3+1)   elementary, certified" (he-integrate-certify (list 1 0 0 (/ 5 2)) p3))

(newline)
(display "hyperelliptic (genus 2), p = x^5 + 1:") (newline)
(must "INT dx/sqrt(x^5+1) PROVEN non-elementary (genus 2)" (equal? (car (he-integrate (list 1) p5)) (quote non-elementary)))
(must "INT x/sqrt(x^5+1) PROVEN non-elementary (genus 2, a first-kind differential)" (equal? (car (he-integrate (list 0 1) p5)) (quote non-elementary)))
(must "INT (5x^4/2)/sqrt(x^5+1) dx = sqrt(x^5+1)         elementary, certified" (he-integrate-certify (list 0 0 0 0 (/ 5 2)) p5))

(newline)
(display "BOUNDARY 2 achieved: elliptic and hyperelliptic non-elementarity decided, elementary algebraic parts certified.") (newline)
