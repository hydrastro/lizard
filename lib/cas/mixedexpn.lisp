; -*- lisp -*-
; lib/cas/mixedexpn.lisp -- RUNG 5: MIXED EXPONENTIAL-OVER-ALGEBRAIC integration over the GENERAL superelliptic
; field K = Q(x)[y]/(y^n - g) for arbitrary n.  This lifts the n=2 case (mixedexp.lisp, the sqrt field) to any
; degree, using the general-n field arithmetic of sefield.lisp (docs/TRAGER_ROADMAP.md, Rung 5).
;
; We integrate INT B * exp(h) dx where B is a field element of K and h is rational.  The Risch exponential case:
; the integral is A exp(h) for a field element A iff A' + h' A = B in K.  Writing A = sum_j a_j(x) y^j, the
; derivation of sefield gives A' = sum_j [a_j' + a_j (j/n) g'/g] y^j -- each y^j sector is preserved -- so
;     A' + h' A = sum_j [ a_j' + ((j/n) g'/g + h') a_j ] y^j,
; and matching B = sum_j B_j y^j gives n INDEPENDENT scalar Risch differential equations over Q(x):
;     a_j' + w_j a_j = B_j,    w_j = (j/n) g'/g + h',   j = 0 .. n-1.
; The sectors DECOUPLE completely (the exponential does not couple y-powers, unlike a primitive tower).  For a_j
; sought as a polynomial of bounded degree, each sector is solved by undetermined coefficients (an exact linear
; system on the coefficients), and the assembled A is certified by differentiating in K and checking A'+h'A = B.
;
; Public:
;   mxn-rhs g n hp A        -> B = A' + h' A in K (constructive direction; builds integrands)
;   mxn-certify g n hp A B  -> #t iff A' + h' A = B in K  (the arbiter: INT B exp(h) dx = A exp(h))
;   mxn-solve g n hp B deg  -> A | 'none : solve the RDE over K with each a_j a polynomial of degree <= deg, by
;                              the per-sector undetermined-coefficients method; certificate-checked.
;   mxn-integrate g n hp B degbound -> (list 'elementary A) | 'no-elementary-exp-part
;
; Verified: INT ((1 + x^2/(x^3+1)) y) exp(x) dx = y exp(x) on y^3 = x^3+1 (y = (x^3+1)^(1/3)); a y^2-sector case
; on the same curve; and the n=2 specialization reproducing the sqrt-field result of mixedexp.lisp.
;
; Builds on sefield.lisp (the general-n field) and poly.lisp / tower.lisp.

(import "cas/sefield.lisp")

(define (mxn-nth l k) (if (= k 0) (car l) (mxn-nth (cdr l) (- k 1))))

; ----- constructive direction: B = A' + h' A  (hp = h', a rational function) -----
(define (mxn-rhs g n hp A) (sf-add (sf-deriv g n A) (sf-scale hp A)))

; ----- certificate: A' + h' A = B in K -----
(define (mxn-certify g n hp A B) (sf-equal? (mxn-rhs g n hp A) B))

; ----- the per-sector RDE coefficient w_j = (j/n) g'/g + h' -----
(define (mxn-wj g n hp j) (rat-add (rat-mul (rat-make (list j) (list n)) (rat-make (poly-deriv g) g)) hp))

; ===== solve A' + h' A = B by solving each sector RDE a_j' + w_j a_j = B_j independently =====
; A is assembled from the per-sector solutions; if any sector has no bounded-degree polynomial solution -> 'none.
(define (mxn-solve g n hp B deg)
  (let ((A (mxn-solve-sectors g n hp B 0 deg (quote ()))))
    (if (equal? A (quote none)) (quote none)
        (if (mxn-certify g n hp A B) A (quote none)))))     ; final field certificate
(define (mxn-solve-sectors g n hp B j deg acc)
  (if (>= j n) (reverse acc)
      (let ((aj (mxn-solve-sector (mxn-wj g n hp j) (mxn-nth B j) deg)))
        (if (equal? aj (quote none)) (quote none)
            (mxn-solve-sectors g n hp B (+ j 1) deg (cons aj acc))))))

; one sector: solve a' + w a = Bj for a polynomial a of degree <= deg (a returned as a rational function).
; undetermined coefficients: a = sum_{i=0}^deg c_i x^i; residual a' + w a - Bj is rational; clear and match.
(define (mxn-solve-sector w Bj deg)
  (let ((m (+ deg 1)))
    (let ((r0 (mxn-resid w Bj deg (mxn-zeros m))))
      (let ((cols (mxn-cols w Bj deg m 0 (quote ()))))
        (let ((sol (mxn-lin-solve cols (mxn-vneg r0) (mxn-len r0) m)))
          (if (equal? sol (quote none)) (quote none)
              (let ((a (rat-from-poly (mxn-trimz sol))))
                (if (mxn-sector-ok? w Bj a) a (quote none))))))))) ; per-sector check
(define (mxn-sector-ok? w Bj a) (rat-zero? (rat-sub (rat-add (rat-deriv a) (rat-mul w a)) Bj)))
(define (mxn-trimz l) (mxn-trimz-go (reverse l)))
(define (mxn-trimz-go r) (cond ((null? r) (list 0)) ((= (car r) 0) (mxn-trimz-go (cdr r))) (else (reverse r))))

; residual vector: numerator coefficients of (a' + w a - Bj), a built from coefficient vector cs, padded
(define (mxn-resid w Bj deg cs)
  (let ((a (rat-from-poly (mxn-vec->poly cs))))
    (mxn-cl (rat-sub (rat-add (rat-deriv a) (rat-mul w a)) Bj))))
(define (mxn-vec->poly cs) cs)
(define (mxn-cl r) (mxn-pad (poly-norm (rat-num r)) 12))
(define (mxn-pad l k) (if (= k 0) (quote ()) (cons (if (null? l) 0 (car l)) (mxn-pad (if (null? l) (quote ()) (cdr l)) (- k 1)))))

(define (mxn-zeros m) (if (= m 0) (quote ()) (cons 0 (mxn-zeros (- m 1)))))
(define (mxn-unit m j) (mxn-unit-go m j 0))
(define (mxn-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (mxn-unit-go m j (+ i 1)))))
(define (mxn-vsub a b) (if (null? a) (quote ()) (cons (- (car a) (car b)) (mxn-vsub (cdr a) (cdr b)))))
(define (mxn-vneg a) (if (null? a) (quote ()) (cons (- 0 (car a)) (mxn-vneg (cdr a)))))
(define (mxn-len l) (if (null? l) 0 (+ 1 (mxn-len (cdr l)))))
(define (mxn-cols w Bj deg m j acc)
  (if (= j m) (reverse acc)
      (mxn-cols w Bj deg m (+ j 1) (cons (mxn-vsub (mxn-resid w Bj deg (mxn-unit m j)) (mxn-resid w Bj deg (mxn-zeros m))) acc))))

; ----- exact linear solver (the proven columns -> rows, drop-zero, full Gauss-Jordan from mixedexp) -----
(define (mxn-lin-solve cols b rows m)
  (let ((rowsM (mxn-aug (mxn-rows-from-cols cols rows m) b)))
    (let ((work (mxn-drop-zero-rows rowsM m)))
      (mxn-reduce work m 0 (quote ())))))
(define (mxn-rows-from-cols cols rows m) (mxn-rfc cols rows 0))
(define (mxn-rfc cols rows i) (if (= i rows) (quote ()) (cons (mxn-rowi cols i) (mxn-rfc cols rows (+ i 1)))))
(define (mxn-rowi cols i) (if (null? cols) (quote ()) (cons (mxn-vnth (car cols) i) (mxn-rowi (cdr cols) i))))
(define (mxn-vnth v i) (if (= i 0) (car v) (mxn-vnth (cdr v) (- i 1))))
(define (mxn-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (car b))) (mxn-aug (cdr rows) (cdr b)))))
(define (mxn-drop-zero-rows rows m) (if (null? rows) (quote ()) (if (mxn-row-zero? (car rows) m 0) (mxn-drop-zero-rows (cdr rows) m) (cons (car rows) (mxn-drop-zero-rows (cdr rows) m)))))
(define (mxn-row-zero? row m i) (cond ((= i m) #t) ((= (mxn-vnth row i) 0) (mxn-row-zero? row m (+ i 1))) (else #f)))
(define (mxn-reduce work m c piv)
  (if (= c m) (mxn-read piv m 0 (quote ()))
      (let ((pr (mxn-first-with-col work c)))
        (if (equal? pr (quote none)) (mxn-reduce work m (+ c 1) piv)
            (let ((prn (mxn-scale-row pr (/ 1 (mxn-vnth pr c)))))
              (mxn-reduce (mxn-elim-others (mxn-remove-row work pr) prn c) m (+ c 1) (cons (cons c prn) (mxn-elim-piv piv prn c))))))))
(define (mxn-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (mxn-axpy (cdr (car piv)) prn (- 0 (mxn-vnth (cdr (car piv)) c)))) (mxn-elim-piv (cdr piv) prn c))))
(define (mxn-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (mxn-vnth (car work) c) 0)) (car work)) (else (mxn-first-with-col (cdr work) c))))
(define (mxn-remove-row work pr) (mxn-rr-go work pr #f))
(define (mxn-rr-go work pr removed) (cond ((null? work) (quote ())) ((if (not removed) (mxn-eq-row? (car work) pr) #f) (mxn-rr-go (cdr work) pr #t)) (else (cons (car work) (mxn-rr-go (cdr work) pr removed)))))
(define (mxn-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (mxn-eq-row? (cdr a) (cdr b)) #f)))
(define (mxn-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (mxn-scale-row (cdr row) s))))
(define (mxn-elim-others work prn c) (if (null? work) (quote ()) (cons (mxn-axpy (car work) prn (- 0 (mxn-vnth (car work) c))) (mxn-elim-others (cdr work) prn c))))
(define (mxn-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (mxn-axpy (cdr row) (cdr prn) f))))
(define (mxn-read piv m j acc) (if (= j m) (reverse acc) (let ((pr (mxn-piv-for piv j))) (mxn-read piv m (+ j 1) (cons (if (equal? pr (quote none)) 0 (mxn-vnth pr m)) acc)))))
(define (mxn-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (mxn-piv-for (cdr piv) j))))

; ----- top-level: search bounded sector-degree -----
(define (mxn-integrate g n hp B degbound) (let ((A (mxn-iter g n hp B 0 degbound))) (if (equal? A (quote none)) (quote no-elementary-exp-part) (list (quote elementary) A))))
(define (mxn-iter g n hp B d degbound) (if (> d degbound) (quote none) (let ((A (mxn-solve g n hp B d))) (if (equal? A (quote none)) (mxn-iter g n hp B (+ d 1) degbound) A))))
