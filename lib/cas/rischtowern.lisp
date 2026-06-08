; -*- lisp -*-
; lib/cas/rischtowern.lisp -- the HEIGHT-N recursion: a uniform tower-element algebra and a recursive derivation
; D that descends the tower one level at a time, bottoming out at Q(x).  This is the structural foundation that
; lets the Risch machinery operate at ARBITRARY tower height -- the derivation (and, in rischintn.lisp, the
; integration) at height h calls itself at height h-1 (docs/TRAGER_ROADMAP.md, the summit, "arbitrary height").
;
; Tower and element representation.
;   A tower is a list of level descriptors, BOTTOM-UP: ((exp b_1) (log b_2) ...), where each b_i is a tower
;   element at height i-1 (so b_1 is a Q(x) rational, b_2 a height-1 element, etc.).
;   A tower ELEMENT at height h is:
;     height 0: a rational function over Q, a (num . den) pair (the rat-* representation from tower.lisp);
;     height h > 0: a list of height-(h-1) elements (coefficients low-to-high in theta_h).
;
; The recursive derivation D (te-deriv tower h e), descending on h:
;   height 0: rat-deriv (the Q(x) derivative);
;   height h, level (exp b):  D(sum_k c_k theta^k) = sum_k ( D(c_k) + k D(b) c_k ) theta^k   (diagonal);
;   height h, level (log b):  D(sum_k c_k theta^k) = sum_k ( D(c_k) + (k+1) (D(b)/b) c_{k+1} ) theta^k (shift),
; where D(c_k) and D(b) are computed RECURSIVELY at height h-1, and the products/sums are the recursive
; element arithmetic (te-add, te-mul, te-scale-int) which themselves descend.
;
; Public:
;   te-zero h / te-one h     -> the zero / one element at height h
;   te-add tower h a b       -> a + b at height h (recursive)
;   te-mul tower h a b       -> a * b at height h (recursive; coefficient products descend)
;   te-scale-int h k e       -> integer-scale k*e at height h
;   te-coeff-mul tower h c e  -> multiply a height-(h-1) coefficient c into a height-h element e
;   te-deriv tower h e       -> D(e) at height h (the recursive derivation)
;   te-equal? tower h a b    -> structural equality at height h (recursive)
;   te-level-type tower h    -> 'exp | 'log : the kind of the height-h level
;   te-level-b tower h       -> b_h, the (height h-1) argument of the height-h level
;
; Verified: D at height 1 reproduces the exp and log derivations; D at height 2 gives D(e^{e^x}) = e^x e^{e^x}
; and D(log(log x)) = 1/(x log x); the element arithmetic round-trips.
;
; Builds on tower.lisp (the height-0 rational arithmetic rat-*) and poly.lisp.

(import "cas/tower.lisp")
(import "cas/poly.lisp")

(define (tn-nth l k) (if (= k 0) (car l) (tn-nth (cdr l) (- k 1))))
(define (tn-len l) (if (null? l) 0 (+ 1 (tn-len (cdr l)))))
(define (tn-reverse l) (tn-rev l (quote ())))
(define (tn-rev l acc) (if (null? l) acc (tn-rev (cdr l) (cons (car l) acc))))

; ----- tower accessors: tower is bottom-up; the height-h level is the h-th descriptor (1-indexed) -----
(define (te-level tower h) (tn-nth tower (- h 1)))
(define (te-level-type tower h) (car (te-level tower h)))
(define (te-level-b tower h) (car (cdr (te-level tower h))))

; ----- zero / one at height h -----
(define (te-zero h) (if (= h 0) (rat-zero) (list (te-zero (- h 1)))))
(define (te-one h) (if (= h 0) (rat-one) (list (te-one (- h 1)))))

; ----- coefficient accessor at height h: the k-th height-(h-1) coefficient (zero if absent) -----
(define (te-coeff h e k) (if (if (< k 0) #t (>= k (tn-len e))) (te-zero (- h 1)) (tn-nth e k)))

; ----- recursive add -----
(define (te-add tower h a b) (if (= h 0) (rat-add a b) (te-add-go tower h a b)))
(define (te-add-go tower h a b) (cond ((null? a) b) ((null? b) a) (else (cons (te-add tower (- h 1) (car a) (car b)) (te-add-go tower h (cdr a) (cdr b))))))

; ----- recursive scale by integer k -----
(define (te-scale-int h k e) (if (= h 0) (rat-scale k e) (te-si-go h k e)))
(define (te-si-go h k e) (if (null? e) (quote ()) (cons (te-scale-int (- h 1) k (car e)) (te-si-go h k (cdr e)))))

; ----- recursive multiply.  At height h, polynomial multiply in theta_h with coefficient products at h-1. -----
(define (te-mul tower h a b) (if (= h 0) (rat-mul a b) (te-mul-go tower h a b)))
(define (te-mul-go tower h a b) (if (null? a) (quote ()) (te-add tower h (te-coeff-scale tower h (car a) b) (cons (te-zero (- h 1)) (te-mul-go tower h (cdr a) b)))))
(define (te-coeff-scale tower h c b) (if (null? b) (quote ()) (cons (te-mul tower (- h 1) c (car b)) (te-coeff-scale tower h c (cdr b)))))
; multiply a height-(h-1) coefficient c into a full height-h element e
(define (te-coeff-mul tower h c e) (te-coeff-scale tower h c e))

; ----- recursive derivation -----
(define (te-deriv tower h e) (if (= h 0) (rat-deriv e) (te-deriv-level tower h e (te-level-type tower h))))
(define (te-deriv-level tower h e typ) (cond ((equal? typ (quote exp)) (te-deriv-exp tower h e)) ((equal? typ (quote alg)) (te-deriv-alg tower h e)) (else (te-deriv-log tower h e))))

; algebraic level (alg n a): theta^n = a, so theta' = (a'/(n a)) theta = w theta, giving the diagonal
; D(sum c_k theta^k) = sum (D(c_k) + k w c_k) theta^k, k < n.  Same shape as exp with rate w in place of b'.
(define (te-deriv-alg tower h e) (te-dalg-go tower h e 0 (te-alg-w tower h)))
(define (te-alg-n tower h) (car (cdr (te-level tower h))))
(define (te-alg-a tower h) (car (cdr (cdr (te-level tower h)))))
(define (te-alg-w tower h) (te-rat-div tower (- h 1) (te-deriv tower (- h 1) (te-alg-a tower h)) (te-scale-int (- h 1) (te-alg-n tower h) (te-alg-a tower h))))
(define (te-dalg-go tower h e k w)
  (if (>= k (tn-len e)) (quote ())
      (cons (te-add tower (- h 1) (te-deriv tower (- h 1) (te-coeff h e k)) (te-mul tower (- h 1) (te-scale-int (- h 1) k w) (te-coeff h e k)))
            (te-dalg-go tower h e (+ k 1) w))))

; exp level: D(sum c_k theta^k) = sum (D(c_k) + k D(b) c_k) theta^k
(define (te-deriv-exp tower h e) (te-dexp-go tower h e 0 (te-deriv tower (- h 1) (te-level-b tower h))))
(define (te-dexp-go tower h e k Db)
  (if (>= k (tn-len e)) (quote ())
      (cons (te-add tower (- h 1) (te-deriv tower (- h 1) (te-coeff h e k)) (te-mul tower (- h 1) (te-scale-int (- h 1) k Db) (te-coeff h e k)))
            (te-dexp-go tower h e (+ k 1) Db))))

; log level: D(sum c_k theta^k) = sum (D(c_k) + (k+1) u c_{k+1}) theta^k, u = D(b)/b
(define (te-deriv-log tower h e) (te-dlog-go tower h e 0 (tn-len e) (te-logu tower h)))
(define (te-logu tower h) (te-rat-div tower (- h 1) (te-deriv tower (- h 1) (te-level-b tower h)) (te-level-b tower h)))
(define (te-dlog-go tower h e k m u)
  (if (>= k m) (quote ())
      (cons (te-add tower (- h 1) (te-deriv tower (- h 1) (te-coeff h e k)) (te-mul tower (- h 1) (te-scale-int (- h 1) (+ k 1) u) (te-coeff h e (+ k 1))))
            (te-dlog-go tower h e (+ k 1) m u))))

; ----- division at height h-1 (needed for log's u = b'/b).  At height 0, rat-div; higher heights are only
; needed when b is itself a tower element and b'/b stays in the field -- for the log-of-base case b is height
; h-1 and b'/b is computed via te-rat-div, which at height 0 is rat-div and is only invoked here for the cases
; our towers produce (b a single lower monomial), where the division is exact. -----
(define (te-rat-div tower h a b) (if (= h 0) (rat-div a b) (te-div-poly tower h a b)))
; height>0 division: only used when b is a single power (one nonzero coeff); divide coefficient-wise by that.
(define (te-div-poly tower h a b) (te-divp-go tower h a b))
(define (te-divp-go tower h a b) (if (null? a) (quote ()) (cons (te-rat-div tower (- h 1) (car a) (te-leading b)) (te-divp-go tower h (cdr a) b))))
(define (te-leading b) (te-lead-go b))
(define (te-lead-go b) (cond ((null? b) (car b)) ((null? (cdr b)) (car b)) (else (te-lead-go (cdr b)))))

; ----- recursive equality -----
(define (te-equal? tower h a b) (if (= h 0) (rat-equal? a b) (te-eq-go tower h a b 0 (te-maxlen a b))))
(define (te-maxlen a b) (if (> (tn-len a) (tn-len b)) (tn-len a) (tn-len b)))
(define (te-eq-go tower h a b k m) (if (>= k m) #t (if (te-equal? tower (- h 1) (te-coeff h a k) (te-coeff h b k)) (te-eq-go tower h a b (+ k 1) m) #f)))
