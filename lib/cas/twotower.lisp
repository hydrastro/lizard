; -*- lisp -*-
; lib/cas/twotower.lisp -- RUNG 5: the first genuinely STACKED two-monomial tower.  An element lives in the
; transcendental tower Q(x)(theta)(t) with TWO independent monomials: theta = exp(x) (theta' = theta) and
; t = log(x) (t' = 1/x).  This is the structure where the recursive Risch algorithm really operates -- one
; monomial stacked over another over the rational base (docs/TRAGER_ROADMAP.md, Rung 5).
;
; Representation.  An element is a t-polynomial whose coefficients are theta-polynomials whose coefficients are
; rational functions: a list over t-degree j of (a list over theta-degree k of rational functions c_{j,k}(x)),
; meaning sum_{j,k} c_{j,k}(x) theta^k t^j.  The derivation, from theta' = theta and t' = 1/x, is
;     d/dx ( c theta^k t^j ) = (c' + k c) theta^k t^j  +  (j c / x) theta^k t^{j-1},
; i.e. within a fixed (k) the exponential contributes (c' + k c) in place, and the logarithm couples t-degree j
; down to j-1 with weight 1/x.  Summing over the element gives tt-deriv.
;
; This lets the system integrate genuinely mixed two-monomial integrands and DECIDE elementarity within the
; bounded ansatz: e.g. INT (exp(x) log x + exp(x)/x) dx = exp(x) log(x), where each summand alone is
; non-elementary (an exponential-integral Ei) but the combination is elementary -- exactly the cancellation a
; tower integrator detects.  The answer is found by undetermined coefficients (requiring the residual to vanish
; at sample points, genuinely linear) and certified by differentiating in the tower.
;
; Public:
;   tt-deriv E              -> d/dx E in Q(x)(theta)(t)  (E a t-poly of theta-polys of rationals)
;   tt-certify E B          -> #t iff tt-deriv E = B  (the arbiter: INT B dx = E)
;   tt-solve B tdeg kdeg deg -> E | 'none : integrate B (a 2-monomial element) seeking an answer with t-degree
;                              <= tdeg, theta-degree <= kdeg, rational-function coefficients of degree <= deg
;                              (numerator/denominator built over a fixed denominator x^deg), certificate-checked
;   tt-integrate B          -> (list 'elementary E) | 'no-elementary-form
;
; Verified: INT (exp(x) log x + exp(x)/x) dx = exp(x) log(x); INT (exp(x)) dx = exp(x); and the honest 'none
; when the bounded ansatz has no solution.
;
; Builds on tower.lisp / ratfun.lisp (rational-function arithmetic) and poly.lisp.

(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (tt-nth l k) (if (= k 0) (car l) (tt-nth (cdr l) (- k 1))))
(define (tt-len l) (if (null? l) 0 (+ 1 (tt-len (cdr l)))))

; ----- element access: E is (T_0 T_1 ...) over t-degree; each T_j is (k_0 k_1 ...) over theta-degree of rats -----
(define (tt-tcoeff E j) (if (< j (tt-len E)) (tt-nth E j) (quote ())))            ; theta-poly at t-degree j
(define (tt-kcoeff T k) (if (< k (tt-len T)) (tt-nth T k) (rat-zero)))            ; rational at theta-degree k

; theta-poly arithmetic (lists of rationals over theta-degree)
(define (tt-tp-add A B) (tt-tpa A B 0 (tt-maxlen A B)))
(define (tt-tpa A B i n) (if (>= i n) (quote ()) (cons (rat-add (tt-kcoeff A i) (tt-kcoeff B i)) (tt-tpa A B (+ i 1) n))))
(define (tt-maxlen A B) (if (> (tt-len A) (tt-len B)) (tt-len A) (tt-len B)))
(define (tt-tp-zero? T) (cond ((null? T) #t) ((rat-zero? (car T)) (tt-tp-zero? (cdr T))) (else #f)))
(define (tt-tp-sub A B) (tt-tp-add A (tt-tp-neg B)))
(define (tt-tp-neg T) (if (null? T) (quote ()) (cons (rat-neg (car T)) (tt-tp-neg (cdr T)))))

; ----- derivation of one theta-poly IN PLACE (the theta^k t^j -> theta^k t^j part): c -> c' + k c -----
(define (tt-dtheta T) (tt-dth-go T 0))
(define (tt-dth-go T k) (if (null? T) (quote ()) (cons (rat-add (rat-deriv (car T)) (rat-scale k (car T))) (tt-dth-go (cdr T) (+ k 1)))))
; ----- the log coupling: theta^k t^j -> (1/x) theta^k t^{j-1}: scale the whole theta-poly by 1/x -----
(define (tt-scale-invx T) (tt-sx-go T))
(define (tt-sx-go T) (if (null? T) (quote ()) (cons (rat-mul (rat-make (list 1) (list 0 1)) (car T)) (tt-sx-go (cdr T)))))

; ----- the full derivation: d/dx E = sum_j [ dtheta(T_j) + (j+1) * invx-scaled T_{j+1} ] t^j -----
; (the t^{j} output collects the in-place derivative of T_j and the log-coupling from T_{j+1}, weight (j+1)/x)
(define (tt-deriv E) (tt-trim (tt-deriv-go E 0)))
(define (tt-deriv-go E j)
  (if (>= j (tt-len E)) (quote ())
      (cons (tt-tp-add (tt-dtheta (tt-tcoeff E j)) (tt-jinvx (+ j 1) (tt-tcoeff E (+ j 1))))
            (tt-deriv-go E (+ j 1)))))
; (j+1) * (1/x) * T_{j+1}
(define (tt-jinvx jp1 T) (tt-jx-go jp1 T))
(define (tt-jx-go jp1 T) (if (null? T) (quote ()) (cons (rat-mul (rat-mul (rat-from-poly (list jp1)) (rat-make (list 1) (list 0 1))) (car T)) (tt-jx-go jp1 (cdr T)))))
(define (tt-trim E) (tt-trim-go (tt-reverse E)))
(define (tt-trim-go r) (cond ((null? r) (list (quote ()))) ((tt-tp-zero? (car r)) (tt-trim-go (cdr r))) (else (tt-reverse r))))
(define (tt-reverse l) (tt-rev l (quote ())))
(define (tt-rev l acc) (if (null? l) acc (tt-rev (cdr l) (cons (car l) acc))))

; ----- certificate: d/dx E = B -----
(define (tt-certify E B) (tt-eq? (tt-deriv E) B))
(define (tt-eq? A B) (tt-eq-go A B 0 (tt-maxlen A B)))
(define (tt-eq-go A B j n) (if (>= j n) #t (if (tt-tp-eq? (tt-tcoeff A j) (tt-tcoeff B j)) (tt-eq-go A B (+ j 1) n) #f)))
(define (tt-tp-eq? A B) (tt-tpe A B 0 (tt-maxlen A B)))
(define (tt-tpe A B i n) (if (>= i n) #t (if (rat-equal? (tt-kcoeff A i) (tt-kcoeff B i)) (tt-tpe A B (+ i 1) n) #f)))

; ===== solver: seek E with t-degree <= tdeg, theta-degree <= kdeg, coefficient rationals built as
; (polynomial of degree <= deg) / x^deg.  Unknowns are the numerator coefficients; require d/dx E - B to vanish
; at sample points (linear), then certify. =====
(define (tt-unknowns tdeg kdeg deg) (* (+ tdeg 1) (* (+ kdeg 1) (+ deg 1))))
(define (tt-build tdeg kdeg deg cs) (tt-build-t tdeg kdeg deg cs 0))
(define (tt-build-t tdeg kdeg deg cs j)
  (if (> j tdeg) (quote ())
      (cons (tt-build-k kdeg deg (tt-slice cs (* j (* (+ kdeg 1) (+ deg 1))) (* (+ kdeg 1) (+ deg 1))) 0)
            (tt-build-t tdeg kdeg deg cs (+ j 1)))))
(define (tt-build-k kdeg deg seg k)
  (if (> k kdeg) (quote ())
      (cons (tt-mkrat (tt-slice seg (* k (+ deg 1)) (+ deg 1)) deg)
            (tt-build-k kdeg deg seg (+ k 1)))))
; rational = numerator poly (the (deg+1) coeffs) over denominator x^deg
(define (tt-mkrat coeffs deg) (rat-make coeffs (tt-xpow deg)))
(define (tt-xpow d) (if (= d 0) (list 1) (cons 0 (tt-xpow (- d 1)))))
(define (tt-take l k) (if (= k 0) (quote ()) (if (null? l) (cons 0 (tt-take (quote ()) (- k 1))) (cons (car l) (tt-take (cdr l) (- k 1))))))
(define (tt-drop l k) (if (= k 0) l (if (null? l) (quote ()) (tt-drop (cdr l) (- k 1)))))
(define (tt-slice l s n) (tt-take (tt-drop l s) n))

; residual: (d/dx E - B), each (t-degree, theta-degree) coefficient a rational; evaluate at sample points
(define (tt-resid tdeg kdeg deg cs B) (tt-flatten (tt-dsub (tt-deriv (tt-build tdeg kdeg deg cs)) B) (+ tdeg 1) (+ kdeg 1)))
(define (tt-dsub A B) (tt-dsub-go A B 0 (tt-maxlen A B)))
(define (tt-dsub-go A B j n) (if (>= j n) (quote ()) (cons (tt-tp-sub (tt-tcoeff A j) (tt-tcoeff B j)) (tt-dsub-go A B (+ j 1) n))))
(define (tt-points) (list 2 3 5 7 11 13 17 19 23 29))
(define (tt-flatten R tcount kcount) (tt-fl-j R tcount kcount 0))
(define (tt-fl-j R tcount kcount j) (if (>= j tcount) (quote ()) (append (tt-fl-k (tt-tcoeff R j) kcount 0) (tt-fl-j R tcount kcount (+ j 1)))))
(define (tt-fl-k T kcount k) (if (>= k kcount) (quote ()) (append (tt-eval-rat (tt-kcoeff T k) (tt-points)) (tt-fl-k T kcount (+ k 1)))))
(define (tt-eval-rat r pts) (if (null? pts) (quote ()) (cons (/ (poly-eval (rat-num r) (car pts)) (poly-eval (rat-den r) (car pts))) (tt-eval-rat r (cdr pts)))))

(define (tt-zeros m) (if (= m 0) (quote ()) (cons 0 (tt-zeros (- m 1)))))
(define (tt-unit m j) (tt-unit-go m j 0))
(define (tt-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (tt-unit-go m j (+ i 1)))))
(define (tt-vsub a b) (if (null? a) (quote ()) (cons (- (car a) (car b)) (tt-vsub (cdr a) (cdr b)))))
(define (tt-vneg a) (if (null? a) (quote ()) (cons (- 0 (car a)) (tt-vneg (cdr a)))))

(define (tt-solve B tdeg kdeg deg) (tt-solve-r0 B tdeg kdeg deg (tt-unknowns tdeg kdeg deg)))
(define (tt-solve-r0 B tdeg kdeg deg m) (tt-solve-with B tdeg kdeg deg m (tt-resid tdeg kdeg deg (tt-zeros m) B)))
(define (tt-solve-with B tdeg kdeg deg m r0) (tt-finish B tdeg kdeg deg m (tt-lin-solve (tt-cols B tdeg kdeg deg m r0 0 (quote ())) (tt-vneg r0) (tt-len r0) m)))
(define (tt-finish B tdeg kdeg deg m sol) (if (equal? sol (quote none)) (quote none) (tt-finish2 B (tt-build tdeg kdeg deg sol))))
(define (tt-finish2 B E) (if (tt-certify E B) (tt-trim E) (quote none)))
(define (tt-cols B tdeg kdeg deg m r0 j acc) (if (= j m) (tt-reverse acc) (tt-cols B tdeg kdeg deg m r0 (+ j 1) (cons (tt-vsub (tt-resid tdeg kdeg deg (tt-unit m j) B) r0) acc))))

; ----- exact linear solver (the proven full Gauss-Jordan, flattened) -----
(define (tt-lin-solve cols b rows m) (tt-reduce (tt-drop-zero-rows (tt-aug (tt-rows-from-cols cols rows m) b) m) m 0 (quote ())))
(define (tt-rows-from-cols cols rows m) (tt-rfc cols rows 0))
(define (tt-rfc cols rows i) (if (= i rows) (quote ()) (cons (tt-rowi cols i) (tt-rfc cols rows (+ i 1)))))
(define (tt-rowi cols i) (if (null? cols) (quote ()) (cons (tt-vnth (car cols) i) (tt-rowi (cdr cols) i))))
(define (tt-vnth v i) (if (= i 0) (car v) (tt-vnth (cdr v) (- i 1))))
(define (tt-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (car b))) (tt-aug (cdr rows) (cdr b)))))
(define (tt-drop-zero-rows rows m) (if (null? rows) (quote ()) (if (tt-row-zero? (car rows) m 0) (tt-drop-zero-rows (cdr rows) m) (cons (car rows) (tt-drop-zero-rows (cdr rows) m)))))
(define (tt-row-zero? row m i) (cond ((= i m) #t) ((= (tt-vnth row i) 0) (tt-row-zero? row m (+ i 1))) (else #f)))
(define (tt-reduce work m c piv) (if (= c m) (tt-read piv m 0 (quote ())) (tt-reduce-step work m c piv (tt-first-with-col work c))))
(define (tt-reduce-step work m c piv pr) (if (equal? pr (quote none)) (tt-reduce work m (+ c 1) piv) (tt-reduce-pivot work m c piv (tt-scale-row pr (/ 1 (tt-vnth pr c))))))
(define (tt-reduce-pivot work m c piv prn) (tt-reduce (tt-elim-others (tt-remove-row work prn) prn c) m (+ c 1) (cons (cons c prn) (tt-elim-piv piv prn c))))
(define (tt-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (tt-axpy (cdr (car piv)) prn (- 0 (tt-vnth (cdr (car piv)) c)))) (tt-elim-piv (cdr piv) prn c))))
(define (tt-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (tt-vnth (car work) c) 0)) (car work)) (else (tt-first-with-col (cdr work) c))))
(define (tt-remove-row work prn) (tt-rr-go work prn #f))
(define (tt-rr-go work prn removed) (cond ((null? work) (quote ())) ((if (not removed) (tt-eq-row? (car work) prn) #f) (tt-rr-go (cdr work) prn #t)) (else (cons (car work) (tt-rr-go (cdr work) prn removed)))))
(define (tt-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (tt-eq-row? (cdr a) (cdr b)) #f)))
(define (tt-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (tt-scale-row (cdr row) s))))
(define (tt-elim-others work prn c) (if (null? work) (quote ()) (cons (tt-axpy (car work) prn (- 0 (tt-vnth (car work) c))) (tt-elim-others (cdr work) prn c))))
(define (tt-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (tt-axpy (cdr row) (cdr prn) f))))
(define (tt-read piv m j acc) (if (= j m) (tt-reverse acc) (tt-read piv m (+ j 1) (cons (tt-readval (tt-piv-for piv j) m) acc))))
(define (tt-readval pr m) (if (equal? pr (quote none)) 0 (tt-vnth pr m)))
(define (tt-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (tt-piv-for (cdr piv) j))))

; ----- top-level: escalate gently over the coefficient degree (deg 0 first, then 1) to keep the linear
; systems small; the answer's t-degree is bounded by the integrand's, theta-degree searched up to 2. -----
(define (tt-integrate B) (tt-int-result (tt-int-iter B 0)))
(define (tt-int-iter B deg)
  (if (> deg 2) (quote none)
      (tt-int-step B deg (tt-solve B (tt-len B) 2 deg))))
(define (tt-int-step B deg E) (if (equal? E (quote none)) (tt-int-iter B (+ deg 1)) E))
(define (tt-int-result E) (if (equal? E (quote none)) (quote no-elementary-form) (list (quote elementary) E)))
