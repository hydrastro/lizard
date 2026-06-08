; -*- lisp -*-
; lib/cas/tower2ff.lisp -- fraction-free Rothstein-Trager resultant over K1, to push the memory wall.
;
; The general height-two logarithmic part needs R(z) = Res_theta2(D, A - z D2(D)) evaluated over
; K1 = Q(x)(theta1).  towerrt computes each evaluation by Gaussian elimination over the FIELD K1, whose
; per-step k1-mul performs two rfpoly-gcd (Euclidean) cross-cancellations -- the dominant allocator, which
; makes several evaluations exhaust memory.  Here the same resultant is computed FRACTION-FREE over the
; integral domain Q(x)[theta1] (= rfpoly): the denominators of the coefficients (which come from D2theta2)
; are cleared by a common factor independent of z, the Sylvester matrix is formed over rfpoly, and its
; determinant is taken by the Bareiss one-step fraction-free elimination -- only rfpoly multiply, subtract
; and EXACT division, NO rfpoly-gcd.  Because the clearing factor is the same for every z, it cancels in
; the ratios R(z_k)/R(z_0) that drive the residue interpolation, so the fraction-free resultant values
; give exactly the residues towerrt's field version would, at a fraction of the allocation.  Combined with
; an explicit (gc) between evaluations this keeps the multi-evaluation resultant within the heap for cases
; (e.g. degree-three x-dependent denominators) that the field version cannot reach.  The answer is still
; gated by the same differentiation certificate (int-h2-full-deriv = A/D in K1[theta2]); the fraction-free
; path only changes how the residues are found, never whether the result is accepted.  Builds on
; tower2rt.lisp (it reuses the ratio/interpolation/gcd-argument machinery and the certificate).

(import "cas/tower2rt.lisp")

(define (rf-one) (list (rat-one)))                 ; the unit rfpoly
(define (rf-zero) (quote ()))                      ; the zero rfpoly
(define (rf-sign P s) (if (< s 0) (rfpoly-neg P) P))

; ----- rf2poly = polynomial in theta2 with rfpoly (Q(x)[theta1]) coefficients -----
(define (rf2-trim revP) (if (null? revP) (quote ()) (if (rfpoly-zero? (car revP)) (rf2-trim (cdr revP)) revP)))
(define (rf2-norm P) (reverse (rf2-trim (reverse P))))
(define (rf2-deg P) (- (length (rf2-norm P)) 1))
(define (rf2-neg P) (if (null? P) (quote ()) (cons (rfpoly-neg (car P)) (rf2-neg (cdr P)))))
(define (rf2-sub A B) (cond ((null? A) (rf2-neg B)) ((null? B) A)
                            (else (cons (rfpoly-sub (car A) (car B)) (rf2-sub (cdr A) (cdr B))))))
(define (rf2-iscale z M) (if (null? M) (quote ()) (cons (rf-iscale z (car M)) (rf2-iscale z (cdr M)))))

; ----- denominator clearing: h2poly (K1 coeffs num/den) -> rf2poly, by a common rfpoly factor c -----
(define (ff-denprod h2 acc) (if (null? h2) acc (ff-denprod (cdr h2) (rfpoly-mul acc (car (cdr (car h2)))))))
(define (ff-lcm2 a b) (rfpoly-div (rfpoly-mul a b) (rfpoly-gcd a b)))                 ; lcm via one gcd
(define (ff-denlcm h2 acc) (if (null? h2) acc (ff-denlcm (cdr h2) (ff-lcm2 acc (car (cdr (car h2))))))) ; minimal common denominator
(define (ff-clear-coeff k1 c) (rfpoly-mul (car k1) (rfpoly-div c (car (cdr k1)))))   ; num * (c/den), den | c
(define (ff-clear-poly h2 c) (if (null? h2) (quote ()) (cons (ff-clear-coeff (car h2) c) (ff-clear-poly (cdr h2) c))))

; ----- Sylvester matrix over rfpoly (mirrors k1-sylvester, rfpoly entries) -----
(define (rf-nth lst i) (if (= i 0) (car lst) (rf-nth (cdr lst) (- i 1))))
(define (rf-zeros n) (if (= n 0) (quote ()) (cons (rf-zero) (rf-zeros (- n 1)))))
(define (rf-pad lst n) (if (<= n 0) lst (rf-pad (append lst (list (rf-zero))) (- n 1))))
(define (rf-place chi lc total) (rf-pad (append (rf-zeros lc) chi) (- total (+ lc (length chi)))))
(define (rf-frows fhi i n total) (if (= i n) (quote ()) (cons (rf-place fhi i total) (rf-frows fhi (+ i 1) n total))))
(define (rf-grows ghi i m total) (if (= i m) (quote ()) (cons (rf-place ghi i total) (rf-grows ghi (+ i 1) m total))))
(define (rf-sylvester fhi ghi m n total) (append (rf-frows fhi 0 n total) (rf-grows ghi 0 m total)))

; ----- Bareiss one-step fraction-free determinant over rfpoly (pivoting + sign) -----
(define (ff-piv-go M i r col) (if (null? M) -1
   (if (< i r) (ff-piv-go (cdr M) (+ i 1) r col)
       (if (rfpoly-zero? (rf-nth (car M) col)) (ff-piv-go (cdr M) (+ i 1) r col) i))))
(define (ff-pivot M r col) (ff-piv-go M 0 r col))
(define (ff-swap M a b) (ff-swap-go M 0 a b (rf-nth M a) (rf-nth M b)))
(define (ff-swap-go M i a b ra rb) (if (null? M) (quote ())
   (cons (cond ((= i a) rb) ((= i b) ra) (else (car M))) (ff-swap-go (cdr M) (+ i 1) a b ra rb))))
(define (ff-bs-cols row j k prev Mkk pivrow Mik) (if (null? row) (quote ())
   (cons (if (<= j k) (car row)
             (rfpoly-div (rfpoly-sub (rfpoly-mul Mkk (car row)) (rfpoly-mul Mik (rf-nth pivrow j))) prev))
         (ff-bs-cols (cdr row) (+ j 1) k prev Mkk pivrow Mik))))
(define (ff-bs-rows M i k prev Mkk pivrow) (if (null? M) (quote ())
   (cons (if (<= i k) (car M) (ff-bs-cols (car M) 0 k prev Mkk pivrow (rf-nth (car M) k)))
         (ff-bs-rows (cdr M) (+ i 1) k prev Mkk pivrow))))
(define (ff-bar-loop M k prev sign)
  (if (>= k (length M)) (rf-sign (rf-nth (rf-nth M (- (length M) 1)) (- (length M) 1)) sign)
      (let ((p (ff-pivot M k k)))
        (if (< p 0) (rf-zero)
            (let ((M2 (if (= p k) M (ff-swap M k p))) (sgn (if (= p k) sign (- 0 sign))))
              (ff-bar-loop (ff-bs-rows M2 0 k prev (rf-nth (rf-nth M2 k) k) (rf-nth M2 k)) (+ k 1) (rf-nth (rf-nth M2 k) k) sgn))))))
(define (ff-bareiss M) (if (null? M) (rf-one) (ff-bar-loop M 0 (rf-one) 1)))

; ----- fraction-free resultant Res_theta2(Dbar, gbar), returned as a K1 element (rfpoly over 1) -----
(define (h2-resultant-ff Dbar gbar)
  (let ((m (rf2-deg Dbar)) (n (rf2-deg gbar)))
    (if (if (< m 0) #t (< n 0)) (list (rf-zero) (rf-one))
        (if (= n 0) (list (rfpoly-pow (car (rf2-norm gbar)) m) (rf-one))
            (if (= m 0) (list (rfpoly-pow (car (rf2-norm Dbar)) n) (rf-one))
                (list (ff-bareiss (rf-sylvester (reverse (rf2-norm Dbar)) (reverse (rf2-norm gbar)) m n (+ m n))) (rf-one)))))))

; ----- gcd-free constancy check for the resultant ratios: Res_k = q Res_0 for rational q (no rfpoly-gcd) -----
(define (ff-nth lst i) (if (= i 0) (car lst) (ff-nth (cdr lst) (- i 1))))
(define (ff-first-nonzero rs k) (if (null? rs) -1 (if (rfpoly-zero? (car rs)) (ff-first-nonzero (cdr rs) (+ k 1)) k)))
(define (ff-rat-pure? r) (if (<= (poly-deg (rat-num r)) 0) (<= (poly-deg (rat-den r)) 0) #f))   ; tower-rat is a rational constant
(define (ff-rat-cval r) (/ (if (null? (rat-num r)) 0 (car (rat-num r))) (if (null? (rat-den r)) 1 (car (rat-den r)))))
(define (ff-ratio1 r r0)                          ; rational q with r = q*r0, or 'notconst
  (if (rfpoly-zero? r) 0
      (let ((q (rat-div (rfpoly-lead r) (rfpoly-lead r0))))
        (if (ff-rat-pure? q) (if (rfpoly-equal? r (rfpoly-cscale q r0)) (ff-rat-cval q) (quote notconst)) (quote notconst)))))
(define (ff-ratios rs r0 acc)
  (if (null? rs) (reverse acc)
      (let ((x (ff-ratio1 (car rs) r0))) (if (equal? x (quote notconst)) (quote notconst) (ff-ratios (cdr rs) r0 (cons x acc))))))

; ----- fraction-free logarithmic part: same as towerrt's h2rt-logpart but the resultant values come from
;       the Bareiss path above (clearing factor cancels in the ratios), with (gc) between evaluations -----
(define (ff-Rvals Dbar Abar M k N acc)
  (if (> k N) (reverse acc)
      (let ((r (car (h2-resultant-ff Dbar (rf2-sub Abar (rf2-iscale k M))))))   ; the resultant as a bare rfpoly
        (begin (gc) (ff-Rvals Dbar Abar M (+ k 1) N (cons r acc))))))
(define (h2rt-logpart-ff As Ds Dth2 mono1)
  (begin (gc)
  (let ((D2D (t2-deriv Ds Dth2 mono1)) (N (h2-deg Ds)))
    (let ((cD (ff-denlcm Ds (rf-one))))
      (let ((c (ff-lcm2 (ff-denlcm As (rf-one)) (ff-denlcm D2D (rf-one)))))
        (let ((rs (ff-Rvals (ff-clear-poly Ds cD) (ff-clear-poly As c) (ff-clear-poly D2D c) 0 N (quote ()))))
          (let ((k0 (ff-first-nonzero rs 0)))
            (if (< k0 0) (list (quote degenerate))
                (let ((rats (ff-ratios rs (ff-nth rs k0) (quote ()))))
                  (if (equal? rats (quote notconst)) (list (quote algebraic))
                      (let ((roots (ros-rational-roots (q-lagrange (h2rt-int-list N) rats))))
                        (let ((tt (h2rt-terms-ff roots As Ds D2D Dth2 mono1 (quote ()) 0)))
                          (if (= (car (cdr tt)) (h2-deg Ds)) (list (quote rootsum) (car tt)) (list (quote algebraic)))))))))))))))

; ----- the full height-two integrator using the fraction-free logarithmic part -----
(define (int-h2-full-ff A D Dth2 mono1)
  (let ((H (h2-hermite A D Dth2 mono1)))
    (let ((g (car H)) (As (car (cdr H))) (Ds (car (cdr (cdr H)))))
      (if (h2-zero? As) (list (quote ok) g (quote none))
          (let ((lg (h2rt-logpart-ff As Ds Dth2 mono1)))
            (if (equal? (car lg) (quote rootsum)) (list (quote ok) g lg) (list (quote partial) g As Ds lg)))))))
(define (int-h2-full-ff-elementary? A D Dth2 mono1) (equal? (car (int-h2-full-ff A D Dth2 mono1)) (quote ok)))
(define (int-h2-full-ff-verify A D Dth2 mono1)      ; same differentiation certificate as towerrt
  (let ((res (int-h2-full-ff A D Dth2 mono1)))
    (if (equal? (car res) (quote ok)) (h2tr-equal? (int-h2-full-deriv res Dth2 mono1) (list A D)) #f)))

; ----- fraction-free GCD over Q(x)[theta1] (primitive PRS): keeps theta2-coefficients polynomial and
;       divides out the rfpoly content each step, so the log-argument extraction avoids the K1 Euclidean
;       fraction blow-up that h2-gcd suffers at degree three -----
(define (rf2-lc P) (car (reverse (rf2-norm P))))
(define (rf2-scale-rf r P) (if (null? P) (quote ()) (cons (rfpoly-mul r (car P)) (rf2-scale-rf r (cdr P)))))
(define (rf2-div-rf P r) (if (null? P) (quote ()) (cons (rfpoly-div (car P) r) (rf2-div-rf (cdr P) r))))
(define (rf2-shift P k) (if (<= k 0) P (rf2-shift (cons (rf-zero) P) (- k 1))))
(define (rf2-zero? P) (null? (rf2-norm P)))
(define (rf2-prem-step f g)
  (rf2-sub (rf2-scale-rf (rf2-lc g) f) (rf2-shift (rf2-scale-rf (rf2-lc f) g) (- (rf2-deg f) (rf2-deg g)))))
(define (rf2-prem f g) (if (< (rf2-deg f) (rf2-deg g)) f (rf2-prem (rf2-prem-step f g) g)))
(define (rf2-content-go P acc) (if (null? P) acc (rf2-content-go (cdr P) (rfpoly-gcd acc (car P)))))
(define (rf2-content P) (rf2-content-go (rf2-norm P) (rf-zero)))
(define (rf2-pp P) (let ((c (rf2-content P))) (if (rfpoly-zero? c) P (rf2-div-rf (rf2-norm P) c))))
(define (rf2-gcd-prim F G) (if (rf2-zero? G) (rf2-pp F) (rf2-gcd-prim G (rf2-pp (rf2-prem F G)))))
(define (rf2-clear-gcd Dh gh)
  (let ((Dbar (ff-clear-poly Dh (ff-denlcm Dh (rf-one)))) (gbar (ff-clear-poly gh (ff-denlcm gh (rf-one)))))
    (if (>= (rf2-deg Dbar) (rf2-deg gbar)) (rf2-gcd-prim Dbar gbar) (rf2-gcd-prim gbar Dbar))))

; ----- fraction-free term extraction: v_c = monic gcd(D, A - c D2D), computed fraction-free then made
;       monic over K1 (the rfpoly content drops out under monic-ization) -----
(define (ff-to-h2 rf2) (if (null? rf2) (quote ()) (cons (list (car rf2) (rf-one)) (ff-to-h2 (cdr rf2)))))
(define (h2rt-terms-ff roots As Ds D2D Dth2 mono1 acc tdeg)
  (if (null? roots) (list acc tdeg)
      (let ((c (car roots)))
        (let ((v (h2-monic (ff-to-h2 (rf2-clear-gcd Ds (h2-sub As (h2-cscale (k1-from-rat c) D2D)))))))
          (if (>= (h2-deg v) 1)
              (begin (gc) (h2rt-terms-ff (cdr roots) As Ds D2D Dth2 mono1 (cons (list c v) acc) (+ tdeg (h2-deg v))))
              (begin (gc) (h2rt-terms-ff (cdr roots) As Ds D2D Dth2 mono1 acc tdeg)))))))
