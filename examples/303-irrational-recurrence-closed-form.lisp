; CLOSED FORMS for second-order constant-coefficient recurrences whose characteristic polynomial is IRREDUCIBLE
; over Q (irrational quadratic roots) -- the Binet/Lucas case that linrec.lisp honestly declines.  This closes
; the remaining gap in linear-recurrence solving; the canonical example is Fibonacci, whose closed form needs
; the golden ratio.
;
; For a_n = p a_{n-1} + q a_{n-2}, the characteristic polynomial x^2 - p x - q has discriminant D = p^2 + 4q.
; When D is not a perfect square the roots r, s = (p +- sqrt D)/2 live in Q(sqrt D) and
;   a_n = A r^n + B s^n,  A = (a_1 - a_0 s)/sqrt D,  B = (a_0 r - a_1)/sqrt D,
; with A, B conjugate in Q(sqrt D) so a_n is rational for every n.  The whole computation is exact in Q(sqrt D)
; (elements u + v sqrt D), and the closed form is CERTIFIED by evaluating it against the iterated recurrence.
(import "cas/linrec2.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Closed forms for recurrences with irrational characteristic roots -- Binet's formula and its kin.") (newline) (newline)

(display "Fibonacci  a_n = a_{n-1} + a_{n-2},  a_0 = 0, a_1 = 1  (the golden-ratio closed form, discriminant 5):") (newline)
(define fib (lr2-solve 1 1 0 1))
(display "  F_0 .. F_10 = ") (display (map (lambda (n) (lr2-eval fib n)) (list 0 1 2 3 4 5 6 7 8 9 10))) (newline)
(chk "F_10 = 55 and F_15 = 610 from the closed form" (if (= (lr2-eval fib 10) 55) (= (lr2-eval fib 15) 610) #f))
(chk "Binet's closed form is certified against the iterated recurrence (n = 0..20)" (lr2-certify fib 1 1 0 1 20))

(display "Lucas numbers  a_n = a_{n-1} + a_{n-2},  a_0 = 2, a_1 = 1  (same recurrence, different start):") (newline)
(define luc (lr2-solve 1 1 2 1))
(display "  L_0 .. L_8 = ") (display (map (lambda (n) (lr2-eval luc n)) (list 0 1 2 3 4 5 6 7 8))) (newline)
(chk "L_8 = 47, certified" (if (= (lr2-eval luc 8) 47) (lr2-certify luc 1 1 2 1 20) #f))

(display "Pell numbers  a_n = 2 a_{n-1} + a_{n-2},  a_0 = 0, a_1 = 1  (discriminant 8):") (newline)
(define pell (lr2-solve 2 1 0 1))
(display "  P_0 .. P_6 = ") (display (map (lambda (n) (lr2-eval pell n)) (list 0 1 2 3 4 5 6))) (newline)
(chk "P_6 = 70, certified" (if (= (lr2-eval pell 6) 70) (lr2-certify pell 2 1 0 1 15) #f))

(display "a perfect-square discriminant is handled by the rational-root solver instead:") (newline)
(display "  a_n = 5 a_{n-1} - 6 a_{n-2} (discriminant 1) -> ") (display (lr2-solve 5 -6 0 1)) (newline)
(chk "a rational-root recurrence defers to linrec (use-linrec)" (equal? (lr2-solve 5 -6 0 1) (quote use-linrec)))

(newline)
(display "Quadratic-irrational recurrences solved in closed form over Q(sqrt D): Binet, Lucas, Pell and the like,") (newline)
(display "each computed exactly in the splitting field and certified against the iterated sequence.") (newline)
