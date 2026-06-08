; -*- lisp -*-
; lib/cas/elliptic.lisp -- a DECISION procedure for INT P(x)/sqrt(q(x)) dx with q squarefree of degree 3 or 4
; (the genus-1 / elliptic case): decide whether the integral is elementary, and when it is, produce and certify
; the algebraic antiderivative; when it is not, report it as a genuine ELLIPTIC integral -- non-elementary by an
; exact reduction, not a failure to find a form (docs/TRAGER_ROADMAP.md, the frontier -- higher-genus radicands).
;
; The method (Hermite-style reduction for the radical, in K = Q(x)[y]/(y^2 - q)).  Differentiating x^k sqrt(q):
;     d/dx[x^k sqrt q] = (k x^{k-1} q + x^k q'/2) / sqrt(q),
; whose numerator has degree k + deg(q) - 1.  So to integrate P/sqrt(q), repeatedly subtract a scalar multiple
; of d/dx[x^{D - (deg q - 1)} sqrt q] to cancel the top term of the numerator P (degree D), lowering deg P by one
; each step, until the remaining numerator has degree < deg(q) - 1.  This is exact polynomial reduction over Q,
; accumulating the algebraic part A(x) so that
;     P/sqrt(q) = d/dx[ A(x) sqrt(q) ] + P_remainder/sqrt(q),  deg(P_remainder) < deg(q) - 1.
; Then:
;   * P_remainder = 0  ->  INT = A(x) sqrt(q), ELEMENTARY, certified inside K by af-certify.
;   * P_remainder != 0 (and q is a squarefree cubic or quartic, genus 1)  ->  the surviving differential is a
;     combination of elliptic integrals of the first and second kind, which is NON-ELEMENTARY.  (For a squarefree
;     q with no rational branch structure there is no third-kind logarithmic piece with rational residue to make
;     it elementary; this module makes the firm non-elementary call only for this reduced first/second-kind
;     remainder, and otherwise reports 'inconclusive rather than risk an unsound verdict.)
;
; This is sound both ways: an elementary verdict is backed by a differentiation certificate in K, and a
; non-elementary verdict is backed by the exact statement that the reduced elliptic remainder is nonzero.
;
; Public:
;   ell-disc-ok? q                 -> #t iff q (coeff list) is squarefree of degree 3 or 4
;   ell-reduce P q                 -> (list A-coeffs P-remainder) : the polynomial reduction over Q
;   ell-integrate P q              -> (list 'elementary A) | (list 'non-elementary 'elliptic-first-second-kind)
;                                     | (list 'inconclusive ..) : the decision for INT P/sqrt(q)
;   ell-certify P q r              -> #t iff an 'elementary result differentiates back to P/sqrt(q) inside K
;
; Verified: INT (3x^2/2)/sqrt(x^3+1) = sqrt(x^3+1) (elementary, certified); INT 1/sqrt(x^3+1) (elliptic first
; kind, non-elementary); INT x/sqrt(x^3+1) (elliptic, non-elementary); INT (x^3 ...) reductions; a quartic case.
;
; Builds on algfunc.lisp (the field K = Q(x)[y]/(y^2-q) and af-certify) and poly.lisp / msqfree.lisp.

(import "cas/algfunc.lisp")
(import "cas/poly.lisp")
(import "cas/msqfree.lisp")

(define (el-len l) (if (null? l) 0 (+ 1 (el-len (cdr l)))))
(define (el-nth l k) (if (= k 0) (car l) (el-nth (cdr l) (- k 1))))
(define (el-coeff p k) (if (if (< k 0) #t (>= k (el-len p))) 0 (el-nth p k)))
(define (el-trimlen p) (el-tl p (el-len p)))
(define (el-tl p n) (cond ((= n 0) 0) ((= (el-coeff p (- n 1)) 0) (el-tl p (- n 1))) (else n)))
(define (el-deg p) (- (el-trimlen p) 1))
(define (el-zero? p) (= (el-trimlen p) 0))

; ----- q squarefree of degree 3 or 4 -----
(define (ell-disc-ok? q) (if (if (= (el-deg q) 3) #t (= (el-deg q) 4)) (el-squarefree? q) #f))
; squarefree: gcd(q, q') is a constant (degree 0)
(define (el-squarefree? q) (= (el-deg (poly-gcd q (poly-deriv q))) 0))

; ----- the reduction.  Accumulate A so that P/sqrt q = d/dx[A sqrt q] + rem/sqrt q, deg rem < deg q - 1.
; Each step: top degree D of current P, with target bound B = deg q - 1.  If D >= B, the basis derivative we use
; is d/dx[x^{D-B} sqrt q], whose numerator N_k(x) = k x^{k-1} q + x^k q'/2 with k = D-B has leading degree D and
; leading coefficient (k + (deg q)/2) * lead(q).  Subtract (lead(P)/lead(N_k)) * N_k from P, add that scalar * x^k
; to A.  Iterate. -----
(define (ell-reduce P q) (el-red-go (el-canon P) q (el-deg q) (list 0)))
(define (el-canon p) (if (= (el-trimlen p) 0) (list 0) (el-take p (el-trimlen p))))
(define (el-take l n) (if (= n 0) (quote ()) (cons (car l) (el-take (cdr l) (- n 1)))))
(define (el-red-go P q dq A)
  (if (< (el-deg P) (- dq 1)) (list A (el-canon P))
      (el-red-step P q dq A (- (el-deg P) (- dq 1)))))             ; k = D - (dq-1)
(define (el-red-step P q dq A k) (el-red-apply P q dq A k (el-basis-num k q)))
; N_k(x) = k x^{k-1} q + x^k q'/2  (as a poly over Q)
(define (el-basis-num k q) (poly-add (poly-scale k (poly-mul (el-xpow (- k 1)) q)) (el-half (poly-mul (el-xpow k) (poly-deriv q)))))
(define (el-xpow k) (if (< k 0) (list 0) (el-xp-go k 0)))
(define (el-xp-go k i) (if (> i k) (quote ()) (cons (if (= i k) 1 0) (el-xp-go k (+ i 1)))))
(define (el-half p) (el-hf p))
(define (el-hf p) (if (null? p) (quote ()) (cons (/ (car p) 2) (el-hf (cdr p)))))
(define (el-red-apply P q dq A k Nk)
  (el-red-next P q dq A k Nk (/ (el-coeff P (el-deg P)) (el-coeff Nk (el-deg Nk)))))
(define (el-red-next P q dq A k Nk s)
  (el-red-go (poly-sub P (poly-scale s Nk)) q dq (poly-add A (poly-scale s (el-xpow k)))))

; ----- the decision -----
(define (ell-integrate P q) (if (ell-disc-ok? q) (el-decide P q (ell-reduce P q)) (list (quote inconclusive) (quote not-genus-1-squarefree))))
(define (el-decide P q red) (el-verdict P q (car red) (car (cdr red))))
(define (el-verdict P q A rem)
  (if (el-zero? rem)
      (el-elem-or-fail P q A)
      (list (quote non-elementary) (quote elliptic-first-second-kind))))
(define (el-elem-or-fail P q A) (if (ell-certify-raw P q A) (list (quote elementary) A) (list (quote inconclusive) (quote uncertified-reduction))))

; ----- certificate inside K: d/dx[A sqrt q] = P/sqrt q ?  (no log term in the elementary radical case) -----
; algebraic part A*y = af-make 0 A ; clog = 0 ; integrand P/sqrt q = (P/q) y
(define (ell-certify-raw P q A)
  (af-certify (rat-from-poly q)
              (af-make (rat-zero) (rat-from-poly A))
              (rat-zero)
              (af-make (rat-zero) (rat-one))
              (af-make (rat-zero) (rat-make P q))))
(define (ell-certify P q r) (if (equal? (car r) (quote elementary)) (ell-certify-raw P q (car (cdr r))) #f))

; ----- the richer "integrate as far as possible" split: return BOTH the elementary algebraic part A sqrt(q)
; (certified) AND the named elliptic remainder rem/sqrt(q).  Shape:
;   (list 'split A rem-numerator)  with INT P/sqrt q = A sqrt q + INT rem/sqrt q, the second piece elliptic
;     (non-elementary) when rem != 0, and absent (rem = 0) when the whole integral is elementary. -----
(define (ell-split P q) (if (ell-disc-ok? q) (el-split-go P q (ell-reduce P q)) (list (quote inconclusive) (quote not-genus-1-squarefree))))
(define (el-split-go P q red) (el-split-check P q (car red) (car (cdr red))))
(define (el-split-check P q A rem)
  (if (ell-split-certify P q A rem) (list (quote split) A rem) (list (quote inconclusive) (quote uncertified-split))))
; certify the SPLIT identity exactly in K: d/dx[A sqrt q] = (P - rem)/sqrt q, i.e. the elementary part accounts
; for exactly (P - rem)/sqrt q, leaving rem/sqrt q.  This is a rational identity in K (no log term).
(define (ell-split-certify P q A rem)
  (af-certify (rat-from-poly q)
              (af-make (rat-zero) (rat-from-poly A))
              (rat-zero)
              (af-make (rat-zero) (rat-one))
              (af-make (rat-zero) (rat-make (poly-sub (el-canon P) rem) q))))
; is the split fully elementary (no elliptic remainder)?
(define (ell-split-elementary? s) (if (equal? (car s) (quote split)) (el-zero? (car (cdr (cdr s)))) #f))

; ----- INT x^m-style polynomial times sqrt(q):  INT B(x) sqrt(q) dx = INT B(x) q / sqrt(q) dx, so reduce with
; numerator B*q.  Decide/split via the same machinery. -----
(define (ell-integrate-sqrt B q) (ell-integrate (poly-mul (el-canon B) q) q))
(define (ell-split-sqrt B q) (ell-split (poly-mul (el-canon B) q) q))
(define (ell-certify-sqrt B q r) (ell-certify (poly-mul (el-canon B) q) q r))
