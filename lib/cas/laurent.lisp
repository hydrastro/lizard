; -*- lisp -*-
; lib/cas/laurent.lisp -- LAURENT SERIES: power series allowing finitely many negative-power terms,
; f(x) = sum_{k >= N} a_k x^k with N possibly < 0.  This completes the series capability (Taylor in
; series.lisp, Puiseux in puiseux*.lisp, Laurent here) and provides the analytic backbone for residues and
; principal parts.
;
; Representation: (laurent N coeffs), meaning f = sum_i coeffs[i] * x^(N + i); N is the lowest exponent (an
; integer, possibly negative) and coeffs the coefficient list from x^N upward.  We keep N as the nominal lowest
; exponent (leading coefficients may be zero; lr-normalize trims them and lifts N).
;
; Provided: the Laurent algebra (add, sub, scale, mul, inverse of a unit, derivative, integrate with explicit
; log-term detection), residue (coefficient of x^{-1}) and principal part; and the practically important
; lr-expand-ratfun: the Laurent expansion of a rational function p(x)/q(x) at x = 0 (q may vanish at 0), via
; q = x^v * u(x) with u(0) != 0, so p/q = x^{-v} * (p * u^{-1}) as a Taylor series shifted down by v.  From this
; the residue of any rational function at any point a follows (shift x -> x + a).
;
; Builds on series.lisp (Taylor arithmetic) and poly.lisp.

(import "cas/series.lisp")
(import "cas/poly.lisp")

(define (lr-nth l k) (if (= k 0) (car l) (lr-nth (cdr l) (- k 1))))
(define (lr-idx l i) (if (< i (length l)) (lr-nth l i) 0))

; constructor / accessors
(define (lr-make N coeffs) (list (quote laurent) N coeffs))
(define (lr-lo L) (lr-nth L 1))                                ; lowest exponent N
(define (lr-coeffs L) (lr-nth L 2))
(define (lr-hi L) (+ (lr-lo L) (- (length (lr-coeffs L)) 1)))  ; highest exponent represented

; coefficient of x^k
(define (lr-coeff L k) (let ((i (- k (lr-lo L)))) (if (< i 0) 0 (lr-idx (lr-coeffs L) i))))

; normalize: trim leading zero coefficients, lifting N (but never past the highest term)
(define (lr-normalize L)
  (let ((cs (lr-coeffs L)) (N (lr-lo L)))
    (lr-norm-go cs N)))
(define (lr-norm-go cs N)
  (if (null? cs) (lr-make 0 (list 0))
      (if (= (car cs) 0) (if (null? (cdr cs)) (lr-make 0 (list 0)) (lr-norm-go (cdr cs) (+ N 1)))
          (lr-make N cs))))

; ----- algebra -----
(define (lr-scale c L) (lr-make (lr-lo L) (lr-scale-list c (lr-coeffs L))))
(define (lr-scale-list c l) (if (null? l) (quote ()) (cons (* c (car l)) (lr-scale-list c (cdr l)))))
(define (lr-neg L) (lr-scale -1 L))

; add: align to the common lowest exponent
(define (lr-add A B)
  (let ((N (if (< (lr-lo A) (lr-lo B)) (lr-lo A) (lr-lo B))))
    (let ((hi (if (> (lr-hi A) (lr-hi B)) (lr-hi A) (lr-hi B))))
      (lr-normalize (lr-make N (lr-build-add A B N hi))))))
(define (lr-build-add A B k hi) (if (> k hi) (quote ()) (cons (+ (lr-coeff A k) (lr-coeff B k)) (lr-build-add A B (+ k 1) hi))))
(define (lr-sub A B) (lr-add A (lr-neg B)))

; multiply: exponents add; coefficient convolution
(define (lr-mul A B)
  (let ((N (+ (lr-lo A) (lr-lo B))))
    (let ((ca (lr-coeffs A)) (cb (lr-coeffs B)))
      (lr-normalize (lr-make N (lr-conv ca cb))))))
(define (lr-conv a b) (lr-conv-go a b 0 (+ (length a) (- (length b) 1))))
(define (lr-conv-go a b k tot) (if (>= k tot) (quote ()) (cons (lr-cc a b k) (lr-conv-go a b (+ k 1) tot))))
(define (lr-cc a b k) (lr-cc-go a b k 0 0))
(define (lr-cc-go a b k i s) (if (> i k) s (lr-cc-go a b k (+ i 1) (+ s (* (lr-idx a i) (lr-idx b (- k i)))))))

; inverse of a Laurent unit (lowest coeff nonzero): 1/f.  f = a_N x^N (1 + higher) -> f^{-1} = a_N^{-1} x^{-N}
; (1 + higher)^{-1} (a Taylor inverse).  prec = number of coefficient terms to keep.
(define (lr-inverse L prec)
  (let ((Ln (lr-normalize L)))
    (let ((N (lr-lo Ln)) (cs (lr-coeffs Ln)))
      (let ((u (lr-scale-list (/ 1 (car cs)) cs)))               ; u = f / (a_N x^N), u(0)=1
        (let ((uinv (ser-inverse u prec)))
          (lr-make (- 0 N) (lr-scale-list (/ 1 (car cs)) uinv)))))))
(define (lr-div A B prec) (lr-mul A (lr-inverse B prec)))

; derivative: d/dx sum a_k x^k = sum k a_k x^{k-1}
(define (lr-deriv L) (lr-normalize (lr-make (- (lr-lo L) 1) (lr-deriv-coeffs (lr-lo L) (lr-coeffs L)))))
(define (lr-deriv-coeffs N cs) (if (null? cs) (quote ()) (cons (* N (car cs)) (lr-deriv-coeffs (+ N 1) (cdr cs)))))

; integrate: antiderivative.  The x^{-1} term integrates to a logarithm (reported separately); all other terms
; x^k -> x^{k+1}/(k+1).  Returns (cons laurent-part log-coefficient): integral = laurent-part + log-coeff*log(x).
(define (lr-integrate L)
  (let ((logc (lr-coeff L -1)))
    (cons (lr-normalize (lr-make (+ (lr-lo L) 1) (lr-int-coeffs (lr-lo L) (lr-coeffs L)))) logc)))
(define (lr-int-coeffs N cs) (if (null? cs) (quote ()) (cons (if (= N -1) 0 (/ (car cs) (+ N 1))) (lr-int-coeffs (+ N 1) (cdr cs)))))

; residue = coefficient of x^{-1}; principal part = the strictly-negative-power terms
(define (lr-residue L) (lr-coeff L -1))
(define (lr-principal-part L)
  (if (>= (lr-lo L) 0) (lr-make 0 (list 0))
      (lr-normalize (lr-make (lr-lo L) (lr-take (lr-coeffs L) (- 0 (lr-lo L)))))))
(define (lr-take l n) (if (= n 0) (quote ()) (if (null? l) (quote ()) (cons (car l) (lr-take (cdr l) (- n 1))))))

; ----- Laurent expansion of a rational function p(x)/q(x) at x = 0 -----
; q = x^v * u(x), u(0) != 0; p/q = x^{-v} (p * u^{-1}).  prec = number of Taylor terms of (p u^{-1}).
(define (lr-expand-ratfun p q prec)
  (let ((v (lr-ord q)))
    (let ((u (lr-shiftdown q v)))
      (let ((pu (ser-mul (lr-trunc p (+ prec v 1)) (ser-inverse u (+ prec v 1)) (+ prec v 1))))
        (lr-normalize (lr-make (- 0 v) pu))))))
(define (lr-ord p) (if (null? p) 0 (lr-ord-go p 0)))
(define (lr-ord-go p k) (cond ((null? p) 0) ((not (= (car p) 0)) k) (else (lr-ord-go (cdr p) (+ k 1)))))
(define (lr-shiftdown p v) (if (= v 0) p (lr-shiftdown (cdr p) (- v 1))))
(define (lr-trunc l n) (if (= n 0) (quote ()) (if (null? l) (cons 0 (lr-trunc (quote ()) (- n 1))) (cons (car l) (lr-trunc (cdr l) (- n 1))))))

; residue of a rational function p/q at a point a: shift x -> x + a, expand at 0, take coeff of x^{-1}.
(define (lr-residue-at p q a prec)
  (lr-residue (lr-expand-ratfun (lr-pshift p a) (lr-pshift q a) prec)))
(define (lr-pshift p a) (if (= a 0) p (lr-pshift-go (reverse p) a (list 0))))
(define (lr-pshift-go cs a acc) (if (null? cs) acc (lr-pshift-go (cdr cs) a (poly-add (poly-mul acc (list a 1)) (list (car cs))))))

; pretty terms for display: list of (exponent . coeff) for nonzero coeffs
(define (lr-terms L) (lr-terms-go (lr-lo L) (lr-coeffs L)))
(define (lr-terms-go k cs) (if (null? cs) (quote ()) (if (= (car cs) 0) (lr-terms-go (+ k 1) (cdr cs)) (cons (cons k (car cs)) (lr-terms-go (+ k 1) (cdr cs))))))
