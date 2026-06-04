; -*- lisp -*-
; lib/cas/alglog.lisp -- RUNG 5: the ENTANGLED PRIMITIVE tower -- the LOGARITHM OF AN ALGEBRAIC function,
; t = log(w) with w a field element of K = Q(x)[y]/(y^n - g).  The companion of algexp.lisp (exp of an algebraic
; argument): here the primitive monomial's derivative t' = w'/w lives in K (computed via the field inverse from
; senorm.lisp), not merely as a rational function as in mixedlog/mixedlogn (docs/TRAGER_ROADMAP.md, Rung 5).
;
; We integrate INT (P_1 t + P_0) dx with P_1, P_0 field elements and t = log(w).  An element of the tower K(t)
; is a list of field elements (C_0 C_1 ... C_d) = sum_i C_i t^i.  The derivation is
;     d/dx (sum C_i t^i) = sum (C_i' + (i+1) C_{i+1} t') t^i,
; where C_i' is the sefield derivative and t' = w'/w is a FIELD element, so the product C_{i+1} t' is a full
; field product (sf-product) -- the y-power sectors COUPLE, exactly as in algexp.  Integrating a degree-1 input
; gives Q_2 t^2 + Q_1 t + Q_0 with Q_2' = 0, 2 Q_2 t' + Q_1' = P_1, Q_1 t' + Q_0' = P_0; for Q whose field
; coefficients are polynomials of bounded degree this is one linear system, solved by requiring the residual to
; vanish at sample points, and certified by differentiating in K(t).
;
; Public:
;   al-tprime g n w        -> t' = w'/w as a field element of K (the logarithmic derivative of t = log w)
;   al-deriv g n tp Q      -> d/dx Q in K(t)  (Q a t-polynomial of field coefficients; tp = t' a field element)
;   al-rhs g n tp Q        -> same (constructive direction)
;   al-certify g n tp Q B  -> #t iff d/dx Q = B in K(t)  (the arbiter: INT B dx = Q)
;   al-solve g n tp B deg  -> Q | 'none : solve INT B dx = Q over K (B degree-<=1 in t) with field-coefficient
;                             polynomials of degree <= deg, certificate-checked
;   al-integrate g n tp B degbound -> (list 'elementary Q) | 'no-elementary-form
;
; Verified: INT (w'/w) dx = log(w) for w = sqrt x + 1 on y^2 = x (the pure entangled logarithm); a t^2 case;
; and the n=3 cube-root argument log(x^(1/3) + 1).
;
; Builds on senorm.lisp (the field, derivation, product, and w'/w via the inverse) and poly.lisp / tower.lisp.

(import "cas/senorm.lisp")

(define (al-nth l k) (if (= k 0) (car l) (al-nth (cdr l) (- k 1))))
(define (al-len l) (if (null? l) 0 (+ 1 (al-len (cdr l)))))

; ----- t' = w'/w as a field element (rationalize via the field inverse: w'/w = F/N, then scale F by 1/N) -----
(define (al-tprime g n w) (al-tp-build (sn-logderiv g n w)))
(define (al-tp-build ld) (al-scale-rat (rat-inv (cdr ld)) (car ld)))
(define (al-scale-rat r C) (if (null? C) (quote ()) (cons (rat-mul r (car C)) (al-scale-rat r (cdr C)))))

; t-polynomial of field-element coefficients
(define (al-coeff g n Q i) (if (< i (al-len Q)) (al-nth Q i) (sf-zeros n)))
(define (al-trim g n Q) (al-trim-go g n (al-reverse Q)))
(define (al-trim-go g n r) (cond ((null? r) (list (sf-zeros n))) ((al-zero? (car r)) (al-trim-go g n (cdr r))) (else (al-reverse r))))
(define (al-zero? C) (if (null? C) #t (if (rat-zero? (car C)) (al-zero? (cdr C)) #f)))
(define (al-reverse l) (al-rev-go l (quote ())))
(define (al-rev-go l acc) (if (null? l) acc (al-rev-go (cdr l) (cons (car l) acc))))

; ----- derivation: d/dx (sum C_i t^i) = sum (C_i' + (i+1) C_{i+1} t') t^i, t' a field element -----
(define (al-deriv g n tp Q) (al-trim g n (al-deriv-go g n tp Q 0)))
(define (al-deriv-go g n tp Q i)
  (if (>= i (al-len Q)) (quote ())
      (cons (sf-add (sf-deriv g n (al-coeff g n Q i))
                    (sf-scale-int-product g n (+ i 1) (al-coeff g n Q (+ i 1)) tp))
            (al-deriv-go g n tp Q (+ i 1)))))
; (i+1) * C_{i+1} * t'  : scale the field product by the integer
(define (sf-scale-int-product g n k C tp) (al-iscale k (sf-product g n C tp)))
(define (al-iscale k C) (if (null? C) (quote ()) (cons (rat-mul (rat-from-poly (list k)) (car C)) (al-iscale k (cdr C)))))
(define (al-rhs g n tp Q) (al-deriv g n tp Q))

; ----- certificate: d/dx Q = B in K(t) -----
(define (al-certify g n tp Q B) (al-eq? g n (al-deriv g n tp Q) B))
(define (al-eq? g n A B) (al-eq-go g n A B 0 (al-maxlen A B)))
(define (al-maxlen A B) (if (> (al-len A) (al-len B)) (al-len A) (al-len B)))
(define (al-eq-go g n A B i k) (if (>= i k) #t (if (sf-equal? (al-coeff g n A i) (al-coeff g n B i)) (al-eq-go g n A B (+ i 1) k) #f)))

; ===== solver: Q = Q_2 t^2 + Q_1 t + Q_0, each Q_j a field element with polynomial sectors of degree <= deg.
; one linear system in all (tdeg+1)*n*(deg+1) coefficients; residual required to vanish at sample points. =====
(define (al-unknowns tdeg n deg) (* (+ tdeg 1) (* n (+ deg 1))))
(define (al-build g n tdeg deg cs) (al-build-t g n tdeg deg cs 0))
(define (al-build-t g n tdeg deg cs i)
  (if (> i tdeg) (quote ())
      (cons (al-build-sectors n deg (al-slice cs (* i (* n (+ deg 1))) (* n (+ deg 1))) 0)
            (al-build-t g n tdeg deg cs (+ i 1)))))
(define (al-build-sectors n deg seg s)
  (if (>= s n) (quote ())
      (cons (rat-from-poly (al-take (al-drop seg (* s (+ deg 1))) (+ deg 1)))
            (al-build-sectors n deg seg (+ s 1)))))
(define (al-take l k) (if (= k 0) (quote ()) (if (null? l) (cons 0 (al-take (quote ()) (- k 1))) (cons (car l) (al-take (cdr l) (- k 1))))))
(define (al-drop l k) (if (= k 0) l (if (null? l) (quote ()) (al-drop (cdr l) (- k 1)))))
(define (al-slice l start len) (al-take (al-drop l start) len))

; residual: (d/dx Q - B), each t-coefficient a field element; evaluate every sector at sample points (linear)
(define (al-resid g n tp tdeg deg cs B) (al-flatten-eval g n (al-sub g n (al-deriv g n tp (al-build g n tdeg deg cs)) B) (+ tdeg 1)))
(define (al-sub g n A B) (al-sub-go g n A B 0 (al-maxlen A B)))
(define (al-sub-go g n A B i k) (if (>= i k) (quote ()) (cons (al-elt-sub (al-coeff g n A i) (al-coeff g n B i)) (al-sub-go g n A B (+ i 1) k))))
(define (al-elt-sub C D) (if (null? C) (quote ()) (cons (rat-sub (car C) (car D)) (al-elt-sub (cdr C) (cdr D)))))
(define (al-points) (list 2 3 5 7 11 13 17 19 23 29))
(define (al-flatten-eval g n R tcount) (al-fe-go g n R tcount 0))
(define (al-fe-go g n R tcount i) (if (>= i tcount) (quote ()) (append (al-eval-elt (al-coeff g n R i)) (al-fe-go g n R tcount (+ i 1)))))
(define (al-eval-elt C) (if (null? C) (quote ()) (append (al-eval-rat (car C) (al-points)) (al-eval-elt (cdr C)))))
(define (al-eval-rat r pts) (if (null? pts) (quote ()) (cons (/ (poly-eval (rat-num r) (car pts)) (poly-eval (rat-den r) (car pts))) (al-eval-rat r (cdr pts)))))

(define (al-zeros m) (if (= m 0) (quote ()) (cons 0 (al-zeros (- m 1)))))
(define (al-unit m j) (al-unit-go m j 0))
(define (al-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (al-unit-go m j (+ i 1)))))
(define (al-vsub a b) (if (null? a) (quote ()) (cons (- (car a) (car b)) (al-vsub (cdr a) (cdr b)))))
(define (al-vneg a) (if (null? a) (quote ()) (cons (- 0 (car a)) (al-vneg (cdr a)))))

(define (al-solve g n tp B deg) (al-solve-go g n tp B deg (+ (al-bdeg g n B) 1)))
; the t-degree of the integrand B (highest i with a nonzero field coefficient); answer is one higher
(define (al-bdeg g n B) (al-bdeg-go g n B (- (al-len B) 1)))
(define (al-bdeg-go g n B i) (cond ((< i 0) 0) ((al-zero? (al-coeff g n B i)) (al-bdeg-go g n B (- i 1))) (else i)))
(define (al-solve-go g n tp B deg tdeg) (al-solve-r0 g n tp B deg tdeg (al-unknowns tdeg n deg)))
(define (al-solve-r0 g n tp B deg tdeg m) (al-solve-with g n tp B deg tdeg m (al-resid g n tp tdeg deg (al-zeros m) B)))
(define (al-solve-with g n tp B deg tdeg m r0) (al-finish g n tp B deg tdeg m (al-lin-solve (al-cols g n tp B deg tdeg m r0 0 (quote ())) (al-vneg r0) (al-len r0) m)))
(define (al-finish g n tp B deg tdeg m sol) (if (equal? sol (quote none)) (quote none) (al-finish2 g n tp B (al-build g n tdeg deg sol))))
(define (al-finish2 g n tp B Q) (if (al-certify g n tp Q B) (al-trim g n Q) (quote none)))
(define (al-cols g n tp B deg tdeg m r0 j acc) (if (= j m) (al-reverse acc) (al-cols g n tp B deg tdeg m r0 (+ j 1) (cons (al-vsub (al-resid g n tp tdeg deg (al-unit m j) B) r0) acc))))

; ----- exact linear solver: columns -> rows, drop-zero, full Gauss-Jordan (flattened helpers) -----
(define (al-lin-solve cols b rows m) (al-reduce (al-drop-zero-rows (al-aug (al-rows-from-cols cols rows m) b) m) m 0 (quote ())))
(define (al-rows-from-cols cols rows m) (al-rfc cols rows 0))
(define (al-rfc cols rows i) (if (= i rows) (quote ()) (cons (al-rowi cols i) (al-rfc cols rows (+ i 1)))))
(define (al-rowi cols i) (if (null? cols) (quote ()) (cons (al-vnth (car cols) i) (al-rowi (cdr cols) i))))
(define (al-vnth v i) (if (= i 0) (car v) (al-vnth (cdr v) (- i 1))))
(define (al-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (car b))) (al-aug (cdr rows) (cdr b)))))
(define (al-drop-zero-rows rows m) (if (null? rows) (quote ()) (if (al-row-zero? (car rows) m 0) (al-drop-zero-rows (cdr rows) m) (cons (car rows) (al-drop-zero-rows (cdr rows) m)))))
(define (al-row-zero? row m i) (cond ((= i m) #t) ((= (al-vnth row i) 0) (al-row-zero? row m (+ i 1))) (else #f)))
(define (al-reduce work m c piv) (if (= c m) (al-read piv m 0 (quote ())) (al-reduce-step work m c piv (al-first-with-col work c))))
(define (al-reduce-step work m c piv pr) (if (equal? pr (quote none)) (al-reduce work m (+ c 1) piv) (al-reduce-pivot work m c piv (al-scale-row pr (/ 1 (al-vnth pr c))))))
(define (al-reduce-pivot work m c piv prn) (al-reduce (al-elim-others (al-remove-row work prn) prn c) m (+ c 1) (cons (cons c prn) (al-elim-piv piv prn c))))
(define (al-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (al-axpy (cdr (car piv)) prn (- 0 (al-vnth (cdr (car piv)) c)))) (al-elim-piv (cdr piv) prn c))))
(define (al-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (al-vnth (car work) c) 0)) (car work)) (else (al-first-with-col (cdr work) c))))
(define (al-remove-row work prn) (al-rr-go work prn #f))
(define (al-rr-go work prn removed) (cond ((null? work) (quote ())) ((if (not removed) (al-eq-row? (car work) prn) #f) (al-rr-go (cdr work) prn #t)) (else (cons (car work) (al-rr-go (cdr work) prn removed)))))
(define (al-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (al-eq-row? (cdr a) (cdr b)) #f)))
(define (al-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (al-scale-row (cdr row) s))))
(define (al-elim-others work prn c) (if (null? work) (quote ()) (cons (al-axpy (car work) prn (- 0 (al-vnth (car work) c))) (al-elim-others (cdr work) prn c))))
(define (al-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (al-axpy (cdr row) (cdr prn) f))))
(define (al-read piv m j acc) (if (= j m) (al-reverse acc) (al-read piv m (+ j 1) (cons (al-readval (al-piv-for piv j) m) acc))))
(define (al-readval pr m) (if (equal? pr (quote none)) 0 (al-vnth pr m)))
(define (al-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (al-piv-for (cdr piv) j))))

; ----- top-level -----
(define (al-integrate g n tp B degbound) (al-int-result (al-iter g n tp B 0 degbound)))
(define (al-int-result Q) (if (equal? Q (quote none)) (quote no-elementary-form) (list (quote elementary) Q)))
(define (al-iter g n tp B d degbound) (if (> d degbound) (quote none) (al-iter-step g n tp B d degbound (al-solve g n tp B d))))
(define (al-iter-step g n tp B d degbound Q) (if (equal? Q (quote none)) (al-iter g n tp B (+ d 1) degbound) Q))
