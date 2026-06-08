; -*- lisp -*-
; lib/cas/rischtoweralgn.lisp -- GENERAL-DEGREE algebraic extensions theta^n = a (n >= 2) for the recursive
; tower: the general-n multiplication with reduction theta^n -> a, completing the algebraic level beyond the
; quadratic case (docs/TRAGER_ROADMAP.md, the summit, "general algebraic extensions beyond quadratic").
;
; The recursive derivation (te-deriv-alg in rischtowern.lisp) and the integrator's algebraic case
; (te-crde-alg in rischcrde.lisp) are already general in n: both use the diagonal rate w = a'/(n a) and bound
; theta-degree by n-1, so e.g. cube-root integrals (INT (1/3) x^{-2/3} = x^{1/3}) are decided directly.  What this
; module adds is the general-n MULTIPLICATION: an element c_0 + c_1 theta + ... + c_{n-1} theta^{n-1} times
; another has raw degree up to 2n-2, and the powers theta^{n+r} reduce to a theta^r (r = 0 .. n-2); folding those
; down gives the product back in degree < n.  This is what the certificate and any product-forming step need at
; higher algebraic degree.
;
; Public:
;   tean-mul tower h n x y  -> product of two degree-<n algebraic elements, reduced modulo theta^n = a
;   tean-pow tower h n x k  -> x^k reduced
;   tean-reduce tower h n a p -> reduce a raw coefficient list p (any length) modulo theta^n = a to degree < n
;
; Verified: for theta = x^{1/3} (n=3, a=x): theta * theta^2 = x (degree 0); theta^2 * theta^2 = x theta
; (degree 1); the reduction folds higher powers correctly; consistency with te-deriv-alg via D(theta^3) = D(a).
;
; Builds on rischtowern.lisp (recursive element algebra + derivation) and tower.lisp / poly.lisp.

(import "cas/rischtowern.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (tn2-len l) (if (null? l) 0 (+ 1 (tn2-len (cdr l)))))
(define (tn2-nth l k) (if (= k 0) (car l) (tn2-nth (cdr l) (- k 1))))
(define (tn2-coeff h e k) (if (if (< k 0) #t (>= k (tn2-len e))) (te-zero (- h 1)) (tn2-nth e k)))

; ----- raw (unreduced) polynomial multiply in theta at height h: coefficients are height-(h-1) elements -----
(define (tean-rawmul tower h x y) (tean-rm-go tower h x y 0 (+ (tn2-len x) (tn2-len y))))
(define (tean-rm-go tower h x y k top)
  (if (>= k (- top 1)) (quote ()) (cons (tean-conv tower h x y k) (tean-rm-go tower h x y (+ k 1) top))))
(define (tean-conv tower h x y k) (tean-conv-go tower h x y k 0))
(define (tean-conv-go tower h x y k i)
  (if (> i k) (te-zero (- h 1))
      (te-add tower (- h 1) (te-mul tower (- h 1) (tn2-coeff h x i) (tn2-coeff h y (- k i))) (tean-conv-go tower h x y k (+ i 1)))))

; ----- reduce a raw coefficient list p (length possibly > n) modulo theta^n = a, down to degree < n.
; Repeatedly fold the top coefficient p_m (m >= n) into p_{m-n} via += a * p_m, highest first. -----
(define (tean-reduce tower h n a p) (tean-red-loop tower h n a p))
(define (tean-red-loop tower h n a p)
  (if (<= (tn2-len p) n) (tean-pad tower h n p)
      (tean-red-loop tower h n a (tean-foldtop tower h n a p))))
; fold the single highest coefficient (index L-1, where L = len p, L-1 >= n) into index L-1-n
(define (tean-foldtop tower h n a p) (tean-ft tower h n a p (- (tn2-len p) 1)))
(define (tean-ft tower h n a p m) (tean-setadd tower h (tean-droplast p) (- m n) (te-mul tower (- h 1) a (tn2-coeff h p m))))
(define (tean-droplast p) (tean-dl p (- (tn2-len p) 1)))
(define (tean-dl p k) (if (= k 0) (quote ()) (cons (car p) (tean-dl (cdr p) (- k 1)))))
; add value v into position i of list l (extending if needed)
(define (tean-setadd tower h l i v) (tean-sa-go tower h l i v 0))
(define (tean-sa-go tower h l i v j)
  (cond ((null? l) (if (= j i) (list v) (cons (te-zero (- h 1)) (tean-sa-go tower h (quote ()) i v (+ j 1)))))
        ((= j i) (cons (te-add tower (- h 1) (car l) v) (cdr l)))
        (else (cons (car l) (tean-sa-go tower h (cdr l) i v (+ j 1))))))
; pad/truncate to exactly length n
(define (tean-pad tower h n p) (tean-pad-go tower h n p 0))
(define (tean-pad-go tower h n p j) (if (>= j n) (quote ()) (cons (tn2-coeff h p j) (tean-pad-go tower h n p (+ j 1)))))

; ----- reduced multiplication and power -----
(define (tean-mul tower h n x y) (tean-reduce tower h n (tean-a-of tower h) (tean-rawmul tower h x y)))
(define (tean-a-of tower h) (car (cdr (cdr (te-level tower h)))))
(define (tean-pow tower h n x k) (if (= k 0) (tean-one tower h n) (tean-mul tower h n x (tean-pow tower h n x (- k 1)))))
(define (tean-one tower h n) (cons (te-one (- h 1)) (quote ())))
