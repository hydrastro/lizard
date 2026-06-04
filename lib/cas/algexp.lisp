; -*- lisp -*-
; lib/cas/algexp.lisp -- RUNG 5: an ENTANGLED tower -- the exponential of an ALGEBRAIC function, theta = exp(w)
; with w a field element of K = Q(x)[y]/(y^n - g).  Here the exponential sits genuinely ON TOP of the algebraic
; layer: its logarithmic derivative theta'/theta = w' is itself a FIELD element (e.g. exp(sqrt x), where
; w' = 1/(2 sqrt x) lives in K), not merely a rational function as in the earlier mixed cases
; (docs/TRAGER_ROADMAP.md, Rung 5).
;
; We integrate INT B * theta dx where B in K and theta = exp(w), w in K.  The integral is A theta for a field
; element A iff (A theta)' = B theta, i.e.
;     A' + w' A = B
; -- the Risch differential equation over K, but now with a FIELD-ELEMENT coefficient w'.  Because w' A is a
; full field product (sf-product), the y-power sectors COUPLE: this is one coupled linear system in all the
; sector coefficients of A, unlike the decoupled sector RDEs of the rational-coefficient case (mixedexpn.lisp).
; For A sought with polynomial sector entries of bounded degree, undetermined coefficients gives one exact
; linear system; the assembled A is certified by A' + w' A = B in the field.
;
; Public:
;   ae-wprime g n w        -> w' = d/dx w in K  (the field-element logarithmic derivative of theta = exp(w))
;   ae-rhs g n wp A        -> B = A' + w' A in K  (constructive direction; builds integrands)
;   ae-certify g n wp A B  -> #t iff A' + w' A = B in K  (the arbiter: INT B exp(w) dx = A exp(w))
;   ae-solve g n wp B deg  -> A | 'none : solve the coupled RDE with each sector polynomial of degree <= deg,
;                             certificate-checked
;   ae-integrate g n wp B degbound -> (list 'elementary A) | 'no-elementary-exp-part
;
; Verified: INT (1/(2 sqrt x)) exp(sqrt x) dx = exp(sqrt x); INT ((1+sqrt x)/(2 sqrt x)) exp(sqrt x) dx =
; sqrt(x) exp(sqrt x); both on y^2 = x (w = sqrt x), and a y^2-involving case on y^3 = x.
;
; Builds on sefield.lisp (the field, its derivation and product) and poly.lisp / tower.lisp.

(import "cas/sefield.lisp")

(define (ae-nth l k) (if (= k 0) (car l) (ae-nth (cdr l) (- k 1))))

; the field-element logarithmic derivative of theta = exp(w): w' = d/dx w
(define (ae-wprime g n w) (sf-deriv g n w))

; ----- constructive direction: B = A' + w' A  (wp = w', a field element) -----
(define (ae-rhs g n wp A) (sf-add (sf-deriv g n A) (sf-product g n wp A)))

; ----- certificate: A' + w' A = B in K -----
(define (ae-certify g n wp A B) (sf-equal? (ae-rhs g n wp A) B))

; ===== solver: A = sum_j a_j y^j with each a_j a polynomial of degree <= deg.  Because w' A couples sectors,
; this is ONE coupled linear system in all (n)*(deg+1) coefficients.  Undetermined coefficients + the proven
; Gauss-Jordan, residual computed once for the zero vector. =====
(define (ae-unknowns n deg) (* n (+ deg 1)))
; build A (length-n field element) from the flat coefficient vector cs: sector s gets (deg+1) coeffs
(define (ae-build n deg cs) (ae-build-go n deg cs 0))
(define (ae-build-go n deg cs s) (if (>= s n) (quote ()) (cons (rat-from-poly (ae-take (ae-drop cs (* s (+ deg 1))) (+ deg 1))) (ae-build-go n deg cs (+ s 1)))))
(define (ae-take l k) (if (= k 0) (quote ()) (if (null? l) (cons 0 (ae-take (quote ()) (- k 1))) (cons (car l) (ae-take (cdr l) (- k 1))))))
(define (ae-drop l k) (if (= k 0) l (if (null? l) (quote ()) (ae-drop (cdr l) (- k 1)))))

; residual vector: the field element (A' + w' A - B) must be the zero element, i.e. every sector's rational
; function vanishes.  Because rat auto-reduction makes raw numerator coefficients a NON-linear function of cs
; (denominators vary across perturbations), we instead require each sector rational to vanish at a set of
; numeric sample points -- the residual value at a point is a genuinely linear functional of cs (denominators
; are nonzero constants there), giving a clean linear system.  Enough distinct points over-determine it; the
; final field certificate (ae-certify) is the exact arbiter regardless.
(define (ae-resid g n wp deg cs B) (ae-flatten-eval (ae-elt-sub (ae-rhs g n wp (ae-build n deg cs)) B)))
(define (ae-elt-sub C D) (if (null? C) (quote ()) (cons (rat-sub (car C) (car D)) (ae-elt-sub (cdr C) (cdr D)))))
; sample points (avoid 0 and roots of g for the small models we use): 2,3,5,7,11,13,17,19,23,29
(define (ae-points) (list 2 3 5 7 11 13 17 19 23 29))
(define (ae-flatten-eval C) (if (null? C) (quote ()) (append (ae-eval-rat (car C) (ae-points)) (ae-flatten-eval (cdr C)))))
(define (ae-eval-rat r pts) (if (null? pts) (quote ()) (cons (/ (poly-eval (rat-num r) (car pts)) (poly-eval (rat-den r) (car pts))) (ae-eval-rat r (cdr pts)))))

(define (ae-zeros m) (if (= m 0) (quote ()) (cons 0 (ae-zeros (- m 1)))))
(define (ae-unit m j) (ae-unit-go m j 0))
(define (ae-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (ae-unit-go m j (+ i 1)))))
(define (ae-vsub a b) (if (null? a) (quote ()) (cons (- (car a) (car b)) (ae-vsub (cdr a) (cdr b)))))
(define (ae-vneg a) (if (null? a) (quote ()) (cons (- 0 (car a)) (ae-vneg (cdr a)))))
(define (ae-len l) (if (null? l) 0 (+ 1 (ae-len (cdr l)))))

(define (ae-solve g n wp B deg) (ae-solve-r0 g n wp B deg (ae-unknowns n deg)))
(define (ae-solve-r0 g n wp B deg m) (ae-solve-with g n wp B deg m (ae-resid g n wp deg (ae-zeros m) B)))
(define (ae-solve-with g n wp B deg m r0) (ae-finish g n wp B deg m (ae-lin-solve (ae-cols g n wp B deg m r0 0 (quote ())) (ae-vneg r0) (ae-len r0) m)))
(define (ae-finish g n wp B deg m sol) (if (equal? sol (quote none)) (quote none) (ae-finish2 g n wp B (ae-build n deg sol))))
(define (ae-finish2 g n wp B A) (if (ae-certify g n wp A B) A (quote none)))
(define (ae-cols g n wp B deg m r0 j acc) (if (= j m) (reverse acc) (ae-cols g n wp B deg m r0 (+ j 1) (cons (ae-vsub (ae-resid g n wp deg (ae-unit m j) B) r0) acc))))

; ----- exact linear solver: columns -> rows, drop-zero, full Gauss-Jordan (flattened helpers) -----
(define (ae-lin-solve cols b rows m) (ae-reduce (ae-drop-zero-rows (ae-aug (ae-rows-from-cols cols rows m) b) m) m 0 (quote ())))
(define (ae-rows-from-cols cols rows m) (ae-rfc cols rows 0))
(define (ae-rfc cols rows i) (if (= i rows) (quote ()) (cons (ae-rowi cols i) (ae-rfc cols rows (+ i 1)))))
(define (ae-rowi cols i) (if (null? cols) (quote ()) (cons (ae-vnth (car cols) i) (ae-rowi (cdr cols) i))))
(define (ae-vnth v i) (if (= i 0) (car v) (ae-vnth (cdr v) (- i 1))))
(define (ae-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (car b))) (ae-aug (cdr rows) (cdr b)))))
(define (ae-drop-zero-rows rows m) (if (null? rows) (quote ()) (if (ae-row-zero? (car rows) m 0) (ae-drop-zero-rows (cdr rows) m) (cons (car rows) (ae-drop-zero-rows (cdr rows) m)))))
(define (ae-row-zero? row m i) (cond ((= i m) #t) ((= (ae-vnth row i) 0) (ae-row-zero? row m (+ i 1))) (else #f)))
(define (ae-reduce work m c piv) (if (= c m) (ae-read piv m 0 (quote ())) (ae-reduce-step work m c piv (ae-first-with-col work c))))
(define (ae-reduce-step work m c piv pr) (if (equal? pr (quote none)) (ae-reduce work m (+ c 1) piv) (ae-reduce-pivot work m c piv (ae-scale-row pr (/ 1 (ae-vnth pr c))))))
(define (ae-reduce-pivot work m c piv prn) (ae-reduce (ae-elim-others (ae-remove-row work prn) prn c) m (+ c 1) (cons (cons c prn) (ae-elim-piv piv prn c))))
(define (ae-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (ae-axpy (cdr (car piv)) prn (- 0 (ae-vnth (cdr (car piv)) c)))) (ae-elim-piv (cdr piv) prn c))))
(define (ae-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (ae-vnth (car work) c) 0)) (car work)) (else (ae-first-with-col (cdr work) c))))
(define (ae-remove-row work prn) (ae-rr-go work prn #f))
(define (ae-rr-go work prn removed) (cond ((null? work) (quote ())) ((if (not removed) (ae-eq-row? (car work) prn) #f) (ae-rr-go (cdr work) prn #t)) (else (cons (car work) (ae-rr-go (cdr work) prn removed)))))
(define (ae-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (ae-eq-row? (cdr a) (cdr b)) #f)))
(define (ae-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (ae-scale-row (cdr row) s))))
(define (ae-elim-others work prn c) (if (null? work) (quote ()) (cons (ae-axpy (car work) prn (- 0 (ae-vnth (car work) c))) (ae-elim-others (cdr work) prn c))))
(define (ae-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (ae-axpy (cdr row) (cdr prn) f))))
(define (ae-read piv m j acc) (if (= j m) (reverse acc) (ae-read piv m (+ j 1) (cons (ae-readval (ae-piv-for piv j) m) acc))))
(define (ae-readval pr m) (if (equal? pr (quote none)) 0 (ae-vnth pr m)))
(define (ae-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (ae-piv-for (cdr piv) j))))

; ----- top-level: search bounded sector degree -----
(define (ae-integrate g n wp B degbound) (ae-int-result (ae-iter g n wp B 0 degbound)))
(define (ae-int-result A) (if (equal? A (quote none)) (quote no-elementary-exp-part) (list (quote elementary) A)))
(define (ae-iter g n wp B d degbound) (if (> d degbound) (quote none) (ae-iter-step g n wp B d degbound (ae-solve g n wp B d))))
(define (ae-iter-step g n wp B d degbound A) (if (equal? A (quote none)) (ae-iter g n wp B (+ d 1) degbound) A))
