; -*- lisp -*-
; lib/cas/algresnsf.lisp -- the non-squarefree residue-polynomial case (rational residues), the last
; piece of the single-extension logarithmic part for the common situation.
;
; The Rothstein-Trager residue polynomial R(z) for INT a/d (d squarefree in theta) is squarefree for
; a generic integrand, but it can have repeated factors when several roots of d share the same
; residue.  Writing the squarefree factorization R = prod_i R_i^i, the residues that are roots of R_i
; come with an argument v_c = gcd_theta(d, a - c Dd) of degree i in theta, rather than the linear
; argument of the squarefree case.  This module handles the common branch in which all residues are
; rational: it squarefree-factors R with yun-square-free, and for each rational residue c0 (a root of
; some R_i) forms the degree-i argument v_c entirely in Q(x)[theta] -- no number field is needed --
; contributing c0 log(v_c).  Because the Rothstein-Trager factors multiply back to d, the logarithmic
; derivatives share the common denominator d and the whole part is certified by checking that the sum
; of c0 (Dv_c)/v_c over all rational residues equals a/d (with the exponential base-field correction).
; The entry point int-prim-rational-nsf layers this behind the squarefree handlers: it tries the
; rational base case, then the reducible RootSum closure, then this non-squarefree rational case, and
; otherwise reports 'algebraic -- so a non-squarefree residue polynomial with an irrational repeated
; residue (which would need the number-field norm of a higher-degree argument) is still deferred, not
; mishandled.  The differentiation certificate is the gate throughout.  Builds on algresfull.lisp.

(import "cas/algresfull.lisp")

(define (nsf-all-rational? sf)
  (cond ((null? sf) #t)
        ((= (anr-len (ros-rational-roots (car (cdr (car sf))))) (poly-deg (car (cdr (car sf))))) (nsf-all-rational? (cdr sf)))
        (else #f)))

(define (nsf-loop-roots a d Dd mono roots accT accC)
  (if (null? roots) (list accT accC)
      (let ((c0 (rat-from-poly (list (car roots)))))
        (let ((vc (rfpoly-monic (rfpoly-gcd d (rfpoly-sub a (rfpoly-cscale c0 Dd))))))
          (nsf-loop-roots a d Dd mono (cdr roots)
            (tr-add accT (list (rfpoly-cscale c0 (Drf vc mono)) vc))
            (rat-add accC (if (anr-isexp? mono) (rat-mul c0 (rat-from-poly (list (rfpoly-deg vc)))) (rat-zero))))))))
(define (nsf-loop-facs a d Dd mono sf accT accC)
  (if (null? sf) (list accT accC)
      (let ((rr (nsf-loop-roots a d Dd mono (ros-rational-roots (car (cdr (car sf)))) accT accC)))
        (nsf-loop-facs a d Dd mono (cdr sf) (car rr) (car (cdr rr))))))

; nsf-rootsum a d mono -> (list 'rootsum-nsf trace-tr corr) | (list 'fail)
(define (nsf-rootsum a d mono)
  (let ((Dd (Drf d mono)))
    (let ((mz (trt-monic (trt-buildR a d Dd))))
      (if (if (>= (rfpoly-deg mz) 1) (trt-allconst? mz) #f)
          (let ((sf (yun-square-free (trt-toQ mz))))
            (if (nsf-all-rational? sf)
                (let ((rr (nsf-loop-facs a d Dd mono sf (tr-zero) (rat-zero))))
                  (if (tr-equal? (car rr) (list (rfpoly-add a (rfpoly-cscale (car (cdr rr)) d)) d))
                      (list 'rootsum-nsf (car rr) (car (cdr rr))) (list 'fail)))
                (list 'fail)))
          (list 'fail)))))

; ----- layered proper-case integrator: rational -> reducible RootSum -> non-squarefree rational -----
;   (list 'ok g terms) | (list 'rootsum-full g tr corr) | (list 'rootsum-nsf g tr corr) | (list 'algebraic) | (list 'non-elementary)
(define (int-prim-rational-nsf A D mono)
  (let ((H (hermite A D mono)))
    (let ((g (tr-reduce (car H))) (as (car (cdr H))) (ds (car (cdr (cdr H)))))
      (if (rfpoly-zero? as) (list 'ok g '())
          (let ((rt (trt-logpart as ds mono)))
            (if (equal? (car rt) 'ok) (list 'ok g (car (cdr rt)))
                (if (equal? (car rt) 'algebraic)
                    (let ((ar (arf-rootsum as ds mono)))
                      (if (equal? (car ar) 'rootsum-full) (list 'rootsum-full g (car (cdr ar)) (car (cdr (cdr ar))))
                          (let ((ns (nsf-rootsum as ds mono)))
                            (if (equal? (car ns) 'rootsum-nsf) (list 'rootsum-nsf g (car (cdr ns)) (car (cdr (cdr ns)))) (list 'algebraic)))))
                    rt)))))))
(define (nsf-corr-ok? r A D mono)            ; certificate for the two RootSum branches
  (tr-equal? (tr-add (tr-deriv (car (cdr r)) mono) (car (cdr (cdr r)))) (list (rfpoly-add A (rfpoly-cscale (car (cdr (cdr (cdr r)))) D)) D)))
(define (int-prim-rational-nsf-verify A D mono)
  (let ((r (int-prim-rational-nsf A D mono)))
    (cond ((equal? (car r) 'ok) (tr-equal? (trt-add-logderivs (tr-deriv (car (cdr r)) mono) (car (cdr (cdr r))) mono) (list A D)))
          ((equal? (car r) 'rootsum-full) (nsf-corr-ok? r A D mono))
          ((equal? (car r) 'rootsum-nsf) (nsf-corr-ok? r A D mono))
          (else #f))))
(define (int-prim-rational-nsf-elementary? A D mono)
  (let ((s (car (int-prim-rational-nsf A D mono))))
    (cond ((equal? s 'ok) #t) ((equal? s 'rootsum-full) #t) ((equal? s 'rootsum-nsf) #t) (else #f))))
