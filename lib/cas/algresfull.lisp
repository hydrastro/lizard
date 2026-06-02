; -*- lisp -*-
; lib/cas/algresfull.lisp -- completes the algebraic-residue RootSum for a REDUCIBLE residue
; polynomial, giving the full single-extension logarithmic part for a squarefree denominator.
;
; The Rothstein-Trager residue polynomial R(z) for INT a/d (d squarefree in the monomial theta) may
; factor over Q into several irreducible pieces of mixed degree.  algresn.lisp handled the case where
; R is itself irreducible; this module factors R = prod_j P_j over Q (via factor-Q) and treats each
; factor on its own: a linear P_j contributes an ordinary logarithm c0 log(v_c) with rational residue
; c0, and an irreducible P_j of degree m >= 2 contributes the RootSum sum_{P_j(c)=0} c log(v_c) over
; the number field K_j = Q(x)[c]/(P_j).  In the generic situation each argument v_c = theta - r is
; linear in theta, and then the per-factor norm N_j = prod_sigma sigma(v_c) is exactly the
; characteristic polynomial of the element r in K_j -- obtained from the power sums tr(r^k) by
; Newton's identities run backward, so no resultant and no extension splitting are needed.  Because
; the Rothstein-Trager factors satisfy prod_j N_j = d, the per-factor derivatives sum over the common
; denominator d, and the whole logarithmic part is certified by checking that the sum of the field
; traces equals a/d (plus the exponential base-field correction).  As always the differentiation
; certificate is the gate: a residue polynomial that is not squarefree, or any case whose argument is
; not linear over a factor or whose trace identity fails, is left reported 'algebraic, so this can
; only tighten the verdicts already given.  Builds on algresn.lisp.

(import "cas/algresn.lisp")

(define (arf-app l x) (if (null? l) x (cons (car l) (arf-app (cdr l) x))))
(define (arf-monicq prim) (arf-mq prim (poly-lead prim)))
(define (arf-mq p lc) (if (null? p) '() (cons (rat-from-poly (list (/ (car p) lc))) (arf-mq (cdr p) lc))))
(define (arf-allsf? facs) (cond ((null? facs) #t) ((= (car (car facs)) 1) (arf-allsf? (cdr facs))) (else #f)))

; ----- characteristic polynomial of r in K_j = Q(x)[c]/(Pj) via power sums of the conjugates of r -----
(define (arf-nfpow R e k) (if (= k 0) (nf-const (rat-one)) (nf-mul R e (arf-nfpow R e (- k 1)))))
(define (arf-rpows ps R r k m acc) (if (> k m) (anr-rev acc '()) (arf-rpows ps R r (+ k 1) m (cons (nf-tr ps (arf-nfpow R r k)) acc))))
(define (arf-ek prs es k i acc)            ; e_k = (1/k) sum_{i=1}^k (-1)^{i-1} e_{k-i} pr_i
  (if (> i k) (rat-mul (rat-from-poly (list (/ 1 k))) acc)
      (arf-ek prs es k (+ i 1)
        (rat-add acc (rat-mul (rat-from-poly (list (if (= (remainder (- i 1) 2) 0) 1 -1))) (rat-mul (anr-nth es (- k i)) (anr-nth prs i)))))))
(define (arf-es prs m k es) (if (> k m) es (arf-es prs m (+ k 1) (arf-app es (list (arf-ek prs es k 1 (rat-zero)))))))
(define (arf-charcoeffs es m i)            ; coeff at theta^i = (-1)^{m-i} e_{m-i}
  (if (> i m) '()
      (cons (rat-mul (rat-from-poly (list (if (= (remainder (- m i) 2) 0) 1 -1))) (anr-nth es (- m i))) (arf-charcoeffs es m (+ i 1)))))
(define (arf-charpoly Pj Pq r m)
  (let ((ps (anr-powersums Pq m)))
    (rfpoly-norm (arf-charcoeffs (arf-es (arf-rpows ps Pj r 0 m '()) m 1 (list (rat-one))) m 0))))

; ----- per-factor trace contribution: returns (list TNUM N corr) | 'nonuniform -----
(define (arf-factor-trace a d Dd mono prim)
  (let ((Pj (arf-monicq prim)) (m (poly-deg prim)))
    (if (= m 1)
        (let ((c0 (rat-neg (car Pj))))
          (let ((vc (rfpoly-monic (rfpoly-gcd d (rfpoly-sub a (rfpoly-cscale c0 Dd))))))
            (list (rfpoly-cscale c0 (Drf vc mono)) vc
                  (if (anr-isexp? mono) (rat-mul c0 (rat-from-poly (list (rfpoly-deg vc)))) (rat-zero)))))
        (let ((vc (nfp-monic Pj (nfp-gcd Pj (rf->nfp d) (nfp-sub (rf->nfp a) (nfp-cscale Pj (nf-c) (rf->nfp Dd)))))))
          (if (= (nfp-deg vc) 1)
              (let ((r (nf-neg (car vc))) (Pq (trt-toQ Pj)))
                (let ((Nj (arf-charpoly Pj Pq r m)))
                  (let ((cofac (nfp-quot Pj (rf->nfp Nj) vc)))
                    (list (rfpoly-norm (nfp-trace-rf (anr-powersums Pq m) (nfp-mul Pj (nfp-cscale Pj (nf-c) (nfp-Dtheta vc mono)) cofac)))
                          Nj
                          (if (anr-isexp? mono) (rat-from-poly (list (anr-nth (anr-powersums Pq m) 1))) (rat-zero))))))
              'nonuniform)))))

(define (arf-loop a d Dd mono facs accT accC)
  (if (null? facs)
      (if (tr-equal? accT (list (rfpoly-add a (rfpoly-cscale accC d)) d)) (list 'rootsum-full accT accC) (list 'fail))
      (let ((ft (arf-factor-trace a d Dd mono (car (cdr (car facs))))))
        (if (equal? ft 'nonuniform) (list 'fail)
            (arf-loop a d Dd mono (cdr facs) (tr-add accT (list (car ft) (car (cdr ft)))) (rat-add accC (car (cdr (cdr ft)))))))))

; arf-rootsum a d mono -> (list 'rootsum-full trace-tr corr) | (list 'fail)
(define (arf-rootsum a d mono)
  (let ((Dd (Drf d mono)))
    (let ((mz (trt-monic (trt-buildR a d Dd))))
      (if (if (>= (rfpoly-deg mz) 1) (trt-allconst? mz) #f)
          (let ((facs (car (cdr (factor-Q (trt-toQ mz))))))
            (if (arf-allsf? facs) (arf-loop a d Dd mono facs (tr-zero) (rat-zero)) (list 'fail)))
          (list 'fail)))))

; ----- full proper-case integrator allowing reducible algebraic residues -----
;   (list 'ok g terms) | (list 'rootsum-full g trace-tr corr) | (list 'algebraic) | (list 'non-elementary)
(define (int-prim-rational-full A D mono)
  (let ((H (hermite A D mono)))
    (let ((g (tr-reduce (car H))) (as (car (cdr H))) (ds (car (cdr (cdr H)))))
      (if (rfpoly-zero? as) (list 'ok g '())
          (let ((rt (trt-logpart as ds mono)))
            (if (equal? (car rt) 'ok) (list 'ok g (car (cdr rt)))
                (if (equal? (car rt) 'algebraic)
                    (let ((ar (arf-rootsum as ds mono)))
                      (if (equal? (car ar) 'rootsum-full) (list 'rootsum-full g (car (cdr ar)) (car (cdr (cdr ar)))) (list 'algebraic)))
                    rt)))))))
(define (int-prim-rational-full-verify A D mono)
  (let ((r (int-prim-rational-full A D mono)))
    (cond ((equal? (car r) 'ok)
           (tr-equal? (trt-add-logderivs (tr-deriv (car (cdr r)) mono) (car (cdr (cdr r))) mono) (list A D)))
          ((equal? (car r) 'rootsum-full)
           (tr-equal? (tr-add (tr-deriv (car (cdr r)) mono) (car (cdr (cdr r))))
                      (list (rfpoly-add A (rfpoly-cscale (car (cdr (cdr (cdr r)))) D)) D)))
          (else #f))))
(define (int-prim-rational-full-elementary? A D mono)
  (let ((s (car (int-prim-rational-full A D mono)))) (if (equal? s 'ok) #t (equal? s 'rootsum-full))))
