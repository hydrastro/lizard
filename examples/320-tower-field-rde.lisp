; The TOWER-FIELD Risch differential equation solver: solves y' + f y = g where the coefficients live in a
; height-1 exponential tower K_1 = Q(x)(theta), theta = exp(b), and f is a base-field (Q(x)) coefficient.  This
; is the step that makes the recursive Risch procedure call ITSELF: it reduces the tower-field RDE to one
; base-field RDE per theta-degree, each solved by the rational-coefficient solver one level down (rischrde) --
; closing the recursion at every exponential level (docs/TRAGER_ROADMAP.md, the summit).
;
; For theta = exp(b) the derivation is diagonal in theta-degree, so with f = phi(x) the RDE decouples per degree
; into independent scalar RDEs y_k' + (k b' + phi) y_k = g_k over Q(x); the tower-field RDE is solvable iff every
; per-degree base RDE is, and assembling the y_k gives y in K_1, certified by the diagonal derivation.
(import "cas/rischtfrde.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define b (rat-from-poly (list 0 1)))   ; b = x, theta = exp(x)

(display "The tower-field RDE over Q(x)(exp x): decouples by theta-degree into base-field RDEs.") (newline) (newline)

(display "INT e^x dx = e^x -- the degree-1 reduction is the RDE y_1' + y_1 = 1:") (newline)
(define g1 (list (rat-zero) (rat-one)))
(define y1 (tfr-solve b (rat-zero) g1))
(chk "INT e^x = e^x (y_1 = 1), certified in K_1" (if (equal? y1 (quote no-tower-solution)) #f (tfr-certify b (rat-zero) g1 y1)))

(display "INT x e^x dx = (x-1) e^x -- the degree-1 RDE y_1' + y_1 = x gives y_1 = x - 1:") (newline)
(define g2 (list (rat-zero) (rat-from-poly (list 0 1))))
(define y2 (tfr-solve b (rat-zero) g2))
(chk "INT x e^x = (x-1) e^x, certified" (if (equal? y2 (quote no-tower-solution)) #f (tfr-certify b (rat-zero) g2 y2)))

(display "INT e^x/x dx -- the exponential integral Ei, PROVEN non-elementary (the degree-1 base RDE y_1'+y_1=1/x") (newline)
(display "  has no rational solution):") (newline)
(define g3 (list (rat-zero) (rat-make (list 1) (list 0 1))))
(chk "INT e^x/x PROVEN non-elementary (the per-degree base RDE is unsolvable)" (equal? (tfr-solve b (rat-zero) g3) (quote no-tower-solution)))

(display "a two-degree right-hand side e^x + x e^{2x}, solved degree by degree:") (newline)
(define g4 (list (rat-zero) (rat-one) (rat-from-poly (list 0 1))))
(define y4 (tfr-solve b (rat-zero) g4))
(chk "two-degree RHS solved (degree 2 gives y_2 = x/2 - 1/4), certified" (if (equal? y4 (quote no-tower-solution)) #f (tfr-certify b (rat-zero) g4 y4)))

(display "a nonzero base coefficient: y' + y = e^x gives y = (1/2) e^x (degree-1 RDE y_1' + 2 y_1 = 1):") (newline)
(define g5 (list (rat-zero) (rat-one)))
(define y5 (tfr-solve b (rat-one) g5))
(chk "y' + y = e^x solved, y = (1/2) e^x, certified" (if (equal? y5 (quote no-tower-solution)) #f (tfr-certify b (rat-one) g5 y5)))

(newline)
(display "The tower-field RDE decouples by theta-degree into base-field RDEs, each solved one level down -- the") (newline)
(display "recursive Risch procedure calling itself, with the Ei obstruction detected at the per-degree level.") (newline)
