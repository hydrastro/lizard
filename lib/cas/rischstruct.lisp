; -*- lisp -*-
; lib/cas/rischstruct.lisp -- the LOGARITHMIC PART of the Risch-Trager structure theorem, made constructive and
; certified: given an integrand f and candidate factors g_1, ..., g_m, decide whether f = sum_i c_i (g_i'/g_i)
; for CONSTANTS c_i (so that INT f = sum_i c_i log(g_i), a sum of logarithms), by solving the linear system for
; the c_i over Q and certifying by differentiation.  This is a sound, decidable slice of the structure theorem --
; the step that recognizes when an integral is a sum of logarithms with constant coefficients
; (docs/TRAGER_ROADMAP.md, the summit, "the structure theorem").
;
; Liouville's theorem says INT f is elementary iff f = v' + sum_i c_i u_i'/u_i with the c_i constants and v, u_i
; in the field; this module handles the logarithmic sum (the v' = 0 case, which is exactly the new-logarithm
; content), the part the polynomial integrator does not produce.  Given f and the g_i, the logarithmic
; derivatives L_i = g_i'/g_i are computed, and we solve sum_i c_i L_i = f for rational constants c_i by the exact
; rational linear solver, sampling the rational/field identity at enough points to pin the c_i; the result is
; CERTIFIED by checking sum_i c_i L_i = f exactly, so a returned decomposition is always genuine and a failure to
; certify yields an honest 'no-log-decomposition.
;
; This works over Q(x) directly (f, g_i rational) and, because the logarithmic derivative g'/g and the equality
; test are available at every tower height via te-deriv / te-equal?, the same logic extends to tower elements.
;
; Public:
;   logderiv tower h g            -> g'/g at height h (a height-h element)
;   struct-log-solve f gs         -> (list 'logs (c_1 ... c_m)) | 'no-log-decomposition : over Q(x), solve
;                                    f = sum c_i g_i'/g_i for rational constants c_i (gs a list of rationals)
;   struct-log-certify f gs cs    -> #t iff sum c_i g_i'/g_i = f over Q(x)
;   struct-log-antideriv-string gs cs -> a human-readable sum c_i log(g_i) (for reporting)
;
; Verified: 2x/(x^2-1) = 1*(x^2-1)'/(x^2-1) -> INT = log(x^2-1); the same integrand as 1/(x-1)+1/(x+1) ->
; coefficients (1,1) for factors (x-1, x+1); a non-decomposable f returns 'no-log-decomposition; certificates hold.
;
; Builds on rischtowern.lisp (te-deriv, te-equal? for the tower case), linalg-style exact rational solving
; (reusing the rational Gaussian elimination), and tower.lisp / poly.lisp.

(import "cas/rischtowern.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (st-nth l k) (if (= k 0) (car l) (st-nth (cdr l) (- k 1))))
(define (st-len l) (if (null? l) 0 (+ 1 (st-len (cdr l)))))
(define (st-map2 f a) (if (null? a) (quote ()) (cons (f (car a)) (st-map2 f (cdr a)))))

; ----- logarithmic derivative g'/g at height h -----
(define (logderiv tower h g) (te-rat-div tower h (te-deriv tower h g) g))
; over Q(x) (height 0): g'/g via rat-
(define (logderiv-rat g) (rat-div (rat-deriv g) g))

; ===== over Q(x): solve f = sum c_i (g_i'/g_i) for rational constants c_i =====
; The L_i = g_i'/g_i are rationals; f is rational.  sum c_i L_i = f is linear in the c_i.  We turn the rational
; identity into a linear system by clearing denominators and matching polynomial coefficients: with common
; denominator D = prod (den L_i) ... but more simply we sample the identity at several x-values (enough points)
; to get a numeric linear system for the c_i, solve exactly over Q, then CERTIFY symbolically.
(define (struct-log-solve f gs) (struct-ls-go f gs (st-map2 logderiv-rat gs)))
(define (struct-ls-go f gs Ls) (struct-ls-solve f gs Ls (st-len gs)))
; build an m x m system by sampling at points x = 1, 2, ..., m (avoiding poles heuristically by shifting if a
; denominator vanishes); row i: (L_1(p_i) ... L_m(p_i)) and rhs f(p_i).  Solve over Q.
(define (struct-ls-solve f gs Ls m) (struct-build f Ls m (struct-goodpoints f Ls m)))
(define (struct-build f Ls m pts) (struct-solve-system f Ls (struct-rows Ls pts) (struct-rhs f pts) m))
; collect m pole-free integer points for f and the L_i, scanning p = 1, 2, 3, ... (bounded attempts)
(define (struct-goodpoints f Ls m) (struct-gp-go f Ls m 1 60 (quote ())))
(define (struct-gp-go f Ls m p limit acc)
  (cond ((>= (st-len acc) m) (st-reverse acc))
        ((<= limit 0) (st-reverse acc))
        ((struct-pole? f Ls p) (struct-gp-go f Ls m (+ p 1) (- limit 1) acc))
        (else (struct-gp-go f Ls m (+ p 1) (- limit 1) (cons p acc)))))
; rows: for each point, the vector of L_i evaluated there (as rationals)
(define (struct-rows Ls pts) (st-map2 (lambda (p) (st-map2 (lambda (L) (rat-eval-at L p)) Ls)) pts))
(define (struct-rhs f pts) (st-map2 (lambda (p) (rat-eval-at f p)) pts))
; evaluate a rational (num den) at integer point p, returning a rational value; the caller ensures dens nonzero
(define (st-rnum r) (car r))
(define (st-rden r) (car (cdr r)))
(define (rat-eval-at r p) (rat-make (list (poly-eval (st-rnum r) p)) (list (poly-eval (st-rden r) p))))
; does any of the L_i or f have a vanishing denominator at p?  if so the point is a pole and must be skipped.
(define (struct-pole? f Ls p) (cond ((= (poly-eval (st-rden f) p) 0) #t) (else (struct-anypole? Ls p))))
(define (struct-anypole? Ls p) (cond ((null? Ls) #f) ((= (poly-eval (st-rden (car Ls)) p) 0) #t) (else (struct-anypole? (cdr Ls) p))))

; solve the m x m rational system (rows . rhs) for c_i using the exact rational Gaussian elimination, then
; certify symbolically; if singular or uncertified, return 'no-log-decomposition.
(define (struct-solve-system f Ls rows rhs m) (struct-finish f Ls (st-ratsolve rows rhs m)))
(define (struct-finish f Ls cs) (if (equal? cs (quote none)) (quote no-log-decomposition) (struct-verify f Ls cs)))
(define (struct-verify f Ls cs) (if (struct-check f Ls cs) (list (quote logs) cs) (quote no-log-decomposition)))
; check sum c_i L_i = f symbolically over Q(x)
(define (struct-check f Ls cs) (rat-equal? (struct-combine Ls cs) f))
(define (struct-combine Ls cs) (struct-comb-go Ls cs (rat-zero)))
(define (struct-comb-go Ls cs acc) (if (null? Ls) acc (struct-comb-go (cdr Ls) (cdr cs) (rat-add acc (rat-mul (car cs) (car Ls))))))

(define (struct-log-certify f gs cs) (struct-check f (st-map2 logderiv-rat gs) cs))

; ===== exact rational linear solver (Gaussian elimination over Q), augmented-row form =====
; rows: list of m rows, each a list of m rationals; rhs: list of m rationals.  Returns (c_1..c_m) | 'none.
(define (st-ratsolve rows rhs m) (st-rs-aug rows rhs m))
(define (st-rs-aug rows rhs m) (st-rs-elim (st-aug rows rhs) m 0))
(define (st-aug rows rhs) (if (null? rows) (quote ()) (cons (st-append (car rows) (car rhs)) (st-aug (cdr rows) (cdr rhs)))))
(define (st-append l v) (if (null? l) (list v) (cons (car l) (st-append (cdr l) v))))
(define (st-rget row k) (st-nth row k))
(define (st-rs-elim rows m col) (if (>= col m) (st-rs-extract rows m) (st-rs-piv rows m col)))
(define (st-rs-piv rows m col) (st-rs-found rows m col (st-pivot rows col)))
(define (st-pivot rows col) (st-piv-go rows col))
(define (st-piv-go rows col) (cond ((null? rows) (quote none)) ((if (not (rat-zero? (st-rget (car rows) col))) (st-earlier0? (car rows) col 0) #f) (car rows)) (else (st-piv-go (cdr rows) col))))
(define (st-earlier0? row col k) (cond ((>= k col) #t) ((rat-zero? (st-rget row k)) (st-earlier0? row col (+ k 1))) (else #f)))
(define (st-rs-found rows m col pv) (if (equal? pv (quote none)) (st-rs-elim rows m (+ col 1)) (st-rs-elim (st-elim rows pv col) m (+ col 1))))
(define (st-elim rows pv col) (st-elim-go rows pv (st-normrow pv col) col))
(define (st-normrow pv col) (st-scalerow pv (rat-inv (st-rget pv col))))
(define (st-scalerow row s) (if (null? row) (quote ()) (cons (rat-mul s (car row)) (st-scalerow (cdr row) s))))
(define (st-elim-go rows pv npv col) (if (null? rows) (quote ()) (cons (if (st-roweq? (car rows) pv) npv (st-rowsub (car rows) (st-scalerow npv (st-rget (car rows) col)))) (st-elim-go (cdr rows) pv npv col))))
(define (st-roweq? a b) (cond ((null? a) (null? b)) ((null? b) #f) ((rat-equal? (car a) (car b)) (st-roweq? (cdr a) (cdr b))) (else #f)))
(define (st-rowsub a b) (if (null? a) (quote ()) (cons (rat-sub (car a) (car b)) (st-rowsub (cdr a) (cdr b)))))
(define (st-rs-extract rows m) (st-ext-go rows m 0 (quote ())))
(define (st-ext-go rows m j acc) (if (>= j m) (st-reverse acc) (st-ext-go rows m (+ j 1) (cons (st-ext-find rows j m) acc))))
(define (st-ext-find rows j m) (st-ef-go rows j m))
(define (st-ef-go rows j m) (cond ((null? rows) (rat-zero)) ((st-pivrow? (car rows) j m) (st-rget (car rows) m)) (else (st-ef-go (cdr rows) j m))))
(define (st-pivrow? row j m) (if (rat-equal? (st-rget row j) (rat-one)) (st-others0? row j m 0) #f))
(define (st-others0? row j m k) (cond ((>= k m) #t) ((= k j) (st-others0? row j m (+ k 1))) ((rat-zero? (st-rget row k)) (st-others0? row j m (+ k 1))) (else #f)))
(define (st-reverse l) (st-rev l (quote ())))
(define (st-rev l acc) (if (null? l) acc (st-rev (cdr l) (cons (car l) acc))))
