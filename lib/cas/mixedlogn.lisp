; -*- lisp -*-
; lib/cas/mixedlogn.lisp -- RUNG 5: MIXED LOGARITHMIC-OVER-ALGEBRAIC integration over the GENERAL superelliptic
; field K = Q(x)[y]/(y^n - g) for arbitrary n.  This lifts the n=2 logarithmic case (mixedlog.lisp) to any
; degree, the companion of mixedexpn.lisp, using the general-n field arithmetic of sefield.lisp
; (docs/TRAGER_ROADMAP.md, Rung 5).
;
; We integrate INT (P_1 t + P_0) dx where t = log(h) is a primitive monomial (t' = h'/h) and the coefficients
; P_1, P_0 are field elements of K.  An element of the tower K(t) is a list of field elements (C_0 C_1 ... C_d)
; meaning sum_i C_i t^i, each C_i a length-n sefield element.  The derivation is
;     d/dx (sum C_i t^i) = sum (C_i' + (i+1) C_{i+1} t') t^i,
; with C_i' the sefield derivative and t' rational (it scalar-multiplies C_{i+1}).  Integrating a degree-1 input
; gives Q_2 t^2 + Q_1 t + Q_0 with Q_2' = 0, 2 Q_2 t' + Q_1' = P_1, Q_1 t' + Q_0' = P_0.  For Q whose field
; coefficients are polynomials of bounded degree this is one exact linear system (the field-coefficient sectors
; decouple within each t-degree), solved by undetermined coefficients and certified by differentiating in K(t).
;
; Public:
;   mln-deriv g n tp Q       -> d/dx Q in K(t)  (Q a t-polynomial of sefield coefficients; tp = t')
;   mln-rhs g n tp Q         -> same (constructive direction)
;   mln-certify g n tp Q B   -> #t iff d/dx Q = B in K(t)  (the arbiter: INT B dx = Q)
;   mln-solve g n tp B deg   -> Q | 'none : solve INT B dx = Q over K (B degree-<=1 in t) with field-coefficient
;                               polynomials of degree <= deg, certificate-checked
;   mln-integrate g n tp B degbound -> (list 'elementary Q) | 'no-elementary-form
;
; Verified: INT ((x^2/(x^3+1)) y log x + (1/x) y) dx = y log x on y^3 = x^3+1, t = log x; the t^2 case; and the
; n=2 specialization reproducing mixedlog.
;
; Builds on sefield.lisp and poly.lisp / tower.lisp.

(import "cas/sefield.lisp")

(define (mln-nth l k) (if (= k 0) (car l) (mln-nth (cdr l) (- k 1))))
(define (mln-len l) (if (null? l) 0 (+ 1 (mln-len (cdr l)))))

; t-polynomial of sefield coefficients (low t-degree first)
(define (mln-coeff g n Q i) (if (< i (mln-len Q)) (mln-nth Q i) (sf-zeros n)))
(define (mln-trim g n Q) (mln-trim-go g n (reverse Q)))
(define (mln-trim-go g n r) (cond ((null? r) (list (sf-zeros n))) ((mln-zero? (car r)) (mln-trim-go g n (cdr r))) (else (reverse r))))
(define (mln-zero? C) (mln-allzero C))
(define (mln-allzero C) (if (null? C) #t (if (rat-zero? (car C)) (mln-allzero (cdr C)) #f)))

; ----- derivation in K(t): d/dx (sum C_i t^i) = sum (C_i' + (i+1) C_{i+1} t') t^i -----
(define (mln-deriv g n tp Q) (mln-trim g n (mln-deriv-go g n tp Q 0)))
(define (mln-deriv-go g n tp Q i)
  (if (>= i (mln-len Q)) (quote ())
      (cons (sf-add (sf-deriv g n (mln-coeff g n Q i))
                    (sf-scale (rat-scale (+ i 1) tp) (mln-coeff g n Q (+ i 1))))
            (mln-deriv-go g n tp Q (+ i 1)))))
(define (mln-rhs g n tp Q) (mln-deriv g n tp Q))

; ----- certificate: d/dx Q = B in K(t) -----
(define (mln-certify g n tp Q B) (mln-eq? g n (mln-deriv g n tp Q) B))
(define (mln-eq? g n A B) (mln-eq-go g n A B 0 (mln-maxlen A B)))
(define (mln-maxlen A B) (if (> (mln-len A) (mln-len B)) (mln-len A) (mln-len B)))
(define (mln-eq-go g n A B i k) (if (>= i k) #t (if (sf-equal? (mln-coeff g n A i) (mln-coeff g n B i)) (mln-eq-go g n A B (+ i 1) k) #f)))

; ===== solver over K: seek Q = Q_2 t^2 + Q_1 t + Q_0 (t-degree 2), each Q_j a sefield element whose sector
; polynomials have degree <= deg.  Encode all unknowns in a flat vector; build Q; residual; one linear solve. =====
; layout: for t-degree i in 0..2, for sector s in 0..n-1, (deg+1) coefficients for the u-part... but sefield
; coefficients are rationals; we seek POLYNOMIAL sector entries, so (deg+1) coeffs per (t-degree, sector).
(define (mln-px) (rat-from-poly (list 0 1)))   ; unused base hook (kept for parallel with siblings)

(define (mln-unknowns tdeg n deg) (* (+ tdeg 1) (* n (+ deg 1))))
; build Q (t-polynomial of sefield elements) from the flat coefficient vector cs
(define (mln-build g n tdeg deg cs) (mln-build-t g n tdeg deg cs 0))
(define (mln-build-t g n tdeg deg cs i)
  (if (> i tdeg) (quote ())
      (cons (mln-build-sectors n deg (mln-slice cs (* i (* n (+ deg 1))) (* n (+ deg 1))) 0)
            (mln-build-t g n tdeg deg cs (+ i 1)))))
(define (mln-build-sectors n deg seg s)
  (if (>= s n) (quote ())
      (cons (rat-from-poly (mln-take (mln-drop seg (* s (+ deg 1))) (+ deg 1)))
            (mln-build-sectors n deg seg (+ s 1)))))
(define (mln-take l k) (if (= k 0) (quote ()) (if (null? l) (cons 0 (mln-take (quote ()) (- k 1))) (cons (car l) (mln-take (cdr l) (- k 1))))))
(define (mln-drop l k) (if (= k 0) l (if (null? l) (quote ()) (mln-drop (cdr l) (- k 1)))))
(define (mln-slice l start len) (mln-take (mln-drop l start) len))

; residual vector of (d/dx Q - B): flatten each t-coefficient's n sector numerators, padded
(define (mln-resid g n tp tdeg deg cs B)
  (mln-flatten g n (mln-sub g n (mln-deriv g n tp (mln-build g n tdeg deg cs)) B) (+ tdeg 1)))
(define (mln-sub g n A B) (mln-sub-go g n A B 0 (mln-maxlen A B)))
(define (mln-sub-go g n A B i k) (if (>= i k) (quote ()) (cons (mln-elt-sub (mln-coeff g n A i) (mln-coeff g n B i)) (mln-sub-go g n A B (+ i 1) k))))
(define (mln-elt-sub C D) (if (null? C) (quote ()) (cons (rat-sub (car C) (car D)) (mln-elt-sub (cdr C) (cdr D)))))
(define (mln-flatten g n R tcount) (mln-fl-go g n R tcount 0))
(define (mln-fl-go g n R tcount i) (if (>= i tcount) (quote ()) (append (mln-flat-elt (mln-coeff g n R i)) (mln-fl-go g n R tcount (+ i 1)))))
(define (mln-flat-elt C) (if (null? C) (quote ()) (append (mln-cl (car C)) (mln-flat-elt (cdr C)))))
(define (mln-cl r) (mln-pad (poly-norm (rat-num r)) 8))
(define (mln-pad l k) (if (= k 0) (quote ()) (cons (if (null? l) 0 (car l)) (mln-pad (if (null? l) (quote ()) (cdr l)) (- k 1)))))

; linear system: residual affine in cs
(define (mln-zeros m) (if (= m 0) (quote ()) (cons 0 (mln-zeros (- m 1)))))
(define (mln-unit m j) (mln-unit-go m j 0))
(define (mln-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (mln-unit-go m j (+ i 1)))))
(define (mln-vsub a b) (if (null? a) (quote ()) (cons (- (car a) (car b)) (mln-vsub (cdr a) (cdr b)))))
(define (mln-vneg a) (if (null? a) (quote ()) (cons (- 0 (car a)) (mln-vneg (cdr a)))))

; solve, then certify the assembled Q in the field
(define (mln-solve g n tp B deg) (mln-solve-go g n tp B deg 2))
(define (mln-solve-go g n tp B deg tdeg)
  (mln-solve-r0 g n tp B deg tdeg (mln-unknowns tdeg n deg)))
(define (mln-solve-r0 g n tp B deg tdeg m)
  (mln-solve-with g n tp B deg tdeg m (mln-resid g n tp tdeg deg (mln-zeros m) B)))
(define (mln-solve-with g n tp B deg tdeg m r0)
  (mln-finish g n tp B deg tdeg m (mln-lin-solve (mln-cols g n tp B deg tdeg m r0 0 (quote ())) (mln-vneg r0) (mln-len r0) m)))
(define (mln-finish g n tp B deg tdeg m sol)
  (if (equal? sol (quote none)) (quote none)
      (mln-finish2 g n tp B (mln-build g n tdeg deg sol))))
(define (mln-finish2 g n tp B Q) (if (mln-certify g n tp Q B) (mln-trim g n Q) (quote none)))
(define (mln-cols g n tp B deg tdeg m r0 j acc)
  (if (= j m) (reverse acc)
      (mln-cols g n tp B deg tdeg m r0 (+ j 1) (cons (mln-vsub (mln-resid g n tp tdeg deg (mln-unit m j) B) r0) acc))))

; ----- exact linear solver (the proven full Gauss-Jordan) -----
(define (mln-lin-solve cols b rows m) (mln-reduce (mln-drop-zero-rows (mln-aug (mln-rows-from-cols cols rows m) b) m) m 0 (quote ())))
(define (mln-rows-from-cols cols rows m) (mln-rfc cols rows 0))
(define (mln-rfc cols rows i) (if (= i rows) (quote ()) (cons (mln-rowi cols i) (mln-rfc cols rows (+ i 1)))))
(define (mln-rowi cols i) (if (null? cols) (quote ()) (cons (mln-vnth (car cols) i) (mln-rowi (cdr cols) i))))
(define (mln-vnth v i) (if (= i 0) (car v) (mln-vnth (cdr v) (- i 1))))
(define (mln-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (car b))) (mln-aug (cdr rows) (cdr b)))))
(define (mln-drop-zero-rows rows m) (if (null? rows) (quote ()) (if (mln-row-zero? (car rows) m 0) (mln-drop-zero-rows (cdr rows) m) (cons (car rows) (mln-drop-zero-rows (cdr rows) m)))))
(define (mln-row-zero? row m i) (cond ((= i m) #t) ((= (mln-vnth row i) 0) (mln-row-zero? row m (+ i 1))) (else #f)))
(define (mln-reduce work m c piv)
  (if (= c m) (mln-read piv m 0 (quote ()))
      (mln-reduce-step work m c piv (mln-first-with-col work c))))
(define (mln-reduce-step work m c piv pr)
  (if (equal? pr (quote none)) (mln-reduce work m (+ c 1) piv)
      (mln-reduce-pivot work m c piv (mln-scale-row pr (/ 1 (mln-vnth pr c))))))
(define (mln-reduce-pivot work m c piv prn)
  (mln-reduce (mln-elim-others (mln-remove-row work prn) prn c) m (+ c 1) (cons (cons c prn) (mln-elim-piv piv prn c))))
(define (mln-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (mln-axpy (cdr (car piv)) prn (- 0 (mln-vnth (cdr (car piv)) c)))) (mln-elim-piv (cdr piv) prn c))))
(define (mln-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (mln-vnth (car work) c) 0)) (car work)) (else (mln-first-with-col (cdr work) c))))
(define (mln-remove-row work prn) (mln-rr-go work prn #f))
(define (mln-rr-go work prn removed) (cond ((null? work) (quote ())) ((if (not removed) (mln-eq-row? (car work) prn) #f) (mln-rr-go (cdr work) prn #t)) (else (cons (car work) (mln-rr-go (cdr work) prn removed)))))
(define (mln-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (mln-eq-row? (cdr a) (cdr b)) #f)))
(define (mln-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (mln-scale-row (cdr row) s))))
(define (mln-elim-others work prn c) (if (null? work) (quote ()) (cons (mln-axpy (car work) prn (- 0 (mln-vnth (car work) c))) (mln-elim-others (cdr work) prn c))))
(define (mln-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (mln-axpy (cdr row) (cdr prn) f))))
(define (mln-read piv m j acc) (if (= j m) (reverse acc) (mln-read piv m (+ j 1) (cons (mln-readval (mln-piv-for piv j) m) acc))))
(define (mln-readval pr m) (if (equal? pr (quote none)) 0 (mln-vnth pr m)))
(define (mln-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (mln-piv-for (cdr piv) j))))

; ----- top-level: search bounded field-coefficient degree -----
(define (mln-integrate g n tp B degbound) (mln-int-result (mln-iter g n tp B 0 degbound)))
(define (mln-int-result Q) (if (equal? Q (quote none)) (quote no-elementary-form) (list (quote elementary) Q)))
(define (mln-iter g n tp B d degbound) (if (> d degbound) (quote none) (mln-iter-step g n tp B d degbound (mln-solve g n tp B d))))
(define (mln-iter-step g n tp B d degbound Q) (if (equal? Q (quote none)) (mln-iter g n tp B (+ d 1) degbound) Q))
