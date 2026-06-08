; MULTI-PARAMETER homogeneous bookkeeping for the coupled Risch differential equation: the general form of the
; completeness layer, solving for SEVERAL homogeneous constants jointly via a linear system (exact Gaussian
; elimination over Q) rather than the single degree-0 constant of rischcrdeh.  When a coupled solve has
; homogeneous degrees-of-freedom at multiple degrees, the forced tail depends linearly on the vector of
; constants, so the terminating choice is the solution of a linear system, found by probing, solved over Q, and
; certified (docs/TRAGER_ROADMAP.md, the summit).
(import "cas/rischcrdem.lisp")
(import "cas/rischintn.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(define t1 (list (list (quote exp) (rat-from-poly (list 0 1)))))
(define t1log (list (list (quote log) (rat-from-poly (list 0 1)))))
(define t2 (list (list (quote exp) (rat-from-poly (list 0 1))) (list (quote exp) (list (rat-zero) (rat-one)))))

(display "Multi-parameter homogeneous bookkeeping: several free constants solved jointly via a linear system.") (newline) (newline)

(display "the exact rational linear solver underneath (Gaussian elimination over Q):") (newline)
(define cols (list (list (rat-from-poly (list 1)) (rat-from-poly (list 1)) (rat-from-poly (list 0)))
                   (list (rat-from-poly (list 1)) (rat-from-poly (list -1)) (rat-from-poly (list 1)))
                   (list (rat-from-poly (list 1)) (rat-from-poly (list 0)) (rat-from-poly (list -1)))))
(define rhs (list (rat-from-poly (list 6)) (rat-from-poly (list 0)) (rat-from-poly (list -1))))
(define sol (te-ratsolve t1 1 cols rhs 3))
(chk "a 3x3 rational system solves exactly to (5/3, 5/3, 8/3)"
     (if (rat-equal? (car sol) (rat-make (list 5) (list 3)))
         (if (rat-equal? (car (cdr sol)) (rat-make (list 5) (list 3))) (rat-equal? (car (cdr (cdr sol))) (rat-make (list 8) (list 3))) #f) #f))

(display "the multi-parameter layer reproduces the single-parameter result -- INT (e^x e^{e^x}) = e^{e^x}:") (newline)
(define f2 (list (te-zero 1) (list (rat-zero) (rat-one))))
(define r (te-crdem-integrate t2 2 f2))
(chk "INT (e^x e^{e^x}) = e^{e^x} solved and certified" (if (equal? (car r) (quote elementary)) (te-crde-certify t2 2 (te-zero 2) f2 (car (cdr r))) #f))

(display "a multi-degree logarithmic integrand, exercising a homogeneous freedom at each degree:") (newline)
(define g (list (rat-zero) (rat-zero) (rat-one)))
(define rl (te-crdem-integrate t1log 1 g))
(display "  INT (log x)^2 dx = ") (display (car rl)) (display "  (= x(log x)^2 - 2x log x + 2x)") (newline)
(chk "INT (log x)^2 solved and certified through the multi-parameter path" (if (equal? (car rl) (quote elementary)) (te-int-certify t1log 1 g (car (cdr rl))) #f))

(display "soundness preserved -- the genuine obstructions are unchanged:") (newline)
(chk "INT e^{e^x} still non-elementary" (equal? (car (te-crdem-integrate t2 2 (list (te-zero 1) (te-one 1)))) (quote non-elementary)))
(chk "INT e^x/x still non-elementary" (equal? (car (te-crdem-integrate t1 1 (list (rat-zero) (rat-make (list 1) (list 0 1))))) (quote non-elementary)))

(newline)
(display "The multi-parameter layer collects the homogeneous degrees-of-freedom, builds the tail-system by") (newline)
(display "probing, solves it exactly over Q, and certifies -- the general completeness algorithm, subsuming the") (newline)
(display "single-parameter case, with soundness held throughout by the differentiation certificate.") (newline)
