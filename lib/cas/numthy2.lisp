; -*- lisp -*-
; lib/cas/numthy2.lisp -- a cluster of classical multiplicative / additive number theory: the Mobius function and
; Dirichlet convolution, perfect and amicable numbers, the Frobenius number (Chicken McNugget) for two and three
; coprime denominations, and the Stern-Brocot / Farey mediant structure.  Each result is exact and gated by an
; arithmetic check, extending numbertheory.lisp (docs/CAS.md -- closing classical number-theory gaps).
;
; - Mobius mu(n): 0 if n has a squared prime factor, else (-1)^(number of prime factors).  Dirichlet convolution
;   (f * g)(n) = sum_{d | n} f(d) g(n/d); the Mobius and unit functions verify the Mobius-inversion identity
;   (mu * 1)(n) = [n = 1] exactly.
; - sigma(n) = sum of divisors; n is PERFECT iff sigma(n) = 2n; (m, n) are AMICABLE iff s(m) = n and s(n) = m
;   with m != n, where s = sigma - identity (aliquot sum).
; - Frobenius number g(a, b) = a b - a - b for coprime a, b (Sylvester); for three coprime denominations we
;   compute it by the exact round-robin (Apery-set) shortest-path over residues mod the smallest.
; - Stern-Brocot mediant of a/b and c/d is (a+c)/(b+d); successive Farey neighbours satisfy bc - ad = 1, the
;   unimodular certificate, and the mediant lies strictly between.
;
; Public:
;   moebius n                 -> mu(n)
;   dirichlet f g n           -> (f * g)(n)  (f, g unary functions on positive integers)
;   divisor-sum n             -> sigma(n) ; aliquot n -> sigma(n) - n
;   perfect? n                -> #t iff n is perfect ; amicable? m n -> #t iff m, n amicable (m != n)
;   frobenius2 a b            -> a b - a - b (a, b coprime) ; frobenius-list (a b c...) -> Frobenius number
;   sb-mediant a b c d        -> (num . den) mediant of a/b and c/d
;   farey-neighbours? a b c d -> #t iff a/b, c/d are adjacent in some Farey sequence (bc - ad = 1)
;
; Verified: mu(1)=1, mu(6)=1, mu(12)=0, mu(30)=-1; Mobius inversion (mu*1)(n)=[n=1]; 6, 28, 496 perfect;
; (220, 284) amicable; frobenius2(3,5)=7, frobenius-list(6,9,20)=43 (the McNugget number); mediant 1/2,1/1 = 2/3;
; Farey neighbours 1/3, 1/2 certified by 1*2 - 3*1 = -1 -> |.|=1.
;
; Builds on numbertheory.lisp (factor-int, igcd) and is self-contained otherwise.

(import "cas/numbertheory.lisp")

(define (n2-len l) (if (null? l) 0 (+ 1 (n2-len (cdr l)))))
(define (n2-nth l k) (if (= k 0) (car l) (n2-nth (cdr l) (- k 1))))

; ----- Mobius function from the factorization ((p . e) ...) -----
(define (moebius n) (if (= n 1) 1 (n2-mu-go (factor-int n) 1)))
(define (n2-mu-go fs sign) (cond ((null? fs) sign) ((> (cdr (car fs)) 1) 0) (else (n2-mu-go (cdr fs) (- 0 sign)))))

; ----- divisors of n (as a list), via the factorization -----
(define (n2-divisors n) (n2-div-build (factor-int n)))
(define (n2-div-build fs) (if (null? fs) (list 1) (n2-div-mix (n2-ppows (car (car fs)) (cdr (car fs))) (n2-div-build (cdr fs)))))
(define (n2-ppows p e) (n2-pp-go p e 0 1))
(define (n2-pp-go p e i acc) (if (> i e) (quote ()) (cons acc (n2-pp-go p e (+ i 1) (* acc p)))))
(define (n2-div-mix pls dls) (n2-dm-outer pls dls))
(define (n2-dm-outer pls dls) (if (null? pls) (quote ()) (n2-append (n2-dm-inner (car pls) dls) (n2-dm-outer (cdr pls) dls))))
(define (n2-dm-inner pk dls) (if (null? dls) (quote ()) (cons (* pk (car dls)) (n2-dm-inner pk (cdr dls)))))
(define (n2-append a b) (if (null? a) b (cons (car a) (n2-append (cdr a) b))))

; ----- Dirichlet convolution (f * g)(n) = sum_{d|n} f(d) g(n/d) -----
(define (dirichlet f g n) (n2-dir-go f g n (n2-divisors n)))
(define (n2-dir-go f g n ds) (if (null? ds) 0 (+ (* (f (car ds)) (g (quotient n (car ds)))) (n2-dir-go f g n (cdr ds)))))
; the two standard arithmetic functions for the inversion check
(define (n2-one n) 1)                              ; the constant-1 function
(define (n2-unit n) (if (= n 1) 1 0))              ; the Dirichlet identity epsilon

; ----- divisor sum sigma, aliquot, perfect, amicable -----
(define (divisor-sum n) (n2-sum (n2-divisors n)))
(define (n2-sum l) (if (null? l) 0 (+ (car l) (n2-sum (cdr l)))))
(define (aliquot n) (- (divisor-sum n) n))
(define (perfect? n) (= (divisor-sum n) (* 2 n)))
(define (amicable? m n) (if (= m n) #f (if (= (aliquot m) n) (= (aliquot n) m) #f)))

; ----- Frobenius numbers -----
(define (frobenius2 a b) (- (* a b) (+ a b)))      ; Sylvester, for coprime a, b
; general (>=2 coprime denominations): exact via the Apery/round-robin over residues mod the smallest.
; reachable residues: shortest sum congruent to each r mod m0; Frobenius = max over r of (that sum) - m0.
(define (frobenius-list coins) (n2-frob (n2-sort-asc coins)))
(define (n2-frob coins) (n2-frob-run (car coins) coins))
(define (n2-frob-run m0 coins) (n2-frob-max (n2-apery m0 coins) m0))
; Apery set: dist[r] = least value reachable that is = r (mod m0), via Dijkstra-like relaxation (m0 small loop).
(define (n2-apery m0 coins) (n2-relax-fix (n2-init-dist m0) m0 coins))
(define (n2-init-dist m0) (cons 0 (n2-bigs (- m0 1))))   ; dist[0]=0, rest = "infinity"
(define (n2-bigs k) (if (<= k 0) (quote ()) (cons -1 (n2-bigs (- k 1)))))   ; -1 marks unreached
; relax until no change: for each coin c and each residue r with finite dist, candidate at (r+c) mod m0
(define (n2-relax-fix dist m0 coins) (n2-rf-go dist m0 coins 0))
(define (n2-rf-go dist m0 coins guard)
  (if (> guard (* m0 (n2-len coins))) dist
      (n2-rf-step dist m0 coins guard (n2-relax-once dist m0 coins))))
(define (n2-rf-step dist m0 coins guard nd) (if (n2-dist-eq dist nd) dist (n2-rf-go nd m0 coins (+ guard 1))))
(define (n2-dist-eq a b) (cond ((null? a) (null? b)) ((= (car a) (car b)) (n2-dist-eq (cdr a) (cdr b))) (else #f)))
(define (n2-relax-once dist m0 coins) (n2-relax-coins dist m0 coins))
(define (n2-relax-coins dist m0 coins) (if (null? coins) dist (n2-relax-coins (n2-relax-coin dist m0 (car coins)) m0 (cdr coins))))
(define (n2-relax-coin dist m0 c) (n2-rc-go dist m0 c 0))
(define (n2-rc-go dist m0 c r)
  (if (>= r m0) dist
      (n2-rc-go (n2-rc-upd dist m0 c r) m0 c (+ r 1))))
; if dist[r] reached, relax dist[(r+c) mod m0] with dist[r]+c
(define (n2-rc-upd dist m0 c r)
  (if (< (n2-nth dist r) 0) dist
      (n2-set dist (remainder (+ r c) m0) (n2-minpos (n2-nth dist (remainder (+ r c) m0)) (+ (n2-nth dist r) c)))))
(define (n2-minpos a b) (cond ((< a 0) b) ((< b 0) a) ((< a b) a) (else b)))
(define (n2-set l i v) (n2-set-go l i v 0))
(define (n2-set-go l i v j) (if (null? l) (quote ()) (cons (if (= j i) v (car l)) (n2-set-go (cdr l) i v (+ j 1)))))
(define (n2-frob-max apery m0) (- (n2-max-list apery) m0))
(define (n2-max-list l) (if (null? (cdr l)) (car l) (n2-maxb (car l) (n2-max-list (cdr l)))))
(define (n2-maxb a b) (if (> a b) a b))
(define (n2-sort-asc l) (n2-isort l (quote ())))
(define (n2-isort l acc) (if (null? l) acc (n2-isort (cdr l) (n2-ins (car l) acc))))
(define (n2-ins x l) (cond ((null? l) (list x)) ((<= x (car l)) (cons x l)) (else (cons (car l) (n2-ins x (cdr l))))))

; ----- Stern-Brocot mediant and Farey adjacency -----
(define (sb-mediant a b c d) (cons (+ a c) (+ b d)))
(define (farey-neighbours? a b c d) (= (n2-abs (- (* b c) (* a d))) 1))   ; unimodular: |bc - ad| = 1
(define (n2-abs x) (if (< x 0) (- 0 x) x))
