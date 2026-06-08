; -*- lisp -*-
; lib/cas/rischlaurent.lisp -- LAURENT INTEGRANDS over a logarithmic level: integrate f = sum_k c_k theta^k where
; the sum may include NEGATIVE powers of theta = log(b), extending the polynomial-in-theta recursion to the
; Laurent case and capturing the classic new-logarithm integrals (docs/TRAGER_ROADMAP.md, the summit, "Laurent
; integrands, e.g. li").
;
; The structure (theta = log(b), u = theta' = b'/b, a base-field element).  A Laurent integrand splits into:
;   - the POLYNOMIAL part (powers k >= 0): integrated by the existing height-n log integrator (te-integrate);
;   - the theta^{-1} coefficient c_{-1}: by the Risch theory, INT c_{-1} theta^{-1} is elementary as a NEW
;     logarithm exactly when c_{-1}/u is a constant m (a base-field element with zero derivative), giving
;     INT (m u) / theta = m INT theta'/theta = m log(theta) = m log(log b).  (The canonical case
;     INT 1/(x log x) = log log x has c_{-1} = 1/x = u, so m = 1.)
;   - deeper negative powers (theta^{-2}, ...) or a non-constant c_{-1}/u: not handled here, returned as an
;     honest 'deferred -- soundness is preserved because every elementary result is certified by differentiation.
;
; Representation.  A Laurent integrand is given as a pair (neg . poly): `neg` is the list of negative-power
; coefficients (neg = (c_{-1} c_{-2} ...), so (car neg) is the coefficient of theta^{-1}), and `poly` is the
; usual low-to-high list of non-negative-power coefficients.  The result, when elementary, is reported as
; (list 'elementary 'poly-part y 'log-part m) where y integrates the polynomial part and m is the coefficient of
; the new logarithm log(theta) (zero if none); a convenience certifier checks the polynomial part.
;
; Public:
;   laurent-integrate tower h neg poly -> (list 'elementary y m) | (list 'non-elementary ..) | (list 'deferred ..)
;       integrate sum_k c_k theta^k over Q(...)(log b); y the polynomial-part antiderivative, m the new-log coeff
;   laurent-newlog-coeff tower h cm1   -> m | 'none : the constant m = c_{-1}/u if constant, else 'none
;   laurent-int-x-logx                 -> the worked example INT 1/(x log x) dx = log log x (m = 1)
;
; Verified: INT 1/(x log x) = log log x (m = 1, no polynomial part); INT (log x + 1/(x log x)) combines a
; polynomial-part antiderivative with the new logarithm; a non-constant c_{-1}/u defers; theta^{-2} defers.
;
; Builds on rischintn.lisp (the height-n integrator for the polynomial part) and rischtowern.lisp / tower.lisp.

(import "cas/rischintn.lisp")
(import "cas/rischtowern.lisp")
(import "cas/tower.lisp")

(define (lau-nth l k) (if (= k 0) (car l) (lau-nth (cdr l) (- k 1))))
(define (lau-len l) (if (null? l) 0 (+ 1 (lau-len (cdr l)))))

; ----- the new-logarithm coefficient: m = c_{-1}/u if it is a base-field constant (zero derivative), else none.
; theta = log(b) at height h, u = b'/b is a height-(h-1) element. -----
(define (laurent-u tower h) (te-rat-div tower (- h 1) (te-deriv tower (- h 1) (te-level-b tower h)) (te-level-b tower h)))
(define (laurent-newlog-coeff tower h cm1)
  (laurent-checkconst tower h (te-rat-div tower (- h 1) cm1 (laurent-u tower h))))
(define (laurent-checkconst tower h m)
  (if (te-equal? tower (- h 1) (te-deriv tower (- h 1) m) (te-zero (- h 1))) m (quote none)))

; ----- integrate the Laurent integrand -----
(define (laurent-integrate tower h neg poly)
  (laurent-dispatch tower h neg poly (laurent-negok? tower h neg)))
; the negative part is handled only if it is at most theta^{-1} (length <= 1) and that term yields a new log.
(define (laurent-negok? tower h neg)
  (cond ((null? neg) (quote nolog))                         ; no negative part
        ((null? (cdr neg)) (laurent-newlog-coeff tower h (car neg)))  ; only theta^{-1}: try the new-log coeff
        (else (quote deep))))                               ; theta^{-2} or deeper: not handled
(define (laurent-dispatch tower h neg poly status)
  (cond ((equal? status (quote deep)) (list (quote deferred) (quote deep-laurent)))
        ((equal? status (quote none)) (list (quote deferred) (quote nonconstant-residue)))
        ((equal? status (quote nolog)) (laurent-polyonly tower h poly))
        (else (laurent-withlog tower h poly status))))
; polynomial part only: delegate to the height-n integrator, new-log coefficient zero.
(define (laurent-polyonly tower h poly) (laurent-poly-result (te-integrate tower h poly) (te-zero (- h 1))))
(define (laurent-poly-result r m)
  (cond ((equal? (car r) (quote elementary)) (list (quote elementary) (car (cdr r)) m))
        ((equal? (car r) (quote non-elementary)) (list (quote non-elementary) (quote poly-part-obstruction)))
        (else (list (quote deferred) (quote poly-part-deferred)))))
; polynomial part + a new logarithm with coefficient m.
(define (laurent-withlog tower h poly m) (laurent-poly-result (te-integrate tower h poly) m))

; ----- the worked example: INT 1/(x log x) dx = log log x.  tower = ((log x)), theta = log x, u = 1/x.
; integrand = u theta^{-1}, so neg = (u) = (1/x), poly = () (no non-negative part). -----
(define (laurent-int-x-logx) (laurent-integrate (list (list (quote log) (rat-from-poly (list 0 1)))) 1 (list (rat-make (list 1) (list 0 1))) (quote ())))
