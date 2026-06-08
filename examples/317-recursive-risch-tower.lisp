; The RECURSIVE Risch decision procedure: ONE procedure that decides elementarity over a multi-level
; transcendental tower by reducing, level by level, to integration subproblems ONE LEVEL DOWN, bottoming out at
; Q(x) (rational integration, always elementary).  This unifies the per-class deciders (liouville,
; liouvillelog, liouvillerat) into a single tower-aware recursion -- the structural heart of the full Risch
; algorithm (docs/TRAGER_ROADMAP.md, the summit).
;
; The exponential reduction (theta = exp(b), theta' = b' theta): an integrand sum_i a_i theta^i reduces, per
; degree, to INT a_i theta^i = c_i theta^i iff the Risch differential equation c_i' + i b' c_i = a_i is solvable
; ONE LEVEL DOWN (degree 0 is an ordinary integration there).  The integral is elementary iff every degree's
; subproblem is solvable.
;
; The deep phenomenon: the iterated exponential E_n = exp(E_{n-1}).  INT E_n needs the RDE c' + E_{n-1}' c = 1,
; and since E_{n-1}' = E_1...E_{n-1} (a nonconstant exponential) the formal solution has a NON-TERMINATING
; degree tail -- so INT E_n is NON-ELEMENTARY for n >= 2, decided by the recursion, sitting exactly opposite the
; elementary full-product INT(E_1...E_n) = E_n.
(import "cas/rischtower.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The recursive Risch decision procedure: deciding integrals over a multi-level tower by recursion.") (newline) (newline)

(display "the per-level decision content -- solvability of the Risch differential equation c' + w c = target:") (newline)
(chk "c' + c = x is solvable (c = x - 1)" (equal? (rt-rde-exp-const-solvable? (list 1) (list 0 1)) #t))
(chk "c' + 2x c = 1 is NOT solvable (the e^{x^2} obstruction, one level down)" (equal? (rt-rde-exp-const-solvable? (list 0 2) (list 1)) #f))
(chk "c' + 2x c = 2x is solvable (c = 1)" (equal? (rt-rde-exp-const-solvable? (list 0 2) (list 0 2)) #t))

(display "the recursion deciding the iterated exponential E_n = exp(E_{n-1}):") (newline)
(display "  INT E_1 dx (= e^x): ") (display (rt-decide-iterated-exp 1)) (newline)
(chk "INT E_1 (e^x) is elementary" (equal? (car (rt-decide-iterated-exp 1)) (quote elementary)))
(display "  INT E_2 dx (= e^{e^x}): ") (display (rt-decide-iterated-exp 2)) (newline)
(chk "INT E_2 (e^{e^x}) is PROVEN non-elementary -- the recursion hits a non-terminating RDE tail" (equal? (car (rt-decide-iterated-exp 2)) (quote non-elementary)))
(display "  INT E_3 dx (= e^{e^{e^x}}): ") (display (rt-decide-iterated-exp 3)) (newline)
(chk "INT E_3 is PROVEN non-elementary" (equal? (car (rt-decide-iterated-exp 3)) (quote non-elementary)))

(display "the same recursion gives the OPPOSITE verdict for the full product (which IS elementary):") (newline)
(display "  INT (E_1 E_2 E_3) dx = E_3: ") (display (rt-decide-iterated-product 3)) (newline)
(chk "INT (E_1 ... E_n) = E_n is elementary -- distinguished from the single monomial INT E_n" (equal? (car (rt-decide-iterated-product 3)) (quote elementary)))

(display "the reduction made explicit -- INT (a_0 + a_1 e^x) dx produces its per-degree subproblems:") (newline)
(display "  ") (display (rt-reduce-exp (list (list 0 1) (list 1)) (list 0 1))) (newline)
(display "  (degree 0: integrate a_0 in the base; degree 1: the RDE c' + b' c = a_1, one level down)") (newline)
(chk "the recursion bottoms out at Q(x) -- rational integration, always elementary" (equal? (rt-bottom-rational) (quote elementary)))

(newline)
(display "One recursive procedure decides integrals over the whole tower: it reduces each level to subproblems") (newline)
(display "below, proves INT e^{e^x} and INT e^{e^{e^x}} non-elementary by the non-terminating RDE tail, and bottoms") (newline)
(display "out at rational integration -- the structural heart of the Risch algorithm.") (newline)
