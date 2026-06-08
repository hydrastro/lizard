; -*- lisp -*-
; lib/cas/hyperaperiodic.lisp -- the UNCONDITIONAL aperiodicity certificate for sqrt(f): a finite, exact proof that
; a hyperelliptic curve y^2 = f has NO Pell unit, by traversing the full cycle of reduced complete quotients of the
; continued fraction until one repeats (docs/TRAGER_ROADMAP.md -- the full third-kind construction; turning the
; bounded "aperiodic-up-to-B" negative of polycf into a real theorem).
;
; polycf expands sqrt(f) as a continued fraction with complete quotients (P_i + sqrt f)/Q_i and reports a Pell unit
; exactly when some Q_i returns to a nonzero CONSTANT.  When none does within a search bound it can only say
; "aperiodic up to B" -- a bounded negative, not a proof.  This module supplies the proof.  Over Q[x] with
; deg f = 2d the CF of sqrt(f) is PURELY periodic and the REDUCED complete quotients are FINITE in number (P_i is
; pinned to degree d matching a0 at the top, and Q_i is a factor of f - P_i^2 of degree <= d), so the sequence of
; pairs (P_i, Q_i) must eventually REPEAT.  Tracking the pairs until the first repeat means the ENTIRE cycle has
; been traversed; then exactly one of two things has happened:
;     * some Q_i in the cycle was a nonzero constant  ->  a Pell unit exists (periodic), or
;     * the cycle closed with NO Q_i a nonzero constant  ->  there is NO Pell unit, UNCONDITIONALLY.
; In the second case the class of the divisor at infinity is non-torsion in the Jacobian, so the first-kind
; integral INT dx/sqrt(f) -- and any third-kind integral whose residue divisor is a nonzero multiple of it -- is
; NON-ELEMENTARY, and this is now a finite proof rather than a failed bounded search.  The traversal is gated by a
; safety bound only to stay finite against a representation slip; under correct arithmetic the repeat is reached
; well within it, and the verdict distinguishes "cycle closed" (a real proof) from "bound hit" (still bounded).
;
; Everything is exact over Q[x]: the pairs (P_i, Q_i) are polynomials, equality is polynomial equality, and the
; constant test is a degree test.  No floating point, no heuristic.
;
; Public (f a polynomial of even degree; B a safety bound on the cycle length):
;   ap-cycle f B               -> the list of complete-quotient pairs ((P0 Q0)(P1 Q1)...) until the first repeat,
;                                 or until B (each pair the (P, Q) of a complete quotient)
;   ap-cycle-closed? f B        -> #t iff a pair repeated within B (the full cycle was seen, a real proof basis)
;   ap-has-constant-Q? f B      -> #t iff some Q in the traversed cycle is a nonzero constant (a Pell unit exists)
;   ap-no-unit-proof f B        -> (list 'proven-no-unit len) iff the cycle closed with no constant Q (UNCONDITIONAL),
;                                  (list 'has-unit len) iff a constant Q occurred, (list 'bound-hit B) if neither
;   ap-integral-verdict f B     -> 'non-elementary (proven) | 'elementary (Pell unit) | 'undecided-bounded
;   ap-is-torsion-class? f B    -> #t iff a Pell unit exists (the infinity class is torsion), #f iff proven none,
;                                  'undecided-bounded if the bound was hit
;
; Verified: y^2 = x^6 + 1 has a constant Q at the first step (a Pell unit -> elementary, torsion class); a curve
; whose cycle closes with no constant Q is proven to have no unit (non-elementary, non-torsion), unconditionally;
; the bound-hit case is reported distinctly from a closed cycle so the negative is never overstated.
;
; Builds on polycf.lisp (the CF step and unit machinery) and poly.lisp.

(import "cas/polycf.lisp")

(define (ap-len l) (if (null? l) 0 (+ 1 (ap-len (cdr l)))))

; ----- a complete-quotient pair is (P Q); equality is polynomial equality on both -----
(define (ap-pair-eq? a b) (if (ap-poly-eq? (car a) (car b)) (ap-poly-eq? (car (cdr a)) (car (cdr b))) #f))
(define (ap-poly-eq? p q) (equal? (pcf-trim p) (pcf-trim q)))
(define (ap-member-pair? x l) (cond ((null? l) #f) ((ap-pair-eq? x (car l)) #t) (else (ap-member-pair? x (cdr l)))))

; ----- traverse the CF, collecting complete-quotient pairs (P,Q) until one repeats or bound B -----
(define (ap-cycle f B) (ap-rev (ap-cyc-go f (list 0) (list 1) (quote ()) B)))
(define (ap-cyc-go f P Q seen k)
  (if (<= k 0) seen
      (ap-cyc-dispatch f P Q seen k (list P Q))))
(define (ap-cyc-dispatch f P Q seen k pair)
  (if (ap-member-pair? pair seen) (cons pair seen)            ; repeat: close the cycle (include the repeat marker)
      (ap-cyc-next f P Q (cons pair seen) k)))
(define (ap-cyc-next f P Q seen k)
  (let ((st (pcf-step f P Q)))
    (ap-cyc-go f (car (cdr st)) (car (cdr (cdr st))) seen (- k 1))))
(define (ap-rev l) (ap-rev-go l (quote ())))
(define (ap-rev-go l a) (if (null? l) a (ap-rev-go (cdr l) (cons (car l) a))))

; ----- did the cycle close (a pair repeated) within B? -----
; the cycle as collected ends with the repeated pair appended; detect by a duplicate among the pairs.
(define (ap-cycle-closed? f B) (ap-has-dup? (ap-cycle f B)))
(define (ap-has-dup? l) (cond ((null? l) #f) ((ap-member-pair? (car l) (cdr l)) #t) (else (ap-has-dup? (cdr l)))))

; ----- is some Q_i (i >= 1) in the cycle a nonzero constant? (=> a Pell unit, periodic) -----
; the FIRST pair is the trivial start (P0=0, Q0=1); Q0 is constant by construction and must be skipped, so the
; test runs over the tail of the cycle only.
(define (ap-has-constant-Q? f B) (ap-any-const-Q? (ap-tail (ap-cycle f B))))
(define (ap-tail l) (if (null? l) (quote ()) (cdr l)))
(define (ap-any-const-Q? l) (cond ((null? l) #f) ((ap-const-nonzero? (car (cdr (car l)))) #t) (else (ap-any-const-Q? (cdr l)))))
(define (ap-const-nonzero? Q) (if (<= (poly-deg Q) 0) (not (ap-zero? Q)) #f))
(define (ap-zero? Q) (null? (pcf-trim Q)))

; ----- the proof verdict -----
(define (ap-no-unit-proof f B) (ap-proof-go (ap-cycle f B) f B))
(define (ap-proof-go cyc f B)
  (cond ((ap-any-const-Q? (ap-tail cyc)) (list (quote has-unit) (ap-len cyc)))
        ((ap-has-dup? cyc) (list (quote proven-no-unit) (ap-len cyc)))   ; closed cycle, no constant Q: UNCONDITIONAL
        (else (list (quote bound-hit) B))))

; ----- the integral elementarity verdict for INT dx/sqrt(f) -----
(define (ap-integral-verdict f B) (ap-verdict-go (ap-no-unit-proof f B)))
(define (ap-verdict-go p)
  (cond ((equal? (car p) (quote has-unit)) (quote elementary))
        ((equal? (car p) (quote proven-no-unit)) (quote non-elementary))
        (else (quote undecided-bounded))))

; ----- the torsion verdict on the infinity class -----
(define (ap-is-torsion-class? f B) (ap-tor-go (ap-no-unit-proof f B)))
(define (ap-tor-go p)
  (cond ((equal? (car p) (quote has-unit)) #t)
        ((equal? (car p) (quote proven-no-unit)) #f)
        (else (quote undecided-bounded))))
