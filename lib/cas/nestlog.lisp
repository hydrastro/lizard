; -*- lisp -*-
; lib/cas/nestlog.lisp -- RUNG 5: the NESTED LOGARITHM tower Q(x)(t1)(t2) with t1 = log x and t2 = log(t1) =
; log(log x).  This is a genuinely nested depth-2 tower: the second monomial t2 is the logarithm of the first,
; t2' = t1'/t1 = 1/(x t1) -- so t2's derivative has t1 in the DENOMINATOR, and the coefficient ring is rational
; (not just polynomial) in t1 (docs/TRAGER_ROADMAP.md, Rung 5).
;
; Layering.  The base is Q(x) (rationals in x, from ratfun/tower).  Over it, Q(x)(t1) elements are RATIONAL
; FUNCTIONS in t1: a pair (N . D) of t1-polynomials whose coefficients are Q(x) rationals, meaning N(t1)/D(t1).
; The inner derivation uses t1' = 1/x:  d/dx( sum p_j(x) t1^j ) = sum ( p_j'(x) + (j+1) p_{j+1}(x)/x ) t1^j, and
; the quotient rule lifts it to N/D.  Over THAT, a tower element is a polynomial in t2 whose coefficients are
; Q(x)(t1) elements; the outer derivation uses t2' = 1/(x t1):
;     d/dx( sum C_i t2^i ) = sum ( C_i' + (i+1) C_{i+1} t2' ) t2^i .
;
; This lets us integrate genuinely nested-log integrands; the cleanest is INT 1/(x log x) dx = log(log x), where
; the integrand is exactly t2' and the answer is t2.  Every result is certified by differentiating in the tower
; and matching the integrand (nl-certify), the differentiation certificate as arbiter.
;
; Public:
;   nl-t1deriv P            -> d/dx of a t1-polynomial P (Q(x) coeffs), using t1' = 1/x
;   nl-q1deriv c            -> d/dx of a Q(x)(t1) element c = (N . D), by the quotient rule (a (N . D) pair)
;   nl-t2prime              -> t2' = 1/(x t1) as a Q(x)(t1) element
;   nl-deriv E              -> d/dx E in the full tower (E a t2-poly of Q(x)(t1) elements)
;   nl-certify E B          -> #t iff nl-deriv E = B  (the arbiter: INT B dx = E)
;   nl-int-loglog           -> the certified statement INT 1/(x log x) dx = log(log x): returns the answer E=t2
;                              together with #t from nl-certify against the integrand t2'
;
; Verified: d/dx(log x)=1/x, d/dx((log x)^2)=2 log x / x (inner); d/dx(log log x)=1/(x log x) (outer); and the
; nested-log integral INT 1/(x log x) dx = log(log x).
;
; Builds on tower.lisp / ratfun.lisp (Q(x) rational arithmetic) and poly.lisp.

(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (nl-nth l k) (if (= k 0) (car l) (nl-nth (cdr l) (- k 1))))
(define (nl-len l) (if (null? l) 0 (+ 1 (nl-len (cdr l)))))
(define (nl-nthor P j) (if (< j (nl-len P)) (nl-nth P j) (rat-zero)))

; ----- inner derivation on a t1-polynomial (Q(x) coeffs), t1' = 1/x -----
; d/dx( sum p_j t1^j ) = sum ( p_j' + (j+1) p_{j+1}/x ) t1^j
(define (nl-t1deriv P) (nl-t1d-go P 0 (nl-len P)))
(define (nl-t1d-go P j n)
  (if (>= j n) (quote ())
      (cons (rat-add (rat-deriv (nl-nthor P j)) (rat-mul (rat-from-poly (list (+ j 1))) (rat-mul (nl-invx) (nl-nthor P (+ j 1)))))
            (nl-t1d-go P (+ j 1) n))))
(define (nl-invx) (rat-make (list 1) (list 0 1)))       ; 1/x

; ----- t1-polynomial arithmetic (lists of Q(x) rationals over t1-degree) -----
(define (nl-padd A B) (cond ((null? A) B) ((null? B) A) (else (cons (rat-add (car A) (car B)) (nl-padd (cdr A) (cdr B))))))
(define (nl-psub A B) (nl-padd A (nl-pneg B)))
(define (nl-pneg A) (if (null? A) (quote ()) (cons (rat-neg (car A)) (nl-pneg (cdr A)))))
(define (nl-pmul A B) (if (null? A) (quote ()) (nl-padd (nl-scale-shift (car A) B 0) (nl-pmul-shift (cdr A) B))))
(define (nl-pmul-shift A B) (if (null? A) (quote ()) (cons (rat-zero) (nl-pmul A B))))
(define (nl-scale-shift c B k) (if (null? B) (quote ()) (cons (rat-mul c (car B)) (nl-scale-shift c (cdr B) k))))
(define (nl-pzero? A) (cond ((null? A) #t) ((rat-zero? (car A)) (nl-pzero? (cdr A))) (else #f)))
(define (nl-peq? A B) (nl-pzero? (nl-psub A B)))

; ----- Q(x)(t1) element = (N . D), t1-polys; quotient-rule derivation -----
(define (nl-q1deriv c) (nl-qd (car c) (cdr c)))
(define (nl-qd N D) (cons (nl-psub (nl-pmul (nl-t1deriv N) D) (nl-pmul N (nl-t1deriv D))) (nl-pmul D D)))
; Q(x)(t1) arithmetic
(define (nl-q-add a b) (cons (nl-padd (nl-pmul (car a) (cdr b)) (nl-pmul (car b) (cdr a))) (nl-pmul (cdr a) (cdr b))))
(define (nl-q-sub a b) (nl-q-add a (cons (nl-pneg (car b)) (cdr b))))
(define (nl-q-mul a b) (cons (nl-pmul (car a) (car b)) (nl-pmul (cdr a) (cdr b))))
(define (nl-q-zero) (cons (quote ()) (list (rat-one))))
(define (nl-q-one) (cons (list (rat-one)) (list (rat-one))))
; cross-multiply equality: a=N1/D1, b=N2/D2 equal iff N1 D2 = N2 D1
(define (nl-q-eq? a b) (nl-peq? (nl-pmul (car a) (cdr b)) (nl-pmul (car b) (cdr a))))
; integer scale of a Q(x)(t1) element
(define (nl-q-iscale k a) (cons (nl-scale-shift (rat-from-poly (list k)) (car a) 0) (cdr a)))

; ----- t2' = 1/(x t1) as a Q(x)(t1) element: numerator 1/x (t1^0), denominator t1 -----
(define (nl-t2prime) (cons (list (nl-invx)) (list (rat-zero) (rat-one))))   ; (1/x) / t1

; ----- outer derivation in the full tower: E a t2-poly of Q(x)(t1) elements -----
; d/dx( sum C_i t2^i ) = sum ( C_i' + (i+1) C_{i+1} t2' ) t2^i
(define (nl-tcoeff E i) (if (< i (nl-len E)) (nl-nth E i) (nl-q-zero)))
(define (nl-deriv E) (nl-trim (nl-deriv-go E 0)))
(define (nl-deriv-go E i)
  (if (>= i (nl-len E)) (quote ())
      (cons (nl-q-add (nl-q1deriv (nl-tcoeff E i)) (nl-q-iscale (+ i 1) (nl-q-mul (nl-tcoeff E (+ i 1)) (nl-t2prime))))
            (nl-deriv-go E (+ i 1)))))
(define (nl-trim E) (nl-trim-go (nl-reverse E)))
(define (nl-trim-go r) (cond ((null? r) (list (nl-q-zero))) ((nl-q-eq? (car r) (nl-q-zero)) (nl-trim-go (cdr r))) (else (nl-reverse r))))
(define (nl-reverse l) (nl-rev l (quote ())))
(define (nl-rev l acc) (if (null? l) acc (nl-rev (cdr l) (cons (car l) acc))))

; ----- certificate: d/dx E = B in the tower -----
(define (nl-certify E B) (nl-eq? (nl-deriv E) B))
(define (nl-eq? A B) (nl-eq-go A B 0 (nl-maxlen A B)))
(define (nl-maxlen A B) (if (> (nl-len A) (nl-len B)) (nl-len A) (nl-len B)))
(define (nl-eq-go A B i n) (if (>= i n) #t (if (nl-q-eq? (nl-tcoeff A i) (nl-tcoeff B i)) (nl-eq-go A B (+ i 1) n) #f)))

; ----- the nested-log integral INT 1/(x log x) dx = log(log x) -----
; answer E = t2 (t2-poly: C_0 = 0, C_1 = the Q(x)(t1) element 1); integrand B = t2' (t2-poly: C_0 = t2', degree 0).
(define (nl-answer-t2) (list (nl-q-zero) (nl-q-one)))
(define (nl-integrand-loglog) (list (nl-t2prime)))
(define (nl-int-loglog) (list (quote answer) (quote log-log-x) (nl-certify (nl-answer-t2) (nl-integrand-loglog))))
