; -*- lisp -*-
; lib/cas/nestalg.lisp -- RUNG 5, the fusion step: a NESTED LOGARITHM over the ALGEBRAIC base.  This combines
; the nested-log tower (nestlog.lisp) with the entangled algebraic logarithm (alglog.lisp): the base is the
; algebraic field K = Q(x)[y]/(y^n - g) (e.g. y = sqrt x), and over it we build t1 = log(w) for a field element
; w, then t2 = log(t1) = log(log(w)).  This is the first tower that is BOTH nested AND over the algebraic base
; (docs/TRAGER_ROADMAP.md, Rung 5) -- the genuine fusion the roadmap flagged as open.
;
; Derivatives.  From alglog, t1 = log(w) has t1' = w'/w, a FIELD element of K (computed via the field inverse).
; Then t2 = log(t1) has t2' = t1'/t1 = (w'/w)/t1, which carries t1 in its DENOMINATOR (the nestlog structure)
; with a FIELD element in its numerator (the alglog structure).
;
; Representation.  Over K, a K(t1) element is a rational function in t1: a pair (N . D) of t1-polynomials whose
; coefficients are sefield elements (sf-* over K).  Its derivation uses t1' = w'/w (a field element):
;     d/dx( sum p_j t1^j ) = sum ( p_j' + (j+1) p_{j+1} t1' ) t1^j ,
; where p_j' is the sefield derivative and the product p_{j+1} t1' is a field product (sf-product); the quotient
; rule lifts this to N/D.  Over that, a tower element is a polynomial in t2 with K(t1) coefficients, with the
; outer derivation t2' = t1'/t1.  The cleanest integral is INT (t1'/t1) dx = t2 = log(log(w)); every result is
; certified by differentiating in the tower (na-certify), the differentiation certificate as the arbiter.
;
; Public:
;   na-t1prime g n w        -> t1' = w'/w as a field element of K (reuses alglog's al-tprime)
;   na-t1deriv g n tp P     -> d/dx of a t1-polynomial P (field-element coeffs), using the field element t1' = tp
;   na-q1deriv g n tp c     -> d/dx of a K(t1) element c = (N . D), by the quotient rule
;   na-t2prime g n tp       -> t2' = t1'/t1 as a K(t1) element ((field-elt numerator)/t1)
;   na-deriv g n tp E       -> d/dx E in the full tower (E a t2-poly of K(t1) elements)
;   na-certify g n tp E B   -> #t iff na-deriv = B  (the arbiter: INT B dx = E)
;   na-int-loglogw g n w    -> (list 'answer 'log-log-w #t/#f): the certified INT (w'/w)/log(w) dx = log(log w)
;
; Verified for w = sqrt x + 1 on y^2 = x: t1' = w'/w is a field element; d/dx(log(log w)) = (w'/w)/log(w);
; INT (w'/w)/log(w) dx = log(log(sqrt x + 1)); and a t2^2 case.  Also n=3 (cube-root base).
;
; Builds on alglog.lisp (the algebraic logarithm t1' = w'/w and the sefield K) and senorm/sefield.

(import "cas/alglog.lisp")

(define (na-nth l k) (if (= k 0) (car l) (na-nth (cdr l) (- k 1))))
(define (na-len l) (if (null? l) 0 (+ 1 (na-len (cdr l)))))
(define (na-nthor P j fallback) (if (< j (na-len P)) (na-nth P j) fallback))

; ----- t1' = w'/w as a field element (reuse alglog) -----
(define (na-t1prime g n w) (al-tprime g n w))

; ----- t1-polynomial over field-element coefficients: arithmetic -----
(define (na-fz n) (sf-zeros n))
(define (na-padd g n A B) (cond ((null? A) B) ((null? B) A) (else (cons (sf-add (car A) (car B)) (na-padd g n (cdr A) (cdr B))))))
(define (na-pneg g n A) (if (null? A) (quote ()) (cons (sf-neg (car A)) (na-pneg g n (cdr A)))))
(define (na-psub g n A B) (na-padd g n A (na-pneg g n B)))
(define (na-pmul g n A B) (if (null? A) (quote ()) (na-padd g n (na-smul g n (car A) B) (cons (na-fz n) (na-pmul g n (cdr A) B)))))
(define (na-smul g n c B) (if (null? B) (quote ()) (cons (sf-product g n c (car B)) (na-smul g n c (cdr B)))))
(define (na-pzero? g n A) (cond ((null? A) #t) ((sf-equal? (car A) (na-fz n)) (na-pzero? g n (cdr A)) ) (else #f)))
(define (na-peq? g n A B) (na-pzero? g n (na-psub g n A B)))

; ----- inner derivation of a t1-poly: d/dx(sum p_j t1^j) = sum (p_j' + (j+1) p_{j+1} t1') t1^j -----
(define (na-t1deriv g n tp P) (na-t1d-go g n tp P 0 (na-len P)))
(define (na-t1d-go g n tp P j m)
  (if (>= j m) (quote ())
      (cons (sf-add (sf-deriv g n (na-nthor P j (na-fz n)))
                    (na-iscale-prod g n (+ j 1) (na-nthor P (+ j 1) (na-fz n)) tp))
            (na-t1d-go g n tp P (+ j 1) m))))
; (j+1) * p_{j+1} * t1'  (integer scale of a field product)
(define (na-iscale-prod g n k pj tp) (na-fiscale n k (sf-product g n pj tp)))
(define (na-fiscale n k C) (sf-scale (rat-from-poly (list k)) C))

; ----- K(t1) element = (N . D) of t1-polys; quotient-rule derivation -----
(define (na-q1deriv g n tp c) (na-qd g n tp (car c) (cdr c)))
(define (na-qd g n tp N D) (cons (na-psub g n (na-pmul g n (na-t1deriv g n tp N) D) (na-pmul g n N (na-t1deriv g n tp D))) (na-pmul g n D D)))
; K(t1) arithmetic on (N . D) pairs
(define (na-q-add g n a b) (cons (na-padd g n (na-pmul g n (car a) (cdr b)) (na-pmul g n (car b) (cdr a))) (na-pmul g n (cdr a) (cdr b))))
(define (na-q-mul g n a b) (cons (na-pmul g n (car a) (car b)) (na-pmul g n (cdr a) (cdr b))))
(define (na-q-zero n) (cons (list (na-fz n)) (list (sf-one n))))
(define (na-q-one n) (cons (list (sf-one n)) (list (sf-one n))))
(define (na-q-iscale g n k a) (cons (na-smul g n (sf-scale (rat-from-poly (list k)) (sf-one n)) (car a)) (cdr a)))
; cross-multiply equality: a=N1/D1 == b=N2/D2 iff N1 D2 = N2 D1
(define (na-q-eq? g n a b) (na-peq? g n (na-pmul g n (car a) (cdr b)) (na-pmul g n (car b) (cdr a))))

; ----- t2' = t1'/t1 as a K(t1) element: numerator = t1' (field elt at t1^0), denominator = t1 (t1^1) -----
(define (na-t2prime g n tp) (cons (list tp) (list (na-fz n) (sf-one n))))

; ----- outer derivation: E a t2-poly of K(t1) elements. d/dx(sum C_i t2^i)=sum(C_i'+(i+1)C_{i+1} t2') t2^i ----
(define (na-tcoeff g n E i) (if (< i (na-len E)) (na-nth E i) (na-q-zero n)))
(define (na-deriv g n tp E) (na-trim g n (na-deriv-go g n tp E 0 (na-len E))))
(define (na-deriv-go g n tp E i m)
  (if (>= i m) (quote ())
      (cons (na-q-add g n (na-q1deriv g n tp (na-tcoeff g n E i)) (na-q-iscale g n (+ i 1) (na-q-mul g n (na-tcoeff g n E (+ i 1)) (na-t2prime g n tp))))
            (na-deriv-go g n tp E (+ i 1) m))))
(define (na-trim g n E) (na-trim-go g n (na-reverse E)))
(define (na-trim-go g n r) (cond ((null? r) (list (na-q-zero n))) ((na-q-eq? g n (car r) (na-q-zero n)) (na-trim-go g n (cdr r))) (else (na-reverse r))))
(define (na-reverse l) (na-rev l (quote ())))
(define (na-rev l acc) (if (null? l) acc (na-rev (cdr l) (cons (car l) acc))))

; ----- certificate -----
(define (na-certify g n tp E B) (na-eq? g n (na-deriv g n tp E) B))
(define (na-eq? g n A B) (na-eq-go g n A B 0 (na-maxlen A B)))
(define (na-maxlen A B) (if (> (na-len A) (na-len B)) (na-len A) (na-len B)))
(define (na-eq-go g n A B i m) (if (>= i m) #t (if (na-q-eq? g n (na-tcoeff g n A i) (na-tcoeff g n B i)) (na-eq-go g n A B (+ i 1) m) #f)))

; ----- the nested-log-over-algebraic integral INT (w'/w)/log(w) dx = log(log(w)) -----
; answer E = t2 (C_0=0, C_1 = K(t1)-one); integrand B = t2' (degree 0 in t2).
(define (na-answer-t2 n) (list (na-q-zero n) (na-q-one n)))
(define (na-integrand g n tp) (list (na-t2prime g n tp)))
(define (na-int-loglogw g n w) (na-build-result g n (na-t1prime g n w)))
(define (na-build-result g n tp) (list (quote answer) (quote log-log-w) (na-certify g n tp (na-answer-t2 n) (na-integrand g n tp))))
