; The third-kind SEARCH: given an integrand omega on the curve y^2 = q(x), constructively FIND g = u(x)+sqrt(q)
; (u a polynomial of bounded degree) with omega = g'/g, so INT omega = log(g) -- the inverse of the recognizer
; (docs/TRAGER_ROADMAP.md, frontier 1: finding g from the integrand, not merely certifying a supplied one).
;
; A bounded constructive search over candidate u: compute g'/g in K = Q(x)[y]/(y^2-q) for each and test equality
; with omega.  Sound -- every reported g is certified by differentiation, and exhausting the family yields an
; honest not-found rather than a false claim about elementarity.
(import "cas/elliptic3solve.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Third-kind search: recover g = u + sqrt(q) from a logarithmic-derivative integrand, certified.") (newline) (newline)

(display "genus 0 (sanity): omega = d/dx log(x + sqrt(x^2-1)); the search recovers g = x + sqrt(x^2-1):") (newline)
(define q1 (rat-from-poly (list -1 0 1)))
(define om1 (e3-logderiv q1 (af-make (rat-from-poly (list 0 1)) (rat-one))))
(define r1 (e3s-search q1 om1 1))
(chk "g found and certified for x + sqrt(x^2-1)" (if (equal? (car r1) (quote found)) (e3-certify q1 (car (cdr r1)) om1) #f))

(display "GENUINE genus 1: omega = d/dx log(x + sqrt(x^3+1)) over the elliptic curve y^2 = x^3+1:") (newline)
(define q3 (rat-from-poly (list 1 0 0 1)))
(define om3 (e3-logderiv q3 (af-make (rat-from-poly (list 0 1)) (rat-one))))
(define r3 (e3s-search q3 om3 1))
(chk "the search recovers g = x + sqrt(x^3+1), certified" (if (equal? (car r3) (quote found)) (e3-certify q3 (car (cdr r3)) om3) #f))
(chk "INT omega = log(g) reported elementary-log" (equal? (car (e3s-integrate q3 om3 1)) (quote elementary-log)))

(display "soundness: a first-kind integrand 1/sqrt(x^3+1) has no polynomial-u log form -> honest not-found:") (newline)
(chk "1/sqrt(x^3+1) returns not-found, not a false verdict" (equal? (car (e3s-search q3 (af-make (rat-zero) (rat-make (list 1) (list 1 0 0 1))) 1)) (quote not-found)))

(newline)
(display "The third-kind search closes the inverse direction: given a logarithmic-derivative integrand over an") (newline)
(display "elliptic curve, it recovers g and returns log(g), certified -- and reports not-found honestly when no") (newline)
(display "candidate in the bounded family matches.  The full decision (unbounded g, rational-residue analysis on") (newline)
(display "the curve) and genus >= 2 are the continuing frontier.") (newline)
