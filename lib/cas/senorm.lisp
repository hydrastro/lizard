; -*- lisp -*-
; lib/cas/senorm.lisp -- the NORM, INVERSE, and rationalized LOGARITHMIC DERIVATIVE in the superelliptic field
; K = Q(x)[y]/(y^n - g), the layer that lets third-kind logarithms be presented with ordinary polynomial
; denominators and is the gateway to Rothstein-Trager residue analysis over the field (Rung 4,
; docs/TRAGER_ROADMAP.md).
;
; For u in K, the Norm N(u) = product of the n conjugates (y -> zeta^k g^{1/n}) is a rational function of x.
; It is computed here as the determinant of the multiplication-by-u matrix on the basis {1, y, ..., y^{n-1}}
; (a Q(x)-linear map), via cofactor expansion over rational functions -- reusing the field multiplication from
; sefield.lisp.  The adjugate gives the conjugate-product ubar (polynomial-coefficient field element) with
; u * ubar = N(u), hence the inverse u^{-1} = ubar / N(u).  This rationalizes the logarithmic derivative:
;     u'/u = u' * ubar / N(u),
; a field element over the SCALAR (in y) denominator N(u) in Q(x) -- exactly the form needed to read off
; residues and to present log(u) integrals concretely.
;
; Public: sn-mult-matrix g n u; sn-norm g n u (the determinant, a rational function); sn-ubar g n u (the
; conjugate-product field element, u*ubar = N(u)); sn-inverse g n u (the field element u^{-1}); sn-logderiv g n
; u (returns (cons F N) with F a field element and N a rational function meaning u'/u = F / N) ; sn-logderiv-check
; verifies u * (u'/u) = u'.  Verified: N(y) = g, N(x+y+2y^2) on y^3=x^3+1 equals the cubic norm form
; 8x^6-6x^4+18x^3-6x+9; ubar(y) = y^2 with y*y^2 = g; and the rationalized u'/u for u = x + y.
;
; Builds on sefield.lisp (the field) and tower.lisp (rational-function arithmetic).

(import "cas/sefield.lisp")

(define (sn-nth l k) (if (= k 0) (car l) (sn-nth (cdr l) (- k 1))))

; ----- multiplication-by-u matrix on the basis {1, y, ..., y^{n-1}} -----
(define (sn-yp g n k) (if (= k 0) (sf-one n) (sf-product g n (sf-y n) (sn-yp g n (- k 1)))))
(define (sn-cols g n u j) (if (>= j n) (quote ()) (cons (sf-product g n u (sn-yp g n j)) (sn-cols g n u (+ j 1)))))
; transpose the list-of-columns into matrix[row][col]
(define (sn-mat-row cols i) (if (null? cols) (quote ()) (cons (sn-nth (car cols) i) (sn-mat-row (cdr cols) i))))
(define (sn-mat-rows cols n i) (if (>= i n) (quote ()) (cons (sn-mat-row cols i) (sn-mat-rows cols n (+ i 1)))))
(define (sn-mult-matrix g n u) (sn-mat-rows (sn-cols g n u 0) n 0))

; ----- determinant over rational functions (cofactor expansion along row 0) -----
(define (sn-det M n) (if (= n 1) (car (car M)) (sn-det-go M n 0 (rat-zero))))
(define (sn-det-go M n j acc)
  (if (>= j n) acc
      (let ((sign (if (= (remainder j 2) 0) (rat-one) (rat-from-poly (list -1)))))
        (sn-det-go M n (+ j 1) (rat-add acc (rat-mul sign (rat-mul (sn-rc (car M) j) (sn-det (sn-minor M n 0 j) (- n 1)))))))))
(define (sn-rc row j) (if (= j 0) (car row) (sn-rc (cdr row) (- j 1))))
(define (sn-minor M n ri cj) (sn-minor-rows M n ri cj 0))
(define (sn-minor-rows M n ri cj i) (if (>= i n) (quote ()) (if (= i ri) (sn-minor-rows M n ri cj (+ i 1)) (cons (sn-drop-col (sn-rc M i) cj) (sn-minor-rows M n ri cj (+ i 1))))))
(define (sn-drop-col row cj) (sn-dc-go row cj 0))
(define (sn-dc-go row cj j) (if (null? row) (quote ()) (if (= j cj) (sn-dc-go (cdr row) cj (+ j 1)) (cons (car row) (sn-dc-go (cdr row) cj (+ j 1))))))

; ----- the Norm: determinant of the multiplication matrix -----
(define (sn-norm g n u) (sn-det (sn-mult-matrix g n u) n))

; ----- the conjugate-product ubar: ubar_i = (-1)^i * minor(M, 0, i), so u * ubar = N(u) -----
(define (sn-ubar g n u) (let ((M (sn-mult-matrix g n u))) (sn-ubar-go M n 0)))
(define (sn-ubar-go M n i)
  (if (>= i n) (quote ())
      (cons (rat-mul (if (= (remainder i 2) 0) (rat-one) (rat-from-poly (list -1))) (sn-det (sn-minor M n 0 i) (- n 1)))
            (sn-ubar-go M n (+ i 1)))))

; ----- the field inverse u^{-1} = ubar / N(u) -----
(define (sn-inverse g n u)
  (let ((N (sn-norm g n u)) (ub (sn-ubar g n u)))
    (sn-scale-rat (rat-inv N) ub)))
(define (sn-scale-rat r a) (if (null? a) (quote ()) (cons (rat-mul r (car a)) (sn-scale-rat r (cdr a)))))

; ----- rationalized logarithmic derivative: u'/u = (u' * ubar) / N(u) -----
; returns (cons F N): F a field element (numerator), N a rational function (scalar denominator); u'/u = F/N.
(define (sn-logderiv g n u)
  (let ((du (sf-deriv g n u)) (ub (sn-ubar g n u)) (N (sn-norm g n u)))
    (cons (sf-product g n du ub) N)))

; check the rationalization: u * (F/N) = u', i.e. u * F = N * u'  (a field identity, no division)
(define (sn-logderiv-check g n u)
  (let ((ld (sn-logderiv g n u)))
    (let ((F (car ld)) (N (cdr ld)))
      (sf-equal? (sf-product g n u F) (sn-scale-rat N (sf-deriv g n u))))))
