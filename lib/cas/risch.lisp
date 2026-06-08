; -*- lisp -*-
; lib/cas/risch.lisp — transcendental Risch integration over a single monomial
; extension of Q(x): theta = exp(u) or theta = log(x), with polynomial
; coefficients.  This is the first rung of the actual Risch decision procedure:
; it computes an elementary antiderivative when one exists and PROVES non-
; elementarity when one does not (e.g. INT e^(x^2) dx).
;
; Representation.  A "tower polynomial" is a polynomial in theta whose
; coefficients are polynomials in x: a list (low-to-high in theta) of poly
; coefficient-lists.  The zero tower polynomial is ().
;
; Derivation.  For theta = exp(u):   D(sum a_i theta^i) = sum (a_i' + i u' a_i) theta^i
;              (diagonal in the theta-degree).
;              For theta = log(x):    D(sum a_i theta^i) = sum (a_i' + (i+1) a_{i+1}/x) theta^i.
;
; Decision + certificate.  Exponential: INT a_i theta^i = b_i theta^i where b_i
; solves the Risch differential equation  b_i' + i u' b_i = a_i  over Q[x]; with
; polynomial data any rational solution is polynomial, so a degree bound + linear
; solve DECIDES it.  Logarithmic (primitive): a triangular recurrence of
; integrations.  Every elementary result is checked by differentiating it back
; with D and comparing to the integrand (tpoly-equal?), so a wrong antiderivative
; cannot be reported.
;
; Top-level helpers only; builds on lib/cas/poly.lisp.

(import "cas/poly.lisp")

; ---- polynomial antiderivative over Q (power rule, zero constant term) ----
(define (pint p k) (if (null? p) '() (cons (/ (car p) (+ k 1)) (pint (cdr p) (+ k 1)))))
(define (poly-integral p) (poly-norm (cons 0 (pint p 0))))

; ============================================================
;  tower-polynomial normalization / equality / display
; ============================================================
(define (tdrop rc) (cond ((null? rc) '()) ((poly-zero? (car rc)) (tdrop (cdr rc))) (else rc)))
(define (tpoly-norm A) (reverse (tdrop (reverse (map poly-norm A)))))
(define (tpoly-equal? A B) (equal? (tpoly-norm A) (tpoly-norm B)))
(define (tpoly-zero? A) (null? (tpoly-norm A)))

(define (tpoly->string A th)
  (let ((n (tpoly-norm A))) (if (null? n) "0" (tp-terms (reverse n) (- (length n) 1) th #t))))
(define (tp-terms rc deg th first)
  (if (null? rc) (if first "0" "")
    (if (poly-zero? (car rc)) (tp-terms (cdr rc) (- deg 1) th first)
      (string-append (if first "" " + ")
                     "(" (poly->string (car rc) "x") ")"
                     (cond ((= deg 0) "") ((= deg 1) (string-append "*" th))
                           (else (string-append "*" th "^" (number->string deg))))
                     (tp-terms (cdr rc) (- deg 1) th #f)))))

; ============================================================
;  derivation D
; ============================================================
(define (D-exp-terms A i up)                ; theta = exp(u), up = u'
  (if (null? A) '()
    (cons (poly-add (poly-deriv (car A)) (poly-scale i (poly-mul up (car A))))
          (D-exp-terms (cdr A) (+ i 1) up))))
(define (D-exp A u) (D-exp-terms A 0 (poly-deriv u)))

(define (D-log-terms B i)                   ; theta = log(x), theta' = 1/x
  (if (null? B) '()
    (cons (poly-add (poly-deriv (car B))
                    (if (null? (cdr B)) '()
                        (poly-scale (+ i 1) (poly-div (car (cdr B)) (list 0 1)))))   ; (i+1) a_{i+1}/x
          (D-log-terms (cdr B) (+ i 1)))))
(define (D-log B) (D-log-terms B 0))

; ============================================================
;  Risch differential equation over Q[x]:  solve  b' + f b = g  for b in Q[x]
;  Returns the polynomial b, or 'none if no polynomial (hence no rational) sol.
; ============================================================
(define (rde-degree-bound f g)
  (cond ((poly-zero? f) (+ (poly-deg g) 1))
        ((= (poly-deg f) 0) (poly-deg g))
        (else (- (poly-deg g) (poly-deg f)))))

(define (L-of-monomial f c j)               ; c * (d/dx + f)(x^j) = c j x^{j-1} + c f x^j
  (poly-add (if (= j 0) '() (poly-monomial (* j c) (- j 1)))
            (poly-scale c (poly-mul f (poly-monomial 1 j)))))

(define (rde-peel f g j b)                  ; determine coeff of x^j down to x^0
  (if (< j 0) (if (poly-zero? g) b 'none)
    (let ((c (/ (poly-coeff g (+ j (poly-deg f))) (poly-lead f))))
      (rde-peel f (poly-sub g (L-of-monomial f c j)) (- j 1) (poly-add b (poly-monomial c j))))))

(define (rde-solve f g)
  (cond ((poly-zero? g) '())                                  ; b = 0
        ((poly-zero? f) (poly-integral g))                    ; b' = g
        (else (let ((n (rde-degree-bound f g)))
                (if (< n 0) 'none (rde-peel f g n '()))))))

; ============================================================
;  exponential case:  INT (sum a_i theta^i) dx,  theta = exp(u)
;  -> (list 'elementary B) | (list 'non-elementary i)
; ============================================================
(define (int-exp-terms A i up acc)
  (if (null? A) (list 'elementary (reverse acc))
    (let ((bi (if (= i 0) (poly-integral (car A)) (rde-solve (poly-scale i up) (car A)))))
      (if (equal? bi 'none) (list 'non-elementary i)
        (int-exp-terms (cdr A) (+ i 1) up (cons bi acc))))))
(define (integrate-exp A u) (int-exp-terms A 0 (poly-deriv u) '()))

; ============================================================
;  logarithmic primitive case:  INT (sum a_i theta^i) dx, theta = log(x)
;  triangular recurrence (always elementary for polynomial coefficients)
; ============================================================
(define (nth-coeff A i) (if (or (null? A) (< i 0)) '() (if (= i 0) (car A) (nth-coeff (cdr A) (- i 1)))))
(define (int-log-build A i bnext acc)       ; i counts DOWN from top; bnext = b_{i+1}
  (if (< i 0) acc
    (let ((bi (poly-integral (poly-sub (nth-coeff A i)
                                       (poly-scale (+ i 1) (poly-div bnext (list 0 1)))))))
      (int-log-build A (- i 1) bi (cons bi acc)))))
(define (integrate-log A) (list 'elementary (int-log-build A (- (length (tpoly-norm A)) 1) '() '())))

; ============================================================
;  certified drivers
; ============================================================
(define (risch-exp A u)                     ; -> (list 'elementary B) certified, or 'non-elementary
  (let ((r (integrate-exp A u)))
    (if (equal? (car r) 'non-elementary) r
      (if (tpoly-equal? (D-exp (car (cdr r)) u) A) r (list 'certificate-failed)))))
(define (risch-log A)                       ; theta = log(x)
  (let ((r (integrate-log A)))
    (if (tpoly-equal? (D-log (car (cdr r))) A) r (list 'certificate-failed))))

(define (risch-result->string r th)
  (cond ((equal? (car r) 'non-elementary)
         (string-append "no elementary antiderivative (proved; theta^" (number->string (car (cdr r))) " term)"))
        ((equal? (car r) 'certificate-failed) "CERTIFICATE FAILED")
        (else (tpoly->string (car (cdr r)) th))))
