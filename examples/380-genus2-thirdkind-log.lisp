; The GENUS-2 (and general hyperelliptic) THIRD-KIND LOGARITHM: INT (g'/g) dx = log(g) for g = a(x) + y on
; y^2 = f, with a recovered from the differential, certified -- the Trager third-kind step over a hyperelliptic
; field of any genus (docs/TRAGER_ROADMAP.md -- the full third-kind construction beyond genus 1).
;
; The field K = Q(x)[y]/(y^2 - f) and its derivation are handled by algfunc for ANY radicand f, so they cover the
; genus-2 (deg f = 5) case unchanged.  For g = a + y the rationalized logarithmic derivative has denominator the
; norm N(a + y) = a^2 - f, so from a third-kind differential over a denominator D the candidate is a = sqrt(D + f)
; when D + f is a perfect square; the integral is log(a + y), certified by differentiation in K.  A non-square
; denominator is rejected, never assigned a spurious logarithm.
(import "cas/hyperthird.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define f (list 1 0 0 0 0 1))   ; y^2 = x^5 + 1, a genus-2 curve

(display "The third-kind logarithm over the genus-2 field y^2 = x^5 + 1: INT (g'/g) dx = log(g), g = a(x) + y.") (newline) (newline)

(display "the construction INT (d log(x + y)) dx = log(x + y), certified by differentiation in K:") (newline)
(must "the certificate (x + y)*omega = D(x + y) holds" (ht-cert f (list 0 1)))
(display "  the norm N(x + y) = a^2 - f = ") (display (poly-norm (ht-norm f (list 0 1)))) (newline)
(must "N(x + y) = x^2 - x^5 - 1" (equal? (poly-norm (ht-norm f (list 0 1))) (poly-norm (list -1 0 1 0 0 -1))))

(display "recovering a from the third-kind differential's denominator D = a^2 - f:") (newline)
(must "a = x is recovered from D = N(x + y)" (equal? (poly-norm (ht-recover-a f (ht-norm f (list 0 1)))) (list 0 1)))
(must "the differential over that D is recognized as a logarithm" (equal? (car (ht-recognize f (ht-norm f (list 0 1)))) (quote log)))

(display "higher-degree arguments, including ones with intermediate terms:") (newline)
(must "INT (d log(x^2 + y)) is certified" (ht-cert f (list 0 0 1)))
(must "a = x^2 is recovered" (equal? (poly-norm (ht-recover-a f (ht-norm f (list 0 0 1)))) (list 0 0 1)))
(must "INT (d log(x^2 + 1 + y)) is certified" (ht-cert f (list 1 0 1)))
(must "a = x^2 + 1 is recovered (an argument with a middle term)" (equal? (poly-norm (ht-recover-a f (ht-norm f (list 1 0 1)))) (list 1 0 1)))

(display "soundness: a denominator that is not a^2 - f for any polynomial a is rejected:") (newline)
(must "a non-third-kind differential returns not-third-kind-a+y" (equal? (ht-recognize f (list 1 1 1)) (quote not-third-kind-a+y)))

(display "the construction is genus-agnostic -- it agrees with the genus-1 third-kind on an elliptic curve:") (newline)
(define fe (list 1 0 0 1))   ; y^2 = x^3 + 1
(must "INT (d log(x + y)) on y^2 = x^3 + 1 is certified" (ht-cert fe (list 0 1)))
(must "a = x is recovered there too" (equal? (poly-norm (ht-recover-a fe (ht-norm fe (list 0 1)))) (list 0 1)))

(newline)
(display "The explicit third-kind algebraic logarithm now works over the genus-2 hyperelliptic field: the argument") (newline)
(display "a + y is recovered from the differential via the norm a^2 - f and certified by differentiation, the same") (newline)
(display "construction across genus 1 and 2.  Arguments beyond the a + y shape, and the full divisor-class") (newline)
(display "construction at arbitrary genus, remain the open summit.") (newline)
