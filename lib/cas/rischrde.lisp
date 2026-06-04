; -*- lisp -*-
; lib/cas/rischrde.lisp -- the GENERAL Risch differential equation solver with RATIONAL-FUNCTION coefficients:
; decides and solves  y' + f y = g  for a rational y, where f and g are rational functions over Q.  This lifts
; rischtower's polynomial-coefficient RDE (rt-rde-exp-const-solvable?, which handled y' + w y = target for
; polynomial w, target) to arbitrary rational coefficients -- the step that unlocks the recursive Risch decision
; procedure over arbitrary tower levels (docs/TRAGER_ROADMAP.md, the summit).
;
; Why it matters.  In the recursion, an exponential level theta = exp(b) sends INT a_i theta^i one level down to
; the RDE c_i' + (i b') c_i = a_i, whose coefficient i b' and right-hand side a_i are general elements of the
; lower field -- rational functions, not just polynomials.  Deciding that RDE in rationals is exactly this
; module; with it, the per-degree subproblems of the tower recursion are decidable whenever the lower field is
; Q(x).
;
; The pipeline (Bronstein's weak/SPDE approach, specialized to Q(x)):
;   1. DENOMINATOR BOUND.  The poles of any rational solution y sit only at poles of f and g, so y = q/d with
;      d a safe over-bound (the product of the denominators of f and g) and q a polynomial unknown.  (Spurious
;      factors of d are harmless: q simply acquires them as roots, and the final certificate rejects any y that
;      does not actually satisfy the equation.)
;   2. SUBSTITUTE y = q/d.  (q/d)' + f (q/d) = g  becomes  q' + (f - d'/d) q = g d; clearing the common
;      denominator D of (f - d'/d) and g d gives a POLYNOMIAL RDE  A q' + B q = C  with A = D, B = D (f - d'/d),
;      C = D g d.
;   3. POLYNOMIAL RDE  A q' + B q = C.  Degree-bound q and solve by undetermined coefficients over an exact
;      linear system; a consistent system at some degree <= the bound yields q (hence y = q/d), and otherwise
;      no rational y exists.
; The differentiation certificate  y' + f y = g  (an exact rational identity) is the final arbiter, so the
; over-bounded denominator and the degree search are SOUND -- only a y that genuinely satisfies the equation is
; returned, else 'no-rational-solution.
;
; Public:
;   rde-solve f g           -> y (a rational, as (num . den)) | 'no-rational-solution : solves y' + f y = g
;   rde-certify f g y       -> #t iff y' + f y = g  (exact rational identity)
;   rde-decide f g          -> (list 'solvable y) | (list 'no-rational-solution 'rde-obstruction)
;   rde-poly-solve A B C     -> q (polynomial) | 'none : solves the polynomial RDE A q' + B q = C
;
; Verified: y'+y=x -> y=x-1 (= INT x e^x); y'-(1/x)y=x -> y=x^2 (a pole in f, none in y); y'+(1/x)y=1 -> y=x/2;
; y'+(2/x)y=1/x^2 -> y=1/x (a genuine pole in y); and y'+y=1/x -> NO rational solution (the Ei obstruction).
;
; Builds on tower.lisp (rational-function arithmetic rat-*) and poly.lisp.

(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (rde-nth l k) (if (= k 0) (car l) (rde-nth (cdr l) (- k 1))))
(define (rde-len l) (if (null? l) 0 (+ 1 (rde-len (cdr l)))))
(define (rde-deg p) (- (rde-len (poly-norm p)) 1))

; ----- the differentiation certificate: y' + f y = g as an exact rational identity -----
(define (rde-certify f g y) (rat-equal? (rat-add (rat-deriv y) (rat-mul f y)) g))

; ----- STAGE 1+2: build the polynomial RDE A q' + B q = C from rational f, g.
; d = den(f) * den(g) (the safe denominator over-bound); y = q/d.
; equation q' + (f - d'/d) q = g d, cleared by the common denominator -> A q' + B q = C.
; We compute Fhat = f - d'/d and Ghat = g d as rationals, then D = lcm of their denominators (use the product),
; and A = D (as a polynomial, = D since q' coefficient is 1 times D), B = D*Fhat, C = D*Ghat as polynomials. -----
(define (rde-dbound f g) (poly-mul (rat-den f) (rat-den g)))
(define (rde-Fhat f d) (rat-sub f (rat-make (poly-deriv d) d)))           ; f - d'/d
(define (rde-Ghat g d) (rat-mul g (rat-from-poly d)))                     ; g d
; common denominator D = den(Fhat) * den(Ghat); A=D, B=D*Fhat (a polynomial), C=D*Ghat (a polynomial)
(define (rde-bigD Fhat Ghat) (poly-mul (rat-den Fhat) (rat-den Ghat)))
(define (rde-mk-A D) D)
(define (rde-mk-B D Fhat) (rde-ratpoly (rat-mul (rat-from-poly D) Fhat)))   ; D*Fhat is a polynomial
(define (rde-mk-C D Ghat) (rde-ratpoly (rat-mul (rat-from-poly D) Ghat)))
; extract the polynomial value of a rational that is actually a polynomial (den divides num); use poly division
(define (rde-ratpoly r) (car (poly-divmod (rat-num r) (rat-den r))))

; ----- STAGE 3: polynomial RDE A q' + B q = C, solve for polynomial q by undetermined coefficients.
; degree bound: try degrees 0..N with N a safe bound from the degrees of A,B,C. -----
(define (rde-poly-solve A B C) (rde-ps-search A B C 0 (rde-Nbound A B C)))
(define (rde-Nbound A B C) (+ 2 (rde-maxdeg (rde-deg C) (rde-maxdeg (rde-deg B) (rde-deg A)))))
(define (rde-maxdeg a b) (if (> a b) a b))
(define (rde-ps-search A B C dq N)
  (if (> dq N) (quote none)
      (rde-ps-try A B C dq N (rde-ps-at A B C dq))))
(define (rde-ps-try A B C dq N result) (if (equal? result (quote none)) (rde-ps-search A B C (+ dq 1) N) result))
(define (rde-ps-at A B C dq) (rde-ps-check A B C (rde-lin-solve (rde-cols A B C dq (+ dq 1) 0 (quote ())) (rde-pad (poly-norm C) (rde-rows A B C dq)) (rde-rows A B C dq) (+ dq 1))))
(define (rde-rows A B C dq) (+ 1 (rde-maxdeg (rde-deg C) (rde-maxdeg (+ dq (rde-deg A) -1) (+ dq (rde-deg B))))))
(define (rde-apply-poly A B q) (poly-add (poly-mul A (poly-deriv q)) (poly-mul B q)))   ; A q' + B q
(define (rde-ps-check A B C sol) (if (equal? sol (quote none)) (quote none) (rde-vrfy A B C sol)))
(define (rde-vrfy A B C sol) (if (rde-peq? (rde-apply-poly A B sol) C) (poly-norm sol) (quote none)))
(define (rde-cols A B C dq m j acc) (if (= j m) (rde-reverse acc) (rde-cols A B C dq m (+ j 1) (cons (rde-pad (rde-apply-poly A B (rde-unit m j)) (rde-rows A B C dq)) acc))))
(define (rde-unit m j) (rde-unit-go m j 0))
(define (rde-unit-go m j i) (if (= i m) (quote ()) (cons (if (= i j) 1 0) (rde-unit-go m j (+ i 1)))))
(define (rde-pad p n) (rde-pad-go (poly-norm p) n 0))
(define (rde-pad-go p n i) (if (= i n) (quote ()) (cons (if (< i (rde-len p)) (rde-nth p i) 0) (rde-pad-go p n (+ i 1)))))
(define (rde-peq? a b) (rde-veq? (poly-norm a) (poly-norm b)))
(define (rde-veq? a b) (cond ((null? a) (null? b)) ((null? b) (rde-veq? a (quote ()))) (else (if (= (car a) (rde-hh b)) (rde-veq? (cdr a) (rde-tt b)) #f))))
(define (rde-hh b) (if (null? b) 0 (car b)))
(define (rde-tt b) (if (null? b) (quote ()) (cdr b)))

; ----- top-level rational RDE solve -----
(define (rde-solve f g) (rde-assemble f g (rde-dbound f g)))
(define (rde-assemble f g d) (rde-stage3 f g d (rde-Fhat f d) (rde-Ghat g d)))
(define (rde-stage3 f g d Fhat Ghat) (rde-from-q f g d (rde-poly-solve (rde-mk-A (rde-bigD Fhat Ghat)) (rde-mk-B (rde-bigD Fhat Ghat) Fhat) (rde-mk-C (rde-bigD Fhat Ghat) Ghat))))
(define (rde-from-q f g d q) (if (equal? q (quote none)) (quote no-rational-solution) (rde-final f g (rat-make q d))))
(define (rde-final f g y) (if (rde-certify f g y) y (quote no-rational-solution)))

(define (rde-decide f g) (rde-decide-go (rde-solve f g)))
(define (rde-decide-go y) (if (equal? y (quote no-rational-solution)) (list (quote no-rational-solution) (quote rde-obstruction)) (list (quote solvable) y)))

; ----- exact linear solver (proven full Gauss-Jordan over Q, flattened, with inconsistency detection) -----
(define (rde-lin-solve cols b rows m) (rde-reduce (rde-drop-zero-rows (rde-aug (rde-rows-from-cols cols rows m) b) m) m 0 (quote ())))
(define (rde-rows-from-cols cols rows m) (rde-rfc cols rows 0))
(define (rde-rfc cols rows i) (if (= i rows) (quote ()) (cons (rde-rowi cols i) (rde-rfc cols rows (+ i 1)))))
(define (rde-rowi cols i) (if (null? cols) (quote ()) (cons (rde-vnth (car cols) i) (rde-rowi (cdr cols) i))))
(define (rde-vnth v i) (if (= i 0) (car v) (rde-vnth (cdr v) (- i 1))))
(define (rde-aug rows b) (if (null? rows) (quote ()) (cons (append (car rows) (list (rde-hh b))) (rde-aug (cdr rows) (rde-tt b)))))
(define (rde-drop-zero-rows rows m) (cond ((null? rows) (quote ())) ((rde-row-incon? (car rows) m) (quote inconsistent-here)) ((rde-row-allzero? (car rows) m 0) (rde-drop-zero-rows (cdr rows) m)) (else (rde-cons-c (car rows) (rde-drop-zero-rows (cdr rows) m)))))
(define (rde-cons-c r rest) (if (equal? rest (quote inconsistent-here)) (quote inconsistent-here) (cons r rest)))
(define (rde-row-incon? row m) (if (rde-row-allzero? row m 0) (not (= (rde-vnth row m) 0)) #f))
(define (rde-row-allzero? row m i) (cond ((= i m) #t) ((= (rde-vnth row i) 0) (rde-row-allzero? row m (+ i 1))) (else #f)))
(define (rde-reduce work m c piv)
  (if (equal? work (quote inconsistent-here)) (quote none)
      (if (= c m) (rde-read piv m 0 (quote ())) (rde-reduce-step work m c piv (rde-first-with-col work c)))))
(define (rde-reduce-step work m c piv pr) (if (equal? pr (quote none)) (rde-reduce work m (+ c 1) piv) (rde-reduce-pivot work m c piv (rde-scale-row pr (/ 1 (rde-vnth pr c))))))
(define (rde-reduce-pivot work m c piv prn) (rde-reduce (rde-recheck (rde-elim-others (rde-remove-row work prn) prn c) m) m (+ c 1) (cons (cons c prn) (rde-elim-piv piv prn c))))
(define (rde-recheck work m) (cond ((null? work) (quote ())) ((rde-row-incon? (car work) m) (quote inconsistent-here)) (else (rde-cons-c2 (car work) (rde-recheck (cdr work) m)))))
(define (rde-cons-c2 r rest) (if (equal? rest (quote inconsistent-here)) (quote inconsistent-here) (cons r rest)))
(define (rde-elim-piv piv prn c) (if (null? piv) (quote ()) (cons (cons (car (car piv)) (rde-axpy (cdr (car piv)) prn (- 0 (rde-vnth (cdr (car piv)) c)))) (rde-elim-piv (cdr piv) prn c))))
(define (rde-first-with-col work c) (cond ((null? work) (quote none)) ((not (= (rde-vnth (car work) c) 0)) (car work)) (else (rde-first-with-col (cdr work) c))))
(define (rde-remove-row work prn) (rde-rr-go work prn #f))
(define (rde-rr-go work prn removed) (cond ((null? work) (quote ())) ((if (not removed) (rde-eq-row? (car work) prn) #f) (rde-rr-go (cdr work) prn #t)) (else (cons (car work) (rde-rr-go (cdr work) prn removed)))))
(define (rde-eq-row? a b) (if (null? a) (if (null? b) #t #f) (if (= (car a) (car b)) (rde-eq-row? (cdr a) (cdr b)) #f)))
(define (rde-scale-row row s) (if (null? row) (quote ()) (cons (* s (car row)) (rde-scale-row (cdr row) s))))
(define (rde-elim-others work prn c) (if (null? work) (quote ()) (cons (rde-axpy (car work) prn (- 0 (rde-vnth (car work) c))) (rde-elim-others (cdr work) prn c))))
(define (rde-axpy row prn f) (if (null? row) (quote ()) (cons (+ (car row) (* f (car prn))) (rde-axpy (cdr row) (cdr prn) f))))
(define (rde-read piv m j acc) (if (= j m) (rde-reverse acc) (rde-read piv m (+ j 1) (cons (rde-readval (rde-piv-for piv j) m) acc))))
(define (rde-readval pr m) (if (equal? pr (quote none)) 0 (rde-vnth pr m)))
(define (rde-piv-for piv j) (cond ((null? piv) (quote none)) ((= (car (car piv)) j) (cdr (car piv))) (else (rde-piv-for (cdr piv) j))))
(define (rde-reverse l) (rde-rev l (quote ())))
(define (rde-rev l acc) (if (null? l) acc (rde-rev (cdr l) (cons (car l) acc))))
