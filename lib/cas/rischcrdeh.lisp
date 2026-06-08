; -*- lisp -*-
; lib/cas/rischcrdeh.lisp -- the COUPLED-RDE COMPLETENESS layer: the homogeneous-constant bookkeeping that lets
; solvable coupled cases (which the sound-but-incomplete rischcrde reports as 'inconclusive) actually be SOLVED.
; This recovers integrals like INT (e^x e^{e^x}) = e^{e^x} through the recursion, closing the gap left open by
; rischcrde while preserving its soundness (docs/TRAGER_ROADMAP.md, the summit, "coupled completeness").
;
; The gap and the fix.  In the exp-level banded recurrence (rischcrde), the degree-n RDE
; D(y_n) + (n Db + F_0) y_n = g_n - sum_{j>=1} F_j y_{n-j} has, when its coefficient vanishes (the degree-0
; homogeneous case D(y_0) = RHS), a solution determined only up to an additive CONSTANT.  rischcrde's bottom-up
; solve picks the no-constant branch, which can force a spurious non-terminating tail even when a specific
; constant would terminate it (e.g. the height-1 subproblem D(c) + theta_1 c = theta_1, where c = 1 terminates
; but the no-constant branch does not).  The tail depends LINEARLY on that degree-0 constant V, so we determine
; V by two probe solves -- the recurrence run with V = 0 and with V = 1 -- and solve the resulting linear
; condition at the leading tail position for the V that cancels the tail.  Re-running with that V and CERTIFYING
; (te-crde-certify) makes the result sound: a solution is returned only if it actually satisfies D y + F y = g.
;
; Scope.  This implements the single-degree-0-constant case, which is exactly what the height-n integrator's
; exp-over-exp subproblems need (the top-degree coupled RDE with a constant homogeneous freedom at the bottom).
; Deeper multi-parameter homogeneous spaces remain future work; when the one-parameter fix does not certify, we
; fall back to rischcrde's honest 'inconclusive, so soundness always holds.
;
; Public:
;   te-crdeh-solve tower h F g -> y | 'no-solution | 'inconclusive : the completed coupled solve
;   te-crdeh-integrate tower h f -> (list 'elementary y) | (list 'non-elementary ..) | (list 'deferred ..)
;
; Verified: INT (e^x e^{e^x}) = e^{e^x} now SOLVED and certified through the recursion; INT e^{e^x} still
; correctly non-elementary; height-1 cases unaffected; all results certificate-checked.
;
; Builds on rischcrde.lisp (the sound coupled solver) and rischtowern.lisp.

(import "cas/rischcrde.lisp")
(import "cas/rischtowern.lisp")

(define (ch-nth l k) (if (= k 0) (car l) (ch-nth (cdr l) (- k 1))))
(define (ch-len l) (if (null? l) 0 (+ 1 (ch-len (cdr l)))))
(define (ch-append l v) (if (null? l) (list v) (cons (car l) (ch-append (cdr l) v))))
(define (ch-reverse l) (ch-rev l (quote ())))
(define (ch-rev l acc) (if (null? l) acc (ch-rev (cdr l) (cons (car l) acc))))

; ===== the completed solve: try the sound solver; if inconclusive, attempt the homogeneous-constant fix =====
(define (te-crdeh-solve tower h F g)
  (te-crdeh-dispatch tower h F g (te-crde-solve tower h F g)))
(define (te-crdeh-dispatch tower h F g base)
  (if (equal? base (quote inconclusive)) (te-crdeh-try tower h F g) base))

; the homogeneous-constant fix, exp level only (the integrator's coupled subproblems are exp).  We run the
; banded recurrence with the degree-0 coefficient constant set to 0 and to 1, read the leading tail term in each,
; solve the linear condition for the terminating constant V, and re-run + certify.
(define (te-crdeh-try tower h F g)
  (if (= h 0) (quote inconclusive)
      (if (equal? (te-level-type tower h) (quote exp)) (te-crdeh-exp tower h F g) (quote inconclusive))))

; run the bottom-up exp recurrence with an injected degree-0 constant V (a height-(h-1) element); return the
; full coefficient list (no tail trimming) up to the bound, so we can inspect the tail.
(define (te-crdeh-run tower h F g V)
  (te-cr-go tower h F g (te-deriv tower (- h 1) (te-level-b tower h)) 0 (ch-bound g) (quote ()) V))
(define (ch-bound g) (+ 3 (ch-len g)))
(define (te-cr-go tower h F g Db n N ys V)
  (if (> n N) ys
      (te-cr-step tower h F g Db n N ys V (te-cr-solve-deg tower h F g Db n ys V))))
(define (te-cr-solve-deg tower h F g Db n ys V)
  (te-cr-pick tower h n V
              (te-add tower (- h 1) (te-scale-int (- h 1) n Db) (te-coeff h F 0))
              (te-sub tower (- h 1) (te-coeff h g n) (te-cr-conv tower h F ys n))))
; at degree 0 with zero coefficient, use the injected constant V; otherwise solve the base RDE RECURSIVELY with
; the completeness layer (so homogeneous freedom at any level is handled).
(define (te-cr-pick tower h n V coeff rhs)
  (if (if (= n 0) (te-equal? tower (- h 1) coeff (te-zero (- h 1))) #f)
      V
      (te-crdeh-solve tower (- h 1) coeff rhs)))
(define (te-cr-conv tower h F ys n) (te-cr-conv-go tower h F ys n 1))
(define (te-cr-conv-go tower h F ys n j)
  (if (> j n) (te-zero (- h 1))
      (te-add tower (- h 1) (te-mul tower (- h 1) (te-coeff h F j) (te-cr-yget tower h ys (- n j))) (te-cr-conv-go tower h F ys n (+ j 1)))))
(define (te-cr-yget tower h ys i) (if (if (< i 0) #t (>= i (ch-len ys))) (te-zero (- h 1)) (ch-nth ys i)))
(define (te-cr-step tower h F g Db n N ys V yn)
  (cond ((equal? yn (quote no-solution)) ys)
        ((equal? yn (quote no-rational-solution)) ys)
        ((equal? yn (quote inconclusive)) ys)
        (else (te-cr-go tower h F g Db (+ n 1) N (ch-append ys yn) V))))

; the leading tail term: the first coefficient beyond g's support (index = support length).  Returns the
; height-(h-1) element at that index in the run (zero if the run stopped short).
(define (te-crdeh-tail tower h g ys) (te-cr-yget tower h ys (ch-len g)))

; solve for V.  tail(V) is affine in V: tail(V) = tail0 + (tail1 - tail0) * V (V scalar at the bottom).  Here V
; ranges over height-(h-1) constants; for the integrator's case the relevant freedom is a base constant, so we
; take V in {0, 1} as height-(h-1) elements (te-zero, te-one) and solve tail0 + (tail1 - tail0) V = 0 for a
; rational V at the base, then lift.  We restrict to the case where tail0 and (tail1 - tail0) are base-field
; constants (height-(h-1) elements whose only nonzero part is a rational), solving V = tail0 / (tail0 - tail1).
(define (te-crdeh-exp tower h F g)
  (te-crdeh-exp-go tower h F g (te-crdeh-trim tower h (te-crdeh-run tower h F g (te-zero (- h 1))))))
; if the no-constant (V=0) run already gives a certified solution, take it; else attempt the constant solve.
(define (te-crdeh-exp-go tower h F g y0)
  (if (te-crde-certify tower h F g y0) y0 (te-crdeh-solveV-pass tower h F g)))
(define (te-crdeh-solveV-pass tower h F g)
  (te-crdeh-finish tower h F g
                   (te-crdeh-run tower h F g (te-zero (- h 1)))
                   (te-crdeh-run tower h F g (te-one (- h 1)))))
(define (te-crdeh-finish tower h F g run0 run1)
  (te-crdeh-checkV tower h F g (te-solveV tower h (te-crdeh-tail tower h g run0) (te-crdeh-tail tower h g run1))))
; V = tail0 / (tail0 - tail1) as a base-field (height h-1) element, when both tails are base constants.
(define (te-solveV tower h tail0 tail1)
  (te-vdiv tower h tail0 (te-sub tower (- h 1) tail0 tail1)))
(define (te-vdiv tower h a b)
  (if (te-equal? tower (- h 1) b (te-zero (- h 1))) (quote nov) (te-rat-div tower (- h 1) a b)))
; re-run with the solved V, trim, and certify; if anything fails, fall back to inconclusive (soundness).
(define (te-crdeh-checkV tower h F g V)
  (if (equal? V (quote nov)) (quote inconclusive)
      (te-crdeh-verify tower h F g (te-crdeh-trim tower h (te-crdeh-run tower h F g V)))))
(define (te-crdeh-trim tower h ys) (ch-reverse (te-crdeh-dropz tower h (ch-reverse ys))))
(define (te-crdeh-dropz tower h l) (cond ((null? l) (quote ())) ((te-equal? tower (- h 1) (car l) (te-zero (- h 1))) (te-crdeh-dropz tower h (cdr l))) (else l)))
(define (te-crdeh-verify tower h F g y) (if (te-crde-certify tower h F g y) y (quote inconclusive)))

; ===== integration entry with the completeness layer =====
(define (te-crdeh-integrate tower h f) (te-crdeh-result (te-crdeh-solve tower h (te-zero h) f)))
(define (te-crdeh-result y)
  (cond ((equal? y (quote no-solution)) (list (quote non-elementary) (quote tower-rde-obstruction)))
        ((equal? y (quote inconclusive)) (list (quote deferred) (quote needs-deeper-homogeneous)))
        (else (list (quote elementary) y))))
