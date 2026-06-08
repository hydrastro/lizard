; The GENERAL coupled tower-field Risch differential equation, for BOTH exponential and logarithmic levels of a
; height-1 tower K_1 = Q(x)(theta), with an arbitrary tower-element coefficient.  This generalizes the
; exp-over-exp special case to arbitrary coefficients and to the logarithmic level (whose derivation shifts
; theta-degree), running the recursive Risch descent on the differential-equation machinery at full generality
; for height 1 (docs/TRAGER_ROADMAP.md, the summit).
;
; EXPONENTIAL level: the diagonal derivation makes D y + f y = g a BANDED system (degree n couples to lower
; degrees), solved BOTTOM-UP, each step a base RDE one level down.  LOGARITHMIC level: the derivation shifts
; degree down, so D y + f0 y = g couples each degree to the next-higher, solved TOP-DOWN.
(import "cas/rischcoupled.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define b (rat-from-poly (list 0 1)))   ; b = x, theta = exp(x)

(display "The general coupled tower-field RDE: arbitrary-coefficient exponential (banded) and logarithmic levels.") (newline) (newline)

(display "exponential level, arbitrary coefficient f = 1 + exp(x): y' + (1 + e^x) y = 1 has a non-terminating") (newline)
(display "  tail (y_0 = 1, y_1 = -1/2, y_2 = 1/6, ...), so no bounded-degree solution exists:") (newline)
(define v1 (rc-exp-solve b (list (rat-one) (rat-one)) (list (rat-one)) 4))
(display "  ") (display v1) (newline)
(chk "y' + (1 + e^x) y = 1 has a non-terminating tail (non-elementary)" (equal? (car v1) (quote non-elementary)))

(display "a solvable coupled exponential case: y' + e^x y = e^x + e^{2x} recovers y = e^x:") (newline)
(define ytest (list (rat-zero) (rat-one)))
(define gtest (rc-add (rc-exp-deriv b ytest) (rc-mul (list (rat-zero) (rat-one)) ytest)))
(define v2 (rc-exp-solve b (list (rat-zero) (rat-one)) gtest 5))
(chk "solvable coupled exp case recovers y = e^x, certified" (if (equal? (car v2) (quote solvable)) (rc-exp-certify b (list (rat-zero) (rat-one)) gtest (car (cdr v2))) #f))

(display "logarithmic level (the degree-shifting derivation, solved top-down): INT log x dx = x log x - x:") (newline)
(define vlog (rc-int-log-x))
(display "  y = ") (display vlog) (display "  (y_1 = x, y_0 = -x)") (newline)
(chk "INT log x = x log x - x, certified via the log-level top-down solve" (if (equal? (car vlog) (quote solvable)) (rc-log-certify (rat-make (list 1) (list 0 1)) (rat-zero) (list (rat-zero) (rat-one)) (car (cdr vlog))) #f))

(display "another logarithmic case: INT 2 log x dx = 2(x log x - x):") (newline)
(define vlog2 (rc-log-solve (rat-make (list 1) (list 0 1)) (rat-zero) (list (rat-zero) (rat-from-poly (list 2)))))
(chk "INT 2 log x certified" (if (equal? (car vlog2) (quote solvable)) (rc-log-certify (rat-make (list 1) (list 0 1)) (rat-zero) (list (rat-zero) (rat-from-poly (list 2))) (car (cdr vlog2))) #f))

(newline)
(display "The coupled RDE handles both level types at full height-1 generality: the exponential banded recurrence") (newline)
(display "solved bottom-up (with non-terminating-tail detection) and the logarithmic degree-shift solved top-down,") (newline)
(display "each step a base RDE one level down -- the recursive descent on the differential-equation machinery.") (newline)
