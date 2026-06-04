; -*- lisp -*-
; lib/cas/nestexp.lisp -- RUNG 5: the NESTED EXPONENTIAL tower Q(x)(s1)(s2) with s1 = exp(x) and
; s2 = exp(exp(x)) = exp(s1).  The multiplicative-tower counterpart of the nested logarithm (nestlog.lisp):
; here the second monomial's derivative s2' = s1' s2 = s1 s2 MULTIPLIES by the first monomial rather than
; dividing by it, so the coefficient ring stays POLYNOMIAL (a two-variable polynomial ring over Q(x)), but the
; derivation raises the s1-degree (docs/TRAGER_ROADMAP.md, Rung 5).
;
; Representation.  A tower element is a polynomial in s2 whose coefficients are polynomials in s1 with Q(x)
; (rational-function) coefficients: a list over s2-degree m of (a list over s1-degree k of Q(x) rationals),
; meaning sum_{m,k} c_{m,k}(x) s1^k s2^m.  From s1' = s1 and s2' = s1 s2, the derivation of a single term is
;     d/dx ( c s1^k s2^m ) = (c' + k c) s1^k s2^m  +  (m c) s1^{k+1} s2^m,
; i.e. WITHIN a fixed s2-degree m the s1-polynomial C_m is sent to ds1(C_m) + m * (s1-shift of C_m), where
; ds1(P) = sum (p_k' + k p_k) s1^k is the in-place exponential derivation and the s1-shift multiplies by s1.
; The s2-degree is preserved (s2' s2^{m-1} = s1 s2^m), so the derivation is block-diagonal across s2-degrees --
; which is exactly what makes the nested-exp integral solvable by undetermined coefficients.
;
; The clean nested-exp integral is INT exp(x) exp(exp(x)) dx = exp(exp(x)): the integrand is s1 s2 (the
; derivative of s2) and the answer is s2.  Every result is certified by differentiating in the tower
; (ne-certify), the differentiation certificate as the arbiter.
;
; Public:
;   ne-ds1 P                -> in-place exponential derivation of an s1-polynomial P (Q(x) coeffs): s1' = s1
;   ne-s1shift P            -> multiply an s1-polynomial by s1 (raise every s1-degree by one)
;   ne-deriv E              -> d/dx E in the tower (E an s2-poly of s1-polys)
;   ne-certify E B          -> #t iff ne-deriv E = B  (the arbiter: INT B dx = E)
;   ne-solve B s2deg s1deg  -> E | 'none : integrate B seeking an answer with s2-degree <= s2deg and
;                              s1-degree <= s1deg and Q(x)-rational coefficients, by undetermined coefficients
;                              (residual vanishing at sample points), certificate-checked
;   ne-integrate B          -> (list 'elementary E) | 'no-elementary-form
;
; Verified: d/dx(exp x)=exp x (inner); d/dx(exp(exp x))=exp(x)exp(exp x) (=s1 s2); the nested-exp integral
; INT exp(x) exp(exp(x)) dx = exp(exp(x)); and the s2^2 case d/dx((exp exp x)^2)=2 exp(x)(exp exp x)^2.
;
; Builds on tower.lisp / ratfun.lisp (Q(x) rational arithmetic) and poly.lisp.

(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (ne-nth l k) (if (= k 0) (car l) (ne-nth (cdr l) (- k 1))))
(define (ne-len l) (if (null? l) 0 (+ 1 (ne-len (cdr l)))))
(define (ne-nthor P k) (if (< k (ne-len P)) (ne-nth P k) (rat-zero)))

; ----- s1-polynomial arithmetic (lists of Q(x) rationals over s1-degree) -----
(define (ne-padd A B) (cond ((null? A) B) ((null? B) A) (else (cons (rat-add (car A) (car B)) (ne-padd (cdr A) (cdr B))))))
(define (ne-pneg A) (if (null? A) (quote ()) (cons (rat-neg (car A)) (ne-pneg (cdr A)))))
(define (ne-psub A B) (ne-padd A (ne-pneg B)))
(define (ne-pscale c P) (if (null? P) (quote ()) (cons (rat-mul c (car P)) (ne-pscale c (cdr P)))))
(define (ne-piscale k P) (ne-pscale (rat-from-poly (list k)) P))
(define (ne-pzero? A) (cond ((null? A) #t) ((rat-zero? (car A)) (ne-pzero? (cdr A))) (else #f)))
(define (ne-peq? A B) (ne-pzero? (ne-psub A B)))

; ----- in-place exponential derivation of an s1-poly: ds1(P) = sum (p_k' + k p_k) s1^k -----
(define (ne-ds1 P) (ne-ds1-go P 0 (ne-len P)))
(define (ne-ds1-go P k n) (if (>= k n) (quote ()) (cons (rat-add (rat-deriv (ne-nthor P k)) (ne-piscale-one k (ne-nthor P k))) (ne-ds1-go P (+ k 1) n))))
(define (ne-piscale-one k r) (rat-mul (rat-from-poly (list k)) r))
; ----- multiply an s1-poly by s1 (shift up one degree) -----
(define (ne-s1shift P) (cons (rat-zero) P))

; ----- tower element access -----
(define (ne-scoeff E m) (if (< m (ne-len E)) (ne-nth E m) (quote ())))   ; the s1-poly at s2-degree m

; ----- derivation in the tower: at s2-degree m, C_m -> ds1(C_m) + m * s1shift(C_m) -----
; (the second term is the s2'=s1 s2 contribution; s2-degree is preserved)
(define (ne-deriv E) (ne-trim (ne-deriv-go E 0 (ne-len E))))
(define (ne-deriv-go E m n)
  (if (>= m n) (quote ())
      (cons (ne-padd (ne-ds1 (ne-scoeff E m)) (ne-piscale m (ne-s1shift (ne-scoeff E m))))
            (ne-deriv-go E (+ m 1) n))))
(define (ne-trim E) (ne-trim-go (ne-reverse E)))
(define (ne-trim-go r) (cond ((null? r) (list (quote ()))) ((ne-pzero? (car r)) (ne-trim-go (cdr r))) (else (ne-reverse r))))
(define (ne-reverse l) (ne-rev l (quote ())))
(define (ne-rev l acc) (if (null? l) acc (ne-rev (cdr l) (cons (car l) acc))))

; ----- certificate: d/dx E = B -----
(define (ne-certify E B) (ne-eq? (ne-deriv E) B))
(define (ne-eq? A B) (ne-eq-go A B 0 (ne-maxlen A B)))
(define (ne-maxlen A B) (if (> (ne-len A) (ne-len B)) (ne-len A) (ne-len B)))
(define (ne-eq-go A B m n) (if (>= m n) #t (if (ne-peq? (ne-scoeff A m) (ne-scoeff B m)) (ne-eq-go A B (+ m 1) n) #f)))

; ===== solver: because the derivation is block-diagonal across s2-degree, solve each s2-degree independently.
; seek E with s2-degree <= s2deg, s1-degree <= s1deg, Q(x)-rational coefficients built as (poly deg<=d)/x^d.
; require the residual to vanish at sample points (linear), then certify. =====
(define (ne-points) (list 2 3 5 7 11 13 17 19 23 29))
(define (ne-unknowns s2deg s1deg deg) (* (+ s2deg 1) (* (+ s1deg 1) (+ deg 1))))
(define (ne-build s2deg s1deg deg cs) (ne-build-m s2deg s1deg deg cs 0))
(define (ne-build-m s2deg s1deg deg cs m)
  (if (> m s2deg) (quote ())
      (cons (ne-build-k s1deg deg (ne-slice cs (* m (* (+ s1deg 1) (+ deg 1))) (* (+ s1deg 1) (+ deg 1))) 0)
            (ne-build-m s2deg s1deg deg cs (+ m 1)))))
(define (ne-build-k s1deg deg seg k)
  (if (> k s1deg) (quote ())
      (cons (rat-make (ne-slice seg (* k (+ deg 1)) (+ deg 1)) (ne-xpow deg)) (ne-build-k s1deg deg seg (+ k 1)))))
(define (ne-xpow d) (if (= d 0) (list 1) (cons 0 (ne-xpow (- d 1)))))
(define (ne-take l k) (if (= k 0) (quote ()) (if (null? l) (cons 0 (ne-take (quote ()) (- k 1))) (cons (car l) (ne-take (cdr l) (- k 1))))))
(define (ne-drop l k) (if (= k 0) l (if (null? l) (quote ()) (ne-drop (cdr l) (- k 1)))))
(define (ne-slice l s n) (ne-take (ne-drop l s) n))

(define (ne-resid s2deg s1deg deg cs B) (ne-flatten (ne-dsub (ne-deriv (ne-build s2deg s1deg deg cs)) B) (+ s2deg 1) (+ s1deg 1)))
(define (ne-dsub A B) (ne-dsub-go A B 0 (ne-maxlen A B)))
(define (ne-dsub-go A B m n) (if (>= m n) (quote ()) (cons (ne-psub (ne-scoeff A m) (ne-scoeff B m)) (ne-dsub-go A B (+ m 1) n))))
(define (ne-flatten R mcount kcount) (ne-fl-m R mcount kcount 0))
(define (ne-fl-m R mcount kcount m) (if (>= m mcount) (quote ()) (append (ne-fl-k (ne-scoeff R m) kcount 0) (ne-fl-m R mcount kcount (+ m 1)))))
(define (ne-fl-k C kcount k) (if (>= k kcount) (quote ()) (append (ne-eval-rat (ne-coeffk C k) (ne-points)) (ne-fl-k C kcount (+ k 1)))))
(define (ne-coeffk C k) (if (< k (ne-len C)) (ne-nth C k) (rat-zero)))
(define (ne-eval-rat r pts) (if (null? pts) (quote ()) (cons (/ (poly-eval (rat-num r) (car pts)) (poly-eval (rat-den r) (car pts))) (ne-eval-rat r (cdr pts)))))

(define (ne-zeros m) (if (= m 0) (quote ()) (cons 0 (ne-zeros (- m 1)))))
(define (ne-unit m j) (ne-unit-go m j 0))
(define (ne-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (ne-unit-go m j (+ i 1)))))
(define (ne-vsub a b) (if (null? a) (quote ()) (cons (- (car a) (car b)) (ne-vsub (cdr a) (cdr b)))))
(define (ne-vneg a) (if (null? a) (quote ()) (cons (- 0 (car a)) (ne-vneg (cdr a)))))

(define (ne-solve B s2deg s1deg) (ne-solve-d B s2deg s1deg 0))
(define (ne-solve-d B s2deg s1deg deg) (ne-solve-r0 B s2deg s1deg deg (ne-unknowns s2deg s1deg deg)))
(define (ne-solve-r0 B s2deg s1deg deg m) (ne-solve-with B s2deg s1deg deg m (ne-resid s2deg s1deg deg (ne-zeros m) B)))
(define (ne-solve-with B s2deg s1deg deg m r0) (ne-finish B s2deg s1deg deg m (ne-lin-solve (ne-cols B s2deg s1deg deg m r0 0 (quote ())) (ne-vneg r0) (ne-len r0) m)))
(define (ne-finish B s2deg s1deg deg m sol) (if (equal? sol (quote none)) (quote none) (ne-finish2 B (ne-build s2deg s1deg deg sol))))
(define (ne-finish2 B E) (if (ne-certify E B) (ne-trim E) (quote none)))
(define (ne-cols B s2deg s1deg deg m r0 j acc) (if (= j m) (ne-reverse acc) (ne-cols B s2deg s1deg deg m r0 (+ j 1) (cons (ne-vsub (ne-resid s2deg s1deg deg (ne-unit m j) B) r0) acc))))

; ----- exact linear solver (proven full Gauss-Jordan, flattened) -----
(define (ne-lin-solve cols b rows m) (ne-reduce (ne-drop-zero-rows (ne-aug (ne-rows-from-cols cols rows m) b) m) m 0 (quote ())))
(define (ne-rows-from-cols cols rows m) (ne-rfc cols rows 0))
(define (ne-rfc cols rows i) (if (= i rows) (quote ()) (cons (ne-rowi cols i) (ne-rfc cols rows (+ i 1)))))
(define (ne-rowi cols i) (if (null? cols) (quote ()) (cons (ne-vnth (car cols) i) (ne-rowi (cdr cols) i))))
(define (ne-vnth v i) (if (= i 0) (car v) (ne-vnth (cdr v) (- i 1))))
(define (ne-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (car b))) (ne-aug (cdr rows) (cdr b)))))
(define (ne-drop-zero-rows rows m) (if (null? rows) (quote ()) (if (ne-row-zero? (car rows) m 0) (ne-drop-zero-rows (cdr rows) m) (cons (car rows) (ne-drop-zero-rows (cdr rows) m)))))
(define (ne-row-zero? row m i) (cond ((= i m) #t) ((= (ne-vnth row i) 0) (ne-row-zero? row m (+ i 1))) (else #f)))
(define (ne-reduce work m c piv) (if (= c m) (ne-read piv m 0 (quote ())) (ne-reduce-step work m c piv (ne-first-with-col work c))))
(define (ne-reduce-step work m c piv pr) (if (equal? pr (quote none)) (ne-reduce work m (+ c 1) piv) (ne-reduce-pivot work m c piv (ne-scale-row pr (/ 1 (ne-vnth pr c))))))
(define (ne-reduce-pivot work m c piv prn) (ne-reduce (ne-elim-others (ne-remove-row work prn) prn c) m (+ c 1) (cons (cons c prn) (ne-elim-piv piv prn c))))
(define (ne-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (ne-axpy (cdr (car piv)) prn (- 0 (ne-vnth (cdr (car piv)) c)))) (ne-elim-piv (cdr piv) prn c))))
(define (ne-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (ne-vnth (car work) c) 0)) (car work)) (else (ne-first-with-col (cdr work) c))))
(define (ne-remove-row work prn) (ne-rr-go work prn #f))
(define (ne-rr-go work prn removed) (cond ((null? work) (quote ())) ((if (not removed) (ne-eq-row? (car work) prn) #f) (ne-rr-go (cdr work) prn #t)) (else (cons (car work) (ne-rr-go (cdr work) prn removed)))))
(define (ne-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (ne-eq-row? (cdr a) (cdr b)) #f)))
(define (ne-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (ne-scale-row (cdr row) s))))
(define (ne-elim-others work prn c) (if (null? work) (quote ()) (cons (ne-axpy (car work) prn (- 0 (ne-vnth (car work) c))) (ne-elim-others (cdr work) prn c))))
(define (ne-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (ne-axpy (cdr row) (cdr prn) f))))
(define (ne-read piv m j acc) (if (= j m) (ne-reverse acc) (ne-read piv m (+ j 1) (cons (ne-readval (ne-piv-for piv j) m) acc))))
(define (ne-readval pr m) (if (equal? pr (quote none)) 0 (ne-vnth pr m)))
(define (ne-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (ne-piv-for (cdr piv) j))))

; ----- top-level -----
(define (ne-integrate B) (ne-int-result (ne-solve B (ne-len B) (+ (ne-maxs1 B) 1))))
(define (ne-maxs1 B) (ne-maxs1-go B 0 0))
(define (ne-maxs1-go B m acc) (if (>= m (ne-len B)) acc (ne-maxs1-go B (+ m 1) (ne-maxnum acc (- (ne-len (ne-scoeff B m)) 1)))))
(define (ne-maxnum a b) (if (> a b) a b))
(define (ne-int-result E) (if (equal? E (quote none)) (quote no-elementary-form) (list (quote elementary) E)))
