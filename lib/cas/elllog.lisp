; -*- lisp -*-
; lib/cas/elllog.lisp -- the EXPLICIT ELLIPTIC LOGARITHM, completing Rung 3b (docs/TRAGER_ROADMAP.md).
;
; elltorsion decides that INT dx/((x - s) sqrt(p)) on a genus-1 curve y^2 = p is elementary exactly when the
; pole lifts to a TORSION point P = (s, rho) of order n.  This module CONSTRUCTS the answer in that case: an
; algebraic logarithm c log(f) with f in K = Q(x)[y]/(y^2 - p), certified by D(c log f) = integrand inside K.
;
; The function f realizes the principal divisor.  By Abel-Jacobi the residue divisor of dx/((x-s)y) is a
; multiple of [P] - [-P], and n([P] - [-P]) = n[P] - n[-P] is principal (nP = O).  We build the Miller function
; f_P with div(f_P) = n[P] - n[O] by the standard iteration over K, using the chord/tangent LINES and VERTICALS
; of the elliptic group law as elements of K:
;     f_1 = 1,   f_{i+1} = f_i * L_{iP,P} / V_{(i+1)P},   i = 1..n-1,
; where L_{A,B} = y - yA - lambda (x - xA) is the line through A and B (lambda the group-law slope; the vertical
; x - xA when A and B share an x-coordinate, i.e. B = -A), and V_R = x - xR is the vertical at R (V_O = 1).
; After n steps div(f_P) = n[P] - n[O].  The conjugate f_{-P}(x,y) = f_P(x,-y) has div = n[-P] - n[O], so
;     f = f_P / f_{-P}   has   div(f) = n[P] - n[-P] = n([P] - [-P]),
; exactly the principal divisor we need.  The constant c is then found by matching f'/f to the integrand (it is
; a genuine constant, 1/n times a residue factor), and the whole answer is GATED by the differentiation
; certificate -- if the certificate fails, the construction reports 'failed rather than asserting a wrong log.
;
; Builds on algfunc.lisp (the field K and its derivation) and elltorsion.lisp (the group law over Q + order).
;
; SCOPE (honest).  The construction is gated by the differentiation certificate, so it never returns a wrong
; logarithm: it returns (list 'log c f) only when D(c log f) = integrand is verified, and (list 'failed ...)
; otherwise.  In practice it certifies the ODD-order torsion poles (the Miller iteration produces the principal
; divisor cleanly when no intermediate multiple is a 2-torsion / y=0 point); an EVEN-order pole, whose multiples
; pass through a 2-torsion point, is currently deferred (reported 'failed) -- the refinement of Miller's
; verticals at 2-torsion places is left for a later pass.  elltorsion still DECIDES those cases elementary, so
; nothing is lost in the decision; only the explicit log argument is withheld until certified.

(import "cas/algfunc.lisp")
(import "cas/elltorsion.lisp")

; ----- the multiples iP of P, as a list [P, 2P, ..., nP=O] -----
(define (ell-multiples P a2 a1 n) (ell-mult-go P P a2 a1 n 1))
(define (ell-mult-go cur P a2 a1 n i)
  (if (> i n) (quote ())
      (cons cur (ell-mult-go (elt-add cur P a2 a1) P a2 a1 n (+ i 1)))))

; ----- line and vertical as af-elements over K -----
; vertical at R = (xr . yr): x - xr  (u = x - xr, v = 0)
(define (ell-vert R)
  (if (equal? R (quote O)) (af-one)
      (af-make (rat-from-poly (list (- 0 (car R)) 1)) (rat-zero))))
; line through A and B (both finite).  If A and B share x (B = -A), the "line" is the vertical x - xA.
; otherwise slope lambda = (yB - yA)/(xB - xA), and the chord is y - yA - lambda(x - xA).
; for A = B (doubling) slope lambda = (3 xA^2 + 2 a2 xA + a1)/(2 yA).
(define (ell-line A B a2 a1)
  (if (= (car A) (car B))
      (af-make (rat-from-poly (list (- 0 (car A)) 1)) (rat-zero))      ; vertical x - xA
      (let ((lam (/ (- (cdr B) (cdr A)) (- (car B) (car A)))))
        (ell-chord A lam))))
(define (ell-line-double A a2 a1)
  (if (= (cdr A) 0) (af-make (rat-from-poly (list (- 0 (car A)) 1)) (rat-zero))   ; vertical (2-torsion)
      (let ((lam (/ (+ (* 3 (* (car A) (car A))) (+ (* 2 (* a2 (car A))) a1)) (* 2 (cdr A)))))
        (ell-chord A lam))))
; chord y - yA - lam(x - xA): u = -yA - lam(x - xA) = (-yA + lam xA) - lam x ; v = 1
(define (ell-chord A lam)
  (af-make (rat-from-poly (list (+ (- 0 (cdr A)) (* lam (car A))) (- 0 lam))) (rat-from-poly (list 1))))

; ----- the Miller function f_P with div = n[P] - n[O] -----
; iteration: f starts at 1; at step i (i=1..n-1) multiply by L_{iP,P} and divide by V_{(i+1)P}.
; L_{iP,P} is the line through iP and P (a double when i=1 means line through P and P = tangent... but our
; first multiply uses iP=P,B=P only when i=1 -> tangent).  We use the multiples list mults = [P,2P,...].
(define (ell-miller P a2 a1 n p mults)
  (ell-miller-go P a2 a1 n p mults 1 (af-one)))
(define (ell-miller-go P a2 a1 n p mults i f)
  (if (>= i n) f
      (let ((iP (ell-nth mults (- i 1))) (i1P (ell-nth mults i)))   ; iP and (i+1)P
        (let ((L (if (= i 1) (ell-line-double P a2 a1) (ell-line iP P a2 a1)))
              (V (ell-vert i1P)))
          (ell-miller-go P a2 a1 n p mults (+ i 1)
                         (af-div p (af-mul p f L) V))))))
(define (ell-nth l k) (if (= k 0) (car l) (ell-nth (cdr l) (- k 1))))

; conjugate of an af element: y -> -y, i.e. (u, v) -> (u, -v)
(define (ell-conj e) (af-make (af-u e) (rat-neg (af-v e))))

; ----- the full elliptic-log for INT dx/((x - s) sqrt(p)), p an integral squarefree cubic, P=(s,rho) torsion ---
; returns (list 'log c f)  meaning c * log(f), f in K, certified by D(c log f) = integrand
;       | (list 'failed why)
(define (ell-logarithm p s)
  (let ((pp (rat-num p)))
    (let ((ps (poly-eval pp s)))
      (if (= ps 0) (list (quote failed) (quote branch-pole))
          (let ((rho (elt-sqrt-q ps)))
            (if (equal? rho (quote no)) (list (quote failed) (quote needs-extension))
                (let ((a2 (poly-coeff pp 2)) (a1 (poly-coeff pp 1)))
                  (let ((P (cons s rho)))
                    (let ((ord (elt-order P a2 a1)))
                      (if (equal? ord (quote infinite)) (list (quote failed) (quote non-elementary))
                          (ell-build p pp s rho a2 a1 ord)))))))))))

(define (ell-build p pp s rho a2 a1 n)
  (let ((mults (ell-multiples (cons s rho) a2 a1 n)))
    (let ((fP (ell-miller (cons s rho) a2 a1 n p mults)))
      (let ((f (af-div p fP (ell-conj fP))))                         ; f = f_P / conj(f_P)
        (let ((integ (af-make (rat-zero) (rat-make (list 1) (poly-mul (list (- 0 s) 1) pp)))))
          (let ((gpovg (af-div p (af-deriv p f) f)))
            (if (af-zero-elt? gpovg) (list (quote failed) (quote degenerate))
                (let ((c (af-div p integ gpovg))) 
                  (if (ell-const? c)
                      (if (af-certify p (af-zero) (af-u c) f integ) (list (quote log) (af-u c) f) (list (quote failed) (quote certificate)))
                      (list (quote failed) (quote nonconstant)))))))))))
(define (af-zero-elt? e) (if (rat-zero? (af-u e)) (rat-zero? (af-v e)) #f))
; an af element is a (rational) constant if v=0 and u is a degree-0 rational function
(define (ell-const? c) (if (rat-zero? (af-v c)) (if (<= (poly-deg (rat-num (af-u c))) 0) (<= (poly-deg (rat-den (af-u c))) 0) #f) #f))

(define (ell-log-decides? p s) (equal? (car (ell-logarithm p s)) (quote log)))
