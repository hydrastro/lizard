; -*- lisp -*-
; lib/cas/algresn.lisp -- algebraic-residue RootSum closure for an IRREDUCIBLE residue polynomial of
; ANY degree (the Lazard-Rioboo-Trager-style general case, generalizing the quadratic algres.lisp).
;
; For INT a/d (d squarefree in the monomial theta) the Rothstein-Trager residue polynomial
; R(z) = Res_theta(d, a - z Dd) may be monic over Q with an irreducible factor of degree m > 1, whose
; roots are a single conjugate class.  The logarithmic part is then the RootSum
; sum_{R(c)=0} c log(v_c) with v_c = gcd_theta(d, a - c Dd).  This module closes the case in which
; R(z) is itself irreducible of degree m: the argument v_c is computed over the number field
; K = Q(x)[c]/(R(c)), which -- because a K-element is just a polynomial in c over Q(x) -- reuses the
; ordinary rfpoly arithmetic, reduced modulo R.  The answer is certified WITHOUT extracting any
; radical.  The Rothstein-Trager factors satisfy prod_{R(c)=0} v_c = d, so the norm of v_c is d
; itself and the cofactor d/v_c is obtained by one division in K[theta]; the derivative of the RootSum
; is the trace sum_{R(c)=0} c Dv_c/v_c = (trace of c Dv_c (d/v_c)) / d, whose numerator is computed by
; taking the field trace coefficient-by-coefficient.  The field trace down to Q(x) uses the power sums
; of the roots of R obtained from Newton's identities, so no conjugate is ever named.  For the
; exponential tower the elementary answer carries the expnrt base-field correction
; -(sum c deg v_c) x = -deg(v_c) p_1 x, with p_1 the sum of the roots.  Every answer is gated by the
; differentiation certificate trace-numerator/d = a/d (plus correction): cases that do not certify,
; or whose residue polynomial is reducible, are left reported 'algebraic, so the closure strictly
; tightens towerrt.lisp / expnrt.lisp.  Builds on towerrt.lisp and factor.lisp.

(import "cas/towerrt.lisp")
(import "cas/factor.lisp")

(define (anr-rev l a) (if (null? l) a (anr-rev (cdr l) (cons (car l) a))))
(define (anr-len l) (if (null? l) 0 (+ 1 (anr-len (cdr l)))))
(define (anr-nth l k) (if (= k 0) (car l) (anr-nth (cdr l) (- k 1))))
(define (anr-isexp? mono) (equal? (car mono) 'exp))

; ===== number field K = Q(x)[c]/(R) : an element is an rfpoly in c (tower-rat coeffs) =====
(define (nf-const r) (list r))
(define (nf-c) (list (rat-zero) (rat-one)))
(define (nf-add e f) (rfpoly-add e f))
(define (nf-sub e f) (rfpoly-sub e f))
(define (nf-neg e) (rfpoly-neg e))
(define (nf-scale r e) (rfpoly-cscale r e))
(define (nf-zero? e) (rfpoly-zero? e))
(define (nf-mul R e f) (rfpoly-rem (rfpoly-mul e f) R))
(define (nf-inv R e) (let ((bz (rfpoly-bezout e R))) (rfpoly-rem (rfpoly-cscale (rat-inv (rfpoly-lead (car bz))) (car (cdr bz))) R)))
(define (nf-deriv e) (if (null? e) '() (cons (rat-deriv (car e)) (nf-deriv (cdr e)))))

; power sums p_0..p_{m-1} of the roots of monic R (R given as Q-coeff list low->high, length m+1)
(define (anr-elem Rq m i) (* (if (= (remainder i 2) 0) 1 -1) (anr-nth Rq (- m i))))   ; e_i = (-1)^i a_{m-i}
(define (anr-ps-k Rq m ps k i acc)              ; Newton: p_k = sum_{i=1}^{k-1}(-1)^{i-1} e_i p_{k-i} + (-1)^{k-1} k e_k
  (if (= i k)
      (+ acc (* (if (= (remainder (- k 1) 2) 0) 1 -1) (* k (if (> k m) 0 (anr-elem Rq m k)))))
      (anr-ps-k Rq m ps k (+ i 1) (+ acc (* (if (= (remainder (- i 1) 2) 0) 1 -1) (* (anr-elem Rq m i) (anr-nth ps (- k i))))))))
(define (anr-ps-go Rq m ps k) (if (>= k m) (anr-rev ps '()) (anr-ps-go Rq m (cons (anr-ps-k Rq m (anr-rev ps '()) k 1 0) ps) (+ k 1))))
(define (anr-powersums Rq m) (anr-ps-go Rq m (list m) 1))   ; ps as list p_0..p_{m-1} (built reversed, fixed by callers)
; trace of nf element e given power sums ps
(define (nf-trace ps e i acc) (if (null? e) acc (nf-trace ps (cdr e) (+ i 1) (rat-add acc (rat-mul (car e) (rat-from-poly (list (anr-nth ps i))))))))
(define (nf-tr ps e) (nf-trace ps e 0 (rat-zero)))

; ===== K[theta] : polynomials in theta with nf coefficients =====
(define (nfp-drop0 R) (cond ((null? R) '()) ((nf-zero? (car R)) (nfp-drop0 (cdr R))) (else R)))
(define (nfp-trim P) (anr-rev (nfp-drop0 (anr-rev P '())) '()))
(define (nfp-zero? P) (null? (nfp-trim P)))
(define (nfp-deg P) (- (anr-len (nfp-trim P)) 1))
(define (nfp-lead P) (anr-nth (nfp-trim P) (nfp-deg P)))
(define (nfp-neg P) (if (null? P) '() (cons (nf-neg (car P)) (nfp-neg (cdr P)))))
(define (nfp-add P Q) (cond ((null? P) Q) ((null? Q) P) (else (cons (nf-add (car P) (car Q)) (nfp-add (cdr P) (cdr Q))))))
(define (nfp-sub P Q) (cond ((null? Q) P) ((null? P) (nfp-neg Q)) (else (cons (nf-sub (car P) (car Q)) (nfp-sub (cdr P) (cdr Q))))))
(define (nfp-cscale R e P) (if (null? P) '() (cons (nf-mul R e (car P)) (nfp-cscale R e (cdr P)))))
(define (nfp-shift P k) (if (= k 0) P (cons '() (nfp-shift P (- k 1)))))
(define (nfp-mul R P Q) (if (null? P) '() (nfp-add (nfp-cscale R (car P) Q) (nfp-shift (nfp-mul R (cdr P) Q) 1))))
(define (nfp-dm R A B acc)
  (if (if (nfp-zero? A) #t (< (nfp-deg A) (nfp-deg B))) (cons acc (nfp-trim A))
      (let ((co (nf-mul R (nfp-lead A) (nf-inv R (nfp-lead B)))))
        (let ((term (nfp-shift (list co) (- (nfp-deg A) (nfp-deg B)))))
          (nfp-dm R (nfp-trim (nfp-sub A (nfp-mul R term B))) B (nfp-add acc term))))))
(define (nfp-quot R A B) (car (nfp-dm R (nfp-trim A) (nfp-trim B) '())))
(define (nfp-rem R A B) (cdr (nfp-dm R (nfp-trim A) (nfp-trim B) '())))
(define (nfp-gcd R A B) (if (nfp-zero? B) (nfp-trim A) (nfp-gcd R B (nfp-rem R A B))))
(define (nfp-monic R P) (nfp-cscale R (nf-inv R (nfp-lead P)) P))
(define (rf->nfp d) (if (null? d) '() (cons (nf-const (car d)) (rf->nfp (cdr d)))))
(define (nfp-Dlog-go P i)
  (if (null? P) '()
      (nfp-add (list (nf-add (nf-deriv (car P))
                             (if (null? (cdr P)) '()
                                 (nf-scale (rat-make (list 1) (list 0 1)) (nf-scale (rat-from-poly (list (+ i 1))) (car (cdr P)))))))
               (nfp-shift (nfp-Dlog-go (cdr P) (+ i 1)) 1))))
(define (nfp-Dexp-go P i) (if (null? P) '() (cons (nf-add (nf-deriv (car P)) (nf-scale (rat-from-poly (list i)) (car P))) (nfp-Dexp-go (cdr P) (+ i 1)))))
(define (nfp-Dtheta P mono) (if (anr-isexp? mono) (nfp-Dexp-go P 0) (nfp-Dlog-go P 0)))
; trace each theta-coefficient of an nfp down to Q(x), giving an rfpoly in theta
(define (nfp-trace-rf ps P) (if (null? P) '() (cons (nf-tr ps (car P)) (nfp-trace-rf ps (cdr P)))))

(define (anr-irreducible? Rq) (let ((irrs (car (cdr (factor-Q Rq))))) (if (= (anr-len irrs) 1) (if (= (car (car irrs)) 1) (= (poly-deg (car (cdr (car irrs)))) (poly-deg Rq)) #f) #f)))

; ===== the general RootSum closure: a/d, d squarefree, R irreducible of degree m =====
; -> (list 'rootsum R vc corr) | (list 'fail)
(define (ar-rootsum a d mono)
  (let ((Dd (Drf d mono)))
    (let ((mz (trt-monic (trt-buildR a d Dd))))
      (let ((Rq (trt-toQ mz)))
        (if (if (>= (rfpoly-deg mz) 2) (if (trt-allconst? mz) (if (null? (ros-rational-roots Rq)) (anr-irreducible? Rq) #f) #f) #f)
            (let ((ps (anr-powersums Rq (rfpoly-deg mz))) (dm (rfpoly-monic d)))
              (let ((vc (nfp-monic mz (nfp-gcd mz (rf->nfp dm) (nfp-sub (rf->nfp a) (nfp-cscale mz (nf-c) (rf->nfp Dd)))))))
                (let ((cofac (nfp-quot mz (rf->nfp dm) vc)))
                  (let ((tnum (rfpoly-norm (nfp-trace-rf ps (nfp-mul mz (nfp-cscale mz (nf-c) (nfp-Dtheta vc mono)) cofac))))
                        (corr (if (anr-isexp? mono) (* (nfp-deg vc) (anr-nth ps 1)) 0)))
                    (if (tr-equal? (list tnum dm) (list (rfpoly-add a (rfpoly-cscale (rat-from-poly (list corr)) d)) d))
                        (list 'rootsum mz vc corr tnum dm)
                        (list 'fail))))))
            (list 'fail))))))

; ===== integrate A/D proper, rational in theta, allowing an irreducible algebraic residue =====
;   (list 'ok g terms) | (list 'rootsum g R vc corr) | (list 'algebraic) | (list 'non-elementary)
(define (int-prim-rational-algn A D mono)
  (let ((H (hermite A D mono)))
    (let ((g (tr-reduce (car H))) (as (car (cdr H))) (ds (car (cdr (cdr H)))))
      (if (rfpoly-zero? as) (list 'ok g '())
          (let ((rt (trt-logpart as ds mono)))
            (if (equal? (car rt) 'ok) (list 'ok g (car (cdr rt)))
                (if (equal? (car rt) 'algebraic)
                    (let ((ar (ar-rootsum as ds mono)))
                      (if (equal? (car ar) 'rootsum) (list 'rootsum g (anr-nth ar 1) (anr-nth ar 2) (anr-nth ar 3) (anr-nth ar 4) (anr-nth ar 5)) (list 'algebraic)))
                    rt)))))))
(define (int-prim-rational-algn-verify A D mono)
  (let ((r (int-prim-rational-algn A D mono)))
    (cond ((equal? (car r) 'ok)
           (tr-equal? (trt-add-logderivs (tr-deriv (car (cdr r)) mono) (car (cdr (cdr r))) mono) (list A D)))
          ((equal? (car r) 'rootsum)
           (tr-equal? (tr-add (tr-deriv (anr-nth r 1) mono) (list (anr-nth r 5) (anr-nth r 6)))
                      (list (rfpoly-add A (rfpoly-cscale (rat-from-poly (list (anr-nth r 4))) D)) D)))
          (else #f))))
(define (int-prim-rational-algn-elementary? A D mono)
  (let ((s (car (int-prim-rational-algn A D mono)))) (if (equal? s 'ok) #t (equal? s 'rootsum))))
