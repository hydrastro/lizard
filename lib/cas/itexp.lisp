; -*- lisp -*-
; lib/cas/itexp.lisp -- RUNG 5, going deeper: the ARBITRARY-DEPTH iterated exponential tower.  Define the
; iterated exponentials E_0 = x, E_1 = exp(x), E_2 = exp(exp(x)), ..., E_n = exp(E_{n-1}).  This generalizes the
; depth-2 nested exponential (nestexp.lisp) to a tower of arbitrary height n, the first time the system handles
; nesting of unbounded depth (docs/TRAGER_ROADMAP.md, Rung 5).
;
; The key derivative law, proved by induction from E_k = exp(E_{k-1}) (so E_k' = E_{k-1}' E_k):
;     E_k' = E_k * (E_1 E_2 ... E_{k-1}) = E_k * prod_{j=1}^{k-1} E_j .
; In particular d/dx(E_n) = E_1 E_2 ... E_n, the product of the WHOLE tower, so the cleanest depth-n integral is
;     INT (E_1 E_2 ... E_n) dx = E_n      e.g.  INT exp(x) exp(exp x) exp(exp exp x) dx = exp(exp(exp x)).
;
; Representation.  Over Q(x) a tower element is a sum of monomials c(x) * E_1^{a_1} ... E_n^{a_n}, carried as a
; list of (rational-coefficient . exponent-vector) pairs (the exponent vector has length n, entries >= 0).  From
; the law above, the derivation of one monomial is
;     d/dx ( c * prod E_i^{a_i} ) = c' * prod E_i^{a_i}  +  sum_{k: a_k>0} (a_k c) * (prod E_i^{a_i}) * prod_{j<k} E_j ,
; i.e. for each k with a_k > 0 we scale by a_k and RAISE the exponents of E_1..E_{k-1} by one (the E_k' factor),
; plus the c' term in place.  Summing over the element and collecting like monomials gives ie-deriv.  Everything
; is certified by differentiating in the tower and matching the integrand (ie-certify), the differentiation
; certificate as arbiter.
;
; Public:
;   ie-Ek-deriv n k         -> the monomial list for E_k' = E_k prod_{j<k} E_j (exponent vector of length n)
;   ie-deriv n E            -> d/dx E in the depth-n tower (E a list of (rat . expvec) monomials)
;   ie-certify n E B        -> #t iff ie-deriv n E = B  (the arbiter: INT B dx = E)
;   ie-top n                -> the element E_n (the single monomial of the top iterated exponential)
;   ie-full-product n       -> the element E_1 E_2 ... E_n (the full-tower product, = d/dx E_n)
;   ie-int-full-product n   -> (list 'answer 'E_n #t/#f) : the certified statement INT (E_1..E_n) dx = E_n
;
; Verified: E_1'=E_1, E_2'=E_1 E_2, E_3'=E_1 E_2 E_3 (the law); d/dx(E_n)=E_1..E_n; and the depth-n integrals
; INT(E_1 E_2)=E_2, INT(E_1 E_2 E_3)=E_3, INT(E_1 E_2 E_3 E_4)=E_4 (depths 2,3,4), all certified.
;
; Builds on tower.lisp (rational-function coefficient arithmetic via rat-*) and poly.lisp.

(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (ie-nth l k) (if (= k 0) (car l) (ie-nth (cdr l) (- k 1))))
(define (ie-len l) (if (null? l) 0 (+ 1 (ie-len (cdr l)))))

; ----- exponent-vector helpers (length n, entries >= 0) -----
(define (ie-zerovec n) (if (= n 0) (quote ()) (cons 0 (ie-zerovec (- n 1)))))
(define (ie-unitvec n i) (ie-uv-go n i 0))           ; E_i has exponent 1 at position i-1 (1-indexed i)
(define (ie-uv-go n i p) (if (= p n) (quote ()) (cons (if (= p (- i 1)) 1 0) (ie-uv-go n i (+ p 1)))))
(define (ie-vec-eq? a b) (cond ((null? a) (null? b)) ((null? b) #f) ((= (car a) (car b)) (ie-vec-eq? (cdr a) (cdr b))) (else #f)))
; raise the first (k-1) entries of an exponent vector by one (the prod_{j<k} E_j factor)
(define (ie-raise-prefix a k) (ie-rp-go a k 0))
(define (ie-rp-go a k p) (if (null? a) (quote ()) (cons (if (< p (- k 1)) (+ (car a) 1) (car a)) (ie-rp-go (cdr a) k (+ p 1)))))
(define (ie-vnth a i) (if (= i 0) (car a) (ie-vnth (cdr a) (- i 1))))

; ----- monomial = (rat-coeff . expvec); element = list of monomials. collect like terms. -----
(define (ie-mcoeff m) (car m))
(define (ie-mvec m) (cdr m))
(define (ie-make-mono c v) (cons c v))

(define (ie-collect E) (ie-collect-go E (quote ())))
(define (ie-collect-go E acc) (if (null? E) (ie-drop-zeros acc) (ie-collect-go (cdr E) (ie-insert (car E) acc))))
(define (ie-insert m acc) (ie-insert-go m acc (quote ()) #f))
(define (ie-insert-go m rest seen done)
  (cond ((null? rest) (if done (ie-reverse seen) (ie-reverse (cons m seen))))
        ((if (not done) (ie-vec-eq? (ie-mvec m) (ie-mvec (car rest))) #f)
         (ie-insert-go m (cdr rest) (cons (ie-make-mono (rat-add (ie-mcoeff m) (ie-mcoeff (car rest))) (ie-mvec m)) seen) #t))
        (else (ie-insert-go m (cdr rest) (cons (car rest) seen) done))))
(define (ie-drop-zeros E) (cond ((null? E) (quote ())) ((rat-zero? (ie-mcoeff (car E))) (ie-drop-zeros (cdr E))) (else (cons (car E) (ie-drop-zeros (cdr E))))))
(define (ie-reverse l) (ie-rev l (quote ())))
(define (ie-rev l acc) (if (null? l) acc (ie-rev (cdr l) (cons (car l) acc))))

; ----- E_k' = E_k prod_{j<k} E_j as a monomial list: a single monomial, coeff 1, expvec = unit(k) with the
; first (k-1) entries also raised by one -----
(define (ie-Ek-deriv n k) (list (ie-make-mono (rat-one) (ie-raise-prefix (ie-unitvec n k) k))))

; ----- derivation of one monomial (c . a): c' in place, plus for each k with a_k>0 a term (a_k c) with the
; E_k' factor folded in (raise first k-1 entries by one; the a remains, so multiply expvecs = add) -----
(define (ie-mono-deriv n m) (ie-md-collect (ie-cprime-term m) (ie-md-sum n m 1)))
(define (ie-cprime-term m) (ie-cp (rat-deriv (ie-mcoeff m)) (ie-mvec m)))
(define (ie-cp c v) (if (rat-zero? c) (quote ()) (list (ie-make-mono c v))))
(define (ie-md-sum n m k)
  (if (> k n) (quote ())
      (ie-app (ie-md-term n m k) (ie-md-sum n m (+ k 1)))))
(define (ie-md-term n m k)
  (if (= (ie-vnth (ie-mvec m) (- k 1)) 0) (quote ())
      (list (ie-make-mono (rat-mul (rat-from-poly (list (ie-vnth (ie-mvec m) (- k 1)))) (ie-mcoeff m))
                          (ie-raise-prefix (ie-mvec m) k)))))
(define (ie-md-collect a b) (ie-app a b))
(define (ie-app a b) (if (null? a) b (cons (car a) (ie-app (cdr a) b))))

; ----- derivation of an element: differentiate each monomial, concatenate, collect like terms -----
(define (ie-deriv n E) (ie-collect (ie-deriv-go n E)))
(define (ie-deriv-go n E) (if (null? E) (quote ()) (ie-app (ie-mono-deriv n (car E)) (ie-deriv-go n (cdr E)))))

; ----- certificate: d/dx E = B (as collected monomial sets) -----
(define (ie-certify n E B) (ie-set-eq? (ie-deriv n E) (ie-collect B)))
(define (ie-set-eq? A B) (if (= (ie-len A) (ie-len B)) (ie-subset? A B) #f))
(define (ie-subset? A B) (cond ((null? A) #t) ((ie-find (car A) B) (ie-subset? (cdr A) B)) (else #f)))
(define (ie-find m B) (cond ((null? B) #f) ((if (ie-vec-eq? (ie-mvec m) (ie-mvec (car B))) (rat-equal? (ie-mcoeff m) (ie-mcoeff (car B))) #f) #t) (else (ie-find m (cdr B)))))

; ----- the top monomial E_n, the full product E_1..E_n, and the certified depth-n integral -----
(define (ie-top n) (list (ie-make-mono (rat-one) (ie-unitvec n n))))
(define (ie-full-product n) (list (ie-make-mono (rat-one) (ie-allones n))))
(define (ie-allones n) (if (= n 0) (quote ()) (cons 1 (ie-allones (- n 1)))))
(define (ie-int-full-product n) (list (quote answer) (quote E_n) (ie-certify n (ie-top n) (ie-full-product n))))
