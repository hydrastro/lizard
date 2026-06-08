; -*- lisp -*-
; lib/cas/tower2herm.lisp -- Hermite reduction at height two: the rational part of integrating a
; rational function of a second monomial theta2 over the height-one field K1 = Q(x)(theta1).
;
; This is the first substantive rung of full height-two integration.  It mirrors the height-one
; Hermite reduction (tower.lisp) one level up: every operation on Q(x)[theta1] is replaced by the
; analogous operation on K1[theta2], whose coefficient field K1 carries the field operations built in
; tower2.lisp, and the height-one derivation Drf is replaced by the two-level derivation t2-deriv.
; The setting is theta2 primitive over K1 (a logarithm, so D theta2 is an element of K1), for instance
; theta1 = e^x and theta2 = log(e^x + 1).  Given a proper rational function A/D of theta2 over K1 with
; D monic, the reduction returns a rational part g (a fraction of polynomials in theta2 over K1) and a
; remainder A*/D* with D* squarefree in theta2, such that D2(g) + A*/D* = A/D, where D2 is the
; two-level derivation.  The result is certified by differentiating g with D2 (quotient rule, using
; t2-deriv on numerator and denominator) and checking the identity by cross-multiplication over
; K1[theta2].  Squarefree factorization in theta2 uses the formal theta2-derivative; the genuine
; derivation D2 enters only through the derivatives of the squarefree factors, exactly as at height
; one.  Builds on tower2.lisp.

(import "cas/tower2.lisp")

; ----- K1 = Q(x)(theta1) field operations: reduced fractions, cancel-before-multiply -----
(define (k1-normalize a)                     ; assume gcd(num,den)=1; make denominator monic
  (if (rfpoly-zero? (car a)) (tr-zero)
      (let ((lc (rat-inv (rfpoly-lead (car (cdr a)))))) (list (rfpoly-cscale lc (car a)) (rfpoly-cscale lc (car (cdr a)))))))
(define (k1-add a b) (tr-reduce (tr-add a b)))
(define (k1-neg a) (list (rfpoly-neg (car a)) (car (cdr a))))
(define (k1-sub a b) (tr-reduce (tr-add a (k1-neg b))))
(define (k1-mul a b)                         ; a,b reduced: cancel cross factors, then multiply
  (if (if (rfpoly-zero? (car a)) #t (rfpoly-zero? (car b))) (tr-zero)
    (let ((g1 (rfpoly-gcd (car a) (car (cdr b)))) (g2 (rfpoly-gcd (car b) (car (cdr a)))))
      (k1-normalize (list (rfpoly-mul (rfpoly-div (car a) g1) (rfpoly-div (car b) g2))
                          (rfpoly-mul (rfpoly-div (car (cdr a)) g2) (rfpoly-div (car (cdr b)) g1)))))))
(define (k1-inv a) (k1-normalize (t2-trinv a)))
(define (k1-div a b) (k1-mul a (k1-inv b)))
(define (k1-zero) (tr-zero))
(define (k1-one) (t2-trone))
(define (k1-zero? a) (rfpoly-zero? (car a)))
(define (k1-iscale i a) (tr-reduce (list (rf-iscale i (car a)) (car (cdr a)))))

; ----- K1[theta2]: polynomials in theta2 with K1 coefficients, low -> high -----
(define (h2-drop0 R) (cond ((null? R) '()) ((k1-zero? (car R)) (h2-drop0 (cdr R))) (else R)))
(define (h2-norm P) (reverse (h2-drop0 (reverse P))))
(define (h2-zero? P) (null? (h2-norm P)))
(define (h2-len l) (if (null? l) 0 (+ 1 (h2-len (cdr l)))))
(define (h2-deg P) (- (h2-len (h2-norm P)) 1))
(define (h2-nth l k) (if (= k 0) (car l) (h2-nth (cdr l) (- k 1))))
(define (h2-lead P) (h2-nth (h2-norm P) (h2-deg P)))
(define (h2-neg P) (if (null? P) '() (cons (k1-neg (car P)) (h2-neg (cdr P)))))
(define (h2-add P Q) (cond ((null? P) Q) ((null? Q) P) (else (cons (k1-add (car P) (car Q)) (h2-add (cdr P) (cdr Q))))))
(define (h2-sub P Q) (h2-add P (h2-neg Q)))
(define (h2-cscale e P) (if (null? P) '() (cons (k1-mul e (car P)) (h2-cscale e (cdr P)))))
(define (h2-iscale i P) (if (null? P) '() (cons (k1-iscale i (car P)) (h2-iscale i (cdr P)))))
(define (h2-shift P k) (if (= k 0) P (cons (k1-zero) (h2-shift P (- k 1)))))
(define (h2-monomial c k) (h2-shift (list c) k))
(define (h2-mul P Q) (if (null? P) '() (h2-add (h2-cscale (car P) Q) (h2-shift (h2-mul (cdr P) Q) 1))))
(define (h2-divmod-loop r d q)
  (if (< (h2-deg r) (h2-deg d)) (list (h2-norm q) (h2-norm r))
    (let ((c (k1-div (h2-lead r) (h2-lead d))) (k (- (h2-deg r) (h2-deg d))))
      (let ((t (h2-monomial c k)))
        (h2-divmod-loop (h2-sub r (h2-mul t d)) d (h2-add q t))))))
(define (h2-divmod a b) (h2-divmod-loop (h2-norm a) (h2-norm b) '()))
(define (h2-div a b) (car (h2-divmod a b)))
(define (h2-rem a b) (car (cdr (h2-divmod a b))))
(define (h2-monic P) (if (h2-zero? P) P (h2-cscale (k1-inv (h2-lead P)) P)))
(define (h2-gcd a b) (if (h2-zero? b) (h2-monic a) (h2-gcd b (h2-rem a b))))
(define (h2-pow P k) (if (= k 0) (list (k1-one)) (h2-mul P (h2-pow P (- k 1)))))
(define (h2-equal? a b) (h2-zero? (h2-sub a b)))
(define (h2-dtheta2-terms P i) (if (null? P) '() (cons (k1-iscale i (car P)) (h2-dtheta2-terms (cdr P) (+ i 1)))))
(define (h2-dtheta2 P) (if (or (null? P) (null? (cdr P))) '() (h2-dtheta2-terms (cdr P) 1)))

; extended Euclid over K1[theta2]
(define (h2-eea oa r osa s ot t)
  (if (h2-zero? r) (list oa osa ot)
    (let ((q (h2-div oa r)))
      (h2-eea r (h2-sub oa (h2-mul q r)) s (h2-sub osa (h2-mul q s)) t (h2-sub ot (h2-mul q t))))))
(define (h2-bezout a b) (h2-eea a b (list (k1-one)) '() '() (list (k1-one))))
(define (h2-invmod p v)                       ; inverse of p modulo v (gcd a constant in theta2)
  (let ((res (h2-bezout (h2-rem p v) v)))
    (h2-rem (h2-cscale (k1-inv (car (h2-norm (car res)))) (car (cdr res))) v)))

; Yun squarefree factorization in theta2 (formal theta2-derivative): list of (mult . factor)
(define (h2-yun f)
  (let ((a (h2-gcd f (h2-dtheta2 f))))
    (let ((b (h2-div f a)) (c (h2-div (h2-dtheta2 f) a)))
      (h2-yun-loop b (h2-sub c (h2-dtheta2 b)) 1 '()))))
(define (h2-yun-loop b d i acc)
  (if (<= (h2-deg b) 0) (reverse acc)
    (let ((g (h2-gcd b d)))
      (let ((bn (h2-div b g)) (cn (h2-div d g)))
        (h2-yun-loop bn (h2-sub cn (h2-dtheta2 bn)) (+ i 1)
                     (if (> (h2-deg g) 0) (cons (cons i (h2-monic g)) acc) acc))))))
(define (h2-max-mult sf best) (if (null? sf) best (h2-max-mult (cdr sf) (if (> (car (car sf)) (car best)) (car sf) best))))

; ----- height-two rational accumulator (fraction of K1[theta2] polynomials) -----
(define (h2tr-zero) (list '() (list (k1-one))))
(define (h2tr-add a b) (list (h2-add (h2-mul (car a) (car (cdr b))) (h2-mul (car b) (car (cdr a)))) (h2-mul (car (cdr a)) (car (cdr b)))))
(define (h2tr-deriv N D Dth2 mono1) (list (h2-sub (h2-mul (t2-deriv N Dth2 mono1) D) (h2-mul N (t2-deriv D Dth2 mono1))) (h2-mul D D)))
(define (h2tr-equal? p q) (h2-equal? (h2-mul (car p) (car (cdr q))) (h2-mul (car q) (car (cdr p)))))

; ----- one Hermite step at height two (mirror of hermite-step, with D2 = t2-deriv) -----
(define (h2-hermite-step a d m v Dth2 mono1)
  (let ((w (h2-div d (h2-pow v m))) (Dv (t2-deriv v Dth2 mono1)))
    (let ((b (h2-rem (h2-mul (h2-neg a) (h2-invmod (h2-iscale (- m 1) (h2-mul w Dv)) v)) v)))
      (let ((Db (t2-deriv b Dth2 mono1)))
        (let ((num (h2-add a (h2-add (h2-neg (h2-mul w (h2-mul Db v))) (h2-iscale (- m 1) (h2-mul w (h2-mul b Dv)))))))
          (list (list b (h2-pow v (- m 1)))
                (h2-div num v)
                (h2-mul (h2-pow v (- m 1)) w)))))))
(define (h2-hermite-loop a d g Dth2 mono1)
  (let ((hi (h2-max-mult (h2-yun d) (cons 0 '()))))
    (if (<= (car hi) 1) (list g a d)
      (let ((step (h2-hermite-step a d (car hi) (cdr hi) Dth2 mono1)))
        (h2-hermite-loop (car (cdr step)) (car (cdr (cdr step))) (h2tr-add g (car step)) Dth2 mono1)))))
(define (h2-hermite a d Dth2 mono1) (h2-hermite-loop a d (h2tr-zero) Dth2 mono1))  ; -> (g-h2tr a* d*), d* squarefree

(define (h2-hermite-verify A D Dth2 mono1)
  (let ((H (h2-hermite A D Dth2 mono1)))
    (h2tr-equal? (h2tr-add (h2tr-deriv (car (car H)) (car (cdr (car H))) Dth2 mono1) (list (car (cdr H)) (car (cdr (cdr H))))) (list A D))))
