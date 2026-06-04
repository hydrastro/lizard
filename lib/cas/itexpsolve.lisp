; -*- lisp -*-
; lib/cas/itexpsolve.lisp -- RUNG 5, going deeper: a GENERAL integrator for the iterated exponential tower
; (itexp.lisp), beyond the single full-product identity.  Given an arbitrary element B = sum of monomials
; c(x) E_1^{a_1}...E_n^{a_n} of the depth-n tower Q(x)(E_1)...(E_n), find an antiderivative E in the tower (if
; one exists with the same monomial support and constant rational coefficients) by undetermined coefficients,
; and certify it by differentiating in the tower (docs/TRAGER_ROADMAP.md, Rung 5).
;
; Method.  ie-deriv (from itexp) sends each monomial of the answer to a small set of monomials (the in-place c'
; term plus, for each exponent k that is positive, a copy with E_1..E_{k-1} raised by one).  For integrands and
; answers with CONSTANT rational coefficients, d/dx acts LINEARLY on the (finite) set of monomials appearing.
; So: take the candidate monomial support to be the set of monomials in B together with, for each, the monomials
; obtained by LOWERING a one-step prefix (the inverse of the raising the derivation performs) -- in practice the
; support of B plus the "primitive" monomials whose derivative lands in B.  Posit E = sum u_i M_i over that
; support with unknown rational u_i, apply ie-deriv, and require d/dx(E) - B = 0 as a linear system in the u_i,
; solved exactly with the Gauss-Jordan solver; the certificate confirms the result.
;
; To keep the candidate set finite and well-chosen we use the monomials of B themselves as the answer support
; (which suffices for the integrals whose answer monomials are exactly the "one-derivative-down" forms of B's
; monomials, e.g. INT(E_1 + E_1 E_2) = E_1 + E_2, INT(E_1 E_2 + E_1^2 E_2) = E_1 E_2) together with their
; prefix-lowered forms; the certificate is the final arbiter, so a spurious candidate simply gets coefficient 0.
;
; Public:
;   ies-candidates n B      -> the candidate answer-monomial support (B's monomials plus prefix-lowered forms)
;   ies-solve n B           -> E | 'none : an antiderivative of B in the tower, certificate-checked
;   ies-integrate n B       -> (list 'elementary E) | 'no-elementary-form
;
; Verified: INT (E_1) = E_1; INT (E_1 + E_1 E_2) = E_1 + E_2; INT (E_1 E_2 + E_1^2 E_2) = E_1 E_2; the
; full-product INT (E_1...E_n) = E_n recovered as a special case; and a soundness control.
;
; Builds on itexp.lisp (the tower derivation and certificate) and poly.lisp.

(import "cas/itexp.lisp")
(import "cas/poly.lisp")

(define (ies-nth l k) (if (= k 0) (car l) (ies-nth (cdr l) (- k 1))))
(define (ies-len l) (if (null? l) 0 (+ 1 (ies-len (cdr l)))))

; ----- candidate support: the monomials of B, plus for each the prefix-lowered forms (lower the first k
; entries by one, for k = 1..n, when that keeps exponents >= 0) -- these are the monomials whose derivative can
; land on B's monomials. -----
(define (ies-lower-prefix a k) (ies-lp-go a k 0))
(define (ies-lp-go a k p) (if (null? a) (quote ()) (cons (if (< p k) (- (car a) 1) (car a)) (ies-lp-go (cdr a) k (+ p 1)))))
(define (ies-nonneg? a) (cond ((null? a) #t) ((< (car a) 0) #f) (else (ies-nonneg? (cdr a)))))
(define (ies-vec-eq? a b) (cond ((null? a) (null? b)) ((null? b) #f) ((= (car a) (car b)) (ies-vec-eq? (cdr a) (cdr b))) (else #f)))

(define (ies-candidates n B) (ies-dedup (ies-cand-go n B)))
(define (ies-cand-go n B) (if (null? B) (quote ()) (ies-app (ies-cand-one n (ie-mvec (car B))) (ies-cand-go n (cdr B)))))
(define (ies-cand-one n v) (cons v (ies-lowers n v 1)))
(define (ies-lowers n v k) (if (> k n) (quote ()) (ies-lower-add n v k (ies-lower-prefix v k))))
(define (ies-lower-add n v k lowered) (if (ies-nonneg? lowered) (cons lowered (ies-lowers n v (+ k 1))) (ies-lowers n v (+ k 1))))
(define (ies-app a b) (if (null? a) b (cons (car a) (ies-app (cdr a) b))))
(define (ies-dedup l) (ies-dedup-go l (quote ())))
(define (ies-dedup-go l seen) (cond ((null? l) (ies-reverse seen)) ((ies-member? (car l) seen) (ies-dedup-go (cdr l) seen)) (else (ies-dedup-go (cdr l) (cons (car l) seen)))))
(define (ies-member? v seen) (cond ((null? seen) #f) ((ies-vec-eq? v (car seen)) #t) (else (ies-member? v (cdr seen)))))
(define (ies-reverse l) (ies-rev l (quote ())))
(define (ies-rev l acc) (if (null? l) acc (ies-rev (cdr l) (cons (car l) acc))))

; ----- build a candidate element from a coefficient vector over the support -----
(define (ies-build support cs) (if (null? support) (quote ()) (cons (cons (rat-from-poly (list (car cs))) (car support)) (ies-build (cdr support) (cdr cs)))))

; ----- the monomials appearing in B and in d/dx(candidate); we evaluate the residual as exact rational
; coefficients keyed by monomial.  Collect the full monomial universe = B's monomials union d/dx(support). -----
(define (ies-universe n support B) (ies-dedup (ies-app (ies-vecs B) (ies-vecs (ie-deriv n (ies-build support (ies-ones (ies-len support))))))))
(define (ies-vecs E) (if (null? E) (quote ()) (cons (ie-mvec (car E)) (ies-vecs (cdr E)))))
(define (ies-ones k) (if (= k 0) (quote ()) (cons 1 (ies-ones (- k 1)))))

; coefficient of a given monomial vector in an element (a rational; 0 if absent)
(define (ies-coeff-of E v) (cond ((null? E) (rat-zero)) ((ies-vec-eq? (ie-mvec (car E)) v) (ie-mcoeff (car E))) (else (ies-coeff-of (cdr E) v))))

; ----- residual vector: for the candidate with coefficient vector cs, (d/dx E - B) evaluated coefficient-wise
; over the monomial universe, each entry a rational reduced to a number via a sample (the coefficients are
; rational functions of x; we test them at sample points to get a linear system over Q). -----
(define (ies-points) (list 2 3 5 7 11 13 17 19 23 29))
(define (ies-resid n support universe B cs) (ies-resid-go n support universe B (ies-build support cs)))
(define (ies-resid-go n support universe B E) (ies-flatten (ie-deriv n E) B universe))
(define (ies-flatten dE B universe) (if (null? universe) (quote ()) (ies-app (ies-eval-rat (rat-sub (ies-coeff-of dE (car universe)) (ies-coeff-of B (car universe))) (ies-points)) (ies-flatten dE B (cdr universe)))))
(define (ies-eval-rat r pts) (if (null? pts) (quote ()) (cons (/ (poly-eval (rat-num r) (car pts)) (poly-eval (rat-den r) (car pts))) (ies-eval-rat r (cdr pts)))))

(define (ies-zeros m) (if (= m 0) (quote ()) (cons 0 (ies-zeros (- m 1)))))
(define (ies-unit m j) (ies-unit-go m j 0))
(define (ies-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (ies-unit-go m j (+ i 1)))))
(define (ies-vsub a b) (if (null? a) (quote ()) (cons (- (car a) (car b)) (ies-vsub (cdr a) (cdr b)))))
(define (ies-vneg a) (if (null? a) (quote ()) (cons (- 0 (car a)) (ies-vneg (cdr a)))))

(define (ies-solve n B) (ies-solve-sup n B (ies-candidates n B)))
(define (ies-solve-sup n B support) (ies-solve-u n B support (ies-universe n support B)))
(define (ies-solve-u n B support universe) (ies-solve-m n B support universe (ies-len support)))
(define (ies-solve-m n B support universe m) (ies-solve-r0 n B support universe m (ies-resid n support universe B (ies-zeros m))))
(define (ies-solve-r0 n B support universe m r0) (ies-finish n B support (ies-lin-solve (ies-cols n support universe B m r0 0 (quote ())) (ies-vneg r0) (ies-len r0) m)))
(define (ies-finish n B support sol) (if (equal? sol (quote none)) (quote none) (ies-finish2 n B (ies-build support sol))))
(define (ies-finish2 n B E) (if (ie-certify n E B) (ie-collect E) (quote none)))
(define (ies-cols n support universe B m r0 j acc) (if (= j m) (ies-reverse acc) (ies-cols n support universe B m r0 (+ j 1) (cons (ies-vsub (ies-resid n support universe B (ies-unit m j)) r0) acc))))

; ----- exact linear solver (proven full Gauss-Jordan, flattened) -----
(define (ies-lin-solve cols b rows m) (ies-reduce (ies-drop-zero-rows (ies-aug (ies-rows-from-cols cols rows m) b) m) m 0 (quote ())))
(define (ies-rows-from-cols cols rows m) (ies-rfc cols rows 0))
(define (ies-rfc cols rows i) (if (= i rows) (quote ()) (cons (ies-rowi cols i) (ies-rfc cols rows (+ i 1)))))
(define (ies-rowi cols i) (if (null? cols) (quote ()) (cons (ies-vnth (car cols) i) (ies-rowi (cdr cols) i))))
(define (ies-vnth v i) (if (= i 0) (car v) (ies-vnth (cdr v) (- i 1))))
(define (ies-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (car b))) (ies-aug (cdr rows) (cdr b)))))
(define (ies-drop-zero-rows rows m) (if (null? rows) (quote ()) (if (ies-row-zero? (car rows) m 0) (ies-drop-zero-rows (cdr rows) m) (cons (car rows) (ies-drop-zero-rows (cdr rows) m)))))
(define (ies-row-zero? row m i) (cond ((= i m) #t) ((= (ies-vnth row i) 0) (ies-row-zero? row m (+ i 1))) (else #f)))
(define (ies-reduce work m c piv) (if (= c m) (ies-read piv m 0 (quote ())) (ies-reduce-step work m c piv (ies-first-with-col work c))))
(define (ies-reduce-step work m c piv pr) (if (equal? pr (quote none)) (ies-reduce work m (+ c 1) piv) (ies-reduce-pivot work m c piv (ies-scale-row pr (/ 1 (ies-vnth pr c))))))
(define (ies-reduce-pivot work m c piv prn) (ies-reduce (ies-elim-others (ies-remove-row work prn) prn c) m (+ c 1) (cons (cons c prn) (ies-elim-piv piv prn c))))
(define (ies-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (ies-axpy (cdr (car piv)) prn (- 0 (ies-vnth (cdr (car piv)) c)))) (ies-elim-piv (cdr piv) prn c))))
(define (ies-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (ies-vnth (car work) c) 0)) (car work)) (else (ies-first-with-col (cdr work) c))))
(define (ies-remove-row work prn) (ies-rr-go work prn #f))
(define (ies-rr-go work prn removed) (cond ((null? work) (quote ())) ((if (not removed) (ies-eq-row? (car work) prn) #f) (ies-rr-go (cdr work) prn #t)) (else (cons (car work) (ies-rr-go (cdr work) prn removed)))))
(define (ies-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (ies-eq-row? (cdr a) (cdr b)) #f)))
(define (ies-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (ies-scale-row (cdr row) s))))
(define (ies-elim-others work prn c) (if (null? work) (quote ()) (cons (ies-axpy (car work) prn (- 0 (ies-vnth (car work) c))) (ies-elim-others (cdr work) prn c))))
(define (ies-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (ies-axpy (cdr row) (cdr prn) f))))
(define (ies-read piv m j acc) (if (= j m) (ies-reverse acc) (ies-read piv m (+ j 1) (cons (ies-readval (ies-piv-for piv j) m) acc))))
(define (ies-readval pr m) (if (equal? pr (quote none)) 0 (ies-vnth pr m)))
(define (ies-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (ies-piv-for (cdr piv) j))))

; ----- top-level -----
(define (ies-integrate n B) (ies-int-result (ies-solve n B)))
(define (ies-int-result E) (if (equal? E (quote none)) (quote no-elementary-form) (list (quote elementary) E)))
