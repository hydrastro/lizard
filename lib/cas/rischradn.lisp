; -*- lisp -*-
; lib/cas/rischradn.lisp -- INT P(x)/sqrt(p) dx for an ARBITRARY polynomial numerator P and a monic quadratic
; radicand p, by reduction of order in the algebraic function field K = Q(x)[y]/(y^2 - p).  This extends the
; quadratic-radical integration of algfunc.lisp (which covers degree <= 1 numerators and INT sqrt(p)) to any
; polynomial numerator, closing the higher-numerator part of the P(x)/sqrt(quadratic) row
; (docs/TRAGER_ROADMAP.md, the frontier -- general algebraic functions).
;
; The method.  For p = x^2 + b1 x + c0 (monic), the antiderivative of P/sqrt(p) has the form
;     INT P/sqrt(p) dx = A(x) sqrt(p) + c * log( x + b1/2 + sqrt(p) ),
; with A a polynomial of degree deg(P) - 1 and c a constant.  Differentiating (inside K, y' = p'/(2y) so
; d/dx[A y] = A' y + A p'/(2y) = (A' p + A p'/2)/sqrt(p), and d/dx of the log gives 1/sqrt(p)):
;     d/dx[ A sqrt(p) + c log(...) ] = ( A' p + A p'/2 + c ) / sqrt(p).
; Matching to P/sqrt(p) gives the POLYNOMIAL identity
;     A' p + A p'/2 + c = P,
; an exact triangular linear system over Q in the coefficients of A (deg P - 1) and the constant c, solved by
; matching coefficients from the top degree down.  The result is then CERTIFIED inside K by af-certify (the same
; differentiation arbiter used throughout), so a wrong answer is never returned.
;
; Scope: monic quadratic p, any polynomial P.  The non-monic radicand reduces by scaling; genuinely higher-genus
; radicands (cubic/quartic p -- elliptic and hyperelliptic, mostly non-elementary) are the open summit beyond.
;
; Public:
;   radn-integrate P b1 c0   -> (list 'elementary A-coeffs c) : INT P/sqrt(x^2+b1 x+c0), A-coeffs low-to-high, c
;                               the logarithm coefficient (the log argument is x + b1/2 + sqrt(p))
;   radn-certify P b1 c0 r   -> #t iff the result differentiates back to P/sqrt(p) inside K
;
; Verified: INT x^2/sqrt(x^2+1) = (x/2) sqrt(x^2+1) - (1/2) log(x + sqrt(x^2+1)); INT x^3/sqrt(x^2+1);
; INT (x^2+x+1)/sqrt(x^2+1); INT 1/sqrt(x^2+1) reproduced (A empty, c = 1); each certified inside K by af-certify.
;
; Builds on algfunc.lisp (the field K = Q(x)[y]/(y^2-p) and its certificate) and poly.lisp / tower.lisp.

(import "cas/algfunc.lisp")
(import "cas/poly.lisp")
(import "cas/tower.lisp")

(define (rn-len l) (if (null? l) 0 (+ 1 (rn-len (cdr l)))))
(define (rn-nth l k) (if (= k 0) (car l) (rn-nth (cdr l) (- k 1))))
(define (rn-deg p) (- (rn-trim-len p) 1))
(define (rn-trim-len p) (rn-tl-go p (rn-len p)))
(define (rn-tl-go p n) (cond ((= n 0) 0) ((= (rn-coeff p (- n 1)) 0) (rn-tl-go p (- n 1))) (else n)))
(define (rn-coeff p k) (if (if (< k 0) #t (>= k (rn-len p))) 0 (rn-nth p k)))

; ----- solve A' p + A p'/2 + c = P for A (deg P - 1) and constant c, over Q, by matching coefficients top-down.
; A = sum_{i=0}^{m-1} a_i x^i where m = deg P.  Let L(A) = A' p + A p'/2.  We need L(A) + c = P.
; The leading behavior: for p monic quadratic (deg 2, p' deg 1), A deg (m-1):
;   A' p has degree (m-2)+2 = m ; A p'/2 has degree (m-1)+1 = m.  So L(A) has degree m = deg P. Good: matching
; the top coefficient pins a_{m-1}, then descend.  c absorbs the degree-0 slack.  We build L as a linear map and
; solve by Gaussian elimination over Q on the (m+1) unknowns (a_0..a_{m-1}, c) against P's coefficients. -----
(define (radn-integrate P b1 c0) (radn-build (rn-canon P) b1 c0 (rn-deg (rn-canon P))))
(define (rn-canon P) (if (= (rn-trim-len P) 0) (list 0) (rn-take P (rn-trim-len P))))
(define (rn-take l n) (if (= n 0) (quote ()) (cons (car l) (rn-take (cdr l) (- n 1)))))
; p and p'
(define (rn-p b1 c0) (list c0 b1 1))
(define (rn-pp b1 c0) (poly-deriv (rn-p b1 c0)))
; the linear map applied to a candidate A (coeff list): L(A) = A' p + A p'/2  (as a poly over Q)
(define (rn-L A b1 c0) (poly-add (poly-mul (poly-deriv A) (rn-p b1 c0)) (rn-halfmul (poly-mul A (rn-pp b1 c0)))))
(define (rn-halfmul p) (rn-scale-poly p (rat-make-q 1 2)))
; scale a polynomial (integer/rational coeffs) by a rational q: we keep coeffs as exact rationals via mult
(define (rn-scale-poly p q) (rn-sp-go p q))
(define (rn-sp-go p q) (if (null? p) (quote ()) (cons (rn-qmul q (car p)) (rn-sp-go (cdr p) q))))
; rationals here are plain Lisp numbers (the poly layer is over Q's numbers), so use exact division
(define (rat-make-q a b) (/ a b))
(define (rn-qmul q v) (* q v))

; build and solve the (m+1)x(m+1) system for unknowns (a_0..a_{m-1}, c).  Column j (j<m) is L(x^j) as a poly;
; column m is the constant 1 (for c).  RHS is P.  Match coefficients of x^0..x^m.
(define (radn-build P b1 c0 m)
  (if (< m 0) (list (quote elementary) (quote ()) (rn-solve-const P))
      (radn-solve P b1 c0 m)))
; degenerate: P is the zero polynomial or constant handled in the general path with m>=0; for m=0 (P constant),
; A is empty (deg -1) and c = P_0 (since L(empty)=0).  We treat m = deg P; unknowns a_0..a_{m-1} and c.
(define (rn-solve-const P) (rn-coeff P 0))
(define (radn-solve P b1 c0 m)
  (radn-finish P b1 c0 m (rn-gauss (rn-matrix b1 c0 m) (rn-rhs P (+ m 1)) (+ m 1))))
; matrix rows indexed by output degree 0..m ; columns: a_0..a_{m-1} then c.
(define (rn-matrix b1 c0 m) (rn-rows b1 c0 m 0 (+ m 1)))
(define (rn-rows b1 c0 m d top) (if (> d m) (quote ()) (cons (rn-row b1 c0 m d) (rn-rows b1 c0 m (+ d 1) top))))
(define (rn-row b1 c0 m d) (rn-append-col (rn-Lcols b1 c0 m d 0) (if (= d 0) 1 0)))
(define (rn-Lcols b1 c0 m d j) (if (>= j m) (quote ()) (cons (rn-coeff (rn-L (rn-unit j) b1 c0) d) (rn-Lcols b1 c0 m d (+ j 1)))))
(define (rn-unit j) (rn-unit-go j 0))
(define (rn-unit-go j i) (if (> i j) (quote ()) (cons (if (= i j) 1 0) (rn-unit-go j (+ i 1)))))
(define (rn-append-col row v) (rn-ac row v))
(define (rn-ac row v) (if (null? row) (list v) (cons (car row) (rn-ac (cdr row) v))))
(define (rn-rhs P n) (rn-rhs-go P n 0))
(define (rn-rhs-go P n d) (if (>= d n) (quote ()) (cons (rn-coeff P d) (rn-rhs-go P n (+ d 1)))))

; exact Gaussian elimination over Q (Lisp numbers) on an n x n augmented system; returns the solution vector
; (a_0..a_{m-1}, c) of length n = m+1, or 'none.
(define (rn-gauss rows rhs n) (rn-g-elim (rn-aug rows rhs) n 0))
(define (rn-aug rows rhs) (if (null? rows) (quote ()) (cons (rn-ac (car rows) (car rhs)) (rn-aug (cdr rows) (cdr rhs)))))
(define (rn-g-elim rows n col) (if (>= col n) (rn-extract rows n) (rn-g-piv rows n col)))
(define (rn-g-piv rows n col) (rn-g-found rows n col (rn-findpiv rows col)))
(define (rn-findpiv rows col) (rn-fp rows col))
(define (rn-fp rows col) (cond ((null? rows) (quote none)) ((if (not (= (rn-nth (car rows) col) 0)) (rn-earlier0 (car rows) col 0) #f) (car rows)) (else (rn-fp (cdr rows) col))))
(define (rn-earlier0 row col k) (cond ((>= k col) #t) ((= (rn-nth row k) 0) (rn-earlier0 row col (+ k 1))) (else #f)))
(define (rn-g-found rows n col pv) (if (equal? pv (quote none)) (rn-g-elim rows n (+ col 1)) (rn-g-elim (rn-elim rows pv col) n (+ col 1))))
(define (rn-elim rows pv col) (rn-el rows pv (rn-normrow pv col) col))
(define (rn-normrow pv col) (rn-scalerow pv (/ 1 (rn-nth pv col))))
(define (rn-scalerow row s) (if (null? row) (quote ()) (cons (* s (car row)) (rn-scalerow (cdr row) s))))
(define (rn-el rows pv npv col) (if (null? rows) (quote ()) (cons (if (rn-roweq (car rows) pv) npv (rn-rowsub (car rows) (rn-scalerow npv (rn-nth (car rows) col)))) (rn-el (cdr rows) pv npv col))))
(define (rn-roweq a b) (cond ((null? a) (null? b)) ((null? b) #f) ((= (car a) (car b)) (rn-roweq (cdr a) (cdr b))) (else #f)))
(define (rn-rowsub a b) (if (null? a) (quote ()) (cons (- (car a) (car b)) (rn-rowsub (cdr a) (cdr b)))))
(define (rn-extract rows n) (rn-ex rows n 0 (quote ())))
(define (rn-ex rows n j acc) (if (>= j n) (rn-reverse acc) (rn-ex rows n (+ j 1) (cons (rn-exfind rows j n) acc))))
(define (rn-exfind rows j n) (rn-ef rows j n))
(define (rn-ef rows j n) (cond ((null? rows) 0) ((rn-pivrow (car rows) j n) (rn-nth (car rows) n)) (else (rn-ef (cdr rows) j n))))
(define (rn-pivrow row j n) (if (= (rn-nth row j) 1) (rn-others0 row j n 0) #f))
(define (rn-others0 row j n k) (cond ((>= k n) #t) ((= k j) (rn-others0 row j n (+ k 1))) ((= (rn-nth row k) 0) (rn-others0 row j n (+ k 1))) (else #f)))
(define (rn-reverse l) (rn-rev l (quote ())))
(define (rn-rev l acc) (if (null? l) acc (rn-rev (cdr l) (cons (car l) acc))))

; split the solution vector into A (first m entries) and c (last)
(define (radn-finish P b1 c0 m sol) (if (equal? sol (quote none)) (list (quote deferred) (quote no-solution)) (radn-result P b1 c0 m sol)))
(define (radn-result P b1 c0 m sol) (radn-verify P b1 c0 (rn-take sol m) (rn-nth sol m)))
; certify inside K via af-certify, then return
(define (radn-verify P b1 c0 A c) (if (radn-check P b1 c0 A c) (list (quote elementary) A c) (list (quote deferred) (quote uncertified))))
; the algebraic part is A(x)*sqrt(p) = A*y, i.e. af-make 0 A ; clog = c ; g = x + b1/2 + y ; integrand = P/y = (P/p) y
; NOTE: the algfunc helpers (af-hp, af-half-b, af-mul) require the radicand p as a RATIONAL (rat-from-poly).
(define (radn-check P b1 c0 A c)
  (af-certify (rat-from-poly (rn-p b1 c0))
              (af-make (rat-zero) (rat-from-poly A))
              (rat-make (list c) (list 1))
              (rn-logarg b1 c0)
              (rn-integrand P b1 c0)))
; g = x + b1/2 + sqrt(p): u = x + b1/2 (rat), v = 1
(define (rn-logarg b1 c0) (af-make (rat-from-poly (list (/ b1 2) 1)) (rat-one)))
; integrand P/sqrt(p) = P / y = (P/p) y  (since y^2 = p), i.e. af-make 0 (P/p)
(define (rn-integrand P b1 c0) (af-make (rat-zero) (rat-make P (rn-p b1 c0))))
; public certifier
(define (radn-certify P b1 c0 r) (if (equal? (car r) (quote elementary)) (radn-check P b1 c0 (car (cdr r)) (car (cdr (cdr r)))) #f))
