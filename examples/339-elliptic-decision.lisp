; A DECISION procedure for INT P(x)/sqrt(q(x)) dx with q squarefree of degree 3 or 4 (the genus-1 / elliptic
; case): decide whether the integral is elementary, certify the algebraic antiderivative when it is, and
; otherwise report it as a genuine ELLIPTIC integral -- non-elementary by an exact reduction, not a failure
; (docs/TRAGER_ROADMAP.md, the frontier -- higher-genus radicands).
;
; Hermite-style reduction in K = Q(x)[y]/(y^2-q): repeatedly subtract a multiple of d/dx[x^k sqrt q] to lower the
; numerator degree, accumulating the algebraic part A so that P/sqrt q = d/dx[A sqrt q] + rem/sqrt q with
; deg(rem) < deg(q)-1.  Remainder zero -> elementary (A sqrt q, certified in K); remainder nonzero (genus 1) ->
; a first/second-kind elliptic differential, non-elementary.
(import "cas/elliptic.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Elliptic integrals INT P(x)/sqrt(q), q a squarefree cubic or quartic: decided, certified both ways.") (newline) (newline)

(define q3 (list 1 0 0 1))    ; x^3 + 1
(define q4 (list 1 0 0 0 1))  ; x^4 + 1

(display "the rare ELEMENTARY case -- the numerator is exactly the derivative pattern:") (newline)
(display "INT (3x^2/2)/sqrt(x^3+1) dx = sqrt(x^3+1)  [since (x^3+1)' = 3x^2]:") (newline)
(define r1 (ell-integrate (list 0 0 (/ 3 2)) q3))
(chk "INT (3x^2/2)/sqrt(x^3+1) elementary, A = 1, certified in K" (if (equal? (car r1) (quote elementary)) (ell-certify (list 0 0 (/ 3 2)) q3 r1) #f))

(display "INT 2x^3/sqrt(x^4+1) dx = sqrt(x^4+1)  [quartic, since (x^4+1)' = 4x^3]:") (newline)
(define r5 (ell-integrate (list 0 0 0 2) q4))
(chk "INT 2x^3/sqrt(x^4+1) elementary, certified" (if (equal? (car r5) (quote elementary)) (ell-certify (list 0 0 0 2) q4 r5) #f))

(display "the GENUINE ELLIPTIC integrals -- proven non-elementary by the surviving reduced differential:") (newline)
(display "INT 1/sqrt(x^3+1) dx -- the elliptic integral of the first kind:") (newline)
(chk "INT 1/sqrt(x^3+1) non-elementary (elliptic)" (equal? (car (ell-integrate (list 1) q3)) (quote non-elementary)))
(display "INT x/sqrt(x^3+1) dx -- elliptic, non-elementary:") (newline)
(chk "INT x/sqrt(x^3+1) non-elementary" (equal? (car (ell-integrate (list 0 1) q3)) (quote non-elementary)))
(display "INT 1/sqrt(x^4+1) dx -- the lemniscatic elliptic integral:") (newline)
(chk "INT 1/sqrt(x^4+1) non-elementary" (equal? (car (ell-integrate (list 1) q4)) (quote non-elementary)))

(display "a mixed integrand: the elementary part reduces but a remainder survives -> non-elementary overall:") (newline)
(display "INT (3x^2/2 + 1)/sqrt(x^3+1) dx -- the 3x^2/2 integrates to sqrt(x^3+1) but the +1 is first-kind:") (newline)
(chk "INT (3x^2/2 + 1)/sqrt(x^3+1) non-elementary" (equal? (car (ell-integrate (list 1 0 (/ 3 2)) q3)) (quote non-elementary)))

(display "soundness guard: a non-squarefree radicand is not forced -- INT 1/sqrt(x^4) has sqrt(x^4)=x^2 rational:") (newline)
(chk "non-squarefree quartic reported inconclusive, not faked" (equal? (car (ell-integrate (list 1) (list 0 0 0 0 1))) (quote inconclusive)))

(display "INTEGRATE AS FAR AS POSSIBLE -- split off the elementary part, name the elliptic remainder:") (newline)
(display "INT sqrt(x^3+1) dx = (2/5) x sqrt(x^3+1) + (3/5) INT 1/sqrt(x^3+1)  [the classic reduction of sqrt(cubic)]:") (newline)
(define ss (ell-split-sqrt (list 1) q3))
(chk "INT sqrt(x^3+1) splits: elementary (2/5)x sqrt(q) + elliptic remainder 3/5" (if (equal? (car ss) (quote split)) (if (= (el-coeff (car (cdr ss)) 1) (/ 2 5)) (= (el-coeff (car (cdr (cdr ss))) 0) (/ 3 5)) #f) #f))
(display "INT (3x^2/2 + 1)/sqrt(x^3+1) dx = sqrt(x^3+1) + INT 1/sqrt(x^3+1)  [elementary part + first-kind remainder]:") (newline)
(chk "the mixed integrand splits into a certified elementary part and a named elliptic remainder" (equal? (car (ell-split (list 1 0 (/ 3 2)) q3)) (quote split)))

(newline)
(display "Elliptic integrals are now DECIDED: the rare elementary case is integrated and certified inside the") (newline)
(display "function field, and the genuine elliptic integrals (first and second kind) are proven non-elementary by") (newline)
(display "the exact reduction -- a proof of which higher-genus integrals have no elementary form.  Full Trager") (newline)
(display "algebraic integration with integral bases (third-kind logarithmic parts, higher genus) is beyond.") (newline)
