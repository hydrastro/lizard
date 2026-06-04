; The unified HEIGHT-1 Risch integration decider: decides INT f dx for f in a height-1 tower K_1 = Q(x)(theta),
; theta = exp(b) OR log(b), by reducing to the general coupled tower-field RDE (rischcoupled) with a ZERO
; multiplier coefficient -- so INT f is exactly the y with D y = f.  ONE entry point integrating over either kind
; of height-1 transcendental extension, dispatching on the level type and bottoming out at the rational RDE over
; Q(x) (docs/TRAGER_ROADMAP.md, the summit, "even more").
;
; Antidifferentiation INT f = y is solving D y = f, the homogeneous-coefficient coupled RDE: for an exponential
; level it decouples by theta-degree into base RDEs (bottom-up); for a logarithmic level the degree-shifting
; derivation is solved top-down.  A solution proves elementarity (certified by re-differentiating); its absence
; proves non-elementarity.
(import "cas/rischint1.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define b (rat-from-poly (list 0 1)))   ; b = x

(display "One Risch integration decider over both height-1 tower types, via D y = f and the coupled RDE.") (newline) (newline)

(display "exponential tower Q(x)(e^x):") (newline)
(define e1 (ri1-integrate-exp b (list (rat-zero) (rat-one))))
(display "  INT e^x dx = ") (display e1) (display "  (y = e^x)") (newline)
(chk "INT e^x = e^x elementary, certified" (if (equal? (car e1) (quote elementary)) (ri1-certify-exp b (list (rat-zero) (rat-one)) (car (cdr e1))) #f))
(define e2 (ri1-integrate-exp b (list (rat-zero) (rat-from-poly (list 0 1)))))
(chk "INT x e^x = (x-1) e^x elementary, certified" (if (equal? (car e2) (quote elementary)) (ri1-certify-exp b (list (rat-zero) (rat-from-poly (list 0 1))) (car (cdr e2))) #f))
(display "  INT e^x/x dx -- the exponential integral Ei, PROVEN non-elementary:") (newline)
(display "  ") (display (ri1-integrate-exp b (list (rat-zero) (rat-make (list 1) (list 0 1))))) (newline)
(chk "INT e^x/x PROVEN non-elementary" (equal? (car (ri1-integrate-exp b (list (rat-zero) (rat-make (list 1) (list 0 1))))) (quote non-elementary)))

(display "logarithmic tower Q(x)(log x):") (newline)
(define l1 (ri1-integrate-log (rat-make (list 1) (list 0 1)) (list (rat-zero) (rat-one))))
(display "  INT log x dx = ") (display l1) (display "  (= x log x - x)") (newline)
(chk "INT log x = x log x - x elementary, certified" (if (equal? (car l1) (quote elementary)) (ri1-certify-log (rat-make (list 1) (list 0 1)) (list (rat-zero) (rat-one)) (car (cdr l1))) #f))
(define l2 (ri1-integrate-log (rat-make (list 1) (list 0 1)) (list (rat-zero) (rat-zero) (rat-one))))
(display "  INT (log x)^2 dx = ") (display l2) (display "  (= x(log x)^2 - 2x log x + 2x)") (newline)
(chk "INT (log x)^2 elementary, certified" (if (equal? (car l2) (quote elementary)) (ri1-certify-log (rat-make (list 1) (list 0 1)) (list (rat-zero) (rat-zero) (rat-one)) (car (cdr l2))) #f))

(newline)
(display "A single decider integrates over either height-1 tower: D y = f reduces to the coupled RDE, decoupling") (newline)
(display "for exponentials and degree-shifting for logarithms, bottoming out at the rational RDE -- the complete") (newline)
(display "height-1 Risch integral, with erf-relative Ei proven non-elementary and the log integrals found in closed form.") (newline)
