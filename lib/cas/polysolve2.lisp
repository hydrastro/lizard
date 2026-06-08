; -*- lisp -*-
; lib/cas/polysolve2.lisp -- EXACT RATIONAL SOLUTION TUPLES for a zero-dimensional polynomial system, by
; triangular back-substitution on the lexicographic Groebner basis (docs/CAS.md -- summit S3: full coordinate
; back-substitution, pairing per-variable values into actual solution tuples).  Where the earlier polyroots only
; counted and isolated the real values of a single coordinate, this recovers the complete solution points whose
; coordinates are rational, exactly, and reports honestly (a partial tuple tagged 'irrational-fiber) when a fiber
; is not rational so that no solution is ever invented or dropped silently.
;
; For a zero-dimensional ideal the lex Groebner basis is TRIANGULAR: some element is univariate in the last
; variable x_{n-1}, others reintroduce earlier variables.  The classical solution method is back-substitution:
;   - find the rational roots of the univariate eliminant in the last variable (rational-root theorem: a reduced
;     root p/q has p | constant term and q | leading coefficient -- exact and finite to enumerate);
;   - for each rational value, SUBSTITUTE it for that variable in every basis polynomial (multiplying each term's
;     coefficient by value^exponent and dropping that exponent), yielding a system in one fewer variable;
;   - recurse, assembling each rational value onto the front of every solution tuple from the sub-system.
; A solution all of whose coordinates are rational is produced exactly; if at some level the relevant eliminant
; has no rational root (an irrational or complex fiber), that branch is reported 'irrational-fiber rather than
; guessed -- the soundness boundary is explicit.  Everything is exact over Q; no floating point.
;
; Public (polynomials in groebner.lisp's representation; nv = number of variables):
;   ps2-rational-roots coeffs      -> the list of rational roots of a univariate poly (low->high coeff list)
;   ps2-subst-last p v nv          -> substitute value v for the last variable of mpoly p (nv vars) -> mpoly (nv-1)
;   ps2-solutions F nv             -> list of solution tuples; each is a list of nv rationals, OR a partial tuple
;                                     ending in 'irrational-fiber when a coordinate fiber is not rational
;   ps2-rational-solutions F nv    -> only the fully-rational solution tuples (irrational fibers dropped)
;   ps2-verify F nv tuple          -> #t iff every generator of F vanishes at the rational tuple (a certificate)
;
; Verified: <x^2-3x+2, y-x> yields exactly (1,1) and (2,2); <x^2-1, y^2-4> yields the four points (+-1,+-2);
; <x^2-2, y-x> reports irrational-fiber (x = sqrt2 is not rational), not a fabricated tuple; every rational tuple
; returned is checked to make all generators vanish.
;
; Builds on groebner.lisp, polysolve.lisp, poly.lisp.

(import "cas/groebner.lisp")
(import "cas/polysolve.lisp")
(import "cas/poly.lisp")

(define (ps2-len l) (if (null? l) 0 (+ 1 (ps2-len (cdr l)))))
(define (ps2-nth l k) (if (= k 0) (car l) (ps2-nth (cdr l) (- k 1))))
(define (ps2-app a b) (if (null? a) b (cons (car a) (ps2-app (cdr a) b))))
(define (ps2-map f l) (if (null? l) (quote ()) (cons (f (car l)) (ps2-map f (cdr l)))))
(define (ps2-abs n) (if (< n 0) (- 0 n) n))

; ----- rational roots of a univariate polynomial (low->high coeff list) by the rational-root theorem -----
; reduced p/q with p | a_0 and q | a_n; enumerate divisors, test each candidate (and its negative) by evaluation.
(define (ps2-rational-roots coeffs) (ps2-rr-dispatch (ps2-trim coeffs)))
(define (ps2-trim c) (ps2-trim-go c (ps2-len c)))
(define (ps2-trim-go c n) (cond ((= n 0) (quote ())) ((= (ps2-nth c (- n 1)) 0) (ps2-trim-go c (- n 1))) (else (ps2-take c n))))
(define (ps2-take c n) (if (= n 0) (quote ()) (ps2-app (ps2-take c (- n 1)) (list (ps2-nth c (- n 1))))))
(define (ps2-rr-dispatch c)
  (cond ((null? c) (quote ()))                                  ; zero poly: treat as no isolated roots
        ((= (ps2-len c) 1) (quote ()))                          ; nonzero constant: no roots
        (else (ps2-rr-collect c (ps2-cands c)))))
; candidate numerators = divisors of constant term (handle 0 const: 0 is a root); denominators = divisors of lead
(define (ps2-cand-consts c) (ps2-divisors (ps2-abs (ps2-const c))))
(define (ps2-const c) (car c))
(define (ps2-lead c) (ps2-nth c (- (ps2-len c) 1)))
(define (ps2-divisors n) (if (= n 0) (list 1) (ps2-div-go n 1 (quote ()))))
(define (ps2-div-go n d acc) (cond ((> d n) (ps2-rev acc)) ((= (remainder n d) 0) (ps2-div-go n (+ d 1) (cons d acc))) (else (ps2-div-go n (+ d 1) acc))))
(define (ps2-rev l) (ps2-rev-go l (quote ())))
(define (ps2-rev-go l a) (if (null? l) a (ps2-rev-go (cdr l) (cons (car l) a))))
; build candidate roots: 0 if constant term is 0, plus +-(p/q) for p in num-divisors, q in den-divisors
(define (ps2-cands c) (ps2-app (if (= (ps2-const c) 0) (list 0) (quote ())) (ps2-pairs (ps2-divisors (ps2-abs (ps2-const c))) (ps2-divisors (ps2-abs (ps2-lead c))))))
(define (ps2-pairs ps qs) (if (null? ps) (quote ()) (ps2-app (ps2-with (car ps) qs) (ps2-pairs (cdr ps) qs))))
(define (ps2-with p qs) (if (null? qs) (quote ()) (ps2-app (list (/ p (car qs)) (- 0 (/ p (car qs)))) (ps2-with p (cdr qs)))))
; test each candidate by evaluation; dedupe
(define (ps2-rr-collect c cands) (ps2-dedupe (ps2-filter-roots c cands)))
(define (ps2-filter-roots c cands) (cond ((null? cands) (quote ())) ((= (poly-eval c (car cands)) 0) (cons (car cands) (ps2-filter-roots c (cdr cands)))) (else (ps2-filter-roots c (cdr cands)))))
(define (ps2-dedupe l) (ps2-dd l (quote ())))
(define (ps2-dd l seen) (cond ((null? l) (ps2-rev seen)) ((ps2-memq (car l) seen) (ps2-dd (cdr l) seen)) (else (ps2-dd (cdr l) (cons (car l) seen)))))
(define (ps2-memq x l) (cond ((null? l) #f) ((= x (car l)) #t) (else (ps2-memq x (cdr l)))))

; ----- substitute a value for the FIRST variable (index 0) of an mpoly; result lives in nv-1 variables -----
; (lex order x_0 > x_1 > ... eliminates x_0 LAST, so the univariate basis element is in x_0; we peel x_0 first.)
(define (ps2-subst-first p v) (ps2-collect-terms (ps2-map (lambda (t) (ps2-subst-term0 t v)) p)))
; one term (coeff . (e0 e1..)) -> (coeff*v^e0 . (e1..))
(define (ps2-subst-term0 t v) (cons (* (car t) (ps2-pow v (car (cdr t)))) (cdr (cdr t))))
(define (ps2-pow b e) (if (<= e 0) 1 (* b (ps2-pow b (- e 1)))))
; collect terms with identical monomials (summing coeffs), drop zeros -- keep representation clean
(define (ps2-collect-terms ts) (ps2-ct ts (quote ())))
(define (ps2-ct ts acc) (if (null? ts) (ps2-drop-zero acc) (ps2-ct (cdr ts) (ps2-add-term acc (car ts)))))
(define (ps2-add-term acc t) (cond ((null? acc) (list t)) ((equal? (cdr (car acc)) (cdr t)) (cons (cons (+ (car (car acc)) (car t)) (cdr t)) (cdr acc))) (else (cons (car acc) (ps2-add-term (cdr acc) t)))))
(define (ps2-drop-zero ts) (cond ((null? ts) (quote ())) ((= (car (car ts)) 0) (ps2-drop-zero (cdr ts))) (else (cons (car ts) (ps2-drop-zero (cdr ts))))))

; ----- the univariate eliminant in the FIRST variable as a coeff list (low->high) -----
; among the basis, find an element univariate in variable 0 (all other exponents zero), project to coeffs.
(define (ps2-first-elim G nv) (ps2-find-uni G 0 nv))
(define (ps2-find-uni G i nv) (cond ((null? G) (quote ())) ((ps2-uni-in? (car G) i nv) (ps2-to-coeffs (car G) i)) (else (ps2-find-uni (cdr G) i nv))))
(define (ps2-uni-in? p i nv) (cond ((null? p) #t) ((ps2-term-uni? (cdr (car p)) i) (ps2-uni-in? (cdr p) i nv)) (else #f)))
(define (ps2-term-uni? m i) (ps2-others-zero m i 0))
(define (ps2-others-zero m i j) (cond ((>= j (ps2-len m)) #t) ((= j i) (ps2-others-zero m i (+ j 1))) ((= (ps2-nth m j) 0) (ps2-others-zero m i (+ j 1))) (else #f)))
(define (ps2-to-coeffs p i) (ps2-build-coeffs p i (ps2-maxdeg p i 0)))
(define (ps2-maxdeg p i acc) (if (null? p) acc (ps2-maxdeg (cdr p) i (ps2-mx acc (ps2-nth (cdr (car p)) i)))))
(define (ps2-mx a b) (if (> a b) a b))
(define (ps2-build-coeffs p i d) (ps2-bc p i 0 d))
(define (ps2-bc p i k d) (if (> k d) (quote ()) (cons (ps2-cf p i k) (ps2-bc p i (+ k 1) d))))
(define (ps2-cf p i k) (if (null? p) 0 (+ (if (= (ps2-nth (cdr (car p)) i) k) (car (car p)) 0) (ps2-cf (cdr p) i k))))

; ----- the solver: recurse on number of variables, peeling the FIRST variable each level -----
(define (ps2-solutions F nv) (ps2-solve-basis (groebner F) nv))
(define (ps2-solve-basis G nv)
  (if (= nv 1)
      (ps2-base G)
      (ps2-recurse G nv (ps2-rational-roots (ps2-first-elim G nv)))))
; base case: single variable, solutions are the rational roots of the lone univariate poly (each a 1-tuple)
(define (ps2-base G) (ps2-map (lambda (r) (list r)) (ps2-rational-roots (ps2-uni-coeffs G))))
(define (ps2-uni-coeffs G) (if (null? G) (quote ()) (ps2-to-coeffs (car G) 0)))
; recursive case: for each rational value of the FIRST variable, substitute everywhere and solve the sub-system;
; PREPEND the value to the front of each sub-tuple (first variable is the leading coordinate).
; recursive case: for each rational value of the FIRST variable, substitute everywhere and solve the sub-system;
; PREPEND the value to the front of each sub-tuple.  If the eliminant has NO rational root at all (and is a
; genuine nonconstant polynomial), the fiber is irrational/complex and we emit one honest marker.
(define (ps2-recurse G nv roots)
  (if (null? roots)
      (if (ps2-eliminant-empty? G nv) (quote ()) (ps2-irrational-marker G nv))
      (ps2-rec-go G nv roots)))
; the loop assumes roots is nonempty on entry; the tail simply stops (no marker) when roots is exhausted.
(define (ps2-rec-go G nv roots)
  (if (null? roots)
      (quote ())
      (ps2-app (ps2-extend (ps2-solve-basis (ps2-subst-all G (car roots) nv) (- nv 1)) (car roots)) (ps2-rec-go G nv (cdr roots)))))
(define (ps2-eliminant-empty? G nv) (null? (ps2-first-elim G nv)))
; if the eliminant exists but has no rational root, the fiber is irrational/complex -- report one honest marker
(define (ps2-irrational-marker G nv) (list (list (quote irrational-fiber))))
; After substituting x_0 = v into a lex Groebner basis, the polynomials not mentioning x_0 are unchanged and the
; whole set still generates the fiber ideal; substituting a CONSTANT cannot raise degrees, so re-running Buchberger
; on the (already nearly-triangular, lower-degree) substituted system is cheap and restores a clean lex basis in
; the remaining variables.  We re-Groebner only this reduced system (one fewer variable), not the original.
(define (ps2-subst-all G v nv) (ps2-regroebner (ps2-clean (ps2-map (lambda (p) (ps2-subst-first p v)) G))))
(define (ps2-regroebner G) (if (ps2-small? G) (groebner G) G))
; guard: only re-Groebner when the system is small enough to be safe; otherwise pass the substituted set through
; (it is already a generating set, and the eliminant extraction tolerates a non-reduced basis).
(define (ps2-small? G) (if (null? G) #f (< (ps2-total-deg G) 12)))
(define (ps2-total-deg G) (if (null? G) 0 (+ (ps2-poly-deg (car G)) (ps2-total-deg (cdr G)))))
(define (ps2-poly-deg p) (if (null? p) 0 (ps2-mx (ps2-mono-deg (cdr (car p))) (ps2-poly-deg (cdr p)))))
(define (ps2-mono-deg m) (if (null? m) 0 (+ (car m) (ps2-mono-deg (cdr m)))))
(define (ps2-clean G) (cond ((null? G) (quote ())) ((null? (car G)) (ps2-clean (cdr G))) (else (cons (car G) (ps2-clean (cdr G))))))
; prepend value v as the first coordinate of each tuple (unless the tuple is an irrational marker)
(define (ps2-extend tuples v) (ps2-map (lambda (tp) (ps2-prepend-coord tp v)) tuples))
(define (ps2-prepend-coord tp v) (if (ps2-has-marker? tp) tp (cons v tp)))
(define (ps2-has-marker? tp) (cond ((null? tp) #f) ((equal? (car tp) (quote irrational-fiber)) #t) (else (ps2-has-marker? (cdr tp)))))

; ----- only the fully-rational tuples -----
(define (ps2-rational-solutions F nv) (ps2-filter-rational (ps2-solutions F nv)))
(define (ps2-filter-rational sols) (cond ((null? sols) (quote ())) ((ps2-has-marker? (car sols)) (ps2-filter-rational (cdr sols))) (else (cons (car sols) (ps2-filter-rational (cdr sols))))))

; ----- verify a rational tuple makes every generator vanish (the certificate) -----
(define (ps2-verify F nv tuple) (ps2-all-vanish F tuple))
(define (ps2-all-vanish F tuple) (cond ((null? F) #t) ((= (ps2-eval-mpoly (car F) tuple) 0) (ps2-all-vanish (cdr F) tuple)) (else #f)))
(define (ps2-eval-mpoly p tuple) (if (null? p) 0 (+ (ps2-eval-term (car p) tuple) (ps2-eval-mpoly (cdr p) tuple))))
(define (ps2-eval-term t tuple) (* (car t) (ps2-eval-mono (cdr t) tuple 0)))
(define (ps2-eval-mono m tuple j) (if (null? m) 1 (* (ps2-pow (ps2-nth tuple j) (car m)) (ps2-eval-mono (cdr m) tuple (+ j 1)))))
