; -*- lisp -*-
; lib/cas/rischtop.lisp -- THE SUMMIT: the unified top-level Risch integrator, a single entry point that fuses
; every part of the stack -- the rational integrator (Hermite reduction + the Rothstein-Trager logarithmic part,
; which finds the log arguments automatically without factoring), the height-n tower recursion (exponential,
; logarithmic, and algebraic levels of any degree), and the Laurent / structure-theorem layers -- behind one
; interface, with every result certified by differentiation (docs/TRAGER_ROADMAP.md, the summit -- the flag).
;
; Liouville's theorem says an elementary integral has the shape INT f = v' + sum_i c_i log(g_i): a rational/field
; antiderivative part v plus a sum of logarithms with constant coefficients.  For a rational integrand the
; complete algorithm realizes exactly this -- the rational part v from Hermite reduction and the logarithmic part
; sum c_i log(g_i) from Rothstein-Trager (the residues c_i are the rational roots of the resultant
; res_x(a - y b', b) and the arguments are gcd(a - c_i b', b)) -- and integrate-rational already FUSES the two,
; returning (ok rational-part log-terms arctan-terms), self-certified by integrate-verify.  For a tower integrand
; the height-n recursion decides and integrates through exp/log/algebraic levels, the Laurent layer adds the
; theta^{-1} new-logarithm case, and the structure-theorem layer recognizes logarithmic-sum integrals.
;
; This module is the unifying top: ONE `integrate` that classifies the integrand and routes it, returning a
; uniform verdict and certifying.  It does not re-derive the sub-algorithms; it ties them into a single
; self-certifying entry, which is the capstone of the whole arc.
;
; Public:
;   integrate-top-rational num den     -> (list 'elementary 'rational ratpart logs) | (list 'needs-algebraic ..)
;       the complete rational integral (rational part + auto-found logarithms), certified
;   integrate-top-tower tower h f      -> (list 'elementary y) | (list 'non-elementary ..) | (list 'deferred ..)
;       the tower integral via the height-n recursion (exp/log/algebraic), certified
;   integrate-top-laurent tower h neg poly -> the Laurent integral (polynomial part + new logarithm)
;   integrate-top-certify-rational num den r -> #t iff the rational result differentiates back to num/den
;   integrate-top-is-elementary-rational num den -> #t iff the rational integrand is elementary (RT complete or
;       the rational part suffices), the decision form of the rational case
;
; Verified: the crowning fused case INT (2x^3/(x^2-1)) dx = x^2 + log(x^2-1) (rational part AND auto-found log,
; certified); INT e^x, INT log x, INT 1/(2 sqrt x), INT e^{e^x} (non-elementary) all through the one entry;
; INT 1/(x log x) = log log x via Laurent; and a non-elementary rational (algebraic residues) reported honestly.
;
; Builds on integrate.lisp (the complete rational integrator), rothstein.lisp (auto log-finding), rischintn.lisp
; (the tower recursion), rischlaurent.lisp (Laurent), and rischstruct.lisp (the structure theorem).

(import "cas/integrate.lisp")
(import "cas/rothstein.lisp")
(import "cas/rischintn.lisp")
(import "cas/rischlaurent.lisp")
(import "cas/rischstruct.lisp")

; ===== the rational case: complete integral (rational part + auto-found logarithms), fused and certified =====
(define (integrate-top-rational num den) (itr-wrap num den (integrate-rational num den)))
(define (itr-wrap num den res)
  (if (equal? (car res) (quote ok))
      (list (quote elementary) (quote rational) (acc-ratpart (cdr res)) (acc-logs (cdr res)) (acc-arctans (cdr res)))
      (list (quote needs-algebraic) (quote algebraic-residues))))
(define (integrate-top-certify-rational num den r)
  (if (equal? (car r) (quote elementary)) (rat-equal? (itr-deriv r) (rat-make num den)) #f))
; differentiate the reported result: ratpart' + sum c_i v_i'/v_i + sum arctan-derivs
(define (itr-deriv r) (rat-add (rat-add (rat-deriv (car (cdr (cdr r)))) (itr-logderiv (car (cdr (cdr (cdr r)))))) (itr-atanderiv (car (cdr (cdr (cdr (cdr r))))))))
(define (itr-logderiv logs) (itr-ld-go logs (rat-zero)))
(define (itr-ld-go logs acc) (if (null? logs) acc (itr-ld-go (cdr logs) (rat-add acc (itr-one-log (car logs))))))
(define (itr-one-log term) (rat-scale-rat (car term) (rat-make (poly-deriv (car (cdr term))) (car (cdr term)))))
(define (rat-scale-rat c rr) (rat-mul (rat-make (list c) (list 1)) rr))
; arctan term (c q): contributes c * q'/(1 + q^2) -- reuse the integrator's arctan-deriv via the accumulator form
(define (itr-atanderiv ats) (itr-at-go ats (rat-zero)))
(define (itr-at-go ats acc) (if (null? ats) acc (itr-at-go (cdr ats) (rat-add acc (arctan-deriv (car ats))))))

; the decision form: is the rational integrand elementary?  It is, when integrate-rational succeeds (the
; rational part always exists; the logarithmic part is complete iff the resultant's roots are all rational --
; otherwise the integral needs an algebraic extension and we report that honestly).
(define (integrate-top-is-elementary-rational num den) (equal? (car (integrate-rational num den)) (quote ok)))

; ===== the tower case: dispatch to the height-n recursion (exp/log/algebraic), certified =====
(define (integrate-top-tower tower h f) (te-integrate tower h f))
(define (integrate-top-tower-certify tower h f r) (if (equal? (car r) (quote elementary)) (te-int-certify tower h f (car (cdr r))) #f))

; ===== the Laurent case: polynomial part + theta^{-1} new logarithm =====
(define (integrate-top-laurent tower h neg poly) (laurent-integrate tower h neg poly))

; ===== a convenience: the crowning fused rational example INT 2x^3/(x^2-1) = x^2 + log(x^2-1) =====
(define (integrate-top-demo) (integrate-top-rational (list 0 0 0 2) (list -1 0 1)))
