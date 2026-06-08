; -*- lisp -*-
; lib/cas/tower2rt.lisp -- the general height-two Rothstein-Trager logarithmic part.
;
; This completes the logarithmic part of integrating a rational function of a second monomial theta2
; primitive over K1 = Q(x)(theta1), going beyond the single-logarithm recognizer of tower2int.lisp to
; the general case of several constant residues.  After Hermite reduces A/D to a rational part g and a
; remainder A*/D* with D* squarefree in theta2, the logarithmic part is a sum, over residues c, of
; c log(v_c), where v_c is the gcd in theta2 over K1 of D* and A* - c D2(D*), and the residues are the
; roots of the Rothstein-Trager resultant R(z) = Res_theta2(D*, A* - z D2(D*)).  That resultant is an
; element of K1[z]; for the integral to be elementary with constant residues, the residues lie in the
; constant field Q.  R(z) is obtained by evaluation and interpolation: the resultant at a numeric z is
; the Sylvester determinant over K1, built by cofactor expansion using the (now cancel-before-multiply)
; K1 field arithmetic.  Dividing each evaluated resultant by a fixed nonzero one cancels the K1 content,
; so the resulting ratios are rational exactly when the residues are constant; those ratios interpolate
; to a rational polynomial whose roots are the residues.  For each rational residue the gcd over K1[theta2]
; gives the logarithm's argument.  The whole answer g plus the RootSum is certified by differentiating
; with the two-level derivation D2 and checking equality with A/D over K1[theta2].  When the residues are
; not all rational constants the logarithmic part is algebraic and is reported as such rather than forced.
; Builds on tower2int.lisp and reuses ros-rational-roots from towerrt.lisp.

(import "cas/tower2int.lisp")
(import "cas/towerrt.lisp")

; ----- constants of K1 as rationals, and back -----
(define (k1-from-int n) (k1-iscale n (k1-one)))
(define (k1-from-rat c) (k1-normalize (list (rf-const (rat-make (list (numerator c)) (list (denominator c)))) (rf-const (rat-one)))))
(define (k1rt-rf-c0 p) (if (null? p) (rat-zero) (car p)))
(define (k1rt-rat-q r) (/ (if (null? (car r)) 0 (car (car r))) (if (null? (car (cdr r))) 1 (car (car (cdr r))))))
(define (k1-to-rational c) (/ (k1rt-rat-q (k1rt-rf-c0 (car c))) (k1rt-rat-q (k1rt-rf-c0 (car (cdr c))))))
(define (k1-pow a n) (if (= n 0) (k1-one) (k1-mul a (k1-pow a (- n 1)))))

; ----- Sylvester resultant over K1 (resultant in theta2 of two h2polys) -----
(define (k1-drop-col row c) (if (= c 0) (cdr row) (cons (car row) (k1-drop-col (cdr row) (- c 1)))))
(define (k1-minor-go rows i j r) (if (null? rows) '()
   (if (= r i) (k1-minor-go (cdr rows) i j (+ r 1)) (cons (k1-drop-col (car rows) j) (k1-minor-go (cdr rows) i j (+ r 1))))))
(define (k1mat-minor M i j) (k1-minor-go M i j 0))
(define (k1mat-det M) (if (null? M) (k1-one) (if (null? (cdr M)) (car (car M)) (k1det-row (car M) M 0 (k1-zero) (k1-one)))))
(define (k1det-row row M j acc sign)
  (if (null? row) acc
      (k1det-row (cdr row) M (+ j 1)
                 (k1-add acc (k1-mul sign (k1-mul (car row) (k1mat-det (k1mat-minor M 0 j))))) (k1-neg sign))))
(define (k1-zeros n) (if (= n 0) '() (cons (k1-zero) (k1-zeros (- n 1)))))
(define (k1-pad lst n) (if (<= n 0) lst (k1-pad (append lst (list (k1-zero))) (- n 1))))
(define (k1-place coeffs-hi lead-col total) (k1-pad (append (k1-zeros lead-col) coeffs-hi) (- total (+ lead-col (length coeffs-hi)))))
(define (k1-frows fhi i n total) (if (= i n) '() (cons (k1-place fhi i total) (k1-frows fhi (+ i 1) n total))))
(define (k1-grows ghi i m total) (if (= i m) '() (cons (k1-place ghi i total) (k1-grows ghi (+ i 1) m total))))
(define (k1-sylvester fhi ghi m n total) (append (k1-frows fhi 0 n total) (k1-grows ghi 0 m total)))
(define (h2-eval-go hi b acc) (if (null? hi) acc (h2-eval-go (cdr hi) b (k1-add (k1-mul acc b) (car hi)))))
(define (h2-eval-k1 p b) (h2-eval-go (reverse (h2-norm p)) b (k1-zero)))   ; evaluate h2poly at b in K1 (Horner)
; determinant over the field K1 by Gaussian elimination with pivoting (O(n^3); replaces cofactor for n>=2)
(define (k1ge-nth lst i) (if (= i 0) (car lst) (k1ge-nth (cdr lst) (- i 1))))
(define (k1ge-rm lst i) (if (= i 0) (cdr lst) (cons (car lst) (k1ge-rm (cdr lst) (- i 1)))))
(define (k1ge-pivot M i) (if (null? M) -1 (if (k1-zero? (car (car M))) (k1ge-pivot (cdr M) (+ i 1)) i)))
(define (k1ge-vscale s v) (if (null? v) (quote ()) (cons (k1-mul s (car v)) (k1ge-vscale s (cdr v)))))
(define (k1ge-vsub a b) (if (null? a) (quote ()) (cons (k1-normalize (k1-sub (car a) (car b))) (k1ge-vsub (cdr a) (cdr b)))))
(define (k1ge-elim pivrow rows) (if (null? rows) (quote ())
   (cons (k1ge-vsub (cdr (car rows)) (k1ge-vscale (k1-normalize (k1-div (car (car rows)) (car pivrow))) (cdr pivrow)))
         (k1ge-elim pivrow (cdr rows)))))
(define (k1mat-det-gauss M)
  (if (null? M) (k1-one)
      (let ((p (k1ge-pivot M 0)))
        (if (< p 0) (k1-zero)
            (let ((pivrow (k1ge-nth M p)))
              (let ((d (k1-mul (car pivrow) (k1mat-det-gauss (k1ge-elim pivrow (k1ge-rm M p))))))
                (if (= (remainder p 2) 0) d (k1-neg d))))))))
(define (h2-resultant f g)
  (let ((m (h2-deg f)) (n (h2-deg g)))
    (if (if (< m 0) #t (< n 0)) (k1-zero)
        (if (= n 0) (k1-pow (car (h2-norm g)) m)
            (if (= n 1) (k1-mul (k1-pow (car (cdr (h2-norm g))) m)
                                (h2-eval-k1 f (k1-neg (k1-div (car (h2-norm g)) (car (cdr (h2-norm g)))))))
                (k1mat-det-gauss (k1-sylvester (reverse (h2-norm f)) (reverse (h2-norm g)) m n (+ m n))))))))

; ----- rational Lagrange interpolation (xs integers, ys rationals) -> Q[z] poly low->high -----
(define (k1z-nth lst i) (if (= i 0) (car lst) (k1z-nth (cdr lst) (- i 1))))
(define (q-basis-num xs k j acc) (if (null? xs) acc
   (if (= j k) (q-basis-num (cdr xs) k (+ j 1) acc) (q-basis-num (cdr xs) k (+ j 1) (poly-mul acc (list (- 0 (car xs)) 1))))))
(define (q-basis-den xs k j xk acc) (if (null? xs) acc
   (if (= j k) (q-basis-den (cdr xs) k (+ j 1) xk acc) (q-basis-den (cdr xs) k (+ j 1) xk (* acc (- xk (car xs)))))))
(define (q-lag-go xs ys k acc) (if (>= k (length xs)) acc
   (q-lag-go xs ys (+ k 1) (poly-add acc (poly-scale (/ (k1z-nth ys k) (q-basis-den xs k 0 (k1z-nth xs k) 1)) (q-basis-num xs k 0 (list 1)))))))
(define (q-lagrange xs ys) (q-lag-go xs ys 0 '()))

; ----- the logarithmic part -----
(define (h2rt-Rvals As Ds D2D k N acc)            ; (gc) between evals frees transient K1 garbage (else OOM at N>=3)
  (if (> k N) (reverse acc)
      (let ((r (h2-resultant Ds (h2-sub As (h2-cscale (k1-from-int k) D2D)))))
        (begin (gc) (h2rt-Rvals As Ds D2D (+ k 1) N (cons r acc))))))
(define (h2rt-first-nonzero rs k) (if (null? rs) -1 (if (k1-zero? (car rs)) (h2rt-first-nonzero (cdr rs) (+ k 1)) k)))
(define (h2rt-ratios rs r0 mono1 acc)            ; -> list of rationals | 'notconst
  (if (null? rs) (reverse acc)
      (let ((s (k1-div (car rs) r0)))
        (if (k1-constant? s mono1) (h2rt-ratios (cdr rs) r0 mono1 (cons (k1-to-rational s) acc)) 'notconst))))
(define (h2rt-int-list n) (h2rt-int-go 0 n))
(define (h2rt-int-go k n) (if (> k n) '() (cons k (h2rt-int-go (+ k 1) n))))
(define (h2rt-terms roots As Ds D2D Dth2 mono1 acc tdeg)   ; build (c v) for each rational residue
  (if (null? roots) (list acc tdeg)
      (let ((c (car roots)))
        (let ((v (h2-monic (h2-gcd Ds (h2-sub As (h2-cscale (k1-from-rat c) D2D))))))
          (if (>= (h2-deg v) 1)
              (h2rt-terms (cdr roots) As Ds D2D Dth2 mono1 (cons (list c v) acc) (+ tdeg (h2-deg v)))
              (h2rt-terms (cdr roots) As Ds D2D Dth2 mono1 acc tdeg))))))
(define (h2rt-logpart As Ds Dth2 mono1)          ; (list 'rootsum terms) | (list 'algebraic ...) | (list 'degenerate)
  (begin (gc)
  (let ((D2D (t2-deriv Ds Dth2 mono1)) (N (h2-deg Ds)))
    (let ((rs (h2rt-Rvals As Ds (t2-deriv Ds Dth2 mono1) 0 N '())))
      (let ((k0 (h2rt-first-nonzero rs 0)))
        (if (< k0 0) (list 'degenerate)
            (let ((rats (h2rt-ratios rs (k1z-nth rs k0) mono1 '())))
              (if (equal? rats 'notconst) (list 'algebraic)
                  (let ((roots (ros-rational-roots (q-lagrange (h2rt-int-list N) rats))))
                    (let ((tt (h2rt-terms roots As Ds D2D Dth2 mono1 '() 0)))
                      (if (= (car (cdr tt)) (h2-deg Ds)) (list 'rootsum (car tt)) (list 'algebraic))))))))))))

; ----- the full height-two integrator: Hermite rational part + general logarithmic part -----
(define (int-h2-full A D Dth2 mono1)
  (let ((H (h2-hermite A D Dth2 mono1)))
    (let ((g (car H)) (As (car (cdr H))) (Ds (car (cdr (cdr H)))))
      (if (h2-zero? As) (list 'ok g 'none)
          (let ((lg (h2rt-logpart As Ds Dth2 mono1)))
            (if (equal? (car lg) 'rootsum) (list 'ok g lg) (list 'partial g As Ds lg)))))))

; certificate: D2(g + sum c log v_c) = A/D
(define (h2rt-logderiv terms Dth2 mono1 acc)
  (if (null? terms) acc
      (h2rt-logderiv (cdr terms) Dth2 mono1
        (h2tr-add acc (list (h2-cscale (k1-from-rat (car (car terms))) (t2-deriv (car (cdr (car terms))) Dth2 mono1)) (car (cdr (car terms))))))))
(define (int-h2-full-deriv res Dth2 mono1)
  (let ((g (car (cdr res))) (lg (car (cdr (cdr res)))))
    (let ((dg (h2tr-deriv (car g) (car (cdr g)) Dth2 mono1)))
      (if (equal? lg 'none) dg (h2tr-add dg (h2rt-logderiv (car (cdr lg)) Dth2 mono1 (h2tr-zero)))))))
(define (int-h2-full-verify A D Dth2 mono1)
  (let ((res (int-h2-full A D Dth2 mono1)))
    (if (equal? (car res) 'ok) (h2tr-equal? (int-h2-full-deriv res Dth2 mono1) (list A D)) #f)))
(define (int-h2-full-elementary? A D Dth2 mono1) (equal? (car (int-h2-full A D Dth2 mono1)) 'ok))
