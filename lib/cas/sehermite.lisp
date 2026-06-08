; -*- lisp -*-
; lib/cas/sehermite.lisp -- SUPERELLIPTIC HERMITE REDUCTION: integration of differentials P(x) y^j / g(x) dx on
; the curve y^n = g(x), the direct generalization of the hyperelliptic (n=2) Hermite reduction in hyperell.lisp
; to arbitrary degree n.  This is a step of the Rung-4 integration payoff (docs/TRAGER_ROADMAP.md): turning the
; analysis of a general algebraic curve into actual integration on it.
;
; The derivation on y^n = g gives y' = g' y / (n g), so for a monomial x^k y^j,
;     D(x^k y^j) = k x^{k-1} y^j + x^k j y^{j-1} y' = [ k x^{k-1} g + (j/n) x^k g' ] y^j / g.
; Crucially the power y^j is PRESERVED (the y^{j-1} y' brings one factor of y back), so each y^j sector reduces
; independently, exactly like the hyperelliptic case with p replaced by g and the constant 1/2 replaced by j/n.
; Writing L_k = k x^{k-1} g + (j/n) x^k g' (the y^j-numerator of D(x^k y^j)), a numerator P of degree >= deg g
; is reduced by subtracting multiples of the L_k descending in degree, accumulating Q, until the remainder S has
; degree < deg g - 1.  Then
;     INT (P y^j / g) dx = Q y^j + INT (S y^j / g) dx,
; with S y^j / g the first-kind (holomorphic) remainder.  If S = 0 the integral is elementary, equal to Q y^j,
; and is certified by the polynomial identity  Q' g + (j/n) Q g' = P  (the numerator of D(Q y^j) = P y^j / g).
; If S != 0 the remaining differential has a nonzero holomorphic part and the integral is not elementary by this
; reduction (the first-kind obstruction), reported honestly rather than guessed.
;
; Public: se-Lk k g j n; se-reduce P g j n -> (list Q S); se-integrate P g j n ->
;   (list 'elementary Q) | (list 'non-elementary-first-kind S); se-certify P g j n -> #t for an elementary
; verdict iff Q' g + (j/n) Q g' = P.  Verified: INT (3x^4+2x) y/(x^3+1) dx = x^2 y on y^3 = x^3+1; and the
; n=2 specialization reproduces hyperell on y^2 = p.
;
; Self-contained over poly.lisp (exact rational arithmetic).

(import "cas/poly.lisp")

(define (se-shift q i) (if (= i 0) q (cons 0 (se-shift q (- i 1)))))   ; x^i * q
(define (se-mono c j) (if (= j 0) (list c) (cons 0 (se-mono c (- j 1))))) ; c x^j

; L_k = k x^{k-1} g + (j/n) x^k g'   (the y^j-numerator of D(x^k y^j))
(define (se-Lk k g j n)
  (let ((gp (poly-deriv g)))
    (poly-add (if (= k 0) (quote ()) (poly-scale k (se-shift g (- k 1))))
              (poly-scale (/ j n) (se-shift gp k)))))

; reduce P (mod the L_k) to remainder S of degree < deg g - 1, accumulating Q.  Returns (list Q S).
(define (se-reduce P g j n) (se-reduce-go (poly-norm P) g j n (quote ())))
(define (se-reduce-go R g j n Qacc)
  (let ((D (poly-deg g)))
    (if (< (poly-deg R) (- D 1)) (list (poly-norm Qacc) (poly-norm R))
        (let ((k (- (poly-deg R) (- D 1))))                  ; choose k so deg L_k = k+D-1 = deg R
          (let ((Lk (se-Lk k g j n)))
            (let ((q (/ (poly-lead R) (poly-lead Lk))))
              (se-reduce-go (poly-norm (poly-sub R (poly-scale q Lk))) g j n
                            (poly-add Qacc (se-mono q k)))))))))

; the decision for INT P y^j / g dx
(define (se-integrate P g j n)
  (let ((rs (se-reduce P g j n)))
    (let ((Q (car rs)) (S (car (cdr rs))))
      (if (poly-zero? S)
          (list (quote elementary) Q)
          (list (quote non-elementary-first-kind) S)))))

; certificate for an elementary verdict: D(Q y^j) = P y^j / g  <=>  Q' g + (j/n) Q g' = P (polynomial identity)
(define (se-certify P g j n)
  (let ((r (se-integrate P g j n)))
    (if (equal? (car r) (quote elementary))
        (let ((Q (car (cdr r))))
          (poly-zero? (poly-sub (poly-add (poly-mul (poly-deriv Q) g) (poly-scale (/ j n) (poly-mul Q (poly-deriv g)))) P)))
        #f)))

; the numerator of D(Q y^j) as a polynomial: Q' g + (j/n) Q g'  (exposed for inspection / building tests)
(define (se-dnum Q g j n)
  (poly-norm (poly-add (poly-mul (poly-deriv Q) g) (poly-scale (/ j n) (poly-mul Q (poly-deriv g))))))
