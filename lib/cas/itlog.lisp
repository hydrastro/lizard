; -*- lisp -*-
; lib/cas/itlog.lisp -- RUNG 5, going deeper: the ARBITRARY-DEPTH iterated LOGARITHM tower, the dual of the
; iterated exponential (itexp.lisp).  Define L_0 = x, L_1 = log x, L_2 = log(log x), ..., L_n = log(L_{n-1}).
; Where the iterated exponential was multiplicative (E_k' = E_k prod_{j<k} E_j, polynomial monomials), the
; iterated logarithm is its reciprocal mirror: the lower logarithms appear in the DENOMINATOR, so elements are
; LAURENT monomials in L_1..L_n (docs/TRAGER_ROADMAP.md, Rung 5).
;
; The derivative law, proved by induction from L_k = log(L_{k-1}) (so L_k' = L_{k-1}'/L_{k-1}):
;     L_k' = 1 / (L_0 L_1 ... L_{k-1}) = 1 / (x L_1 L_2 ... L_{k-1}) .
; In particular d/dx(L_n) = 1/(x L_1 L_2 ... L_{n-1}), so the cleanest depth-n integral is
;     INT 1/(x L_1 L_2 ... L_{n-1}) dx = L_n ,
; e.g. INT 1/(x log x) dx = log log x (n=2), INT 1/(x log x log log x) dx = log log log x (n=3), and so on.
;
; Representation.  A tower element is a sum of monomials c(x) L_1^{a_1} ... L_n^{a_n} where the exponents a_i are
; INTEGERS (possibly negative -- Laurent) and c(x) is a rational function (the L_0 = x dependence folded in),
; carried as a list of (rat-coeff . exponent-vector) pairs.  From the law above, the derivation of a monomial is
;     d/dx ( c prod L_i^{a_i} ) = c' prod L_i^{a_i}  +  sum_{k: a_k != 0} (a_k c / x) * L_1^{a_1-1} ... L_k^{a_k-1} prod_{i>k} L_i^{a_i} ,
; i.e. for each k with a_k != 0 we scale the coefficient by a_k/x and LOWER the exponents of L_1..L_k each by one
; (the 1/(x L_1...L_{k-1}) from L_k', together with the L_k^{a_k-1} from differentiating L_k^{a_k}).  Summing and
; collecting like monomials gives il-deriv; everything is certified by differentiating in the tower (il-certify).
;
; Public:
;   il-Lk-deriv n k         -> the monomial list for L_k' = 1/(x L_1 ... L_{k-1}) (coeff 1/x, exps -1 on L_1..L_{k-1})
;   il-deriv n E            -> d/dx E in the depth-n iterated-log tower (E a list of (rat . expvec) monomials)
;   il-certify n E B        -> #t iff il-deriv n E = B  (the arbiter: INT B dx = E)
;   il-top n                -> the element L_n (single monomial, exponent 1 on L_n)
;   il-int-denom n          -> the integrand 1/(x L_1 ... L_{n-1}) (= d/dx L_n) as an element
;   il-int-nested n         -> (list 'answer 'L_n #t/#f): the certified INT 1/(x L_1...L_{n-1}) dx = L_n
;
; Verified: L_1'=1/x, L_2'=1/(x L_1), L_3'=1/(x L_1 L_2) (the law); d/dx(L_n)=1/(x L_1...L_{n-1}); and the
; nested-log integrals INT 1/(x L_1...L_{n-1}) dx = L_n at depths 2, 3, 4, 5, with a soundness control.
;
; Builds on tower.lisp (rational-function arithmetic via rat-*) and poly.lisp.

(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (il-nth l k) (if (= k 0) (car l) (il-nth (cdr l) (- k 1))))
(define (il-len l) (if (null? l) 0 (+ 1 (il-len (cdr l)))))

; ----- exponent-vector helpers (length n, entries in Z) -----
(define (il-zerovec n) (if (= n 0) (quote ()) (cons 0 (il-zerovec (- n 1)))))
(define (il-unitvec n i) (il-uv-go n i 0))
(define (il-uv-go n i p) (if (= p n) (quote ()) (cons (if (= p (- i 1)) 1 0) (il-uv-go n i (+ p 1)))))
(define (il-vec-eq? a b) (cond ((null? a) (null? b)) ((null? b) #f) ((= (car a) (car b)) (il-vec-eq? (cdr a) (cdr b))) (else #f)))
(define (il-vnth a i) (if (= i 0) (car a) (il-vnth (cdr a) (- i 1))))
; lower the first k entries of an exponent vector by one (the L_k' = 1/(x L_1..L_{k-1}) and the L_k^{a_k-1})
(define (il-lower-prefix a k) (il-lp-go a k 0))
(define (il-lp-go a k p) (if (null? a) (quote ()) (cons (if (< p k) (- (car a) 1) (car a)) (il-lp-go (cdr a) k (+ p 1)))))

; ----- monomial = (rat-coeff . expvec); element = list of monomials; collect like terms -----
(define (il-mcoeff m) (car m))
(define (il-mvec m) (cdr m))
(define (il-make c v) (cons c v))
(define (il-collect E) (il-collect-go E (quote ())))
(define (il-collect-go E acc) (if (null? E) (il-drop-zeros acc) (il-collect-go (cdr E) (il-insert (car E) acc))))
(define (il-insert m acc) (il-insert-go m acc (quote ()) #f))
(define (il-insert-go m rest seen done)
  (cond ((null? rest) (if done (il-reverse seen) (il-reverse (cons m seen))))
        ((if (not done) (il-vec-eq? (il-mvec m) (il-mvec (car rest))) #f)
         (il-insert-go m (cdr rest) (cons (il-make (rat-add (il-mcoeff m) (il-mcoeff (car rest))) (il-mvec m)) seen) #t))
        (else (il-insert-go m (cdr rest) (cons (car rest) seen) done))))
(define (il-drop-zeros E) (cond ((null? E) (quote ())) ((rat-zero? (il-mcoeff (car E))) (il-drop-zeros (cdr E))) (else (cons (car E) (il-drop-zeros (cdr E))))))
(define (il-reverse l) (il-rev l (quote ())))
(define (il-rev l acc) (if (null? l) acc (il-rev (cdr l) (cons (car l) acc))))

(define (il-invx) (rat-make (list 1) (list 0 1)))       ; 1/x

; ----- L_k' = 1/(x L_1 ... L_{k-1}): coeff 1/x, exponents -1 on L_1..L_{k-1}, 0 elsewhere -----
(define (il-Lk-deriv n k) (list (il-make (il-invx) (il-lower-prefix (il-zerovec n) (- k 1)))))

; ----- derivation of one monomial (c . a): c' in place plus, for each k with a_k != 0, (a_k c / x) with the
; first k exponents lowered by one -----
(define (il-mono-deriv n m) (il-app (il-cprime-term m) (il-md-sum n m 1)))
(define (il-cprime-term m) (if (rat-zero? (rat-deriv (il-mcoeff m))) (quote ()) (list (il-make (rat-deriv (il-mcoeff m)) (il-mvec m)))))
(define (il-md-sum n m k) (if (> k n) (quote ()) (il-app (il-md-term n m k) (il-md-sum n m (+ k 1)))))
(define (il-md-term n m k)
  (if (= (il-vnth (il-mvec m) (- k 1)) 0) (quote ())
      (list (il-make (rat-mul (rat-mul (rat-from-poly (list (il-vnth (il-mvec m) (- k 1)))) (il-invx)) (il-mcoeff m))
                     (il-lower-prefix (il-mvec m) k)))))
(define (il-app a b) (if (null? a) b (cons (car a) (il-app (cdr a) b))))

; ----- derivation of an element -----
(define (il-deriv n E) (il-collect (il-deriv-go n E)))
(define (il-deriv-go n E) (if (null? E) (quote ()) (il-app (il-mono-deriv n (car E)) (il-deriv-go n (cdr E)))))

; ----- certificate: d/dx E = B (as collected monomial sets) -----
(define (il-certify n E B) (il-set-eq? (il-deriv n E) (il-collect B)))
(define (il-set-eq? A B) (if (= (il-len A) (il-len B)) (il-subset? A B) #f))
(define (il-subset? A B) (cond ((null? A) #t) ((il-find (car A) B) (il-subset? (cdr A) B)) (else #f)))
(define (il-find m B) (cond ((null? B) #f) ((if (il-vec-eq? (il-mvec m) (il-mvec (car B))) (rat-equal? (il-mcoeff m) (il-mcoeff (car B))) #f) #t) (else (il-find m (cdr B)))))

; ----- top monomial L_n, the integrand 1/(x L_1...L_{n-1}) (= d/dx L_n), and the certified nested-log integral
(define (il-top n) (list (il-make (rat-one) (il-unitvec n n))))
(define (il-int-denom n) (list (il-make (il-invx) (il-lower-prefix (il-zerovec n) (- n 1)))))
(define (il-int-nested n) (list (quote answer) (quote L_n) (il-certify n (il-top n) (il-int-denom n))))
