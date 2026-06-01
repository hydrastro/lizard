; -*- lisp -*-
; lib/cas/gaussint.lisp -- the Gaussian integers Z[i].
;
; A Gaussian integer a + b i is stored as the pair (a . b).  Z[i] is a Euclidean domain
; under the norm N(a + b i) = a^2 + b^2: dividing z by w, round z * conj(w) / N(w) to the
; nearest Gaussian integer to get a quotient q, and the remainder r = z - q w then has
; N(r) < N(w), which drives a Euclidean gcd.  The units are exactly the four elements of
; norm 1 (1, -1, i, -i).
;
; A rational prime p behaves three ways in Z[i]: p = 2 ramifies as -i (1 + i)^2; a prime
; p = 3 (mod 4) stays inert (still a Gaussian prime); and a prime p = 1 (mod 4) SPLITS as
; (a + b i)(a - b i) with a^2 + b^2 = p -- which is exactly the sum-of-two-squares
; decomposition, so this module reuses that result to produce the Gaussian prime factors.
;
; Three independent identities serve as certificates: the norm is multiplicative
; (N(zw) = N(z) N(w)); Euclidean division genuinely reduces the norm (z = qw + r with
; N(r) < N(w)); and the computed gcd divides both inputs exactly (zero remainder).  Builds
; on twosquares.lisp (and through it numbertheory).

(import "cas/twosquares.lisp")

(define (gi-re z) (car z))
(define (gi-im z) (cdr z))
(define (gi-add z w) (cons (+ (car z) (car w)) (+ (cdr z) (cdr w))))
(define (gi-sub z w) (cons (- (car z) (car w)) (- (cdr z) (cdr w))))
(define (gi-mul z w) (cons (- (* (car z) (car w)) (* (cdr z) (cdr w))) (+ (* (car z) (cdr w)) (* (cdr z) (car w)))))
(define (gi-conj z) (cons (car z) (- 0 (cdr z))))
(define (gi-norm z) (+ (* (car z) (car z)) (* (cdr z) (cdr z))))
(define (gi-zero? z) (and (= (car z) 0) (= (cdr z) 0)))
(define (gi-equal? z w) (and (= (car z) (car w)) (= (cdr z) (cdr w))))

; ---------- Euclidean division: round to the nearest Gaussian integer ----------
(define (rnd-div n d) (floor (+ (/ n d) (/ 1 2))))            ; nearest integer to n/d, d > 0
(define (gi-quotient z w)
  (let ((p (gi-mul z (gi-conj w))) (nd (gi-norm w)))
    (cons (rnd-div (car p) nd) (rnd-div (cdr p) nd))))
(define (gi-mod z w) (gi-sub z (gi-mul (gi-quotient z w) w)))

; ---------- gcd ----------
(define (gi-gcd z w) (if (gi-zero? w) z (gi-gcd w (gi-mod z w))))

; ---------- units and primes ----------
(define (gi-unit? z) (= (gi-norm z) 1))
(define (gi-prime? z)
  (let ((n (gi-norm z)))
    (cond ((prime? n) #t)
          ((and (= (cdr z) 0) (prime? (iabs (car z))) (= (imod (iabs (car z)) 4) 3)) #t)
          ((and (= (car z) 0) (prime? (iabs (cdr z))) (= (imod (iabs (cdr z)) 4) 3)) #t)
          (else #f))))

; ---------- splitting a rational prime into Gaussian primes ----------
(define (gi-split p)                       ; p prime: returns a Gaussian prime factor
  (cond ((= p 2) (cons 1 1))               ; 2 = -i (1+i)^2
        ((= (imod p 4) 1) (let ((r (two-squares p))) (cons (car r) (cdr r))))
        (else (cons p 0))))                ; p = 3 mod 4 is inert

; ---------- certificates ----------
(define (gi-norm-mult-ok? z w) (= (gi-norm (gi-mul z w)) (* (gi-norm z) (gi-norm w))))
(define (gi-div-ok? z w)                   ; z = q w + r with N(r) < N(w)
  (and (not (gi-zero? w))
       (gi-equal? (gi-add (gi-mul (gi-quotient z w) w) (gi-mod z w)) z)
       (< (gi-norm (gi-mod z w)) (gi-norm w))))
(define (gi-gcd-divides? z w)
  (let ((g (gi-gcd z w))) (and (not (gi-zero? g)) (gi-zero? (gi-mod z g)) (gi-zero? (gi-mod w g)))))
(define (gi-split-ok? p)                   ; the two factors multiply back to p (a real integer)
  (let ((f (gi-split p))) (gi-equal? (gi-mul f (gi-conj f)) (cons p 0))))

; ---------- display ----------
(define (gi->string z)
  (let ((a (car z)) (b (cdr z)))
    (cond ((= b 0) (number->string a))
          ((= a 0) (string-append (number->string b) "i"))
          ((< b 0) (string-append (number->string a) " - " (number->string (- 0 b)) "i"))
          (else (string-append (number->string a) " + " (number->string b) "i")))))
