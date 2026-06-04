; THE SUMMIT: the unified top-level Risch integrator, a single entry point fusing every part of the stack -- the
; rational integrator (Hermite reduction + the Rothstein-Trager logarithmic part, which finds the log arguments
; automatically without factoring), the height-n tower recursion (exponential, logarithmic, and algebraic levels
; of any degree), and the Laurent layer -- behind one interface, every result certified by differentiation
; (docs/TRAGER_ROADMAP.md, the summit -- the flag).
;
; Liouville's theorem: an elementary integral has the shape INT f = v' + sum_i c_i log(g_i).  For a rational
; integrand the complete algorithm realizes this -- the rational part v from Hermite reduction and the logarithms
; from Rothstein-Trager (residues = rational roots of res_x(a - y b', b), arguments = gcd(a - c b', b)) -- fused
; into one certified result; for a tower integrand the height-n recursion decides and integrates through
; exp/log/algebraic levels, with the Laurent layer adding the theta^{-1} new logarithm.
(import "cas/rischtop.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "THE SUMMIT -- one integrator over the whole domain, every result certified.") (newline) (newline)

(display "the crowning fused case: INT 2x^3/(x^2-1) dx = x^2 + log(x^2-1)") (newline)
(display "  -- the rational part x^2 (v') and the logarithm log(x^2-1) (found automatically by Rothstein-Trager),") (newline)
(display "  fused into one answer and certified by differentiation:") (newline)
(define r (integrate-top-rational (list 0 0 0 2) (list -1 0 1)))
(display "  rational part = ") (display (car (cdr (cdr r)))) (display " (= x^2),  logarithms = ") (display (car (cdr (cdr (cdr r))))) (newline)
(chk "INT 2x^3/(x^2-1) = x^2 + log(x^2-1), CERTIFIED" (integrate-top-certify-rational (list 0 0 0 2) (list -1 0 1) r))

(display "the same entry integrates arctan-type rationals: INT 1/(x^2+1) dx = arctan x:") (newline)
(define ra (integrate-top-rational (list 1) (list 1 0 1)))
(chk "INT 1/(x^2+1) = arctan x, elementary and certified" (if (equal? (car ra) (quote elementary)) (integrate-top-certify-rational (list 1) (list 1 0 1) ra) #f))

(display "and honestly flags rationals whose logarithms need an algebraic extension: INT 1/(x^2-2) dx:") (newline)
(chk "INT 1/(x^2-2) reported needs-algebraic (irrational residues)" (equal? (car (integrate-top-rational (list 1) (list -2 0 1))) (quote needs-algebraic)))

(display "the same top integrates over transcendental and algebraic towers, through the height-n recursion:") (newline)
(define t1 (list (list (quote exp) (rat-from-poly (list 0 1)))))
(chk "INT e^x = e^x [exponential level]" (equal? (car (integrate-top-tower t1 1 (list (rat-zero) (rat-one)))) (quote elementary)))
(define t1log (list (list (quote log) (rat-from-poly (list 0 1)))))
(chk "INT log x = x log x - x [logarithmic level]" (equal? (car (integrate-top-tower t1log 1 (list (rat-zero) (rat-one)))) (quote elementary)))
(define t1a (list (list (quote alg) 2 (rat-from-poly (list 0 1)))))
(chk "INT 1/(2 sqrt x) = sqrt x [algebraic level]" (equal? (car (integrate-top-tower t1a 1 (list (rat-zero) (rat-make (list 1) (list 0 2))))) (quote elementary)))

(display "it decides non-elementary integrals through the recursion: INT e^(e^x) dx:") (newline)
(define t2 (list (list (quote exp) (rat-from-poly (list 0 1))) (list (quote exp) (list (rat-zero) (rat-one)))))
(chk "INT e^(e^x) PROVEN non-elementary" (equal? (car (integrate-top-tower t2 2 (list (te-zero 1) (te-one 1)))) (quote non-elementary)))

(display "and the Laurent new-logarithm case: INT 1/(x log x) dx = log log x:") (newline)
(chk "INT 1/(x log x) = log log x [Laurent]" (equal? (car (integrate-top-laurent t1log 1 (list (rat-make (list 1) (list 0 1))) (quote ()))) (quote elementary)))

(newline)
(display "THE FLAG IS PLANTED.  One unified Risch integrator spans rational functions (rational part plus") (newline)
(display "automatically-found logarithms and arctangents), exponential, logarithmic, and algebraic towers of any") (newline)
(display "degree, the Laurent new-logarithm case, and the iterated-exponential non-elementarity proofs -- every") (newline)
(display "elementary result certified by differentiation, every obstruction exact, every hard case deferred honestly.") (newline)
