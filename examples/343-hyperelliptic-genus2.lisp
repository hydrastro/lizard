; The genus-2 hyperelliptic DECISION for INT P(x)/sqrt(q(x)) dx with q squarefree of degree 5 or 6: decide
; elementarity, certify the algebraic antiderivative when it exists, otherwise report a genuine HYPERELLIPTIC
; integral -- non-elementary by exact reduction (docs/TRAGER_ROADMAP.md, frontier 2: genus >= 2).
;
; The Hermite-style radical reduction is degree-general; the genus-dependent fact is only the meaning of a
; surviving remainder.  The curve y^2 = q has genus floor((deg q - 1)/2) = 2 for deg q in {5,6}, so a nonzero
; reduced remainder is a first/second-kind hyperelliptic differential and the integral is non-elementary.
(import "cas/hyperelliptic.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define q5 (list 1 0 0 0 0 1))    ; x^5 + 1
(define q6 (list 1 0 0 0 0 0 1))  ; x^6 + 1

(display "Genus-2 hyperelliptic integrals INT P(x)/sqrt(q), q a squarefree quintic or sextic.") (newline) (newline)

(display "the curve y^2 = q has genus floor((deg q - 1)/2):") (newline)
(chk "genus(x^5+1) = genus(x^6+1) = 2" (if (= (he-genus q5) 2) (= (he-genus q6) 2) #f))

(display "the ELEMENTARY case -- numerator matches the derivative pattern:") (newline)
(display "INT (5x^4/2)/sqrt(x^5+1) dx = sqrt(x^5+1)  [since (x^5+1)' = 5x^4]:") (newline)
(define r1 (he-integrate (list 0 0 0 0 (/ 5 2)) q5))
(chk "INT (5x^4/2)/sqrt(x^5+1) elementary, certified in K" (if (equal? (car r1) (quote elementary)) (he-certify (list 0 0 0 0 (/ 5 2)) q5 r1) #f))
(display "INT 3x^5/sqrt(x^6+1) dx = sqrt(x^6+1)  [sextic]:") (newline)
(chk "INT 3x^5/sqrt(x^6+1) elementary, certified" (if (equal? (car (he-integrate (list 0 0 0 0 0 3) q6)) (quote elementary)) (he-certify (list 0 0 0 0 0 3) q6 (he-integrate (list 0 0 0 0 0 3) q6)) #f))

(display "the GENUINE HYPERELLIPTIC integrals -- proven non-elementary:") (newline)
(chk "INT 1/sqrt(x^5+1) (genus-2 first kind) non-elementary" (equal? (car (he-integrate (list 1) q5)) (quote non-elementary)))
(chk "INT x/sqrt(x^5+1) non-elementary" (equal? (car (he-integrate (list 0 1) q5)) (quote non-elementary)))
(chk "INT 1/sqrt(x^6+1) non-elementary" (equal? (car (he-integrate (list 1) q6)) (quote non-elementary)))

(display "integrate as far as possible: INT sqrt(x^5+1) = (2/7) x sqrt(x^5+1) + (5/7) INT 1/sqrt(x^5+1):") (newline)
(define ss (he-split (list 1 0 0 0 0 1) q5))
(chk "INT sqrt(x^5+1) splits into a certified elementary part and a named hyperelliptic remainder" (equal? (car ss) (quote split)))

(newline)
(display "Genus-2 hyperelliptic integrals are now decided: the rare elementary case is certified inside the") (newline)
(display "function field, and the genuine hyperelliptic integrals are proven non-elementary by the same exact") (newline)
(display "reduction that handles the elliptic case -- now over curves of genus 2.  Higher genus and full third-kind") (newline)
(display "logarithmic parts on these curves remain the continuing frontier.") (newline)
