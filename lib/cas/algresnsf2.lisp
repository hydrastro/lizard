; -*- lisp -*-
; lib/cas/algresnsf2.lisp -- completes the single-extension logarithmic part: the RootSum for ANY
; residue polynomial, including a non-squarefree one with IRRATIONAL repeated residues.
;
; The Rothstein-Trager residue polynomial R(z) for INT a/d (d squarefree in theta) is factored over Q
; into distinct irreducible factors with multiplicities by factor-Q.  Each factor is a single
; conjugate class of residues; a factor of degree m occurring to multiplicity i contributes the
; RootSum sum_{P(c)=0} c log(v_c) with v_c = gcd_theta(d, a - c Dd) of degree i over the number field
; K = Q(x)[c]/(P).  A linear factor (rational residue) is handled directly in Q(x)[theta].  An
; irreducible factor of degree m >= 2 needs the per-factor norm N = prod_sigma sigma(v_c) of the
; degree-i argument; this module computes that norm as the determinant of the m-by-m
; multiplication-by-v_c matrix over Q(x)[theta] -- built by reducing v_c c^j modulo the monic P, so no
; division is required, and evaluated by cofactor expansion in pure rfpoly arithmetic.  The cofactor
; d/v_c then comes from one division in K[theta], and the per-factor derivative is the field trace of
; c (Dv_c)(d/v_c), the trace taken from the power sums of P.  Because the Rothstein-Trager factors
; satisfy prod N = d, all the per-factor logarithmic derivatives share the denominator d, and the
; whole part is certified by checking the sum of the traces equals a/d (with the exponential
; correction).  This one handler subsumes the squarefree rational, reducible and irreducible cases as
; well as the non-squarefree rational and algebraic cases.  The differentiation certificate gates
; every answer.  Builds on algresnsf.lisp.

(import "cas/algresnsf.lisp")

; ===== c-polynomials over rfpoly (coeffs are rfpolys, low->high in c); monic-divisor remainder =====
(define (cp-trim P) (cp-t (anr-rev P '())))
(define (cp-t R) (cond ((null? R) '()) ((rfpoly-zero? (car R)) (cp-t (cdr R))) (else (anr-rev R '()))))
(define (cp-deg P) (- (anr-len (cp-trim P)) 1))
(define (cp-lead P) (anr-nth (cp-trim P) (cp-deg P)))
(define (cp-add P Q) (cond ((null? P) Q) ((null? Q) P) (else (cons (rfpoly-add (car P) (car Q)) (cp-add (cdr P) (cdr Q))))))
(define (cp-neg P) (if (null? P) '() (cons (rfpoly-neg (car P)) (cp-neg (cdr P)))))
(define (cp-sub P Q) (cp-add P (cp-neg Q)))
(define (cp-cmul e P) (if (null? P) '() (cons (rfpoly-mul e (car P)) (cp-cmul e (cdr P)))))
(define (cp-shift P k) (if (= k 0) P (cons (rf-const (rat-zero)) (cp-shift P (- k 1)))))
(define (cp-mul P Q) (if (null? P) '() (cp-add (cp-cmul (car P) Q) (cp-shift (cp-mul (cdr P) Q) 1))))
(define (cp-rem A P)                          ; P monic in c (lead = 1): remainder, no division
  (let ((A2 (cp-trim A)) (P2 (cp-trim P)))
    (if (if (null? A2) #t (< (cp-deg A2) (cp-deg P2))) A2
        (cp-rem (cp-sub A2 (cp-shift (cp-cmul (cp-lead A2) P2) (- (cp-deg A2) (cp-deg P2)))) P))))
(define (P->cp P) (if (null? P) '() (cons (rf-const (car P)) (P->cp (cdr P)))))

; ===== determinant over rfpoly by cofactor expansion =====
(define (mm-del row j) (if (= j 0) (cdr row) (cons (car row) (mm-del (cdr row) (- j 1)))))
(define (mm-rows rows j) (if (null? rows) '() (cons (mm-del (car rows) j) (mm-rows (cdr rows) j))))
(define (mat-minor M j) (mm-rows (cdr M) j))
(define (mat-det M) (if (null? (cdr M)) (car (car M)) (mat-det-row (car M) M 0 (rf-const (rat-zero)))))
(define (mat-det-row toprow M j acc)
  (if (null? toprow) acc
      (mat-det-row (cdr toprow) M (+ j 1)
        (rfpoly-add acc (rfpoly-cscale (rat-from-poly (list (if (= (remainder j 2) 0) 1 -1)))
                                       (rfpoly-mul (car toprow) (mat-det (mat-minor M j))))))))

; ===== field norm of V in Q[c]/(P) (P monic) via the multiplication matrix determinant =====
(define (cp-pad P m) (if (= m 0) '() (cons (if (null? P) (rf-const (rat-zero)) (car P)) (cp-pad (if (null? P) '() (cdr P)) (- m 1)))))
(define (nf-mulcol V Pcp j m) (cp-pad (cp-rem (cp-mul V (cp-shift (list (rf-const (rat-one))) j)) Pcp) m))
(define (nf-mulmat V Pcp j m) (if (= j m) '() (cons (nf-mulcol V Pcp j m) (nf-mulmat V Pcp (+ j 1) m))))
(define (col0 M) (if (null? M) '() (cons (car (car M)) (col0 (cdr M)))))
(define (cdrs M) (if (null? M) '() (cons (cdr (car M)) (cdrs (cdr M)))))
(define (transpose M) (if (null? (car M)) '() (cons (col0 M) (transpose (cdrs M)))))
(define (nf-norm V Pcp m) (mat-det (transpose (nf-mulmat V Pcp 0 m))))

; reindex v_c (nfp: theta-poly with nf coeffs) to V (c-poly with rfpoly coeffs): V[j] coeff theta^k = (vc[k])[j]
(define (nsf2-coeff-j e j) (if (< j (anr-len e)) (anr-nth e j) (rat-zero)))
(define (nsf2-Vj vc j) (if (null? vc) '() (cons (nsf2-coeff-j (car vc) j) (nsf2-Vj (cdr vc) j))))
(define (nsf2-reindex vc m j) (if (= j m) '() (cons (rfpoly-norm (nsf2-Vj vc j)) (nsf2-reindex vc m (+ j 1)))))

; ===== per-factor trace: (list TNUM N corr) =====
(define (nsf2-rat-factor a d Dd mono prim)
  (let ((Pj (arf-monicq prim)))
    (let ((c0 (rat-neg (car Pj))))
      (let ((vc (rfpoly-monic (rfpoly-gcd d (rfpoly-sub a (rfpoly-cscale c0 Dd))))))
        (list (rfpoly-cscale c0 (Drf vc mono)) vc
              (if (anr-isexp? mono) (rat-mul c0 (rat-from-poly (list (rfpoly-deg vc)))) (rat-zero)))))))
(define (nsf2-alg-factor a d Dd mono prim)
  (let ((Pj (arf-monicq prim)) (m (poly-deg prim)))
    (let ((vc (nfp-monic Pj (nfp-gcd Pj (rf->nfp d) (nfp-sub (rf->nfp a) (nfp-cscale Pj (nf-c) (rf->nfp Dd)))))))
      (let ((N (nf-norm (nsf2-reindex vc m 0) (P->cp Pj) m)) (ps (anr-powersums (trt-toQ Pj) m)))
        (let ((cofac (nfp-quot Pj (rf->nfp N) vc)))
          (list (rfpoly-norm (nfp-trace-rf ps (nfp-mul Pj (nfp-cscale Pj (nf-c) (nfp-Dtheta vc mono)) cofac)))
                N
                (if (anr-isexp? mono) (rat-from-poly (list (* (nfp-deg vc) (anr-nth ps 1)))) (rat-zero))))))))
(define (nsf2-factor a d Dd mono prim) (if (= (poly-deg prim) 1) (nsf2-rat-factor a d Dd mono prim) (nsf2-alg-factor a d Dd mono prim)))

(define (nsf2-loop a d Dd mono facs accT accC)
  (if (null? facs) (list accT accC)
      (let ((ft (nsf2-factor a d Dd mono (car (cdr (car facs))))))
        (nsf2-loop a d Dd mono (cdr facs) (tr-add accT (list (car ft) (car (cdr ft)))) (rat-add accC (car (cdr (cdr ft))))))))

; nsf2-rootsum a d mono -> (list 'rootsum-c trace-tr corr) | (list 'fail)  -- ANY residue polynomial
(define (nsf2-rootsum a d mono)
  (let ((Dd (Drf d mono)))
    (let ((mz (trt-monic (trt-buildR a d Dd))))
      (if (if (>= (rfpoly-deg mz) 1) (trt-allconst? mz) #f)
          (let ((rr (nsf2-loop a d Dd mono (car (cdr (factor-Q (trt-toQ mz)))) (tr-zero) (rat-zero))))
            (if (tr-equal? (car rr) (list (rfpoly-add a (rfpoly-cscale (car (cdr rr)) d)) d))
                (list 'rootsum-c (car rr) (car (cdr rr))) (list 'fail)))
          (list 'fail)))))

; ===== complete proper-case integrator: rational fast path, else the general RootSum =====
(define (int-prim-rational-complete A D mono)
  (let ((H (hermite A D mono)))
    (let ((g (tr-reduce (car H))) (as (car (cdr H))) (ds (car (cdr (cdr H)))))
      (if (rfpoly-zero? as) (list 'ok g '())
          (let ((rt (trt-logpart as ds mono)))
            (if (equal? (car rt) 'ok) (list 'ok g (car (cdr rt)))
                (if (equal? (car rt) 'algebraic)
                    (let ((ns (nsf2-rootsum as ds mono)))
                      (if (equal? (car ns) 'rootsum-c) (list 'rootsum-c g (car (cdr ns)) (car (cdr (cdr ns)))) (list 'algebraic)))
                    rt)))))))
(define (int-prim-rational-complete-verify A D mono)
  (let ((r (int-prim-rational-complete A D mono)))
    (cond ((equal? (car r) 'ok) (tr-equal? (trt-add-logderivs (tr-deriv (car (cdr r)) mono) (car (cdr (cdr r))) mono) (list A D)))
          ((equal? (car r) 'rootsum-c)
           (tr-equal? (tr-add (tr-deriv (car (cdr r)) mono) (car (cdr (cdr r))))
                      (list (rfpoly-add A (rfpoly-cscale (car (cdr (cdr (cdr r)))) D)) D)))
          (else #f))))
(define (int-prim-rational-complete-elementary? A D mono)
  (let ((s (car (int-prim-rational-complete A D mono)))) (if (equal? s 'ok) #t (equal? s 'rootsum-c))))
