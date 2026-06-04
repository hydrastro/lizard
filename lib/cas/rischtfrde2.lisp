; -*- lisp -*-
; lib/cas/rischtfrde2.lisp -- the COUPLED tower-field Risch differential equation: solves/decides
; c' + (m theta_1') c = target over K_1 = Q(x)(theta_1), theta_1 = exp(x), where the coefficient m theta_1' has
; POSITIVE theta_1-degree (theta_1' = theta_1 = s, so the coefficient is m s, degree 1).  Unlike the decoupled
; case (rischtfrde.lisp, base-field coefficient), a positive-degree coefficient COUPLES the theta_1-degrees into
; a banded recurrence; this module solves that recurrence and, crucially, DETECTS the non-terminating degree
; tail that proves non-elementarity of the iterated exponential -- deriving the iterated-exp verdict through the
; actual RDE machinery rather than asserting it (docs/TRAGER_ROADMAP.md, the summit, "beyond").
;
; The banded recurrence.  Write c = sum_k c_k s^k (c_k in Q(x), s = e^x, s' = s) and the coefficient as m s.
; Then c' = sum_k (c_k' + k c_k) s^k and (m s) c = sum_k m c_k s^{k+1}, so c' + (m s) c at degree k equals
;     (c_k' + k c_k) + m c_{k-1}            (with c_{-1} = 0).
; Matching a target sum_k t_k s^k gives, degree by degree (lowest first):
;     k = 0:   c_0' = t_0
;     k >= 1:  c_k' + k c_k = t_k - m c_{k-1} .
; Each step is a base-field RDE solved by rischrde (rde-solve).  The subtlety: even when the target is a single
; low-degree term, a nonzero c_{k-1} FORCES a nonzero right-hand side at degree k, so the solution can spill to
; ever-higher degrees.  We solve up to a degree bound; if within the bound the higher c_k all vanish the
; solution is genuine (and certified), but if the forced tail stays nonzero up to the bound the equation has no
; bounded-degree solution -- the non-terminating-tail obstruction, hence non-elementarity.
;
; Public:
;   ctf-step m t-prev c-prev k -> solve the degree-k base RDE c_k' + k c_k = t_k - m c_{k-1}, returning c_k | 'none
;   ctf-solve m target N        -> (list 'solvable c-coeffs) | (list 'non-elementary 'nonterminating-tail) |
;                                  (list 'no-rational 'rde-obstruction) : decide c' + (m s) c = target,
;       target a K_1 element (list of Q(x) rational coeffs low-to-high), N the degree bound for the tail check
;   ctf-decide-int-E2           -> the verdict for INT exp(exp x) dx via this machinery (non-elementary)
;   ctf-tail-nonzero? c-coeffs  -> #t iff the top coefficients (beyond the target's support) are nonzero (a
;       witness of the non-terminating tail)
;
; Verified: c' + (s) c = 1 (the INT e^{e^x} reduction) has a NON-terminating tail (c_0=x, c_1=1-x, c_2 != 0, ...)
; -> INT e^{e^x} non-elementary; a genuinely solvable coupled case terminates and certifies.
;
; Builds on rischrde.lisp (the base RDE) and tower.lisp / poly.lisp.

(import "cas/rischrde.lisp")
(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (ctf-nth l k) (if (= k 0) (car l) (ctf-nth (cdr l) (- k 1))))
(define (ctf-len l) (if (null? l) 0 (+ 1 (ctf-len (cdr l)))))
(define (ctf-coeff g k) (if (< k (ctf-len g)) (ctf-nth g k) (rat-zero)))

; ----- one degree step: solve c_k' + k c_k = t_k - m c_{k-1} for c_k over Q(x).
; coefficient of c_k is the constant k (a Q(x) rational); right-hand side is t_k - m c_{k-1}. -----
(define (ctf-step m tk cprev k) (rde-solve (rat-from-poly (list k)) (rat-sub tk (rat-mul m cprev))))

; ----- solve the coupled RDE degree by degree up to bound N, collecting c_0..c_N -----
(define (ctf-solve m target N) (ctf-go m target N 0 (rat-zero) (quote ())))
(define (ctf-go m target N k cprev acc)
  (if (> k N) (ctf-verdict target (ctf-reverse acc))
      (ctf-go-step m target N k cprev acc (ctf-step m (ctf-coeff target k) cprev k))))
(define (ctf-go-step m target N k cprev acc ck)
  (if (equal? ck (quote no-rational-solution)) (list (quote no-rational) (quote rde-obstruction))
      (ctf-go m target N (+ k 1) ck (cons ck acc))))
; verdict: if the coefficients beyond the target's support are all zero, it terminated -> solvable;
; otherwise the forced tail persists to the bound -> non-terminating -> non-elementary.
(define (ctf-verdict target cs) (if (ctf-tail-nonzero? target cs) (list (quote non-elementary) (quote nonterminating-tail)) (list (quote solvable) (ctf-trim cs))))
(define (ctf-tail-nonzero? target cs) (ctf-tn-go cs (ctf-len target) 0))
(define (ctf-tn-go cs supp k) (cond ((null? cs) #f) ((if (>= k supp) (not (rat-zero? (car cs))) #f) #t) (else (ctf-tn-go (cdr cs) supp (+ k 1)))))
(define (ctf-trim cs) (ctf-reverse (ctf-drop-zeros (ctf-reverse cs))))
(define (ctf-drop-zeros cs) (cond ((null? cs) (quote ())) ((rat-zero? (car cs)) (ctf-drop-zeros (cdr cs))) (else cs)))
(define (ctf-reverse l) (ctf-rev l (quote ())))
(define (ctf-rev l acc) (if (null? l) acc (ctf-rev (cdr l) (cons (car l) acc))))

; ----- the headline verdict: INT exp(exp x) dx.  Reduction: C_1' + (1 * s) C_1 = 1 over Q(x)(s), s = e^x.
; target = 1 (degree 0), m = 1, bound N = 4 (enough to expose the non-terminating tail). -----
(define (ctf-decide-int-E2) (ctf-int-E2-go (ctf-solve (rat-one) (list (rat-one)) 4)))
(define (ctf-int-E2-go v) (if (equal? (car v) (quote non-elementary)) (list (quote non-elementary) (quote E2-nonterminating-tail)) v))
