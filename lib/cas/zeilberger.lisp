; -*- lisp -*-
; lib/cas/zeilberger.lisp — Zeilberger's algorithm (creative telescoping):
; find a polynomial recurrence satisfied by a definite hypergeometric sum.
;
; For S(n) = SUM_k F(n,k) with F proper hypergeometric, find polynomials
; a_0(n),...,a_J(n) (not all zero) and a rational certificate R(n,k) with
;   SUM_{j=0}^{J} a_j(n) F(n+j,k) = G(n,k+1) - G(n,k),    G = R F.
; Summing over k telescopes the right side to 0, giving the recurrence
;   SUM_j a_j(n) S(n+j) = 0.
; Dividing the telescoping identity by F(n,k) makes it rational in (n,k): each
; F(n+j,k)/F(n,k) is a product of shifts of r1=F(n+1,k)/F(n,k), and the right side
; uses r2=F(n,k+1)/F(n,k).  Clearing denominators gives a bivariate polynomial
; identity, HOMOGENEOUS and LINEAR in the unknowns (coefficients of the a_j and of
; R=P/D).  A nontrivial solution with some a_j nonzero is a nullspace vector of the
; resulting Q-matrix -- found with the exact kernel routine from normalform.lisp --
; and is re-checked against the exact bivariate identity, so a spurious recurrence
; cannot pass.
;
; The shift denominators satisfy den_0 | den_1 | ... | den_J, so their LCM is den_J;
; using den_J as the common denominator (via tail products) keeps the polynomials
; small enough for order >= 2.  Reuses the Q[n][k] layer (wz.lisp) and the kernel.

(import "cas/wz.lisp")
(import "cas/normalform.lisp")

(define (bp-one) (list (list 1)))
(define (shiftn-i p i) (if (= i 0) p (shiftn-i (bp-shiftn p) (- i 1))))
(define (zn j J a1n acc) (if (>= j J) (reverse acc) (zn (+ j 1) J a1n (cons (bp-mul (car acc) (shiftn-i a1n j)) acc))))
(define (zb-nums a1n J) (zn 0 J a1n (list (bp-one))))
(define (tail-prod a1d j J) (if (>= j J) (bp-one) (bp-mul (shiftn-i a1d j) (tail-prod a1d (+ j 1) J))))

(define (acol-j numj tpj D Dp a2d da)
  (let ((Cj (bp-mul (bp-mul (bp-mul (bp-mul numj tpj) D) Dp) a2d)))
    (map (lambda (p) (bp-scalen (poly-monomial 1 p) Cj)) (iota 0 da))))
(define (acols-go nums a1d D Dp a2d j J da)
  (if (> j J) '()
    (append (acol-j (nth nums j) (tail-prod a1d j J) D Dp a2d da) (acols-go nums a1d D Dp a2d (+ j 1) J da))))
(define (a-columns nums a1d D Dp a2d J da) (acols-go nums a1d D Dp a2d 0 J da))

(define (pcol Q1 Q2 p q) (let ((e (bp-monomial p q))) (bp-add (bp-neg (bp-mul (bp-shiftk e 1) Q1)) (bp-mul e Q2))))
(define (p-columns Q1 Q2 dn dk) (map (lambda (pq) (pcol Q1 Q2 (car pq) (cdr pq))) (pairs 0 dn 0 dk)))

(define (chunks xs sz) (if (null? xs) '() (cons (take-n xs sz) (chunks (drop-n xs sz) sz))))
(define (build-as apart da) (map poly-norm (chunks apart (+ da 1))))
(define (a-nonzero? x asize) (not (poly-zero? (poly-norm (take-n x asize)))))

(define (sum-aj as nums a1d D Dp a2d J j)
  (if (null? as) '()
    (bp-add (bp-mul (bp-mul (bp-mul (bp-mul (bp-scalen (car as) (nth nums j)) (tail-prod a1d j J)) D) Dp) a2d)
            (sum-aj (cdr as) nums a1d D Dp a2d J (+ j 1)))))
(define (zb-residual a1n a1d a2n a2d as P D)
  (let ((J (- (length as) 1)))
    (let ((nums (zb-nums a1n J)) (denJ (tail-prod a1d 0 J)) (Dp (bp-shiftk D 1)))
      (bp-add (sum-aj as nums a1d D Dp a2d J 0)
              (bp-add (bp-neg (bp-mul (bp-mul (bp-shiftk P 1) a2n) (bp-mul denJ D)))
                      (bp-mul P (bp-mul denJ (bp-mul Dp a2d))))))))
(define (zb-verify a1n a1d a2n a2d cand)
  (bp-zero? (zb-residual a1n a1d a2n a2d (car (cdr cand)) (car (cdr (cdr cand))) (car (cdr (cdr (cdr cand)))))))

(define (zb-pick ns asize da dn dk D a1n a1d a2n a2d)
  (cond ((null? ns) 'none)
        ((a-nonzero? (car ns) asize)
          (let ((cand (list 'rec (build-as (take-n (car ns) asize) da) (bp-build (pairs 0 dn 0 dk) (drop-n (car ns) asize)) D)))
            (if (zb-verify a1n a1d a2n a2d cand) cand (zb-pick (cdr ns) asize da dn dk D a1n a1d a2n a2d))))
        (else (zb-pick (cdr ns) asize da dn dk D a1n a1d a2n a2d))))
(define (zb-try a1n a1d a2n a2d D J da dn dk)
  (let ((nums (zb-nums a1n J)) (denJ (tail-prod a1d 0 J)) (Dp (bp-shiftk D 1)))
    (let ((cols (append (a-columns nums a1d D Dp a2d J da)
                        (p-columns (bp-mul (bp-mul a2n denJ) D) (bp-mul (bp-mul denJ Dp) a2d) dn dk))))
      (let ((mons (pairs 0 (maxn cols -1) 0 (maxk cols -1))))
        (let ((M (map (lambda (ab) (map (lambda (c) (bp-coeff c (car ab) (cdr ab))) cols)) mons)))
          (zb-pick (mat-nullspace M) (* (+ J 1) (+ da 1)) da dn dk D a1n a1d a2n a2d))))))

(define (zb-order a1n a1d a2n a2d J)
  (zb-plan a1n a1d a2n a2d J
    (list (list a1d 0 0 1) (list a1d 1 1 2) (list a1d 1 1 3) (list a1d 1 2 3)
          (list (bp-mul a1d a2d) 1 1 2))))
(define (zb-plan a1n a1d a2n a2d J plans)
  (if (null? plans) 'none
    (let ((p (car plans)))
      (let ((r (zb-try a1n a1d a2n a2d (car p) J (car (cdr p)) (car (cdr (cdr p))) (car (cdr (cdr (cdr p)))))))
        (if (equal? r 'none) (zb-plan a1n a1d a2n a2d J (cdr plans)) r)))))
(define (zb-search a1n a1d a2n a2d) (zb-orders a1n a1d a2n a2d 1 3))
(define (zb-orders a1n a1d a2n a2d J Jmax)
  (if (> J Jmax) 'not-found
    (let ((r (zb-order a1n a1d a2n a2d J)))
      (if (equal? r 'none) (zb-orders a1n a1d a2n a2d (+ J 1) Jmax) r))))

(define (zb-terms as j)
  (if (null? as) ""
    (let ((rest (zb-terms (cdr as) (+ j 1))))
      (let ((coef (string-append "[" (poly->string (car as) "n") "]*S(n" (if (= j 0) "" (string-append "+" (number->string j))) ")")))
        (if (poly-zero? (car as)) rest (if (equal? rest "") coef (string-append coef " + " rest)))))))
(define (zb-recurrence->string cand)
  (if (not (pair? cand)) "no recurrence found within search bounds" (string-append (zb-terms (car (cdr cand)) 0) " = 0")))
(define (zb-certificate->string cand)
  (if (not (pair? cand)) ""
    (string-append "R(n,k) = [ " (bp->string (car (cdr (cdr cand)))) " ] / [ " (bp->string (car (cdr (cdr (cdr cand))))) " ]")))
