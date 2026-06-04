; -*- lisp -*-
; lib/cas/mixedlog.lisp -- RUNG 5, the LOGARITHMIC (primitive) case of mixed transcendental-over-algebraic
; integration: INT (P_1 t + P_0) dx where t = log(h) is a primitive monomial (t' = h'/h, a rational function)
; and the coefficients P_1, P_0 are ALGEBRAIC functions -- field elements of K = Q(x)[y]/(y^2 - p).  Together
; with the exponential case (mixedexp.lisp) this builds out Rung 5: a transcendental monomial over an algebraic
; coefficient field (docs/TRAGER_ROADMAP.md).
;
; An element of the tower K(t) is represented as a list of field elements (c_0 c_1 ... c_d) meaning
; c_0 + c_1 t + ... + c_d t^d, each c_i a field element (u + v y) of K.  The derivation in K(t) is
;     d/dx (sum c_i t^i) = sum (c_i' + (i+1) c_{i+1} t') t^i,
; using t' = h'/h and the field derivation for c_i' (since d/dx t^i = i t^{i-1} t', the t' couples adjacent
; degrees).  Integrating a degree-1 input P_1 t + P_0 yields a degree-2 answer Q_2 t^2 + Q_1 t + Q_0 with
;     Q_2' = 0            (Q_2 a constant field element)
;     2 Q_2 t' + Q_1' = P_1
;     Q_1 t' + Q_0' = P_0
; so the problem reduces to two field-antiderivative solves; for Q of bounded polynomial degree it is one exact
; linear system, and the answer is certified by differentiating in K(t) and matching the integrand.
;
; Public:
;   ml-deriv p tp Q       -> d/dx Q in K(t)  (Q a t-polynomial with field coeffs; tp = t' as a rational fn)
;   ml-rhs p tp Q         -> same as ml-deriv (the constructive direction, builds integrands)
;   ml-certify p tp Q B   -> #t iff d/dx Q = B in K(t)  (the arbiter: INT B dx = Q)
;   ml-solve-sqrt tp B degbound -> Q | 'none : solve INT B dx = Q over the field y^2 = x, B a degree-<=1
;                            t-polynomial, by undetermined coefficients (field coeffs of bounded degree),
;                            certificate-checked.
;   ml-integrate-sqrt tp B degbound -> (list 'elementary Q) | 'no-elementary-form
;
; Verified: INT ((1/(2 sqrt x)) log x + 1/sqrt x) dx = sqrt(x) log(x) on y^2 = x, t = log x (t' = 1/x); and the
; honest 'none when no bounded-degree field polynomial solves the system.
;
; Builds on algfunc.lisp (the field K) and poly.lisp / tower.lisp.

(import "cas/algfunc.lisp")

(define (ml-nth l k) (if (= k 0) (car l) (ml-nth (cdr l) (- k 1))))
(define (ml-len l) (if (null? l) 0 (+ 1 (ml-len (cdr l)))))

; t-polynomial helpers: a list of field elements (low degree first).  Pad / trim as needed.
(define (ml-coeff Q i) (if (< i (ml-len Q)) (ml-nth Q i) (af-zero)))
(define (ml-trim Q) (ml-trim-go (reverse Q)))                 ; drop trailing zero field-coeffs
(define (ml-trim-go r) (cond ((null? r) (list (af-zero))) ((ml-zero-el? (car r)) (ml-trim-go (cdr r))) (else (reverse r))))
(define (ml-zero-el? c) (af-equal? c (af-zero)))

; ----- derivation in K(t): d/dx (sum c_i t^i) = sum (c_i' + (i+1) c_{i+1} t') t^i -----
(define (ml-deriv p tp Q) (ml-trim (ml-deriv-go p tp Q 0)))
(define (ml-deriv-go p tp Q i)
  (if (>= i (ml-len Q)) (quote ())
      (cons (af-add (af-deriv p (ml-coeff Q i))
                    (af-mul p (af-from-rat tp) (af-scale-int (+ i 1) (ml-coeff Q (+ i 1)))))
            (ml-deriv-go p tp Q (+ i 1)))))
(define (af-scale-int k c) (af-make (rat-scale k (af-u c)) (rat-scale k (af-v c))))
(define (ml-rhs p tp Q) (ml-deriv p tp Q))

; ----- certificate: d/dx Q = B in K(t) -----
(define (ml-certify p tp Q B) (ml-eq? (ml-deriv p tp Q) B))
(define (ml-eq? A B) (ml-eq-go A B 0 (if (> (ml-len A) (ml-len B)) (ml-len A) (ml-len B))))
(define (ml-eq-go A B i n) (if (>= i n) #t (if (af-equal? (ml-coeff A i) (ml-coeff B i)) (ml-eq-go A B (+ i 1) n) #f)))

; ===== solver over the field y^2 = x (y = sqrt x): seek Q = Q_2 t^2 + Q_1 t + Q_0, each Q_j a field element
; with polynomial u,v parts of degree <= deg.  Solve d/dx Q = B by undetermined coefficients. =====
(define (ml-px) (rat-from-poly (list 0 1)))

; number of scalar unknowns per field-coefficient = 2*(deg+1) (u and v parts); times (tdeg+1) t-coefficients.
; We encode Q's unknowns as a flat vector and build Q from it.
(define (ml-build-Q tdeg deg cs) (ml-bq-go tdeg deg cs 0))
(define (ml-bq-go tdeg deg cs i)
  (if (> i tdeg) (quote ())
      (cons (ml-fieldelt (ml-slice cs (* i (* 2 (+ deg 1))) (* 2 (+ deg 1))) deg)
            (ml-bq-go tdeg deg cs (+ i 1)))))
(define (ml-fieldelt seg deg) (af-make (rat-from-poly (ml-take seg (+ deg 1))) (rat-from-poly (ml-take (ml-drop seg (+ deg 1)) (+ deg 1)))))
(define (ml-take l n) (if (= n 0) (quote ()) (if (null? l) (cons 0 (ml-take (quote ()) (- n 1))) (cons (car l) (ml-take (cdr l) (- n 1))))))
(define (ml-drop l n) (if (= n 0) l (if (null? l) (quote ()) (ml-drop (cdr l) (- n 1)))))
(define (ml-slice l start len) (ml-take (ml-drop l start) len))

; residual vector of (d/dx Q - B): flatten each t-coefficient's two sector numerators, padded.
(define (ml-resid tp tdeg deg cs B)
  (let ((Q (ml-build-Q tdeg deg cs)))
    (let ((R (ml-sub (ml-deriv (ml-px) tp Q) B)))
      (ml-flatten R (+ tdeg 1)))))
(define (ml-sub A B) (ml-sub-go A B 0 (if (> (ml-len A) (ml-len B)) (ml-len A) (ml-len B))))
(define (ml-sub-go A B i n) (if (>= i n) (quote ()) (cons (af-make (rat-sub (af-u (ml-coeff A i)) (af-u (ml-coeff B i))) (rat-sub (af-v (ml-coeff A i)) (af-v (ml-coeff B i)))) (ml-sub-go A B (+ i 1) n))))
(define (ml-flatten R tcount) (ml-fl-go R tcount 0))
(define (ml-fl-go R tcount i) (if (>= i tcount) (quote ()) (append (append (ml-cl (af-u (ml-coeff R i))) (ml-cl (af-v (ml-coeff R i)))) (ml-fl-go R tcount (+ i 1)))))
(define (ml-cl r) (ml-pad (poly-norm (rat-num r)) 8))
(define (ml-pad l n) (if (= n 0) (quote ()) (cons (if (null? l) 0 (car l)) (ml-pad (if (null? l) (quote ()) (cdr l)) (- n 1)))))

; build & solve the linear system (residual affine in cs): reuse a Gauss-Jordan like mixedexp.
(define (ml-zeros m) (if (= m 0) (quote ()) (cons 0 (ml-zeros (- m 1)))))
(define (ml-unit m j) (ml-unit-go m j 0))
(define (ml-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (ml-unit-go m j (+ i 1)))))
(define (ml-vsub a b) (if (null? a) (quote ()) (cons (- (car a) (car b)) (ml-vsub (cdr a) (cdr b)))))
(define (ml-vneg a) (if (null? a) (quote ()) (cons (- 0 (car a)) (ml-vneg (cdr a)))))

(define (ml-solve-sqrt tp B tdeg deg)
  (let ((m (* (+ tdeg 1) (* 2 (+ deg 1)))))
    (let ((r0 (ml-resid tp tdeg deg (ml-zeros m) B)))
      (let ((cols (ml-cols tp tdeg deg m B 0 (quote ()))))
        (let ((sol (ml-lin-solve cols (ml-vneg r0) (ml-len r0) m)))
          (if (equal? sol (quote none)) (quote none)
              (let ((Q (ml-build-Q tdeg deg sol)))
                (if (ml-certify (ml-px) tp Q B) (ml-trim Q) (quote none))))))))) ; certify recovered Q
(define (ml-cols tp tdeg deg m B j acc)
  (if (= j m) (reverse acc)
      (ml-cols tp tdeg deg m B (+ j 1) (cons (ml-vsub (ml-resid tp tdeg deg (ml-unit m j) B) (ml-resid tp tdeg deg (ml-zeros m) B)) acc))))

; ----- exact linear solver (same approach as mixedexp): columns -> rows, drop zero rows, full Gauss-Jordan ---
(define (ml-lin-solve cols b rows m)
  (let ((rowsM (ml-aug (ml-rows-from-cols cols rows m) b)))
    (let ((work (ml-drop-zero-rows rowsM m)))
      (ml-reduce work m 0 (quote ())))))
(define (ml-rows-from-cols cols rows m) (ml-rfc cols rows 0))
(define (ml-rfc cols rows i) (if (= i rows) (quote ()) (cons (ml-rowi cols i) (ml-rfc cols rows (+ i 1)))))
(define (ml-rowi cols i) (if (null? cols) (quote ()) (cons (ml-vnth (car cols) i) (ml-rowi (cdr cols) i))))
(define (ml-vnth v i) (if (= i 0) (car v) (ml-vnth (cdr v) (- i 1))))
(define (ml-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (car b))) (ml-aug (cdr rows) (cdr b)))))
(define (ml-drop-zero-rows rows m) (if (null? rows) (quote ()) (if (ml-row-zero? (car rows) m 0) (ml-drop-zero-rows (cdr rows) m) (cons (car rows) (ml-drop-zero-rows (cdr rows) m)))))
(define (ml-row-zero? row m i) (cond ((= i m) #t) ((= (ml-vnth row i) 0) (ml-row-zero? row m (+ i 1))) (else #f)))
(define (ml-reduce work m c piv)
  (if (= c m) (ml-read piv m 0 (quote ()))
      (let ((pr (ml-first-with-col work c)))
        (if (equal? pr (quote none)) (ml-reduce work m (+ c 1) piv)
            (let ((prn (ml-scale-row pr (/ 1 (ml-vnth pr c)))))
              (ml-reduce (ml-elim-others (ml-remove-row work pr) prn c) m (+ c 1) (cons (cons c prn) (ml-elim-piv piv prn c))))))))
(define (ml-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (ml-axpy (cdr (car piv)) prn (- 0 (ml-vnth (cdr (car piv)) c)))) (ml-elim-piv (cdr piv) prn c))))
(define (ml-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (ml-vnth (car work) c) 0)) (car work)) (else (ml-first-with-col (cdr work) c))))
(define (ml-remove-row work pr) (ml-rr-go work pr #f))
(define (ml-rr-go work pr removed) (cond ((null? work) (quote ())) ((if (not removed) (ml-eq-row? (car work) pr) #f) (ml-rr-go (cdr work) pr #t)) (else (cons (car work) (ml-rr-go (cdr work) pr removed)))))
(define (ml-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (ml-eq-row? (cdr a) (cdr b)) #f)))
(define (ml-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (ml-scale-row (cdr row) s))))
(define (ml-elim-others work prn c) (if (null? work) (quote ()) (cons (ml-axpy (car work) prn (- 0 (ml-vnth (car work) c))) (ml-elim-others (cdr work) prn c))))
(define (ml-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (ml-axpy (cdr row) (cdr prn) f))))
(define (ml-read piv m j acc) (if (= j m) (reverse acc) (let ((pr (ml-piv-for piv j))) (ml-read piv m (+ j 1) (cons (if (equal? pr (quote none)) 0 (ml-vnth pr m)) acc)))))
(define (ml-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (ml-piv-for (cdr piv) j))))

; ----- top-level: search bounded field-coefficient degree -----
(define (ml-integrate-sqrt tp B degbound)
  (let ((Q (ml-iter tp B 0 degbound)))
    (if (equal? Q (quote none)) (quote no-elementary-form) (list (quote elementary) Q))))
(define (ml-iter tp B d degbound)
  (if (> d degbound) (quote none)
      (let ((Q (ml-solve-sqrt tp B 2 d)))                       ; answer t-degree 2 for a degree-1 integrand
        (if (equal? Q (quote none)) (ml-iter tp B (+ d 1) degbound) Q))))
