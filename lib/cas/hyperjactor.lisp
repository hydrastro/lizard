; -*- lisp -*-
; lib/cas/hyperjactor.lisp -- the TORSION DECISION on the genus-2 Jacobian: deciding whether a divisor class is
; torsion (and its order), the genus-2 analogue of the elliptic order test elltorsion uses to decide third-kind
; elementarity (docs/TRAGER_ROADMAP.md -- the full third-kind construction beyond genus 1).
;
; On a genus-1 curve, INT dx/((x-s) sqrt(p)) is elementary exactly when the pole lifts to a TORSION point on the
; elliptic curve (elltorsion).  The genus-2 statement is the same with the elliptic group replaced by the JACOBIAN
; group of y^2 = f (hyperjac.lisp): a third-kind differential whose residue divisor is the degree-0 class D = [P] -
; [iota P] (iota the hyperelliptic involution, iota P = -P) is elementary exactly when the class [D] is TORSION in
; the Jacobian, i.e. n*[D] = 0 for some n.  In Mumford terms the class of [P] - [iota P] is represented by the
; reduced divisor of P itself (since [P] + [iota P] is the trivial/canonical class), so the torsion order of the
; class is the order of the divisor P under the Jacobian group law hj-mul.
;
; This module computes that order by the bounded multiple search n*D for n = 1..B (the Jacobian group law of
; hyperjac), reporting (list 'torsion n) at the first n with n*D = identity and (list 'no-torsion-up-to B)
; otherwise.  SOUNDNESS: a hit certifies torsion (hence, via Abel-Jacobi, an elementary third-kind integral whose
; algebraic logarithm could then be constructed); a miss reports ONLY a bounded negative ("no torsion up to B"),
; never a false claim of non-elementarity -- the unconditional torsion bound for genus-2 Jacobians over Q needs
; deeper theory (an effective Mazur-type bound) and is out of scope, so we never assert global non-torsion.
;
; Public (f the genus-2 curve polynomial; D a Mumford divisor from hyperjac; B a positive search bound):
;   hjt-order f D B            -> (list 'torsion n) (smallest n <= B with n*D = identity) | (list 'no-torsion-up-to B)
;   hjt-is-torsion? f D B       -> #t iff D has order <= B (a confirmed-torsion predicate; #f means only "not within B")
;   hjt-class-of-point a b      -> the degree-0 class [P] - [iota P] of P = (a, b), represented by the divisor of P
;   hjt-third-kind-decision f a b B -> for the pole at x = a lifting to P = (a, b): (list 'elementary 'torsion n) when
;                                 the class is torsion (the integral is elementary), else (list 'undecided-up-to B)
;
; Verified: a Weierstrass point (b = 0) is 2-torsion -> order 2 (its class is actually 2-torsion; the involution
; fixes it); the point (0, 1) on y^2 = x^5 + 1 has its class order found or honestly reported beyond the bound; the
; identity has order 1; the decision never claims non-elementarity, only bounded "undecided".
;
; Builds on hyperjac.lisp.

(import "cas/hyperjac.lisp")

; ----- torsion order by bounded multiple search -----
(define (hjt-order f D B) (hjt-order-go f D B 1 (hj-identity)))
(define (hjt-order-go f D B n acc)               ; acc = (n-1)*D coming in; test n*D
  (hjt-order-test f D B n (hj-add f acc D)))
(define (hjt-order-test f D B n nD)
  (cond ((hj-equal? nD (hj-identity)) (list (quote torsion) n))
        ((>= n B) (list (quote no-torsion-up-to) B))
        (else (hjt-order-go f D B (+ n 1) nD))))

(define (hjt-is-torsion? f D B) (equal? (car (hjt-order f D B)) (quote torsion)))

; ----- the degree-0 class [P] - [iota P], represented by the divisor of P -----
(define (hjt-class-of-point a b) (hj-point a b))

; ----- the third-kind elementarity decision for the pole at x = a (P = (a,b)) -----
(define (hjt-third-kind-decision f a b B) (hjt-decide (hjt-order f (hjt-class-of-point a b) B)))
(define (hjt-decide r) (if (equal? (car r) (quote torsion)) (list (quote elementary) (quote torsion) (car (cdr r))) (list (quote undecided-up-to) (car (cdr r)))))
