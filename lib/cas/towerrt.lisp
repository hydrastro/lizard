; -*- lisp -*-
; lib/cas/towerrt.lisp -- Rothstein-Trager IN THE TOWER, completing the proper (fractional) case of
; integration over a primitive monomial theta = log x with x-dependent coefficients.
;
; tower.lisp reduces A/D by Hermite to a rational part plus a squarefree remainder a/d (d squarefree
; in theta), and resolves only the single-new-logarithm case, returning "partial" otherwise.  This
; module resolves the general logarithmic part.  By the primitive-case residue criterion, with D the
; tower derivation, INT a/d is elementary over Q(x)(theta) iff the polynomial
;     R(z) = Res_theta(d, a - z Dd)
; has constant roots, and then INT a/d = sum_i c_i log gcd_theta(d, a - c_i Dd), the c_i being those
; roots.  R(z) is built by computing the resultant over Q(x) (a fraction-free Euclidean recurrence)
; at z = 0,1,...,deg_theta d and interpolating in z over Q(x); making it monic in z, the integral is
; elementary exactly when every coefficient is a constant.  If so and the roots are rational, the
; logarithms are read off; constant-but-irrational roots are a (deferred) RootSum; a non-constant
; coefficient is a genuine non-elementary obstruction.  Combined with Hermite this gives a complete,
; certified integrator for rational functions of log x with rational residues -- e.g. it finds
; INT ((3/x+1) log x - (3x+1)) / ((log x)^2 - x^2) dx = 2 log(log x + x) + log(log x - x), which the
; single-logarithm reducer cannot.  Every answer is checked by differentiating in the tower.
; Builds on tower.lisp and rothstein.lisp.

(import "cas/tower.lisp")
(import "cas/rothstein.lisp")

; ---------- resultant over Q(x) of two rfpolys (in theta) ----------
(define (trt-ratpow r k) (if (= k 0) (rat-one) (rat-mul r (trt-ratpow r (- k 1)))))
(define (trt-even? n) (= (remainder n 2) 0))
(define (trt-res f g)
  (if (< (rfpoly-deg f) (rfpoly-deg g))
      (let ((s (trt-res g f))) (if (trt-even? (* (rfpoly-deg f) (rfpoly-deg g))) s (rat-neg s)))
      (if (rfpoly-zero? g) (rat-zero)
          (if (= (rfpoly-deg g) 0) (trt-ratpow (rfpoly-lead g) (rfpoly-deg f))
              (let ((r (rfpoly-rem f g)))
                (let ((p (rat-mul (trt-ratpow (rfpoly-lead g) (- (rfpoly-deg f) (rfpoly-deg r))) (trt-res g r))))
                  (if (trt-even? (* (rfpoly-deg f) (rfpoly-deg g))) p (rat-neg p))))))))

; ---------- Lagrange interpolation over Q(x): integer nodes, rat values -> rfpoly in z ----------
(define (trt-nth l i) (if (= i 0) (car l) (trt-nth (cdr l) (- i 1))))
(define (trt-len l) (if (null? l) 0 (+ 1 (trt-len (cdr l)))))
(define (trt-Zvar) (rfpoly-monomial (rat-one) 1))
(define (trt-num xs k i acc)
  (cond ((null? xs) acc) ((= k i) (trt-num (cdr xs) (+ k 1) i acc))
        (else (trt-num (cdr xs) (+ k 1) i (rfpoly-mul acc (rfpoly-sub (trt-Zvar) (rf-const (car xs))))))))
(define (trt-den xs k i xi acc)
  (cond ((null? xs) acc) ((= k i) (trt-den (cdr xs) (+ k 1) i xi acc))
        (else (trt-den (cdr xs) (+ k 1) i xi (rat-mul acc (rat-sub xi (car xs)))))))
(define (trt-lag-sum xs ys i n acc)
  (if (= i n) acc
      (trt-lag-sum xs ys (+ i 1) n
        (rfpoly-add acc (rfpoly-cscale (rat-div (trt-nth ys i) (trt-den xs 0 i (trt-nth xs i) (rat-one)))
                                       (trt-num xs 0 i (rf-const (rat-one))))))))
(define (trt-lag xs ys) (trt-lag-sum xs ys 0 (trt-len xs) '()))

; ---------- build R(z) for INT a/d ----------
(define (trt-intnodes i m) (if (> i m) '() (cons (rat-from-poly (list i)) (trt-intnodes (+ i 1) m))))
(define (trt-vals a d Dd i m) (if (> i m) '() (cons (trt-res d (rfpoly-sub a (rfpoly-cscale (rat-from-poly (list i)) Dd))) (trt-vals a d Dd (+ i 1) m))))
(define (trt-buildR a d Dd) (let ((m (rfpoly-deg d))) (trt-lag (trt-intnodes 0 m) (trt-vals a d Dd 0 m))))

; ---------- elementarity test and root extraction ----------
(define (trt-ratconst? r) (if (<= (poly-deg (car r)) 0) (<= (poly-deg (car (cdr r))) 0) #f))
(define (trt-monic Rz) (rfpoly-cscale (rat-inv (trt-nth Rz (rfpoly-deg Rz))) Rz))
(define (trt-allconst? P) (cond ((null? P) #t) ((trt-ratconst? (car P)) (trt-allconst? (cdr P))) (else #f)))
(define (trt-constval r) (if (null? (car r)) 0 (/ (car (car r)) (car (car (cdr r))))))
(define (trt-toQ P) (if (null? P) '() (cons (trt-constval (car P)) (trt-toQ (cdr P)))))
(define (trt-terms a d Dd roots)
  (if (null? roots) '()
      (cons (cons (car roots) (rfpoly-monic (rfpoly-gcd d (rfpoly-sub a (rfpoly-cscale (rat-from-poly (list (car roots))) Dd)))))
            (trt-terms a d Dd (cdr roots)))))

; INT a/d, d squarefree in theta -> (list 'ok terms) | (list 'algebraic) | (list 'non-elementary)
;   terms = list of (c . v): residue c (rational) and argument v (rfpoly in theta)
(define (trt-logpart a d mono)
  (let ((Dd (Drf d mono)))
    (let ((Rz (trt-buildR a d Dd)))
      (let ((mz (trt-monic Rz)))
        (if (trt-allconst? mz)
            (let ((roots (ros-rational-roots (trt-toQ mz))))
              (if (= (trt-len roots) (rfpoly-deg Rz)) (list 'ok (trt-terms a d Dd roots)) (list 'algebraic)))
            (list 'non-elementary))))))

; ---------- full primitive-case rational integrator: A/D proper, rational in theta=log x ----------
;   (list 'ok g-tr terms) | (list 'algebraic) | (list 'non-elementary)
(define (int-prim-rational A D mono)
  (let ((H (hermite A D mono)))
    (let ((g (tr-reduce (car H))) (as (car (cdr H))) (ds (car (cdr (cdr H)))))
      (if (rfpoly-zero? as) (list 'ok g '())
          (let ((rt (trt-logpart as ds mono)))
            (if (equal? (car rt) 'ok) (list 'ok g (car (cdr rt))) rt))))))

; ---------- certificate: D(g) + sum c Dv/v  =  A/D ----------
(define (trt-add-logderivs acc terms mono)
  (if (null? terms) acc
      (trt-add-logderivs
        (tr-add acc (list (rfpoly-cscale (rat-from-poly (list (car (car terms)))) (Drf (cdr (car terms)) mono)) (cdr (car terms))))
        (cdr terms) mono)))
(define (int-prim-rational-verify A D mono)
  (let ((r (int-prim-rational A D mono)))
    (if (equal? (car r) 'ok)
        (tr-equal? (trt-add-logderivs (tr-deriv (car (cdr r)) mono) (car (cdr (cdr r))) mono) (list A D))
        #f)))
(define (int-prim-rational-elementary? A D mono) (equal? (car (int-prim-rational A D mono)) 'ok))
