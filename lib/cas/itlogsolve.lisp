; -*- lisp -*-
; lib/cas/itlogsolve.lisp -- RUNG 5: a GENERAL integrator for the iterated LOGARITHM tower (itlog.lisp), the
; reciprocal-mirror counterpart of itexpsolve.lisp.  Given an arbitrary Laurent element B = sum of monomials
; c(x) L_1^{a_1}...L_n^{a_n} (integer exponents) of the depth-n iterated-log tower, find an antiderivative E in
; the tower by undetermined coefficients and certify it by differentiating in the tower
; (docs/TRAGER_ROADMAP.md, Rung 5).
;
; Method (mirror of itexpsolve, with the derivation's prefix-LOWERING replaced by prefix-RAISING in the
; candidate construction).  il-deriv sends a monomial M of the answer to a small set of monomials whose
; exponents are M's first-k prefixes LOWERED (scaled by a_k/x); so the candidate ANSWER monomials are B's
; monomials with one-step prefixes RAISED (the inverse), together with B's own monomials.  Posit E over that
; support with unknown rational coefficients, apply il-deriv, and require d/dx(E) - B = 0 as a linear system in
; the coefficients (evaluated coefficient-wise over the finite monomial universe at sample x-points to get an
; exact rational system), solved with Gauss-Jordan; the certificate is the final arbiter, so a spurious
; candidate simply gets coefficient 0.
;
; Public:
;   ils-candidates n B      -> the candidate answer-monomial support (B's monomials plus prefix-raised forms)
;   ils-solve n B           -> E | 'none : an antiderivative of B in the iterated-log tower, certificate-checked
;   ils-integrate n B       -> (list 'elementary E) | 'no-elementary-form
;
; Verified: INT (2 log(log x)/(x log x)) dx = (log log x)^2; INT (1/x + 1/(x log x)) dx = log x + log log x
; (a genuine two-term answer); the structured INT 1/(x L_1...L_{n-1}) dx = L_n recovered as a special case; and
; a soundness control.
;
; Builds on itlog.lisp (the iterated-log derivation and certificate) and poly.lisp.

(import "cas/itlog.lisp")
(import "cas/poly.lisp")

(define (ils-nth l k) (if (= k 0) (car l) (ils-nth (cdr l) (- k 1))))
(define (ils-len l) (if (null? l) 0 (+ 1 (ils-len (cdr l)))))

; ----- candidate support: B's monomials, plus for each the prefix-RAISED forms (raise the first k entries by
; one, k = 1..n) -- the inverse of the lowering that il-deriv performs -----
(define (ils-raise-prefix a k) (ils-rp-go a k 0))
(define (ils-rp-go a k p) (if (null? a) (quote ()) (cons (if (< p k) (+ (car a) 1) (car a)) (ils-rp-go (cdr a) k (+ p 1)))))
(define (ils-vec-eq? a b) (cond ((null? a) (null? b)) ((null? b) #f) ((= (car a) (car b)) (ils-vec-eq? (cdr a) (cdr b))) (else #f)))

(define (ils-candidates n B) (ils-dedup (ils-cand-go n B)))
(define (ils-cand-go n B) (if (null? B) (quote ()) (ils-app (ils-cand-one n (il-mvec (car B))) (ils-cand-go n (cdr B)))))
(define (ils-cand-one n v) (cons v (ils-raises n v 1)))
(define (ils-raises n v k) (if (> k n) (quote ()) (cons (ils-raise-prefix v k) (ils-raises n v (+ k 1)))))
(define (ils-app a b) (if (null? a) b (cons (car a) (ils-app (cdr a) b))))
(define (ils-dedup l) (ils-dedup-go l (quote ())))
(define (ils-dedup-go l seen) (cond ((null? l) (ils-reverse seen)) ((ils-member? (car l) seen) (ils-dedup-go (cdr l) seen)) (else (ils-dedup-go (cdr l) (cons (car l) seen)))))
(define (ils-member? v seen) (cond ((null? seen) #f) ((ils-vec-eq? v (car seen)) #t) (else (ils-member? v (cdr seen)))))
(define (ils-reverse l) (ils-rev l (quote ())))
(define (ils-rev l acc) (if (null? l) acc (ils-rev (cdr l) (cons (car l) acc))))

; ----- build a candidate element from a coefficient vector over the support -----
(define (ils-build support cs) (if (null? support) (quote ()) (cons (cons (rat-from-poly (list (car cs))) (car support)) (ils-build (cdr support) (cdr cs)))))

; ----- monomial universe = B's monomials union d/dx(support) -----
(define (ils-universe n support B) (ils-dedup (ils-app (ils-vecs B) (ils-vecs (il-deriv n (ils-build support (ils-ones (ils-len support))))))))
(define (ils-vecs E) (if (null? E) (quote ()) (cons (il-mvec (car E)) (ils-vecs (cdr E)))))
(define (ils-ones k) (if (= k 0) (quote ()) (cons 1 (ils-ones (- k 1)))))
(define (ils-coeff-of E v) (cond ((null? E) (rat-zero)) ((ils-vec-eq? (il-mvec (car E)) v) (il-mcoeff (car E))) (else (ils-coeff-of (cdr E) v))))

; ----- residual over the monomial universe, evaluated at sample x-points (exact rational system) -----
(define (ils-points) (list 2 3 5 7 11 13 17 19 23 29))
(define (ils-resid n support universe B cs) (ils-flatten (il-deriv n (ils-build support cs)) B universe))
(define (ils-flatten dE B universe) (if (null? universe) (quote ()) (ils-app (ils-eval-rat (rat-sub (ils-coeff-of dE (car universe)) (ils-coeff-of B (car universe))) (ils-points)) (ils-flatten dE B (cdr universe)))))
(define (ils-eval-rat r pts) (if (null? pts) (quote ()) (cons (/ (poly-eval (rat-num r) (car pts)) (poly-eval (rat-den r) (car pts))) (ils-eval-rat r (cdr pts)))))

(define (ils-zeros m) (if (= m 0) (quote ()) (cons 0 (ils-zeros (- m 1)))))
(define (ils-unit m j) (ils-unit-go m j 0))
(define (ils-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (ils-unit-go m j (+ i 1)))))
(define (ils-vsub a b) (if (null? a) (quote ()) (cons (- (car a) (car b)) (ils-vsub (cdr a) (cdr b)))))
(define (ils-vneg a) (if (null? a) (quote ()) (cons (- 0 (car a)) (ils-vneg (cdr a)))))

(define (ils-solve n B) (ils-solve-sup n B (ils-candidates n B)))
(define (ils-solve-sup n B support) (ils-solve-u n B support (ils-universe n support B)))
(define (ils-solve-u n B support universe) (ils-solve-m n B support universe (ils-len support)))
(define (ils-solve-m n B support universe m) (ils-solve-r0 n B support universe m (ils-resid n support universe B (ils-zeros m))))
(define (ils-solve-r0 n B support universe m r0) (ils-finish n B support (ils-lin-solve (ils-cols n support universe B m r0 0 (quote ())) (ils-vneg r0) (ils-len r0) m)))
(define (ils-finish n B support sol) (if (equal? sol (quote none)) (quote none) (ils-finish2 n B (ils-build support sol))))
(define (ils-finish2 n B E) (if (il-certify n E B) (il-collect E) (quote none)))
(define (ils-cols n support universe B m r0 j acc) (if (= j m) (ils-reverse acc) (ils-cols n support universe B m r0 (+ j 1) (cons (ils-vsub (ils-resid n support universe B (ils-unit m j)) r0) acc))))

; ----- exact linear solver (proven full Gauss-Jordan, flattened) -----
(define (ils-lin-solve cols b rows m) (ils-reduce (ils-drop-zero-rows (ils-aug (ils-rows-from-cols cols rows m) b) m) m 0 (quote ())))
(define (ils-rows-from-cols cols rows m) (ils-rfc cols rows 0))
(define (ils-rfc cols rows i) (if (= i rows) (quote ()) (cons (ils-rowi cols i) (ils-rfc cols rows (+ i 1)))))
(define (ils-rowi cols i) (if (null? cols) (quote ()) (cons (ils-vnth (car cols) i) (ils-rowi (cdr cols) i))))
(define (ils-vnth v i) (if (= i 0) (car v) (ils-vnth (cdr v) (- i 1))))
(define (ils-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (car b))) (ils-aug (cdr rows) (cdr b)))))
(define (ils-drop-zero-rows rows m) (if (null? rows) (quote ()) (if (ils-row-zero? (car rows) m 0) (ils-drop-zero-rows (cdr rows) m) (cons (car rows) (ils-drop-zero-rows (cdr rows) m)))))
(define (ils-row-zero? row m i) (cond ((= i m) #t) ((= (ils-vnth row i) 0) (ils-row-zero? row m (+ i 1))) (else #f)))
(define (ils-reduce work m c piv) (if (= c m) (ils-read piv m 0 (quote ())) (ils-reduce-step work m c piv (ils-first-with-col work c))))
(define (ils-reduce-step work m c piv pr) (if (equal? pr (quote none)) (ils-reduce work m (+ c 1) piv) (ils-reduce-pivot work m c piv (ils-scale-row pr (/ 1 (ils-vnth pr c))))))
(define (ils-reduce-pivot work m c piv prn) (ils-reduce (ils-elim-others (ils-remove-row work prn) prn c) m (+ c 1) (cons (cons c prn) (ils-elim-piv piv prn c))))
(define (ils-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (ils-axpy (cdr (car piv)) prn (- 0 (ils-vnth (cdr (car piv)) c)))) (ils-elim-piv (cdr piv) prn c))))
(define (ils-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (ils-vnth (car work) c) 0)) (car work)) (else (ils-first-with-col (cdr work) c))))
(define (ils-remove-row work prn) (ils-rr-go work prn #f))
(define (ils-rr-go work prn removed) (cond ((null? work) (quote ())) ((if (not removed) (ils-eq-row? (car work) prn) #f) (ils-rr-go (cdr work) prn #t)) (else (cons (car work) (ils-rr-go (cdr work) prn removed)))))
(define (ils-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (ils-eq-row? (cdr a) (cdr b)) #f)))
(define (ils-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (ils-scale-row (cdr row) s))))
(define (ils-elim-others work prn c) (if (null? work) (quote ()) (cons (ils-axpy (car work) prn (- 0 (ils-vnth (car work) c))) (ils-elim-others (cdr work) prn c))))
(define (ils-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (ils-axpy (cdr row) (cdr prn) f))))
(define (ils-read piv m j acc) (if (= j m) (ils-reverse acc) (ils-read piv m (+ j 1) (cons (ils-readval (ils-piv-for piv j) m) acc))))
(define (ils-readval pr m) (if (equal? pr (quote none)) 0 (ils-vnth pr m)))
(define (ils-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (ils-piv-for (cdr piv) j))))

; ----- top-level -----
(define (ils-integrate n B) (ils-int-result (ils-solve n B)))
(define (ils-int-result E) (if (equal? E (quote none)) (quote no-elementary-form) (list (quote elementary) E)))
