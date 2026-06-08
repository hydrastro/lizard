; 227-risch-de-rational-f.lisp -- the Risch DE with a rational coefficient f, and integration of
; R(x) e^{u(x)} for rational R and RATIONAL u.
;
; rischde.lisp handled a polynomial f (so u had to be a polynomial).  Here f may be rational, in
; the weakly-normalized case -- and f = u' is always weakly normalized, because the derivative of
; a rational function has all residues zero.  So R(x) e^{u(x)} with u rational is integrable by
; this method: it equals h e^u with h' + u' h = R.  The denominator of h is bounded by
; prod p^{max(0, l - max(k,1))} over the squarefree factors p^l of denom(R), with k the pole
; order of u' at p; for polynomial u every k = 0 and this is the gcd(E,E') bound of rischde.lisp.
; Every answer is differentiation-certified.  `must` raises on failure.

(import "cas/rderat.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'rdr-check-failed)))

(display "Risch DE over Q(x) with rational f, and INT R(x) e^{u(x)} for rational u") (newline) (newline)

(display "1. INT R(x) e^{u(x)} dx with u RATIONAL (beyond rischde.lisp's polynomial u)") (newline)
(display "    INT -1/x^2 e^{1/x} dx  = e^{1/x}") (newline)
(must "  h = 1, certified" (and (equal? (car (int-rat-exp-rat (list -1) (list 0 0 1) (list 1) (list 0 1))) 'elementary)
                                (int-rat-exp-rat-verify (list -1) (list 0 0 1) (list 1) (list 0 1))))
(display "    INT (x-1)/x e^{1/x} dx  = x e^{1/x}") (newline)
(must "  h = x, certified"  (int-rat-exp-rat-verify (list -1 1) (list 0 1) (list 1) (list 0 1)))
(display "    INT 2/x^3 e^{-1/x^2} dx  = e^{-1/x^2}") (newline)
(must "  h = 1, certified"  (int-rat-exp-rat-verify (list 2) (list 0 0 0 1) (list -1) (list 0 0 1)))
(newline)

(display "2. the polynomial-u case still works through the same code") (newline)
(must "INT 2x e^{x^2} dx = e^{x^2}, certified"        (int-rat-exp-rat-verify (list 0 2) (list 1) (list 0 0 1) (list 1)))
(must "INT (x-1)/x^2 e^x dx = e^x/x, certified"       (int-rat-exp-rat-verify (list -1 1) (list 0 0 1) (list 0 1) (list 1)))
(newline)

(display "3. self-checking round trips: for rational g, u set R = g' + u' g, recover h = g") (newline)
(must "g = x/(x+1), u = -1/x  round-trips (certified)"
      (let ((up (rde-rderiv (cons (list 0 -1) (list 0 1)))))
        (let ((g (cons (list 0 1) (list 1 1))))
          (let ((R (rde-radd (rde-rderiv g) (rde-rmul up g))))
            (int-rat-exp-rat-verify (car R) (cdr R) (list 0 -1) (list 0 1))))))
(newline)

(display "4. a proven non-elementary case") (newline)
(must "INT e^{1/x} dx is non-elementary" (equal? (car (int-rat-exp-rat (list 1) (list 1) (list 1) (list 0 1))) 'non-elementary))
(newline)

(display "all rational-f Risch-DE checks passed.") (newline)
