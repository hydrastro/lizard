; -*- lisp -*-
; lib/cas/frobenius.lisp -- the Frobenius number and numerical semigroups.
;
; Given coin denominations with greatest common divisor 1, every sufficiently large amount
; is payable; the largest amount that is NOT a non-negative combination is the Frobenius
; number (the "Chicken McNugget" number).  It is computed from the Apery set: working
; modulo the smallest coin m, dist[r] is the least representable amount congruent to r,
; found by a Bellman-Ford relaxation dist[(r+c) mod m] = min(dist[(r+c) mod m], dist[r]+c)
; over the coins.  Then an amount n is representable exactly when dist[n mod m] <= n, the
; Frobenius number is max(dist) - m, and the genus (how many amounts are unpayable) is
; (sum(dist) - m(m-1)/2) / m.
;
; The results are cross-checked two independent ways: for two coins the Apery computation
; must agree with the classical closed form ab - a - b, and for any coin set the Frobenius
; number must itself be unpayable while the next m consecutive amounts are all payable (so
; everything beyond is payable).  Self-contained over the integers.

(define big 1000000000)
(define (range a b) (if (> a b) '() (cons a (range (+ a 1) b))))
(define (lref l i) (if (= i 0) (car l) (lref (cdr l) (- i 1))))
(define (lset l i v) (if (= i 0) (cons v (cdr l)) (cons (car l) (lset (cdr l) (- i 1) v))))
(define (minl l) (if (null? (cdr l)) (car l) (min (car l) (minl (cdr l)))))
(define (maxl l) (if (null? (cdr l)) (car l) (max (car l) (maxl (cdr l)))))
(define (suml l) (if (null? l) 0 (+ (car l) (suml (cdr l)))))

; ---------- Apery set via Bellman-Ford on residues mod the smallest coin ----------
(define (init-dist m) (cons 0 (map (lambda (i) big) (range 1 (- m 1)))))
(define (try1 m dist r c) (let ((nr (remainder (+ r c) m)) (nd (+ (lref dist r) c))) (if (< nd (lref dist nr)) (lset dist nr nd) dist)))
(define (relax-rc coins m dist r) (if (null? coins) dist (relax-rc (cdr coins) m (try1 m dist r (car coins)) r)))
(define (relax-all coins m dist r) (if (>= r m) dist (relax-all coins m (relax-rc coins m dist r) (+ r 1))))
(define (bf-rounds coins m dist k) (if (= k 0) dist (bf-rounds coins m (relax-all coins m dist 0) (- k 1))))
(define (apery coins) (let ((m (minl coins))) (bf-rounds coins m (init-dist m) m)))

; ---------- Frobenius number, genus, representability ----------
(define (frobenius coins) (- (maxl (apery coins)) (minl coins)))
(define (genus coins) (let ((m (minl coins))) (quotient (- (suml (apery coins)) (quotient (* m (- m 1)) 2)) m)))
(define (representable-with? n coins dist m) (and (>= n 0) (<= (lref dist (remainder n m)) n)))
(define (representable? n coins) (representable-with? n coins (apery coins) (minl coins)))

; ---------- certificates ----------
(define (frobenius-two-ok? a b) (= (frobenius (list a b)) (- (* a b) a b)))
(define (all-rep? lo hi coins dist m) (cond ((> lo hi) #t) ((representable-with? lo coins dist m) (all-rep? (+ lo 1) hi coins dist m)) (else #f)))
(define (frobenius-gap-ok? coins)
  (let ((g (frobenius coins)) (m (minl coins)) (dist (apery coins)))
    (and (not (representable-with? g coins dist m)) (all-rep? (+ g 1) (+ g m) coins dist m))))
(define (count-gaps lo hi coins dist m) (cond ((> lo hi) 0) ((representable-with? lo coins dist m) (count-gaps (+ lo 1) hi coins dist m)) (else (+ 1 (count-gaps (+ lo 1) hi coins dist m)))))
(define (genus-count-ok? coins) (let ((g (frobenius coins)) (m (minl coins)) (dist (apery coins))) (= (genus coins) (count-gaps 0 g coins dist m))))

; ---------- display ----------
(define (frobenius->string coins) (number->string (frobenius coins)))
