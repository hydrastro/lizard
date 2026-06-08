; The elliptic THIRD-KIND recognizer: deciding when an integrand on the curve y^2 = q(x) is a logarithmic
; derivative and hence integrates to a logarithm log(A + B sqrt(q)) -- the elementary third-kind case that the
; pure first/second-kind reduction (elliptic.lisp) leaves inconclusive (docs/TRAGER_ROADMAP.md, beyond genus 1).
;
; For g = A + B sqrt(q) in K = Q(x)[y]/(y^2 - q), d/dx log(g) = g'/g is a specific K-element, computed exactly
; via the conjugate; so a presented logarithmic-derivative integrand integrates to log(g), certified by
; recomputing the K-derivative.  This recognizes third-kind logarithmic integrals over the curve, complementing
; the first/second-kind non-elementarity proofs with genuine elementary logarithmic answers.
(import "cas/elliptic3.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Elliptic third kind: integrands that are logarithmic derivatives in K integrate to log(A + B sqrt q).") (newline) (newline)

(display "the construction: d/dx log(x + sqrt(x^2+1)) = 1/sqrt(x^2+1) (the arcsinh / logarithmic form):") (newline)
(define q2 (rat-from-poly (list 1 0 1)))
(define g2 (af-make (rat-from-poly (list 0 1)) (rat-one)))
(define om2 (e3-logderiv q2 g2))
(chk "INT (g'/g) = log(x + sqrt(x^2+1)), recognized and certified" (if (equal? (car (e3-recognize q2 om2 g2)) (quote elementary-log)) (e3-certify q2 g2 om2) #f))

(display "a GENUINE genus-1 third-kind integral over y^2 = x^3 + 1: INT d/dx log(x + sqrt(x^3+1)):") (newline)
(define q3 (rat-from-poly (list 1 0 0 1)))
(define g3 (af-make (rat-from-poly (list 0 1)) (rat-one)))
(define om3 (e3-logderiv q3 g3))
(chk "the integrand is a logarithmic derivative over the elliptic curve, integral = log(x + sqrt(x^3+1))" (equal? (car (e3-recognize q3 om3 g3)) (quote elementary-log)))
(chk "certified by recomputing the derivative in K" (e3-certify q3 g3 om3))

(display "another K-element: g = (x^2+1) + 2 sqrt(x^3+1), its log-derivative integrates to log(g):") (newline)
(define g3b (af-make (rat-from-poly (list 1 0 1)) (rat-from-poly (list 2))))
(chk "INT d/dx log((x^2+1) + 2 sqrt(x^3+1)) = log((x^2+1) + 2 sqrt(x^3+1)), certified" (e3-certify q3 g3b (e3-logderiv q3 g3b)))

(display "soundness: a candidate that is not the matching logarithm is rejected:") (newline)
(chk "a non-matching omega is rejected (not-this-log)" (equal? (car (e3-recognize q3 (af-make (rat-zero) (rat-one)) g3)) (quote not-this-log)))

(newline)
(display "The third-kind logarithmic integrals over an elliptic curve are now recognized and certified: where the") (newline)
(display "first/second-kind reduction proves non-elementarity, this complements it with the genuine elementary") (newline)
(display "logarithmic answers log(A + B sqrt q).  Full third-kind decision (finding the g from the integrand, with") (newline)
(display "rational-residue analysis on the curve) and genus >= 2 hyperelliptic are the continuing frontier.") (newline)
