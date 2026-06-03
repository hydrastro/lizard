; -*- lisp -*-
; lib/cas/tower2algn.lisp -- height-two RootSums with algebraic residues of ARBITRARY degree.
;
; tower2alg handled an irreducible-QUADRATIC residue at height two via the conjugate-pair trace in K1(alpha).
; This module lifts that to any degree.  When the Rothstein-Trager residue polynomial q(z) (recovered, as
; always, by the fraction-free ratio trick of tower2ff -- the ratios q(z_k)/q(z_0) are rational because q
; in Q[z] is evaluated at integers) is irreducible of degree d >= 2 over Q, the d residues are a full set
; of conjugate algebraic numbers and the antiderivative is the RootSum sum_{q(alpha)=0} alpha log(v_alpha),
; with v_alpha in K1(alpha)[theta2], K1(alpha) = K1[alpha]/(q).  We never split the algebraic closure: the
; RootSum's logarithmic derivative is a TRACE over the conjugates and so descends to K1[theta2],
;     [ Tr_{K1(alpha)/K1} ( alpha D2(v_alpha) (D*/v_alpha) ) ] / D* ,
; and the integral is certified by checking that this trace numerator equals A* exactly over K1[theta2].
; The extension arithmetic is the key reuse: K1(alpha) elements are h2polys over K1 reduced modulo q
; (so k1d-mul = h2-rem . h2-mul, k1d-inv = h2-invmod), and K1(alpha)[theta2] is one more polynomial layer
; on top (h2d-*, mirroring the K1[theta2] long division).  The conjugate trace is the regular-representation
; trace Tr(sum a_i alpha^i) = sum a_i s_i, where s_i are the power sums of q's roots from Newton's identities.
; v_alpha = gcd(D*, A* - alpha D2(D*)) is computed by Euclid over K1(alpha)[theta2].  Builds on tower2ff.

(import "cas/tower2ff.lisp")

; ===== K1(alpha) = K1[alpha]/(qK1), elements are h2polys over K1 of degree < d (reuse h2-* for arithmetic) =====
(define (k1d-mul qK1 x y) (h2-rem (h2-mul x y) qK1))
(define (k1d-inv qK1 x) (h2-invmod x qK1))
(define (k1d-div qK1 x y) (k1d-mul qK1 x (k1d-inv qK1 y)))
(define (k1d-zero) (quote ()))
(define (k1d-one) (list (k1-one)))
(define (k1d-gen) (list (k1-zero) (k1-one)))                 ; alpha
(define (k1d-lift k1) (list k1))                             ; a K1 scalar as a constant of K1(alpha)
(define (k1d-iscale n c) (h2-iscale n c))
(define (k1d-zero? c) (h2-zero? c))
(define (k1d-deriv c mono1) (if (null? c) (quote ()) (cons (tr-deriv (car c) mono1) (k1d-deriv (cdr c) mono1))))  ; D2, alpha constant
(define (k1d-liftq q) (if (null? q) (quote ()) (cons (k1-from-rat (car q)) (k1d-liftq (cdr q)))))                 ; Q-poly -> h2poly over K1

; power sums s_0..s_{d-1} of the roots of monic q (Newton's identities)
(define (algn-ps-nth lst i) (if (= i 0) (car lst) (algn-ps-nth (cdr lst) (- i 1))))
(define (algn-newton-sum q d k acc i s)
  (if (>= i k) s (algn-newton-sum q d k acc (+ i 1) (+ s (* (poly-coeff q (- d i)) (algn-ps-nth acc (- i 1)))))))
(define (algn-ps-go q d k acc)
  (if (>= k d) (reverse acc)
      (algn-ps-go q d (+ k 1) (cons (- 0 (+ (algn-newton-sum q d k acc 1 0) (* k (poly-coeff q (- d k))))) acc))))
(define (algn-powersums q d) (if (= d 0) (quote ()) (algn-ps-go q d 1 (list d))))
(define (k1d-trace svals c) (if (null? c) (k1-zero)
   (k1-add (k1-mul (k1-from-rat (car svals)) (car c)) (k1d-trace (cdr svals) (cdr c)))))

; ===== K1(alpha)[theta2]: h2d, list of k1d coefficients (low->high) =====
(define (h2d-zero? p) (if (null? p) #t (if (k1d-zero? (car p)) (h2d-zero? (cdr p)) #f)))
(define (h2d-strip p) (if (null? p) (quote ()) (if (k1d-zero? (car p)) (h2d-strip (cdr p)) p)))
(define (h2d-norm p) (reverse (h2d-strip (reverse p))))
(define (h2d-len l) (if (null? l) 0 (+ 1 (h2d-len (cdr l)))))
(define (h2d-deg p) (- (h2d-len (h2d-norm p)) 1))
(define (h2d-nth l k) (if (= k 0) (car l) (h2d-nth (cdr l) (- k 1))))
(define (h2d-lead p) (h2d-nth (h2d-norm p) (h2d-deg p)))
(define (h2d-add p q) (cond ((null? p) q) ((null? q) p) (else (cons (h2-add (car p) (car q)) (h2d-add (cdr p) (cdr q))))))
(define (h2d-neg p) (if (null? p) (quote ()) (cons (h2-neg (car p)) (h2d-neg (cdr p)))))
(define (h2d-sub p q) (h2d-add p (h2d-neg q)))
(define (h2d-cscale qK1 s p) (if (null? p) (quote ()) (cons (k1d-mul qK1 s (car p)) (h2d-cscale qK1 s (cdr p)))))
(define (h2d-shift p k) (if (= k 0) p (cons (k1d-zero) (h2d-shift p (- k 1)))))
(define (h2d-monomial c k) (h2d-shift (list c) k))
(define (h2d-mul qK1 p q) (if (null? p) (quote ()) (h2d-add (h2d-cscale qK1 (car p) q) (h2d-shift (h2d-mul qK1 (cdr p) q) 1))))
(define (h2d-divmod-loop qK1 r d q)
  (if (< (h2d-deg r) (h2d-deg d)) (list (h2d-norm q) (h2d-norm r))
    (let ((c (k1d-div qK1 (h2d-lead r) (h2d-lead d))) (k (- (h2d-deg r) (h2d-deg d))))
      (let ((t (h2d-monomial c k))) (h2d-divmod-loop qK1 (h2d-sub r (h2d-mul qK1 t d)) d (h2d-add q t))))))
(define (h2d-divmod qK1 a b) (h2d-divmod-loop qK1 (h2d-norm a) (h2d-norm b) (quote ())))
(define (h2d-div qK1 a b) (car (h2d-divmod qK1 a b)))
(define (h2d-rem qK1 a b) (car (cdr (h2d-divmod qK1 a b))))
(define (h2d-monic qK1 p) (if (h2d-zero? p) p (h2d-cscale qK1 (k1d-inv qK1 (h2d-lead p)) p)))
(define (h2d-gcd qK1 a b) (if (h2d-zero? b) (h2d-monic qK1 a) (begin (gc) (h2d-gcd qK1 b (h2d-rem qK1 a b)))))
(define (h2d-lift h2) (if (null? h2) (quote ()) (cons (k1d-lift (car h2)) (h2d-lift (cdr h2)))))
(define (h2d-trace svals p) (if (null? p) (quote ()) (cons (k1d-trace svals (car p)) (h2d-trace svals (cdr p)))))
; D2 on K1(alpha)[theta2], theta2 primitive (alpha constant): coefficientwise + chain rule
(define (t2d-dcw p mono1) (if (null? p) (quote ()) (cons (k1d-deriv (car p) mono1) (t2d-dcw (cdr p) mono1))))
(define (t2d-sdk p k) (if (null? p) (quote ()) (cons (k1d-iscale k (car p)) (t2d-sdk (cdr p) (+ k 1)))))
(define (t2d-deriv qK1 p Dth2 mono1)
  (if (null? p) (quote ()) (h2d-add (t2d-dcw p mono1) (h2d-cscale qK1 (k1d-lift Dth2) (t2d-sdk (cdr p) 1)))))

; ===== the algebraic-residue logarithmic part =====
(define (h2algn-respoly As Ds Dth2 mono1)        ; monic q(z) in Q[z], or 'notconst
  (let ((D2D (t2-deriv Ds Dth2 mono1)) (N (h2-deg Ds)))
    (let ((cD (ff-denlcm Ds (rf-one))))
      (let ((c (ff-lcm2 (ff-denlcm As (rf-one)) (ff-denlcm D2D (rf-one)))))
        (let ((rs (ff-Rvals (ff-clear-poly Ds cD) (ff-clear-poly As c) (ff-clear-poly D2D c) 0 N (quote ()))))
          (let ((k0 (ff-first-nonzero rs 0)))
            (let ((rats (ff-ratios rs (ff-nth rs k0) (quote ()))))
              (if (equal? rats (quote notconst)) (quote notconst) (poly-monic (q-lagrange (h2rt-int-list N) rats))))))))))
(define (h2algn-irreducible? q) (if (>= (poly-deg q) 2) (null? (ros-rational-roots q)) #f))
(define (h2algn-num As Ds Dth2 mono1 q)          ; Tr[ alpha D2(v_alpha) (D*/v_alpha) ] in K1[theta2]
  (let ((qK1 (k1d-liftq q)) (svals (algn-powersums q (poly-deg q))))
    (let ((Dd (h2d-lift Ds)) (Ad (h2d-lift As)) (D2Dd (h2d-lift (t2-deriv Ds Dth2 mono1))))
      (let ((g (h2d-sub Ad (h2d-cscale qK1 (k1d-gen) D2Dd))))
        (let ((va (begin (gc) (h2d-monic qK1 (h2d-gcd qK1 Dd g)))))
          (let ((cof (begin (gc) (h2d-div qK1 Dd va))))
            (begin (gc) (h2d-trace svals (h2d-mul qK1 (h2d-cscale qK1 (k1d-gen) (t2d-deriv qK1 va Dth2 mono1)) cof)))))))))
(define (h2algn-elementary? As Ds Dth2 mono1)
  (let ((q (h2algn-respoly As Ds Dth2 mono1))) (if (equal? q (quote notconst)) #f (h2algn-irreducible? q))))
(define (h2algn-verify As Ds Dth2 mono1)         ; differentiation/trace certificate
  (let ((q (h2algn-respoly As Ds Dth2 mono1)))
    (if (equal? q (quote notconst)) #f
        (if (h2algn-irreducible? q) (h2-equal? (h2-norm (h2algn-num As Ds Dth2 mono1 q)) (h2-norm As)) #f))))
