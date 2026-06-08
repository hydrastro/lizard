; The SUPERELLIPTIC THIRD-KIND LOGARITHM -- integrating logarithmic differentials u'/u on the curve y^n = g(x)
; and, conversely, RECOGNIZING such a differential and recovering its logarithm.  This is the Rothstein-Trager
; step over the superelliptic field, the logarithmic half of the Rung-4 integration payoff
; (docs/TRAGER_ROADMAP.md), built on the field (sefield.lisp) and its Norm (senorm.lisp).
;
; CONSTRUCTIVE: for a field element u, the differential u'/u integrates to log u; st-log returns the rationalized
; differential (field numerator over the polynomial denominator N(u)) with the certified statement, and
; st-log-certify checks the cleared identity u * (numerator) = N(u) * u'.
;
; RECOGNIZER (the Rothstein-Trager step) for the common third-kind argument u = a(x) + y: here
; N(a+y) = a^n + (-1)^{n+1} g, so from a logarithmic differential's denominator D the candidate a is the n-th
; root of D - (-1)^{n+1} g; if that is an exact polynomial n-th power and reproduces the differential, the
; integral is log(a + y).  This recovers the logarithm of a third-kind differential from the differential itself.
(import "cas/sethird.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Superelliptic third-kind logarithm on y^n = g: constructing log(u) and recovering it (Rothstein-Trager).") (newline) (newline)

(define g (list 1 0 0 1))   ; g = x^3 + 1, n = 3

(display "constructive -- INT (u'/u) dx = log u for u = x + y:") (newline)
(define u (st-a+y (list 0 1) 3))   ; x + y
(define lg (st-log g 3 u))
(display "  rationalized differential has denominator N(u) = ") (display (cdr (st-nth lg 2))) (display "  = 2x^3 + 1") (newline)
(chk "INT (u'/u) dx = log(x + y), certified by u * F = N(u) * u'" (st-log-certify g 3 u))

(display "the closed-form Norm of the third-kind argument:") (newline)
(display "  N(a + y) = a^n + (-1)^{n+1} g ; for a = x:  N(x+y) = x^3 + g = ") (display (st-norm-a+y (list 0 1) g 3)) (display "  = 2x^3 + 1") (newline)
(chk "closed-form N(x+y) = 2x^3 + 1 matches the matrix Norm" (if (equal? (st-norm-a+y (list 0 1) g 3) (list 1 0 0 2)) (rat-equal? (sn-norm g 3 u) (rat-from-poly (st-norm-a+y (list 0 1) g 3))) #f))

(display "the Rothstein-Trager recognizer -- recover the logarithm from a differential's denominator:") (newline)
(define rec (st-recognize g 3 (list 1 0 0 2)))   ; D = 2x^3 + 1
(display "  given D = 2x^3 + 1, recover a = nth-root(D - g) = ") (display (st-nth rec 2)) (display "  ->  INT = log(x + y)") (newline)
(chk "recognizer recovers a = x and the log certifies" (if (equal? (st-nth rec 2) (list 0 1)) (st-recognize-certify g 3 (list 0 1)) #f))

(display "a higher recognizer case, a = x + 1:") (newline)
(define D2 (st-norm-a+y (list 1 1) g 3))   ; N((x+1)+y) = (x+1)^3 + g
(display "  N((x+1)+y) = ") (display D2) (display "  -> recover a = ") (display (st-nth (st-recognize g 3 D2) 2)) (newline)
(chk "recognizer recovers a = x + 1, certified" (if (equal? (st-nth (st-recognize g 3 D2) 2) (list 1 1)) (st-recognize-certify g 3 (list 1 1)) #f))

(display "soundness -- a denominator that is not such a Norm is rejected:") (newline)
(display "  recognize(x^2 + 5) -> ") (display (st-recognize g 3 (list 5 0 1))) (newline)
(chk "non-Norm denominator honestly reported not-third-kind-a+y" (equal? (st-recognize g 3 (list 5 0 1)) (quote not-third-kind-a+y)))

(display "the n = 2 specialization (hyperelliptic third kind, Norm a^2 - g):") (newline)
(define g2 (list 1 0 1))
(chk "n=2 constructive log(x + y) certifies on y^2 = x^2 + 1" (st-log-certify g2 2 (st-a+y (list 0 1) 2)))

(newline)
(display "Third-kind superelliptic logarithm: log(u) constructed and certified, and recovered from a") (newline)
(display "differential by the Rothstein-Trager n-th-root recognizer -- the logarithmic half of Rung 4 for y^n = g.") (newline)
