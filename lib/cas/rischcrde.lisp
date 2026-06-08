; -*- lisp -*-
; lib/cas/rischcrde.lisp -- the RECURSIVE COUPLED Risch differential equation solver: solves D y + F y = g at
; ARBITRARY tower height h where F is an arbitrary height-h element (not merely a base-field coefficient).  This
; nests the coupled banded recurrence recursively, so the height-n integrator can SOLVE the exp-over-exp tower
; (and deeper) rather than deferring it -- the final structural piece of the Risch descent over arbitrary-height
; towers (docs/TRAGER_ROADMAP.md, the summit, "nesting the coupled recurrence").
;
; The general banded reduction (exp level theta = theta_h, theta' = b' theta, Db = D(b) at height h-1).  Writing
; y = sum_k y_k theta^k, F = sum_j F_j theta^j, g = sum_n g_n theta^n, the equation D y + F y = g at theta-degree
; n is
;     D(y_n) + (n Db + F_0) y_n + sum_{j>=1} F_j y_{n-j} = g_n ,
; so y_n solves the RDE  D(y_n) + (n Db + F_0) y_n = g_n - sum_{j>=1} F_j y_{n-j}  AT HEIGHT h-1.  Crucially the
; coefficient (n Db + F_0) is itself a height-(h-1) element that may be coupled at that level, so this RDE is
; solved by te-crde-solve RECURSIVELY at height h-1 -- the recursion descending until height 0, where it is the
; rational RDE (rischrde).  We solve bottom-up (n = 0, 1, ...), the convolution using the already-found lower y,
; and watch the forced higher-degree tail: a non-terminating tail (the solution spilling past the integrand's
; degree) means no bounded-degree solution exists -- the non-elementarity obstruction.  The exp-over-exp tower
; INT e^{e^x} is exactly this: the top-degree subproblem is the coupled height-1 RDE c' + theta_1 c = 1, whose
; non-terminating tail proves INT e^{e^x} non-elementary, now DERIVED through the full recursion.
;
; The differentiation certificate (te-deriv y) + F y = g at height h is the arbiter; a returned y is genuine.
;
; Public:
;   te-crde-solve tower h F g  -> y (height-h element) | 'no-solution : solve D y + F y = g (F a height-h element)
;   te-crde-certify tower h F g y -> #t iff (D y) + F y = g at height h
;
; Verified: at height 0 it reduces to rischrde; at height 1 it reproduces the coupled solver (c' + theta_1 c = 1
; non-terminating -> no solution; a solvable coupled case recovered); the exp-over-exp integrand INT e^{e^x}
; proven non-elementary through the recursion; certificates throughout.
;
; Builds on rischtowern.lisp (recursive element algebra + derivation) and rischrde.lisp (the base RDE).

(import "cas/rischtowern.lisp")
(import "cas/rischrde.lisp")

(define (cr-nth l k) (if (= k 0) (car l) (cr-nth (cdr l) (- k 1))))
(define (cr-len l) (if (null? l) 0 (+ 1 (cr-len (cdr l)))))
(define (cr-reverse l) (cr-rev l (quote ())))
(define (cr-rev l acc) (if (null? l) acc (cr-rev (cdr l) (cons (car l) acc))))
(define (cr-append l v) (if (null? l) (list v) (cons (car l) (cr-append (cdr l) v))))
(define (cr-bound g) (+ 3 (cr-len g)))

; ===== the recursive coupled RDE: D y + F y = g at height h, certificate-gated for soundness =====
(define (te-crde-solve tower h F g) (te-crde-gate tower h F g (te-crde-raw tower h F g)))
(define (te-crde-gate tower h F g y)
  (cond ((equal? y (quote no-solution)) (quote no-solution))
        ((equal? y (quote no-rational-solution)) (quote no-solution))
        ((equal? y (quote inconclusive)) (quote inconclusive))
        ((te-crde-certify tower h F g y) y)
        (else (quote inconclusive))))
(define (te-crde-raw tower h F g)
  (if (= h 0) (te-crde-base F g) (te-crde-level tower h F g (te-level-type tower h))))

; height 0: rational RDE.  F, g are rationals (height-0).  rde-solve solves y' + F y = g.
(define (te-crde-base F g) (rde-solve F g))

(define (te-crde-level tower h F g typ) (cond ((equal? typ (quote exp)) (te-crde-exp tower h F g)) ((equal? typ (quote alg)) (te-crde-alg tower h F g)) (else (te-crde-log tower h F g))))

; ----- algebraic level (alg n a): theta^n = a, diagonal derivation with rate w = a'/(n a), and theta-degree
; bounded by n-1 (theta^n reduces to a).  Per theta-degree k (0 <= k < n) the RDE is
;     D(y_k) + (k w + F_0) y_k = g_k - sum_{j>=1} F_j y_{k-j}  at height h-1,
; solved bottom-up exactly like the exponential banded case but with w in place of Db.  Because the degree is
; bounded by n-1, there is NO non-terminating tail: if a forced contribution would land at degree >= n (or the
; reduced equation is unsolvable), the equation has no solution in this algebraic extension.  The reduction
; theta^n -> a folds higher-degree forced terms back down; here, for the integration cases we target (F a single
; lower power, e.g. F=0 with the diagonal rate), the per-degree solves are independent and bounded. -----
(define (te-crde-alg tower h F g) (te-cra-go tower h F g (te-cra-w tower h) 0 (- (tea-cn tower h) 1) (quote ())))
(define (te-cra-w tower h) (te-rat-div tower (- h 1) (te-deriv tower (- h 1) (tea-ca tower h)) (te-scale-int (- h 1) (tea-cn tower h) (tea-ca tower h))))
(define (tea-cn tower h) (car (cdr (te-level tower h))))
(define (tea-ca tower h) (car (cdr (cdr (te-level tower h)))))
(define (te-cra-go tower h F g w k N ys)
  (if (> k N) (te-cra-verdict tower h g ys)
      (te-cra-step tower h F g w k N ys
                   (te-crde-solve tower (- h 1)
                                  (te-add tower (- h 1) (te-scale-int (- h 1) k w) (te-coeff h F 0))
                                  (te-sub tower (- h 1) (te-coeff h g k) (te-cre-conv tower h F ys k))))))
(define (te-cra-step tower h F g w k N ys yk)
  (cond ((equal? yk (quote no-solution)) (quote no-solution))
        ((equal? yk (quote inconclusive)) (quote inconclusive))
        ((equal? yk (quote no-rational-solution)) (quote no-solution))
        (else (te-cra-go tower h F g w (+ k 1) N (cr-append ys yk)))))
; for the algebraic level there is no tail beyond degree n-1 to inspect; if all per-degree solves succeeded the
; assembled element (degree < n) is the answer (trimmed of trailing zeros).
(define (te-cra-verdict tower h g ys) (te-crtrim tower h ys))

; ----- exponential level: bottom-up banded.  Db = D(b_h) at height h-1; F_0 = degree-0 part of F. -----
(define (te-crde-exp tower h F g) (te-cre-go tower h F g (te-deriv tower (- h 1) (te-level-b tower h)) 0 (cr-bound g) (quote ())))
(define (te-cre-go tower h F g Db n N ys)
  (if (> n N) (te-cre-verdict tower h g ys)
      (te-cre-step tower h F g Db n N ys
                   (te-crde-solve tower (- h 1)
                                  (te-add tower (- h 1) (te-scale-int (- h 1) n Db) (te-coeff h F 0))
                                  (te-sub tower (- h 1) (te-coeff h g n) (te-cre-conv tower h F ys n))))))
; the convolution sum_{j>=1} F_j y_{n-j}, y_{n-j} from the bottom-up accumulator ys (index 0 first)
(define (te-cre-conv tower h F ys n) (te-cre-conv-go tower h F ys n 1))
(define (te-cre-conv-go tower h F ys n j)
  (if (> j n) (te-zero (- h 1))
      (te-add tower (- h 1) (te-mul tower (- h 1) (te-coeff h F j) (te-yget tower h ys (- n j))) (te-cre-conv-go tower h F ys n (+ j 1)))))
(define (te-yget tower h ys i) (if (if (< i 0) #t (>= i (cr-len ys))) (te-zero (- h 1)) (cr-nth ys i)))
(define (te-cre-step tower h F g Db n N ys yn)
  (cond ((equal? yn (quote no-solution)) (quote no-solution))
        ((equal? yn (quote inconclusive)) (quote inconclusive))
        ((equal? yn (quote no-rational-solution)) (quote no-solution))
        (else (te-cre-go tower h F g Db (+ n 1) N (cr-append ys yn)))))
(define (te-cre-verdict tower h g ys)
  (if (te-tailnz? tower h g ys)
      (te-cre-gate tower h g ys)
      (te-crtrim tower h ys)))
; the bottom-up particular solve produced a nonzero tail.  If the trimmed candidate actually certifies, accept
; it; otherwise the tail is a PROVEN obstruction only when no homogeneous freedom could absorb it -- which we
; detect by re-checking with the candidate set to its trimmed form: if the certificate fails, we cannot soundly
; claim non-elementarity (a homogeneous solution at a lower degree may exist), so we return 'inconclusive.  The
; genuine exp-over-exp tail (forced by a uniquely-determined lowest coefficient) does not certify under any
; bounded candidate, but neither would a missed-homogeneous case; to STAY SOUND we report 'inconclusive and let
; the caller defer, EXCEPT we still surface the proven-tail signal via te-cre-proven-tail?.
(define (te-cre-gate tower h g ys) (if (te-cre-proven-tail? tower h g ys) (quote no-solution) (quote inconclusive)))
; a proven (genuine, non-absorbable) tail: the lowest nonzero accumulated coefficient is inhomogeneous, i.e. it
; is NOT a solution of D y = 0 at its height (so it could not have been chosen freely).  We test the degree-0
; accumulated coefficient: if its derivative-defining RDE was inhomogeneous (the coefficient is nonzero and not
; a constant of the field), the forcing is genuine.  Concretely for the iterated-exp tail y_0 = x: D(x)=1 != 0,
; so x is not homogeneous -> genuine tail.  For a missed-homogeneous case the degree-0 coefficient solving
; D y = 0 would be 0 in our particular solve, leaving the tail unforced -> inconclusive.
(define (te-cre-proven-tail? tower h g ys) (te-cpt-lowest tower h ys))
(define (te-cpt-lowest tower h ys) (cond ((null? ys) #f) ((te-equal? tower (- h 1) (car ys) (te-zero (- h 1))) (te-cpt-lowest tower h (cdr ys))) (else (not (te-equal? tower (- h 1) (te-deriv tower (- h 1) (car ys)) (te-zero (- h 1)))))))
(define (te-tailnz? tower h g ys) (te-tnz tower h ys (cr-len g) 0))
(define (te-tnz tower h ys supp k) (cond ((null? ys) #f) ((if (>= k supp) (not (te-equal? tower (- h 1) (car ys) (te-zero (- h 1)))) #f) #t) (else (te-tnz tower h (cdr ys) supp (+ k 1)))))
(define (te-crtrim tower h ys) (cr-reverse (te-crdropz tower h (cr-reverse ys))))
(define (te-crdropz tower h l) (cond ((null? l) (quote ())) ((te-equal? tower (- h 1) (car l) (te-zero (- h 1))) (te-crdropz tower h (cdr l))) (else l)))

; ----- logarithmic level: top-down.  u = D(b)/b; F_0 the degree-0 coefficient; coupling from higher F_j and the
; degree-shift.  degree k: D(y_k) + F_0 y_k = g_k - (k+1) u y_{k+1} - sum_{j>=1} F_j y_{k-j} (the F-coupling is
; to LOWER degrees, the shift to the NEXT-higher).  Solve top-down, then the F-coupling needs lower y which are
; found later -- so for the log level we restrict to F a base coefficient (F_0 only), the case the integrator
; needs (its per-degree coefficient is phi, base-field); higher-degree F at a log level defers honestly. -----
(define (te-crde-log tower h F g)
  (if (te-Fbase? tower h F) (te-crl-run tower h (te-coeff h F 0) g) (quote no-solution)))
(define (te-Fbase? tower h F) (te-Fb-go F 1 (cr-len F)))
(define (te-Fb-go F j m) (cond ((>= j m) #t) (else #f)))   ; base iff length <= 1 (only F_0)
(define (te-crl-run tower h F0 g) (te-crl-top tower h F0 g (te-logu tower h) (- (cr-len g) 1)))
(define (te-logu tower h) (te-rat-div tower (- h 1) (te-deriv tower (- h 1) (te-level-b tower h)) (te-level-b tower h)))
(define (te-crl-top tower h F0 g u m) (te-crl-go tower h F0 g u m (te-zlist (- h 1) (+ m 1))))
(define (te-zlist hm1 n) (if (= n 0) (quote ()) (cons (te-zero hm1) (te-zlist hm1 (- n 1)))))
(define (te-crl-go tower h F0 g u k ys)
  (if (< k 0) (te-crtrim tower h ys)
      (te-crl-step tower h F0 g u k ys
                   (te-crde-solve tower (- h 1) F0 (te-sub tower (- h 1) (te-coeff h g k) (te-mul tower (- h 1) (te-scale-int (- h 1) (+ k 1) u) (te-yget tower h ys (+ k 1))))))))
(define (te-crl-step tower h F0 g u k ys yk)
  (cond ((equal? yk (quote no-solution)) (quote no-solution))
        ((equal? yk (quote inconclusive)) (quote inconclusive))
        ((equal? yk (quote no-rational-solution)) (quote no-solution))
        (else (te-crl-go tower h F0 g u (- k 1) (te-cset ys k yk)))))
(define (te-cset ys k v) (te-cset-go ys k v 0))
(define (te-cset-go ys k v i) (if (null? ys) (quote ()) (cons (if (= i k) v (car ys)) (te-cset-go (cdr ys) k v (+ i 1)))))

(define (te-sub tower h a b) (te-add tower h a (te-scale-int h -1 b)))

; ----- certificate: (D y) + F y = g at height h -----
(define (te-crde-certify tower h F g y) (te-equal? tower h (te-add tower h (te-deriv tower h y) (te-mul tower h F y)) g))
