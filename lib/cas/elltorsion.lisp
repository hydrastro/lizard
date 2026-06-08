; -*- lisp -*-
; lib/cas/elltorsion.lisp -- RUNG 3b of the Trager-Bronstein climb (docs/TRAGER_ROADMAP.md): the GENUS-1
; (elliptic) THIRD-KIND decision via a TORSION TEST on the elliptic curve's group.
;
; For the genus-0 radical the third-kind logarithm always exists (Rung 3a).  On a genus-1 (elliptic) curve
; y^2 = p, p a squarefree cubic, principality of the residue divisor is NO LONGER automatic -- it is the real
; Trager obstruction, and it is an arithmetic-geometric condition: the Abel-Jacobi theorem says a degree-zero
; divisor sum n_i [P_i] (sum n_i = 0) is PRINCIPAL iff its image in the group law, sum [n_i] P_i (with the
; point at infinity as identity O), is O.  So for INT dx/((x - s) sqrt(p)) with the pole lifted to the rational
; point P = (s, rho) on the curve (rho^2 = p(s)), the differential has residues +r at P and -r at its conjugate
; -P = (s, -rho), giving the divisor r([P] - [-P]) = r([P] - [ominus P]); this is principal iff, in the group,
;
;     r * ( P - (ominus P) )  =  r * (2P)  =  O ,    i.e.  P is a TORSION point  (some multiple nP = O).
;
;   * P torsion  => the integral is ELEMENTARY (an algebraic logarithm c log f exists, f realizing the
;     principal divisor); the order n is reported so the logarithm can be built.
;   * P non-torsion => the integral is provably NON-ELEMENTARY (the canonical elliptic obstruction).
;
; This criterion (the pole, lifted to the curve, is a torsion point) is the classical Trager/Davenport result;
; cf. Combot, "Hyperelliptic Integrals to Elliptic Integrals" (arXiv:2303.14013): for y^2 = z(z-1)(z-kappa) the
; third-kind integral is elementary exactly when (u, sqrt(p(u))) is a torsion point of the curve.
;
; The test terminates by Nagell-Lutz + Mazur: on an integral model y^2 = monic cubic with integer coefficients,
; a torsion point has INTEGER coordinates and order <= 12.  So we compute P, 2P, ..., up to the Mazur bound; if
; a multiple is O the point is torsion; the moment a multiple acquires a NON-INTEGER coordinate it cannot be
; torsion (Nagell-Lutz), giving early, sound termination and bounding the rational blow-up.
;
; This is the elliptic group law over Q (not the finite-field ec.lisp); it uses exact rational arithmetic.
;
; IMPORTANT (criterion completeness).  The torsion test below is NECESSARY but NOT SUFFICIENT for elementarity:
; the pole being torsion guarantees the logarithmic part L exists, but the integral is elementary only if the
; first-kind remainder I - L also vanishes (Trager; Combot arXiv:2103.04134).  elt-decide therefore reports
; whether the pole is torsion (the necessary condition and the source of the explicit elliptic logarithm when
; it succeeds); the COMPLETE decision -- torsion AND vanishing first-kind remainder, with the certified log --
; lives in ellint.lisp (ei-integrate).  Use ei-integrate for a sound elementary/non-elementary verdict.

(import "cas/tower.lisp")

; ----- the elliptic group law over Q for y^2 = x^3 + a2 x^2 + a1 x + a0.  Points: 'O or (x . y), x,y rational.
(define (elt-neg P) (if (equal? P (quote O)) (quote O) (cons (car P) (- 0 (cdr P)))))
(define (elt-double P a2 a1)
  (if (equal? P (quote O)) (quote O)
      (if (= (cdr P) 0) (quote O)                       ; 2-torsion: tangent vertical
          (let ((x (car P)) (y (cdr P)))
            (let ((lam (/ (+ (* 3 (* x x)) (+ (* 2 (* a2 x)) a1)) (* 2 y))))
              (let ((x3 (- (- (* lam lam) a2) (* 2 x))))
                (cons x3 (- (* lam (- x x3)) y))))))))
(define (elt-add P Q a2 a1)
  (cond ((equal? P (quote O)) Q)
        ((equal? Q (quote O)) P)
        ((if (= (car P) (car Q)) (= (+ (cdr P) (cdr Q)) 0) #f) (quote O))   ; Q = -P
        ((if (= (car P) (car Q)) (= (cdr P) (cdr Q)) #f) (elt-double P a2 a1))
        (else
         (let ((lam (/ (- (cdr Q) (cdr P)) (- (car Q) (car P)))))
           (let ((x3 (- (- (- (* lam lam) a2) (car P)) (car Q))))
             (cons x3 (- (* lam (- (car P) x3)) (cdr P))))))))
(define (elt-mul n P a2 a1)
  (if (= n 0) (quote O)
      (let ((Q (elt-double (elt-mul (quotient n 2) P a2 a1) a2 a1)))
        (if (= (remainder n 2) 1) (elt-add Q P a2 a1) Q))))

; ----- integrality (Nagell-Lutz): a point with both coordinates integral -----
(define (elt-integral? P) (if (equal? P (quote O)) #t (if (= (denominator (car P)) 1) (= (denominator (cdr P)) 1) #f)))

; ----- order of a point, bounded by the Mazur bound (returns n in 1..12 with nP=O, or 'infinite) -----
; uses Nagell-Lutz early exit: if a multiple is non-integral on an integral model, P is non-torsion.
(define (elt-order P a2 a1) (elt-order-go P P a2 a1 1))
(define (elt-order-go cur P a2 a1 n)
  (cond ((equal? cur (quote O)) n)
        ((> n 12) (quote infinite))
        ((not (elt-integral? cur)) (quote infinite))     ; Nagell-Lutz: torsion points are integral
        (else (elt-order-go (elt-add cur P a2 a1) P a2 a1 (+ n 1)))))
(define (elt-torsion? P a2 a1) (not (equal? (elt-order P a2 a1) (quote infinite))))

; ----- the Rung-3b decision for INT dx/((x - s) sqrt(p)), p a squarefree integral cubic -----
; INPUT p a rat (monic integral cubic, denominator 1), s an integer pole (s not a root of p), rho with rho^2=p(s).
; OUTPUT (list 'elementary 'elliptic-log n)   -- P=(s,rho) is torsion of order n; the log exists
;      | (list 'non-elementary 'infinite-order) -- P non-torsion; the integral is provably non-elementary
;      | (list 'needs-extension)                 -- p(s) not a perfect square (P not rational)
;      | (list 'not-genus1)                       -- p not a cubic / not the elliptic case
(define (elt-decide p s)
  (let ((pp (rat-num p)))
    (if (not (= (poly-deg pp) 3)) (list (quote not-genus1))
        (let ((ps (poly-eval pp s)))
          (if (= ps 0) (list (quote branch-pole))
              (let ((rho (elt-sqrt-q ps)))
                (if (equal? rho (quote no)) (list (quote needs-extension))
                    (let ((a2 (poly-coeff pp 2)) (a1 (poly-coeff pp 1)))
                      (let ((P (cons s rho)))
                        (let ((ord (elt-order P a2 a1)))
                          (if (equal? ord (quote infinite))
                              (list (quote non-elementary) (quote infinite-order))
                              (list (quote elementary) (quote elliptic-log) ord)))))))))))) 
(define (elt-decides-elementary? p s) (equal? (car (elt-decide p s)) (quote elementary)))

; perfect-square test over Q (reused shape from algthird)
(define (elt-sqrt-q q)
  (if (< q 0) (quote no)
      (let ((n (numerator q)) (d (denominator q)))
        (let ((rn (elt-isqrt n)) (rd (elt-isqrt d)))
          (if (if (equal? rn (quote no)) #t (equal? rd (quote no))) (quote no) (/ rn rd))))))
(define (elt-isqrt n) (elt-isqrt-go n 0))
(define (elt-isqrt-go n k) (cond ((= (* k k) n) k) ((> (* k k) n) (quote no)) (else (elt-isqrt-go n (+ k 1)))))
