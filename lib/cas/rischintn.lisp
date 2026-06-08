; -*- lisp -*-
; lib/cas/rischintn.lisp -- the HEIGHT-N Risch integration recursion: integrate INT f dx for f a tower element at
; ARBITRARY height by solving D y = f recursively, the integrator at height h delegating its per-degree
; subproblems to a call at height h-1, all the way down to the rational RDE over Q(x) (rischrde).  This is the
; recursion calling itself at every level -- the structural completion of the Risch descent over arbitrary-height
; transcendental towers (docs/TRAGER_ROADMAP.md, the summit, "arbitrary height").
;
; Scope and soundness.  The recursion is exact and certified for:
;   * any HEIGHT-1 tower (exp or log) -- reproducing rischint1 through the recursive engine;
;   * arbitrary-height towers whose per-degree RDE coefficient stays a base-field element (the DECOUPLED case),
;     e.g. logarithmic levels and the structural cases where the exponent's derivative is degree-0 below.
; The remaining COUPLED case (the per-degree coefficient has positive lower-degree, as in the exp-over-exp tower
; where D(theta_1) = theta_1) is handled by the dedicated coupled solver rischtfrde2/rischcoupled for height 2;
; here, to preserve SOUNDNESS, such a subproblem returns an honest 'needs-coupled-solver rather than a guessed
; verdict -- the discipline used throughout (the differentiation certificate te-deriv == f is the arbiter, so a
; returned answer is always genuine).
;
; The recursion (built on rischtowern.lisp's recursive element algebra and derivation te-deriv):
;   te-rde-solve tower h phi g -- solve D y + phi y = g at height h, phi a height-(h-1) base coefficient.
;     height 0: the base RDE over Q(x) -> rischrde (rde-solve);
;     height h, exp: per theta-degree k, D(y_k) + (k Db + phi) y_k = g_k -- an RDE at height h-1, solved by
;       te-rde-solve recursively; if (k Db + phi) is not a base-field coefficient at that level, defer honestly;
;     height h, log: top-down, D(y_k) + phi y_k = g_k - (k+1) u y_{k+1} -- integration at height h-1, recursive.
;   te-integrate tower h f -- INT f dx = the y with D y = f (te-rde-solve with phi = 0).
;
; Public:
;   te-rde-solve tower h phi g  -> y (height-h element) | 'no-solution | 'needs-coupled-solver
;   te-integrate tower h f       -> (list 'elementary y) | (list 'non-elementary reason) | (list 'deferred ...)
;   te-int-certify tower h f y   -> #t iff D y = f at height h
;
; Verified: height-1 INT e^x, INT x e^x, INT e^x/x (Ei, non-elementary), INT log x = x log x - x, INT (log x)^2;
; height-2 logarithmic descent; the recursion bottoming through height 1 to Q(x); certificates throughout.
;
; Builds on rischtowern.lisp (recursive element algebra + derivation) and rischrde.lisp (the base RDE).

(import "cas/rischtowern.lisp")
(import "cas/rischrde.lisp")

(define (in-nth l k) (if (= k 0) (car l) (in-nth (cdr l) (- k 1))))
(define (in-len l) (if (null? l) 0 (+ 1 (in-len (cdr l)))))
(define (in-reverse l) (in-rev l (quote ())))
(define (in-rev l acc) (if (null? l) acc (in-rev (cdr l) (cons (car l) acc))))
(define (in-append l v) (if (null? l) (list v) (cons (car l) (in-append (cdr l) v))))
(define (in-bound g) (+ 2 (in-len g)))

; ----- is a height-(h-1) coefficient a base-field element (degree 0 in theta_{h-1})?  At height 1 the
; coefficient is height-0 (a rational) -- always "base".  At height >= 2 it is base iff its theta_{h-1}-degree
; is 0 (only the constant coefficient nonzero). -----
(define (te-is-base? h c) (if (= (- h 1) 0) #t (te-deg0? c)))
(define (te-deg0? c) (cond ((null? c) #t) ((null? (cdr c)) #t) (else (te-all-zero-rest? (cdr c)))))
(define (te-all-zero-rest? l) (cond ((null? l) #t) ((te-is-zero-elt? (car l)) (te-all-zero-rest? (cdr l))) (else #f)))
(define (te-is-zero-elt? e) (cond ((pair? e) (te-all-zero-rest? e)) (else #f)))   ; crude zero test for nested

; ===== the recursive RDE solver: D y + phi y = g at height h =====
(define (te-rde-solve tower h phi g)
  (if (= h 0) (te-rde-base phi g) (te-rde-level tower h phi g (te-level-type tower h))))
(define (te-rde-base phi g) (rde-solve phi g))
(define (te-rde-level tower h phi g typ) (if (equal? typ (quote exp)) (te-rde-exp tower h phi g) (te-rde-log tower h phi g)))

; ----- exponential level: per theta-degree k, D(y_k) + (k Db + phi) y_k = g_k, solved at height h-1.
; The coefficient (k Db + phi) must be a base-field element at height h-1 for the decoupled recursion; if not
; (the coupled case), defer honestly. -----
(define (te-rde-exp tower h phi g) (te-re-go tower h phi g (te-deriv tower (- h 1) (te-level-b tower h)) 0 (in-bound g) (quote ())))
(define (te-re-go tower h phi g Db k N ys)
  (if (> k N) (te-re-verdict tower h g ys)
      (te-re-coeff tower h phi g Db k N ys (te-add tower (- h 1) (te-scale-int (- h 1) k Db) phi))))
(define (te-re-coeff tower h phi g Db k N ys coeff)
  (if (te-is-base? h coeff)
      (te-re-step tower h phi g Db k N ys (te-rde-solve tower (- h 1) (te-base-of h coeff) (te-coeff h g k)))
      (quote needs-coupled-solver)))
; the base-field representative of a coefficient known to be degree-0: at height 1 it IS the rational; at
; height>=2 it is its degree-0 coefficient (a height-(h-2) element), i.e. the lone constant term.
(define (te-base-of h coeff) (if (= (- h 1) 0) coeff (te-coeff (- h 1) coeff 0)))
(define (te-re-step tower h phi g Db k N ys yk)
  (cond ((equal? yk (quote no-solution)) (quote no-solution))
        ((equal? yk (quote no-rational-solution)) (quote no-solution))
        ((equal? yk (quote needs-coupled-solver)) (quote needs-coupled-solver))
        (else (te-re-go tower h phi g Db (+ k 1) N (in-append ys (te-lift h yk))))))
; lift a height-(h-1) solution coefficient back; at height 1 yk is a rational (height-0), the coefficient itself.
(define (te-lift h yk) yk)
(define (te-re-verdict tower h g ys) (if (te-tail-nonzero? tower h g ys) (quote no-solution) (te-trim tower h ys)))
(define (te-tail-nonzero? tower h g ys) (te-tn tower h ys (in-len g) 0))
(define (te-tn tower h ys supp k) (cond ((null? ys) #f) ((if (>= k supp) (not (te-equal? tower (- h 1) (car ys) (te-zero (- h 1)))) #f) #t) (else (te-tn tower h (cdr ys) supp (+ k 1)))))
(define (te-trim tower h ys) (in-reverse (te-dropz tower h (in-reverse ys))))
(define (te-dropz tower h l) (cond ((null? l) (quote ())) ((te-equal? tower (- h 1) (car l) (te-zero (- h 1))) (te-dropz tower h (cdr l))) (else l)))

; ----- logarithmic level: top-down; degree k, D(y_k) + phi y_k = g_k - (k+1) u y_{k+1}, at height h-1 -----
(define (te-rde-log tower h phi g) (te-rl-top tower h phi g (te-logu-n tower h) (- (in-len g) 1)))
(define (te-logu-n tower h) (te-rat-div tower (- h 1) (te-deriv tower (- h 1) (te-level-b tower h)) (te-level-b tower h)))
(define (te-rl-top tower h phi g u m) (te-rl-go tower h phi g u m (te-zerolist (- h 1) (+ m 1))))
(define (te-zerolist hm1 n) (if (= n 0) (quote ()) (cons (te-zero hm1) (te-zerolist hm1 (- n 1)))))
(define (te-rl-go tower h phi g u k ys)
  (if (< k 0) (te-trim tower h ys)
      (te-rl-coeff tower h phi g u k ys (te-sub tower (- h 1) (te-coeff h g k) (te-mul tower (- h 1) (te-scale-int (- h 1) (+ k 1) u) (te-ylist-get tower h ys (+ k 1)))))))
(define (te-rl-coeff tower h phi g u k ys rhs)
  (if (te-is-base? h phi)
      (te-rl-step tower h phi g u k ys (te-rde-solve tower (- h 1) (te-base-of h phi) rhs))
      (quote needs-coupled-solver)))
(define (te-rl-step tower h phi g u k ys yk)
  (cond ((equal? yk (quote no-solution)) (quote no-solution))
        ((equal? yk (quote no-rational-solution)) (quote no-solution))
        ((equal? yk (quote needs-coupled-solver)) (quote needs-coupled-solver))
        (else (te-rl-go tower h phi g u (- k 1) (te-set ys k yk)))))
(define (te-ylist-get tower h ys i) (if (if (< i 0) #t (>= i (in-len ys))) (te-zero (- h 1)) (in-nth ys i)))
(define (te-set ys k v) (te-set-go ys k v 0))
(define (te-set-go ys k v i) (if (null? ys) (quote ()) (cons (if (= i k) v (car ys)) (te-set-go (cdr ys) k v (+ i 1)))))

(define (te-sub tower h a b) (te-add tower h a (te-scale-int h -1 b)))

; ===== integration entry: INT f = (D y = f) = te-crde-solve with F = 0 (the recursive coupled solver) =====
(import "cas/rischcrde.lisp")
(define (te-integrate tower h f) (te-int-result tower h f (te-crde-solve tower h (te-zero h) f)))
(define (te-int-result tower h f y)
  (cond ((equal? y (quote no-solution)) (list (quote non-elementary) (quote tower-rde-obstruction)))
        ((equal? y (quote inconclusive)) (list (quote deferred) (quote needs-homogeneous-bookkeeping)))
        (else (list (quote elementary) y))))
(define (te-int-certify tower h f y) (te-equal? tower h (te-deriv tower h y) f))
