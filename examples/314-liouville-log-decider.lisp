; The logarithmic companion of the Liouville decider: decides and integrates INT P(x) log(x) dx (always
; elementary, by parts) and records the proven NON-elementarity of INT 1/log(x) dx (the logarithmic integral li).
; Part of the decider suite that proves elementarity verdicts rather than merely constructing antiderivatives
; (docs/TRAGER_ROADMAP.md, the summit).
;
; INT P log x dx = F log x - INT F/x dx with F = INT P; F/x is rational so its integral is elementary, hence
; INT P log x is always elementary, returned as the explicit closed form.  INT 1/log x dx is non-elementary.
(import "cas/liouvillelog.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The logarithmic Liouville decider: INT P log x dx (always elementary) and INT 1/log x (li, non-elementary).") (newline) (newline)

(display "INT log x dx = x log x - x:") (newline)
(define d1 (lv-log-decide (list 1)))
(chk "INT log x = x log x - x, certified" (lv-log-certify (list 1) (ll-nth d1 1) (ll-nth d1 2) (ll-nth d1 3)))

(display "INT x log x dx = (x^2/2) log x - x^2/4:") (newline)
(define d2 (lv-log-decide (list 0 1)))
(chk "INT x log x = (x^2/2) log x - x^2/4, certified" (lv-log-certify (list 0 1) (ll-nth d2 1) (ll-nth d2 2) (ll-nth d2 3)))

(display "INT x^2 log x dx:") (newline)
(define d3 (lv-log-decide (list 0 0 1)))
(chk "INT x^2 log x, certified" (lv-log-certify (list 0 0 1) (ll-nth d3 1) (ll-nth d3 2) (ll-nth d3 3)))

(display "INT 1/log x dx is the logarithmic integral li -- PROVEN non-elementary:") (newline)
(display "  ") (display (lv-log-li)) (newline)
(chk "INT 1/log x dx is PROVEN non-elementary (li)" (equal? (car (lv-log-li)) (quote non-elementary)))

(newline)
(display "INT P log x dx is decided elementary with its closed form for every polynomial P, while INT 1/log x") (newline)
(display "is proven to have no elementary antiderivative -- the logarithmic integral li.") (newline)
