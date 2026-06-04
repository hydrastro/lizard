; The HEIGHT-N Risch recursion: a uniform tower-element algebra with a recursive derivation D that descends the
; tower one level at a time, and an integrator INT f = (D y = f) that delegates its per-degree subproblems to a
; call one level down, all the way to the rational RDE over Q(x).  This is the recursion calling itself at every
; level -- the structural completion of the Risch descent over arbitrary-height transcendental towers
; (docs/TRAGER_ROADMAP.md, the summit).
;
; A tower element at height h is a polynomial in theta_h with height-(h-1) coefficients (height 0 = Q(x)).  The
; derivation and integration recurse on h: exp levels are diagonal (per-degree RDEs one level down), log levels
; shift degree (top-down).  Soundness is preserved by an honest 'deferred for the coupled case (a per-degree
; coefficient of positive lower-degree), so a returned answer is always certified by D y = f.
(import "cas/rischintn.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The height-n Risch recursion: derivation and integration descending the tower, bottoming at Q(x).") (newline) (newline)

(display "the recursive derivation D, descending on height:") (newline)
(define t1 (list (list (quote exp) (rat-from-poly (list 0 1)))))
(display "  D(e^x) = e^x (height 1): ") (display (te-equal? t1 1 (te-deriv t1 1 (list (rat-zero) (rat-one))) (list (rat-zero) (rat-one)))) (newline)
(define t2 (list (list (quote exp) (rat-from-poly (list 0 1))) (list (quote exp) (list (rat-zero) (rat-one)))))
(define eta (list (te-zero 1) (te-one 1)))
(chk "D(e^{e^x}) = e^x e^{e^x} (height 2 -- D descends 2 -> 1 -> Q(x))" (te-equal? t2 1 (te-coeff 2 (te-deriv t2 2 eta) 1) (list (rat-zero) (rat-one))))

(display "height-1 integration through the recursive engine (reproducing the classic verdicts):") (newline)
(define r1 (te-integrate t1 1 (list (rat-zero) (rat-one))))
(chk "INT e^x = e^x, certified" (if (equal? (car r1) (quote elementary)) (te-int-certify t1 1 (list (rat-zero) (rat-one)) (car (cdr r1))) #f))
(define r2 (te-integrate t1 1 (list (rat-zero) (rat-from-poly (list 0 1)))))
(chk "INT x e^x = (x-1) e^x, certified" (if (equal? (car r2) (quote elementary)) (te-int-certify t1 1 (list (rat-zero) (rat-from-poly (list 0 1))) (car (cdr r2))) #f))
(display "  INT e^x/x (Ei): ") (display (te-integrate t1 1 (list (rat-zero) (rat-make (list 1) (list 0 1))))) (newline)
(chk "INT e^x/x PROVEN non-elementary" (equal? (car (te-integrate t1 1 (list (rat-zero) (rat-make (list 1) (list 0 1))))) (quote non-elementary)))
(define t1log (list (list (quote log) (rat-from-poly (list 0 1)))))
(define r4 (te-integrate t1log 1 (list (rat-zero) (rat-one))))
(chk "INT log x = x log x - x, certified" (if (equal? (car r4) (quote elementary)) (te-int-certify t1log 1 (list (rat-zero) (rat-one)) (car (cdr r4))) #f))

(display "HEIGHT 2 -- a genuine integral computed by the recursion descending 2 -> 1 -> Q(x):") (newline)
(define t2log (list (list (quote exp) (rat-from-poly (list 0 1))) (list (quote log) (list (rat-one) (rat-one)))))
(define integ2 (list (te-zero 1) (te-one 1)))   ; theta_2 = log(e^x + 1)
(define r5 (te-integrate t2log 2 integ2))
(display "  INT log(e^x + 1) dx = ") (display (car r5)) (newline)
(chk "INT log(e^x + 1) computed at height 2 and certified (recursion descends through height 1)" (if (equal? (car r5) (quote elementary)) (te-int-certify t2log 2 integ2 (car (cdr r5))) #f))

(display "the exp-over-exp tower, now SOLVED by nesting the coupled recurrence: INT e^{e^x} is non-elementary,") (newline)
(display "derived through the recursion (the top-degree subproblem is the coupled height-1 RDE c' + e^x c = 1):") (newline)
(display "  INT e^{e^x} dx: ") (display (te-integrate t2 2 (list (te-zero 1) (te-one 1)))) (newline)
(chk "INT e^{e^x} PROVEN non-elementary through the recursion (no longer deferred)" (equal? (car (te-integrate t2 2 (list (te-zero 1) (te-one 1)))) (quote non-elementary)))
(display "and the product INT (e^x e^{e^x}) stays sound (an honest verdict, never an uncertified guess):") (newline)
(define rprod (te-integrate t2 2 (list (te-zero 1) (list (rat-zero) (rat-one)))))
(chk "INT (e^x e^{e^x}) is sound (elementary+certified or honest deferral)" (cond ((equal? (car rprod) (quote non-elementary)) #f) ((equal? (car rprod) (quote deferred)) #t) (else (te-int-certify t2 2 (list (te-zero 1) (list (rat-zero) (rat-one))) (car (cdr rprod))))))

(newline)
(display "The Risch descent now runs at arbitrary height: the derivation and integrator recurse one level at a") (newline)
(display "time, height-1 is complete and certified, a height-2 integral is computed by the descent through height 1") (newline)
(display "to Q(x), and the coupled exp-over-exp tower is now SOLVED -- INT e^{e^x} proven non-elementary by the recursion.") (newline)
