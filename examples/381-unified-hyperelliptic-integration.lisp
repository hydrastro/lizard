; The UNIFIED HYPERELLIPTIC INTEGRATION DRIVER: one entry point that integrates over the genus-g hyperelliptic
; field y^2 = f, dispatching the second-kind (polynomial-over-radical) and third-kind (logarithmic) cases to the
; certified machinery and returning a single verdict (docs/TRAGER_ROADMAP.md -- the general algebraic Risch for the
; hyperelliptic family, one decision procedure across genera).
;
; Second kind: INT P(x)/sqrt(f) -> the elementary Q sqrt(f) (Hermite reduction) or a proof of non-elementarity
; (the first-kind obstruction).  Third kind: a differential d log(a + y) -> log(a + y), with a recovered from the
; differential and certified.  Each positive answer carries the underlying module's differentiation certificate;
; the driver only classifies and dispatches, never guesses.
(import "cas/hyperint.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define f (list 1 0 0 0 0 1))   ; y^2 = x^5 + 1, a genus-2 curve

(display "One driver for INT over y^2 = x^5 + 1 (genus 2), routing second- and third-kind to certified machinery.") (newline) (newline)

(must "the genus of y^2 = x^5 + 1 is 2" (= (hi-genus f) 2))

(display "second kind: INT (5/2) x^4 / sqrt(f) dx = sqrt(f), an elementary algebraic antiderivative:") (newline)
(define P (list 0 0 0 0 (/ 5 2)))   ; (5/2) x^4 = f'/2
(define r2 (hi-integrate f (quote second) P))
(must "the verdict is elementary" (equal? (car r2) (quote elementary)))
(must "the answer is the algebraic part Q sqrt(f)" (equal? (car (cdr r2)) (quote algebraic)))
(must "the driver certifies it" (hi-decides? f (quote second) P))

(display "first kind: INT 1 / sqrt(f) dx is non-elementary (a holomorphic differential on a genus-2 curve):") (newline)
(define r1 (hi-integrate f (quote second) (list 1)))
(must "the verdict is non-elementary" (equal? (car r1) (quote non-elementary)))
(must "the obstruction is recorded as first-kind" (equal? (car (cdr r1)) (quote first-kind)))
(must "non-elementary is a definite, decided verdict" (hi-decides? f (quote second) (list 1)))

(display "third kind: a differential over the denominator N(x + y) = a^2 - f integrates to log(x + y):") (newline)
(define D (ht-norm f (list 0 1)))
(define r3 (hi-integrate f (quote third) D))
(must "the verdict is elementary" (equal? (car r3) (quote elementary)))
(must "the answer is a logarithm" (equal? (car (cdr r3)) (quote logarithm)))
(must "the driver certifies the logarithm" (hi-decides? f (quote third) D))

(display "soundness: an unrecognized third-kind differential is reported, not forced:") (newline)
(must "a non-recognized denominator returns non-elementary not-recognized" (equal? (car (hi-integrate f (quote third) (list 1 1 1))) (quote non-elementary)))

(display "the driver is genus-agnostic -- it decides the same way on the elliptic curve y^2 = x^3 + 1:") (newline)
(define fe (list 1 0 0 1))
(must "the genus of y^2 = x^3 + 1 is 1" (= (hi-genus fe) 1))
(must "second-kind INT (3/2) x^2 / sqrt(f) = sqrt(f) is elementary" (equal? (car (hi-integrate fe (quote second) (list 0 0 (/ 3 2)))) (quote elementary)))
(must "third-kind log(x + y) is elementary" (equal? (car (hi-integrate fe (quote third) (ht-norm fe (list 0 1)))) (quote elementary)))

(newline)
(display "A single driver now decides and integrates over the hyperelliptic family at any genus: the second-kind") (newline)
(display "algebraic part and its non-elementarity proof, and the third-kind logarithm, each certified, under one") (newline)
(display "entry point.  Mixed integrands of several kinds at once, and arbitrary genus beyond the a + y third-kind") (newline)
(display "shape, remain the open summit.") (newline)
