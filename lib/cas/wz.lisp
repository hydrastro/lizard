; -*- lisp -*-
; lib/cas/wz.lisp — Wilf-Zeilberger creative telescoping: machine-checked proofs
; of binomial (hypergeometric) identities.
;
; To prove  SUM_k summand(n,k) = rhs(n),  set F(n,k) = summand/rhs so the claim is
; SUM_k F(n,k) = 1.  A WZ certificate is a rational R(n,k) with
;     F(n+1,k) - F(n,k) = G(n,k+1) - G(n,k),   G = R F.
; Summing over k telescopes the right side to 0, so SUM_k F(n,k) is constant in n;
; comparing one value finishes the proof.  Dividing the identity by F(n,k) gives a
; purely rational identity in (n,k):
;     r1(n,k) - 1 = R(n,k+1) r2(n,k) - R(n,k),
; where r1 = F(n+1,k)/F(n,k) and r2 = F(n,k+1)/F(n,k).  THIS is the certificate and
; it is checked EXACTLY with bivariate polynomial arithmetic -- so a wrong R cannot
; pass.  Discovery posits R = P/D with P an unknown bivariate polynomial of bounded
; degree; clearing denominators makes the identity LINEAR in P's coefficients, an
; exact Q-linear system solved by the Gauss-Jordan solver.  Any certificate found
; is then re-verified; if none is found within the search bounds we report so.
;
; Bivariate polys in (n,k): a poly in k whose coefficients are Q[n] polynomials
; (poly.lisp coefficient lists).  Top-level helpers only; builds on gosper.lisp.

(import "cas/gosper.lisp")

; ===================== bivariate polynomial arithmetic Q[n][k] =====================
; p = (c0 c1 ... cd), each cj a Q[n]-poly (low-to-high), meaning sum_j cj(n) k^j.
(define (drop-zp r) (cond ((null? r) '()) ((poly-zero? (car r)) (drop-zp (cdr r))) (else r)))
(define (bp-norm p) (reverse (drop-zp (reverse p))))
(define (bp-ck p j) (if (< j (length p)) (nth p j) '()))
(define (bp-add2 a b) (cond ((null? a) b) ((null? b) a) (else (cons (poly-add (car a) (car b)) (bp-add2 (cdr a) (cdr b))))))
(define (bp-add a b) (bp-norm (bp-add2 a b)))
(define (bp-neg p) (map (lambda (c) (poly-neg c)) p))
(define (bp-sub a b) (bp-add a (bp-neg b)))
(define (bp-scalen s p) (bp-norm (map (lambda (c) (poly-mul s c)) p)))   ; * Q[n]-poly
(define (bp-scaleq q p) (if (= q 0) '() (map (lambda (c) (poly-scale q c)) p)))
(define (bp-shiftk-by p i) (if (= i 0) p (cons '() (bp-shiftk-by p (- i 1)))))   ; * k^i
(define (bp-mul-i a b i acc) (if (null? a) acc (bp-mul-i (cdr a) b (+ i 1) (bp-add acc (bp-shiftk-by (bp-scalen (car a) b) i)))))
(define (bp-mul a b) (bp-mul-i a b 0 '()))
(define (bp-equal? a b) (equal? (bp-norm a) (bp-norm b)))
(define (bp-zero? p) (null? (bp-norm p)))
(define (bp-shiftn p) (map (lambda (c) (poly-shift c 1)) p))                ; n -> n+1
(define (bpsk rc kc acc) (if (null? rc) acc (bpsk (cdr rc) kc (bp-add (bp-mul acc kc) (list (car rc))))))
(define (bp-shiftk p c) (bpsk (reverse (bp-norm p)) (list (list c) (list 1)) '()))  ; k -> k+c
(define (bp-coeff p a b) (poly-coeff (bp-ck p b) a))                        ; coeff of n^a k^b
(define (bp-degk p) (- (length (bp-norm p)) 1))
(define (bp-dn p m) (if (null? p) m (bp-dn (cdr p) (max m (poly-deg (car p))))))
(define (bp-degn p) (bp-dn (bp-norm p) -1))
; constructors
(define (bp-monomial a b) (bp-shiftk-by (list (poly-monomial 1 a)) b))      ; n^a k^b
(define (bp-from-npoly pn) (if (poly-zero? pn) '() (list pn)))
(define (bp-eval-n p n0) (poly-rationalize (map (lambda (c) (poly-eval c n0)) p)))  ; -> Q[k] (rationals)

; ===================== WZ certificate search =====================
(define (prow i j jmax) (if (> j jmax) '() (cons (cons i j) (prow i (+ j 1) jmax))))
(define (pairs i imax j jmax) (if (> i imax) '() (append (prow i j jmax) (pairs (+ i 1) imax j jmax))))
(define (maxn ps m) (if (null? ps) m (maxn (cdr ps) (max m (bp-degn (car ps))))))
(define (maxk ps m) (if (null? ps) m (maxk (cdr ps) (max m (bp-degk (car ps))))))
(define (wz-col i j M1 M2) (let ((e (bp-monomial i j))) (bp-sub (bp-mul (bp-shiftk e 1) M1) (bp-mul e M2))))
(define (bp-build idx sol) (if (null? idx) '() (bp-add (bp-scaleq (car sol) (bp-monomial (car (car idx)) (cdr (car idx)))) (bp-build (cdr idx) (cdr sol)))))

; given r1 = a1n/a1d, r2 = a2n/a2d (bivariate), denominator D, and degree bounds
; dn (in n) and dk (in k) for the numerator P, try to find R = P/D.
(define (wz-try a1n a1d a2n a2d D dn dk)
  (let ((M1 (bp-mul (bp-mul a2n D) a1d))
        (M2 (bp-mul (bp-mul (bp-shiftk D 1) a2d) a1d))
        (Kc (bp-mul (bp-mul (bp-mul (bp-sub a1n a1d) (bp-shiftk D 1)) a2d) D)))
    (let ((idx (pairs 0 dn 0 dk)))
      (let ((cols (map (lambda (ij) (wz-col (car ij) (cdr ij) M1 M2)) idx)))
        (let ((amax (maxn (cons Kc cols) -1)) (bmax (maxk (cons Kc cols) -1)))
          (let ((mons (pairs 0 amax 0 bmax)))
            (let ((aug (map (lambda (ab) (append (map (lambda (col) (bp-coeff col (car ab) (cdr ab))) cols) (list (bp-coeff Kc (car ab) (cdr ab))))) mons)))
              (let ((sol (lin-solve aug (length idx))))
                (if (equal? sol 'none) 'none
                  (let ((P (bp-build idx sol)))
                    (if (bp-zero? (bp-sub (bp-add (bp-mul (bp-shiftk P 1) M1) (bp-neg (bp-mul P M2))) Kc))
                        (list 'ok P D) 'none)))))))))))

; search over a few (D, degrees); D candidates built from the ratio denominators.
(define (wz-search a1n a1d a2n a2d)
  (wz-plan a1n a1d a2n a2d
    (list (list a1d 0 1) (list a1d 1 1) (list a1d 1 2) (list a1d 1 3)
          (list a1d 2 3) (list (bp-mul a1d a2d) 1 2))))
(define (wz-plan a1n a1d a2n a2d plans)
  (if (null? plans) 'not-found
    (let ((p (car plans)))
      (let ((r (wz-try a1n a1d a2n a2d (car p) (car (cdr p)) (car (cdr (cdr p))))))
        (if (equal? r 'none) (wz-plan a1n a1d a2n a2d (cdr plans)) r)))))

; independent verification of a returned certificate (recompute the rational identity)
(define (wz-verify a1n a1d a2n a2d R)
  (let ((P (car (cdr R))) (D (car (cdr (cdr R)))))
    (let ((M1 (bp-mul (bp-mul a2n D) a1d))
          (M2 (bp-mul (bp-mul (bp-shiftk D 1) a2d) a1d))
          (Kc (bp-mul (bp-mul (bp-mul (bp-sub a1n a1d) (bp-shiftk D 1)) a2d) D)))
      (bp-zero? (bp-sub (bp-add (bp-mul (bp-shiftk P 1) M1) (bp-neg (bp-mul P M2))) Kc)))))

(define (wz-certificate->string R)
  (if (not (pair? R)) "no certificate found within search bounds"
    (string-append "R(n,k) = [ " (bp->string (car (cdr R))) " ] / [ " (bp->string (car (cdr (cdr R))) ) " ]")))

; pretty-print a bivariate poly (sum over k-powers of (Q[n]-poly) k^j)
(define (bp->string p) (let ((q (bp-norm p))) (if (null? q) "0" (bps q 0 ""))))
(define (bps cs j acc)
  (if (null? cs) (if (equal? acc "") "0" acc)
    (let ((term (bp-term (car cs) j)))
      (bps (cdr cs) (+ j 1) (if (equal? term "") acc (if (equal? acc "") term (string-append acc " + " term)))))))
(define (bp-term cn j)
  (if (poly-zero? cn) ""
    (let ((cs (string-append "(" (poly->string cn "n") ")")))
      (cond ((= j 0) cs) ((= j 1) (string-append cs "*k")) (else (string-append cs "*k^" (number->string j)))))))
