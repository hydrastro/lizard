; -*- lisp -*-
; lib/cas/rt.lisp — the Rothstein-Trager logarithmic part of integrating a
; rational function p/q with q squarefree and deg p < deg q.
;
;   R(z) = resultant_x(p - z q', q)        (a polynomial in z over Q)
;   for each residue c (a root of R):
;        term = c * log( gcd(p - c q', q) )
;   INT p/q dx = sum of those terms.
;
; The residues are the roots of R, grouped by the irreducible factors of R over
; Q.  A linear factor gives a RATIONAL residue and a log with rational argument.
; A higher-degree irreducible factor m gives a family of conjugate ALGEBRAIC
; residues; the corresponding log argument is computed once in Q(c) (c = a root
; of m) using polynomials over Q(c), and the real answer is the sum over the
; conjugates -- a "RootSum".  This is what lets us integrate things like
; 1/(x^2-2) and 1/(x^2+1) exactly, which the partial-fraction integrator defers.
;
; The whole log part is CERTIFIED over Q by the trace identity
;     p = sum_residues  c * g_c' * (q / g_c)
; evaluated factor by factor with Trace_{Q(c)/Q} (see rt-certificate).
;
; Top-level helpers only; builds on resultant.lisp, factor.lisp, apoly.lisp.

(import "cas/resultant.lisp")
(import "cas/factor.lisp")
(import "cas/apoly.lisp")

; ============================================================
;  compute the log part: a list of terms
;    (rlog c g)   rational residue c, g a polynomial over Q  ->  c*log(g)
;    (alog m g)   residues = roots of m(c); g a polynomial over Q(c)
;                 ->  sum_{m(c)=0} c*log(g(x))
; ============================================================
(define (rt-one m p q)
  (if (= (poly-deg m) 1)
      (let ((c (/ (- 0 (poly-coeff m 0)) (poly-coeff m 1))))     ; rational residue
        (list 'rlog c (poly-gcd (poly-sub p (poly-scale c (poly-deriv q))) q)))
      (let ((mc (poly-monic m)))                                 ; algebraic residues
        (let ((c (alg-gen mc)))
          (list 'alog mc
                (apoly-gcd (apoly-sub (apoly-embed p mc)
                                      (apoly-scale c (apoly-embed (poly-deriv q) mc)))
                           (apoly-embed q mc)))))))

(define (rt-terms ms p q)
  (if (null? ms) '() (cons (rt-one (car ms) p q) (rt-terms (cdr ms) p q))))

(define (rt-log-part p q)
  (rt-terms (map (lambda (mf) (car (cdr mf))) (car (cdr (factor-Q (rt-resultant p q))))) p q))

; ============================================================
;  display
; ============================================================
(define (coeff-log c arg)
  (cond ((= c 1) (string-append "log(" arg ")"))
        ((= c -1) (string-append "-log(" arg ")"))
        (else (string-append (rat->string c) "*log(" arg ")"))))

(define (rt-term->string t)
  (cond ((equal? (car t) 'rlog)
         (coeff-log (car (cdr t)) (poly->string (car (cdr (cdr t))) "x")))
        (else
         (string-append "RootSum(c: " (poly->string (car (cdr t)) "c")
                        " = 0,  c*log(" (apoly->string (car (cdr (cdr t))) "x") "))"))))

(define (rt-log->string terms)
  (if (null? terms) "0" (rt-join terms)))
(define (rt-join terms)
  (if (null? (cdr terms)) (rt-term->string (car terms))
    (string-append (rt-term->string (car terms)) "  +  " (rt-join (cdr terms)))))

; ============================================================
;  certificate (fully rational):  p = sum_residues  c * g_c' * (q / g_c)
;  For an algebraic factor the sum over conjugate residues is computed as
;  Trace_{Q(c)/Q} of  c * g' * (q/g)  (over Q(c)) applied coefficient-wise.
;  Power sums of the roots come from Newton's identities on the minimal poly.
; ============================================================
(define (newton-e m i) (poly-coeff m (- (poly-deg m) i)))     ; e_i = coeff of z^{deg-i}
(define (ps-sum m svals k i)                                  ; sum_{i=1}^{k-1} e_i s_{k-i}
  (if (>= i k) 0 (+ (* (newton-e m i) (nth svals (- k i))) (ps-sum m svals k (+ i 1)))))
(define (ps-k m k svals) (- 0 (+ (ps-sum m svals k 1) (* k (newton-e m k)))))
(define (ps-build m d k acc) (if (= k d) acc (ps-build m d (+ k 1) (append acc (list (ps-k m k acc))))))
(define (power-sums m) (ps-build m (poly-deg m) 1 (list (poly-deg m))))   ; s_0..s_{d-1}

(define (trace-dot rep svals i)                               ; sum rep_i * s_i
  (if (null? rep) 0 (+ (* (car rep) (nth svals i)) (trace-dot (cdr rep) svals (+ i 1)))))
(define (alg-trace a) (trace-dot (alg-rep a) (power-sums (alg-min a)) 0))

(define (rt-cert-contrib t q)
  (cond ((equal? (car t) 'rlog)
         (poly-scale (car (cdr t)) (poly-mul (poly-deriv (car (cdr (cdr t)))) (poly-div q (car (cdr (cdr t)))))))
        (else
         (let ((m (car (cdr t))) (g (car (cdr (cdr t)))))
           (map alg-trace
                (apoly-mul (apoly-scale (alg-gen m) (apoly-deriv g))
                           (apoly-div (apoly-embed q m) g)))))))
(define (rt-cert-sum terms q)
  (if (null? terms) '() (poly-add (rt-cert-contrib (car terms) q) (rt-cert-sum (cdr terms) q))))
(define (rt-certificate p q terms) (equal? (poly-norm p) (poly-norm (rt-cert-sum terms q))))

; parameterized display: same as rt-log->string but with a caller-chosen variable name
(define (rt-term->string-v t var)
  (cond ((equal? (car t) 'rlog) (coeff-log (car (cdr t)) (poly->string (car (cdr (cdr t))) var)))
        (else (string-append "RootSum(c: " (poly->string (car (cdr t)) "c") " = 0,  c*log(" (apoly->string (car (cdr (cdr t))) var) "))"))))
(define (rt-join-v terms var) (if (null? (cdr terms)) (rt-term->string-v (car terms) var) (string-append (rt-term->string-v (car terms) var) "  +  " (rt-join-v (cdr terms) var))))
(define (rt-log->string-v terms var) (if (null? terms) "0" (rt-join-v terms var)))
