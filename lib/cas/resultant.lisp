; -*- lisp -*-
; lib/cas/resultant.lisp — resultants and discriminants over Q, plus the
; parametric resultant res_x(p - z q', q) needed for the Rothstein-Trager
; logarithmic part of integration.
;
; The resultant is computed as the determinant of the Sylvester matrix (exact
; over Q via Gaussian elimination).  The parametric resultant R(z) is obtained
; by EVALUATION + LAGRANGE INTERPOLATION: evaluate the scalar resultant at
; enough integer z and interpolate, which keeps every step a plain rational
; computation.  res(f,g) = 0  iff  f and g share a root, which is the cheap
; self-check used in the tests.
;
; Top-level helpers only; builds on lib/cas/poly.lisp.

(import "cas/poly.lisp")

; ============================================================
;  exact determinant over Q (Gaussian elimination, recursive)
; ============================================================
(define (vec-sub a b) (if (null? a) '() (cons (- (car a) (car b)) (vec-sub (cdr a) (cdr b)))))
(define (vec-scale f v) (map (lambda (x) (* f x)) v))

(define (find-pivot-idx m i)
  (cond ((null? m) 'none)
        ((not (= (car (car m)) 0)) i)
        (else (find-pivot-idx (cdr m) (+ i 1)))))

(define (set-index lst i v) (if (= i 0) (cons v (cdr lst)) (cons (car lst) (set-index (cdr lst) (- i 1) v))))
(define (swap-first m piv)                 ; bring row `piv` (>0) to the front
  (cons (nth m piv) (set-index (cdr m) (- piv 1) (car m))))

(define (eliminate-row row pivrow p) (vec-sub row (vec-scale (/ (car row) p) pivrow)))

(define (matrix-det m)
  (if (null? m) 1
    (let ((piv (find-pivot-idx m 0)))
      (if (equal? piv 'none) 0
        (let ((m2 (if (= piv 0) m (swap-first m piv))) (sign (if (= piv 0) 1 -1)))
          (let ((p (car (car m2))))
            (* sign p (matrix-det (map cdr (map (lambda (row) (eliminate-row row (car m2) p))
                                                (cdr m2)))))))))))

; ============================================================
;  Sylvester matrix and resultant
; ============================================================
(define (build-rows coeffs nrows j acc)    ; nrows rows, row j = j zeros ++ coeffs ++ (nrows-1-j) zeros
  (if (= j nrows) (reverse acc)
    (build-rows coeffs nrows (+ j 1)
      (cons (append (zeros j) (append coeffs (zeros (- (- nrows 1) j)))) acc))))

(define (sylvester f g)
  (let ((m (poly-deg f)) (n (poly-deg g)))
    (append (build-rows (reverse (poly-norm f)) n 0 '())
            (build-rows (reverse (poly-norm g)) m 0 '()))))

(define (resultant f g)
  (cond ((or (poly-zero? f) (poly-zero? g)) 0)
        ((and (poly-const? f) (poly-const? g)) 1)
        ((poly-const? f) (expt (poly-coeff f 0) (poly-deg g)))
        ((poly-const? g) (expt (poly-coeff g 0) (poly-deg f)))
        (else (matrix-det (sylvester f g)))))

; discriminant:  (-1)^(n(n-1)/2) * res(f, f') / lc(f)
(define (discriminant f)
  (let ((n (poly-deg f)))
    (* (expt -1 (quotient (* n (- n 1)) 2))
       (/ (resultant f (poly-deriv f)) (poly-lead f)))))

; ============================================================
;  Lagrange interpolation over Q   (points = list of (x . y), distinct x)
; ============================================================
(define (lagrange-numer xi all)            ; prod_{xj != xi} (x - xj)
  (cond ((null? all) (list 1))
        ((= (car (car all)) xi) (lagrange-numer xi (cdr all)))
        (else (poly-mul (list (- 0 (car (car all))) 1) (lagrange-numer xi (cdr all))))))
(define (lagrange-denom xi all)            ; prod_{xj != xi} (xi - xj)
  (cond ((null? all) 1)
        ((= (car (car all)) xi) (lagrange-denom xi (cdr all)))
        (else (* (- xi (car (car all))) (lagrange-denom xi (cdr all))))))
(define (lagrange-sum all rem)
  (if (null? rem) '()
    (poly-add (poly-scale (* (cdr (car rem)) (/ 1 (lagrange-denom (car (car rem)) all)))
                          (lagrange-numer (car (car rem)) all))
              (lagrange-sum all (cdr rem)))))
(define (lagrange pts) (lagrange-sum pts pts))

; ============================================================
;  parametric resultant for Rothstein-Trager:  R(z) = res_x(p - z q', q)
;  R has degree <= deg q in z, so interpolate through deg q + 1 points.
; ============================================================
(define (rt-points p qp q z count)
  (if (= z count) '()
    (cons (cons z (resultant (poly-sub p (poly-scale z qp)) q))
          (rt-points p qp q (+ z 1) count))))
; The leading x-coefficient of (p - z q') is (coeff_x^{deg q'} p) - z*lead(q'); at the z where
; this vanishes the x-degree drops and the (adaptive) resultant value is inconsistent with the
; interpolating polynomial.  Avoid that one node so all samples share the full formal degree.
(define (rt-badz p qp)
  (let ((d (poly-deg qp)))
    (if (< (poly-deg p) d) 0 (/ (poly-coeff p d) (poly-lead qp)))))
(define (rt-points-safe p qp q z badz count acc)
  (if (= count 0) (reverse acc)
      (if (= z badz)
          (rt-points-safe p qp q (+ z 1) badz count acc)
          (rt-points-safe p qp q (+ z 1) badz (- count 1)
                          (cons (cons z (resultant (poly-sub p (poly-scale z qp)) q)) acc)))))
(define (rt-resultant p q)
  (let ((qp (poly-deriv q)))
    (lagrange (rt-points-safe p qp q 1 (rt-badz p qp) (+ (poly-deg q) 1) '()))))
