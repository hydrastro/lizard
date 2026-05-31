; -*- lisp -*-
; lib/cas/solve.lisp — solving polynomial equations over Q (and Q-bar), plus
; Sturm sequences for counting real roots.
;
; `solve-poly` factors the polynomial (reusing the certified factorizer) and
; reads roots off the irreducible factors:
;   * linear factor      -> exact rational root
;   * quadratic factor   -> the two conjugate roots as exact surds p +- q*sqrt(D),
;                           represented in Q(sqrt(D)) so they can be checked
;   * factor of degree>=3 -> RootOf(factor), annotated with the number of REAL
;                            roots counted exactly by a Sturm sequence
; Multiplicities are carried through.  Rational and quadratic roots are CERTIFIED
; by substituting them back (in Q for rationals, in Q(sqrt(D)) for surds) and
; checking the result is zero -- so a wrong root cannot be reported.
;
; Top-level helpers only; builds on factor.lisp and algnum.lisp.

(import "cas/factor.lisp")
(import "cas/algnum.lisp")

; ============================================================
;  Sturm sequences and real-root counting
; ============================================================
(define (sturm-chain-loop a b acc)         ; a = f_{i-1}, b = f_i
  (if (poly-zero? b) (reverse acc)
    (sturm-chain-loop b (poly-neg (poly-rem a b)) (cons b acc))))
(define (sturm-chain f) (sturm-chain-loop f (poly-deriv f) (list f)))

(define (sign x) (cond ((> x 0) 1) ((< x 0) -1) (else 0)))

; sign of poly value as x -> +inf  (sign of leading coeff) and -inf
(define (sturm-sign-pos-inf p) (sign (poly-lead p)))
(define (sturm-sign-neg-inf p) (* (sign (poly-lead p)) (expt -1 (poly-deg p))))

(define (count-variations signs)           ; sign changes ignoring zeros
  (count-var-loop (filter-nonzero signs) 0))
(define (filter-nonzero xs) (if (null? xs) '() (if (= (car xs) 0) (filter-nonzero (cdr xs)) (cons (car xs) (filter-nonzero (cdr xs))))))
(define (count-var-loop s acc)
  (if (or (null? s) (null? (cdr s))) acc
    (count-var-loop (cdr s) (if (= (car s) (car (cdr s))) acc (+ acc 1)))))

(define (signs-at chain x) (map (lambda (p) (sign (poly-eval p x))) chain))
(define (signs-pos-inf chain) (map sturm-sign-pos-inf chain))
(define (signs-neg-inf chain) (map sturm-sign-neg-inf chain))

; number of DISTINCT real roots of f over all of R
(define (count-real-roots f)
  (if (poly-const? f) 0
    (let ((chain (sturm-chain f)))
      (- (count-variations (signs-neg-inf chain)) (count-variations (signs-pos-inf chain))))))

; number of distinct real roots in the half-open interval (a, b]
(define (count-real-roots-in f a b)
  (let ((chain (sturm-chain f)))
    (- (count-variations (signs-at chain a)) (count-variations (signs-at chain b)))))

; ============================================================
;  solving
;  a solution is (list descriptor multiplicity); descriptor is one of
;    (rat r)            an exact rational root
;    (alg a)            an algebraic number (surd) in Q(sqrt D)
;    (rootof f n-real)  the deg(f) roots of irreducible f, n-real of them real
; ============================================================
(define (quad-roots f mult)                ; f = (c b a) irreducible -> two surd roots
  (let ((a (poly-coeff f 2)) (b (poly-coeff f 1)) (c (poly-coeff f 0)))
    (let ((disc (- (* b b) (* 4 a c))))
      (let ((sd (simplify-surd disc)))
        (let ((minp (list (- 0 (car (cdr sd))) 0 1))      ; x^2 - d (d squarefree)
              (p0 (/ (- 0 b) (* 2 a))) (q (/ (car sd) (* 2 a))))
          (list (list (list 'alg (alg-make minp (list p0 q))) mult)
                (list (list 'alg (alg-make minp (list p0 (- 0 q)))) mult)))))))

(define (solve-factor mf)                  ; mf = (mult f) -> list of solutions
  (let ((mult (car mf)) (f (car (cdr mf))))
    (cond ((= (poly-deg f) 1)
           (list (list (list 'rat (/ (- 0 (poly-coeff f 0)) (poly-coeff f 1))) mult)))
          ((= (poly-deg f) 2) (quad-roots f mult))
          (else (list (list (list 'rootof (poly-monic f) (count-real-roots f)) mult))))))

(define (solve-factors mfs)
  (if (null? mfs) '() (append (solve-factor (car mfs)) (solve-factors (cdr mfs)))))

(define (solve-poly p)
  (let ((p (poly-norm p)))
    (if (poly-const? p) 'no-finite-solutions     ; constant (incl. 0) -> none / all
      (solve-factors (car (cdr (factor-Q p)))))))

; solve  lhs = rhs  for the given variable symbol
(define (solve-expr lhs rhs var) (solve-poly (poly-sub (expr->poly lhs var) (expr->poly rhs var))))

; ============================================================
;  certificate: substitute each (rational / surd) root back -> 0
; ============================================================
(define (root-verify p sol)
  (let ((d (car sol)))
    (cond ((equal? (car d) 'rat) (= (poly-eval p (car (cdr d))) 0))
          ((equal? (car d) 'alg) (alg-root? p (car (cdr d))))
          (else (poly-divides? (car (cdr d)) p)))))   ; rootof: factor divides p
(define (solutions-verify p sols) (all-true (map (lambda (s) (root-verify p s)) sols)))
(define (all-true xs) (if (null? xs) #t (and (car xs) (all-true (cdr xs)))))

; ============================================================
;  display
; ============================================================
(define (mult-suffix m) (if (= m 1) "" (string-append "  (multiplicity " (number->string m) ")")))
(define (solution-string sol)
  (let ((d (car sol)) (m (car (cdr sol))))
    (cond ((equal? (car d) 'rat) (string-append "x = " (rat->string (car (cdr d))) (mult-suffix m)))
          ((equal? (car d) 'alg) (string-append "x = " (alg->string (car (cdr d))) (mult-suffix m)))
          (else (string-append "x = RootOf(" (poly->string (car (cdr d)) "x") ")  ["
                               (number->string (poly-deg (car (cdr d)))) " roots, "
                               (number->string (car (cdr (cdr d)))) " real]" (mult-suffix m))))))
(define (solutions->string sols)
  (if (equal? sols 'no-finite-solutions) "no solutions (or all x)"
    (sols->str-loop sols)))
(define (sols->str-loop sols)
  (if (null? sols) ""
    (let ((rest (sols->str-loop (cdr sols))))
      (if (equal? rest "") (solution-string (car sols))
        (string-append (solution-string (car sols)) "\n" rest)))))
