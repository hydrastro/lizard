; -*- lisp -*-
; lib/cas/algres.lisp -- algebraic-residue RootSum closure for the single-extension proper case.
;
; When the Rothstein-Trager residue polynomial R(z) for INT a/d (d squarefree in theta) is monic
; over Q with CONSTANT but IRRATIONAL roots, the logarithmic part is a RootSum over a conjugate set
; of algebraic residues, sum over R(c)=0 of c log(v_c), with v_c = gcd_theta(d, a - c Dd).  This
; module closes the most common such case: R(z) an irreducible quadratic z^2 + p z + q over Q (no
; rational roots), whose two roots c, cbar are a conjugate pair.  The argument v_c is computed
; symbolically over the field Q(x)(c) with c^2 = -p c - q, by the Euclidean algorithm in
; Q(x)(c)[theta].  The answer is then certified WITHOUT ever extracting a radical: the derivative of
; the RootSum is the trace sum_{R(c)=0} c Dv_c/v_c, which is a rational function over Q(x) because the
; conjugate contributions cancel the algebraic part (the conjugate of alpha + beta c is
; alpha - p beta - beta c, so a quantity and its conjugate sum to 2 alpha - p beta in Q(x)).  For the
; exponential tower the elementary answer carries the same base-field correction as expnrt, an extra
; (sum c deg v_c) x = -p deg(v_c) x term, so the certified identity is trace + p deg(v_c) = a/d.
; Every answer is gated by this differentiation certificate: if it does not hold the case is left as
; 'algebraic, so the closure never introduces an unjustified result -- it strictly tightens what
; towerrt.lisp and expnrt.lisp already report.  Builds on towerrt.lisp.

(import "cas/towerrt.lisp")

; ===== small list helpers =====
(define (ar-rev l a) (if (null? l) a (ar-rev (cdr l) (cons (car l) a))))
(define (ar-len l) (if (null? l) 0 (+ 1 (ar-len (cdr l)))))
(define (ar-nth l k) (if (= k 0) (car l) (ar-nth (cdr l) (- k 1))))
(define (ar-isexp? mono) (equal? (car mono) 'exp))

; ===== Q(x)(c): element (alpha . beta) = alpha + beta c, with c^2 = -p c - q (p,q tower-rats) =====
(define (qc-mk a b) (cons a b))
(define (qc-a e) (car e)) (define (qc-b e) (cdr e))
(define (qc-add e f) (qc-mk (rat-add (qc-a e) (qc-a f)) (rat-add (qc-b e) (qc-b f))))
(define (qc-sub e f) (qc-mk (rat-sub (qc-a e) (qc-a f)) (rat-sub (qc-b e) (qc-b f))))
(define (qc-neg e) (qc-mk (rat-neg (qc-a e)) (rat-neg (qc-b e))))
(define (qc-scale r e) (qc-mk (rat-mul r (qc-a e)) (rat-mul r (qc-b e))))
(define (qc-mul p q e f)
  (qc-mk (rat-sub (rat-mul (qc-a e) (qc-a f)) (rat-mul q (rat-mul (qc-b e) (qc-b f))))
         (rat-sub (rat-add (rat-mul (qc-a e) (qc-b f)) (rat-mul (qc-b e) (qc-a f))) (rat-mul p (rat-mul (qc-b e) (qc-b f))))))
(define (qc-conj p e) (qc-mk (rat-sub (qc-a e) (rat-mul p (qc-b e))) (rat-neg (qc-b e))))
(define (qc-norm p q e) (qc-a (qc-mul p q e (qc-conj p e))))
(define (qc-inv p q e) (qc-scale (rat-inv (qc-norm p q e)) (qc-conj p e)))
(define (qc-zero? e) (if (rat-zero? (qc-a e)) (rat-zero? (qc-b e)) #f))
(define (qc-deriv e) (qc-mk (rat-deriv (qc-a e)) (rat-deriv (qc-b e))))
(define (qc-zero) (qc-mk (rat-zero) (rat-zero)))
(define (qc-one) (qc-mk (rat-one) (rat-zero)))
(define (qc-cc) (qc-mk (rat-zero) (rat-one)))                  ; the algebraic number c itself
(define (qc-lift r) (qc-mk r (rat-zero)))

; ===== Q(x)(c)[theta]: coefficient lists low -> high =====
(define (qp-drop0 R) (cond ((null? R) '()) ((qc-zero? (car R)) (qp-drop0 (cdr R))) (else R)))
(define (qp-trim P) (ar-rev (qp-drop0 (ar-rev P '())) '()))
(define (qp-zero? P) (null? (qp-trim P)))
(define (qp-deg P) (- (ar-len (qp-trim P)) 1))
(define (qp-lead P) (ar-nth (qp-trim P) (qp-deg P)))
(define (qp-neg P) (if (null? P) '() (cons (qc-neg (car P)) (qp-neg (cdr P)))))
(define (qp-add P Q) (cond ((null? P) Q) ((null? Q) P) (else (cons (qc-add (car P) (car Q)) (qp-add (cdr P) (cdr Q))))))
(define (qp-sub P Q) (cond ((null? Q) P) ((null? P) (qp-neg Q)) (else (cons (qc-sub (car P) (car Q)) (qp-sub (cdr P) (cdr Q))))))
(define (qp-cscale pp q e P) (if (null? P) '() (cons (qc-mul pp q e (car P)) (qp-cscale pp q e (cdr P)))))
(define (qp-shift P k) (if (= k 0) P (cons (qc-zero) (qp-shift P (- k 1)))))
(define (qp-mul pp q P Q) (if (null? P) '() (qp-add (qp-cscale pp q (car P) Q) (qp-shift (qp-mul pp q (cdr P) Q) 1))))
(define (qp-dm pp q R D acc)
  (if (if (qp-zero? R) #t (< (qp-deg R) (qp-deg D))) (cons acc (qp-trim R))
      (let ((co (qc-mul pp q (qp-lead R) (qc-inv pp q (qp-lead D)))))
        (let ((term (qp-shift (list co) (- (qp-deg R) (qp-deg D)))))
          (qp-dm pp q (qp-trim (qp-sub R (qp-mul pp q term D))) D (qp-add acc term))))))
(define (qp-rem pp q P D) (cdr (qp-dm pp q (qp-trim P) (qp-trim D) '())))
(define (qp-gcd pp q P D) (if (qp-zero? D) (qp-trim P) (qp-gcd pp q D (qp-rem pp q P D))))
(define (qp-monic pp q P) (qp-cscale pp q (qc-inv pp q (qp-lead P)) P))
(define (rf->qp R) (if (null? R) '() (cons (qc-lift (car R)) (rf->qp (cdr R)))))
(define (qp->rf P) (if (null? P) '() (cons (qc-a (car P)) (qp->rf (cdr P)))))
(define (qp-conj-poly p P) (if (null? P) '() (cons (qc-conj p (car P)) (qp-conj-poly p (cdr P)))))

; tower derivative of a qcpoly (c is a constant, so it differentiates only the Q(x) parts)
(define (qp-Dlog-go P i)
  (if (null? P) '()
      (qp-add (list (qc-add (qc-deriv (car P))
                            (if (null? (cdr P)) (qc-zero)
                                (qc-scale (rat-make (list 1) (list 0 1)) (qc-scale (rat-from-poly (list (+ i 1))) (car (cdr P)))))))
              (qp-shift (qp-Dlog-go (cdr P) (+ i 1)) 1))))
(define (qp-Dlog P) (qp-Dlog-go P 0))
(define (qp-Dexp-go P i) (if (null? P) '() (cons (qc-add (qc-deriv (car P)) (qc-scale (rat-from-poly (list i)) (car P))) (qp-Dexp-go (cdr P) (+ i 1)))))
(define (qp-Dexp P) (qp-Dexp-go P 0))
(define (qp-Dtheta P mono) (if (ar-isexp? mono) (qp-Dexp P) (qp-Dlog P)))

; ===== the quadratic algebraic RootSum closure =====
; ar-quad a d mono -> (list 'rootsum p q vc corr) | (list 'fail)
;   meaning: INT a/d = sum_{c^2 + p c + q = 0} c log(vc)  [ + corr x for the exponential tower ],
;   certified by trace(c Dvc/vc) + corr = a/d.
(define (ar-quad a d mono)
  (let ((Dd (Drf d mono)))
    (let ((mz (trt-monic (trt-buildR a d Dd))))
      (if (if (= (rfpoly-deg mz) 2) (if (trt-allconst? mz) (null? (ros-rational-roots (trt-toQ mz))) #f) #f)
          (let ((PP (ar-nth mz 1)) (QQ (car mz)))
            (let ((vc (qp-monic PP QQ (qp-gcd PP QQ (rf->qp d) (qp-sub (rf->qp a) (qp-cscale PP QQ (qc-cc) (rf->qp Dd)))))))
              (let ((num (qp-cscale PP QQ (qc-cc) (qp-Dtheta vc mono))))
                (let ((tracenum (qp->rf (qp-add (qp-mul PP QQ num (qp-conj-poly PP vc)) (qp-mul PP QQ (qp-conj-poly PP num) vc))))
                      (normden  (qp->rf (qp-mul PP QQ vc (qp-conj-poly PP vc))))
                      (corr (if (ar-isexp? mono) (* (trt-constval PP) (qp-deg vc)) 0)))
                  (if (tr-equal? (list (rfpoly-norm (rfpoly-add tracenum (rfpoly-cscale (rat-from-poly (list corr)) normden))) (rfpoly-norm normden)) (list a d))
                      (list 'rootsum PP QQ vc corr)
                      (list 'fail))))))
          (list 'fail)))))

; ===== integrate A/D proper, rational in theta, allowing an algebraic quadratic residue =====
;   (list 'ok g terms) | (list 'rootsum g p q vc corr) | (list 'algebraic) | (list 'non-elementary)
(define (int-prim-rational-alg A D mono)
  (let ((H (hermite A D mono)))
    (let ((g (tr-reduce (car H))) (as (car (cdr H))) (ds (car (cdr (cdr H)))))
      (if (rfpoly-zero? as) (list 'ok g '())
          (let ((rt (trt-logpart as ds mono)))
            (if (equal? (car rt) 'ok) (list 'ok g (car (cdr rt)))
                (if (equal? (car rt) 'algebraic)
                    (let ((ar (ar-quad as ds mono)))
                      (if (equal? (car ar) 'rootsum) (list 'rootsum g (ar-nth ar 1) (ar-nth ar 2) (ar-nth ar 3) (ar-nth ar 4)) (list 'algebraic)))
                    rt)))))))

; end-to-end certificate: D(g) + (rootsum derivative) = A/D
(define (ar-rootsum-deriv p q vc corr mono)
  (let ((num (qp-cscale p q (qc-cc) (qp-Dtheta vc mono))))
    (let ((tracenum (qp->rf (qp-add (qp-mul p q num (qp-conj-poly p vc)) (qp-mul p q (qp-conj-poly p num) vc))))
          (normden  (qp->rf (qp-mul p q vc (qp-conj-poly p vc)))))
      (list (rfpoly-norm (rfpoly-add tracenum (rfpoly-cscale (rat-from-poly (list corr)) normden))) (rfpoly-norm normden)))))
(define (int-prim-rational-alg-verify A D mono)
  (let ((r (int-prim-rational-alg A D mono)))
    (cond ((equal? (car r) 'ok)
           (tr-equal? (trt-add-logderivs (tr-deriv (car (cdr r)) mono) (car (cdr (cdr r))) mono) (list A D)))
          ((equal? (car r) 'rootsum)
           (tr-equal? (tr-add (tr-deriv (ar-nth r 1) mono) (ar-rootsum-deriv (ar-nth r 2) (ar-nth r 3) (ar-nth r 4) (ar-nth r 5) mono)) (list A D)))
          (else #f))))
(define (int-prim-rational-alg-elementary? A D mono)
  (let ((s (car (int-prim-rational-alg A D mono)))) (if (equal? s 'ok) #t (equal? s 'rootsum))))
