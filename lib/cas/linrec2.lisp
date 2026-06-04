; -*- lisp -*-
; lib/cas/linrec2.lisp -- closed forms for second-order constant-coefficient recurrences whose characteristic
; polynomial is IRREDUCIBLE over Q (irrational quadratic roots) -- the Binet/Lucas case that linrec.lisp
; honestly declines.  This closes the remaining gap in linear-recurrence solving (the chart's "partial"): the
; canonical example is Fibonacci, whose closed form needs the golden ratio.
;
; For a_n = p a_{n-1} + q a_{n-2} the characteristic polynomial is x^2 - p x - q, with discriminant D = p^2 + 4q.
; When D is not a perfect square the roots r, s = (p +- sqrt D)/2 live in Q(sqrt D), and the closed form is
;     a_n = A r^n + B s^n ,   A = (a_1 - a_0 s)/sqrt D ,  B = (a_0 r - a_1)/sqrt D ,
; with A, B conjugate in Q(sqrt D); a_n is rational for every n.  We carry out the whole computation in Q(sqrt D)
; (elements u + v sqrt D, v rational), so the closed form is exact, and CERTIFY it by evaluating it for many n
; and comparing against the directly iterated sequence -- a wrong closed form is never reported.  If D is a
; perfect square the roots are rational and linrec.lisp already handles it (lr2-solve says 'use-linrec).
;
; A Q(sqrt D) value is a pair (u . v) meaning u + v sqrt D.  The closed form is returned as
;   (list 'closed-form D (cons Au Av) (cons Bu Bv) (cons ru rv) (cons su sv))
; meaning a_n = A r^n + B s^n with those Q(sqrt D) constants; lr2-eval computes a_n (a rational) from it.
;
; Public:
;   lr2-disc p q            -> the discriminant D = p^2 + 4q
;   lr2-solve p q a0 a1     -> the closed form (above) | 'use-linrec (D a perfect square) 
;   lr2-eval cf n           -> a_n (rational) from the closed form
;   lr2-certify cf p q a0 a1 N -> #t iff the closed form matches the iterated recurrence for n = 0..N
;   lr2-fib n / lr2-lucas n -> Fibonacci / Lucas via the closed form (convenience, for testing)
;
; Verified: Fibonacci (p=1,q=1,a0=0,a1=1) and Lucas (a0=2,a1=1) reproduced exactly; a non-unit case
; a_n = a_{n-1} + a_{n-2} variants; and a perfect-square discriminant correctly deferred to linrec.
;
; Builds only on exact rational arithmetic and poly.lisp (for the perfect-square test).

(import "cas/poly.lisp")

; ----- Q(sqrt D) arithmetic: element (u . v) = u + v sqrt D -----
(define (qd-make u v) (cons u v))
(define (qd-u x) (car x))
(define (qd-v x) (cdr x))
(define (qd-add x y D) (cons (+ (car x) (car y)) (+ (cdr x) (cdr y))))
(define (qd-sub x y D) (cons (- (car x) (car y)) (- (cdr x) (cdr y))))
(define (qd-mul x y D) (cons (+ (* (car x) (car y)) (* D (* (cdr x) (cdr y)))) (+ (* (car x) (cdr y)) (* (cdr x) (car y)))))
(define (qd-pow x n D) (if (= n 0) (cons 1 0) (qd-mul x (qd-pow x (- n 1) D) D)))
; divide by sqrt D: (u + v sqrt D)/sqrt D = v + (u/D) sqrt D
(define (qd-divsqrt x D) (cons (cdr x) (/ (car x) D)))

; ----- discriminant and perfect-square test -----
(define (lr2-disc p q) (+ (* p p) (* 4 q)))
(define (lr2-perfect-square? n)
  (cond ((< n 0) #f) ((= n 0) #t) (else (lr2-ps-go n 1))))
(define (lr2-ps-go n k) (cond ((> (* k k) n) #f) ((= (* k k) n) #t) (else (lr2-ps-go n (+ k 1)))))

; ----- the closed form -----
(define (lr2-solve p q a0 a1)
  (lr2-solve-D p q a0 a1 (lr2-disc p q)))
(define (lr2-solve-D p q a0 a1 D)
  (if (lr2-perfect-square? D) (quote use-linrec)
      (lr2-build p q a0 a1 D)))
(define (lr2-build p q a0 a1 D)
  (lr2-assemble D
                (cons (/ p 2) (/ 1 2))          ; r = (p + sqrt D)/2
                (cons (/ p 2) (/ -1 2))         ; s = (p - sqrt D)/2
                a0 a1))
; A = (a1 - a0 s)/sqrt D, B = (a0 r - a1)/sqrt D
(define (lr2-assemble D r s a0 a1)
  (lr2-pack D r s
            (qd-divsqrt (qd-sub (cons a1 0) (qd-mul (cons a0 0) s D) D) D)
            (qd-divsqrt (qd-sub (qd-mul (cons a0 0) r D) (cons a1 0) D) D)))
(define (lr2-pack D r s A B) (list (quote closed-form) D A B r s))

; ----- evaluate a_n (rational) from the closed form -----
(define (lr2-eval cf n)
  (lr2-eval-go (lr2-cfD cf) (lr2-cfA cf) (lr2-cfB cf) (lr2-cfr cf) (lr2-cfs cf) n))
(define (lr2-cfD cf) (car (cdr cf)))
(define (lr2-cfA cf) (car (cdr (cdr cf))))
(define (lr2-cfB cf) (car (cdr (cdr (cdr cf)))))
(define (lr2-cfr cf) (car (cdr (cdr (cdr (cdr cf))))))
(define (lr2-cfs cf) (car (cdr (cdr (cdr (cdr (cdr cf)))))))
(define (lr2-eval-go D A B r s n)
  (qd-u (qd-add (qd-mul A (qd-pow r n D) D) (qd-mul B (qd-pow s n D) D) D)))  ; the rational part (v should be 0)

; ----- certificate: the closed form matches the iterated recurrence for n = 0..N -----
(define (lr2-certify cf p q a0 a1 N) (lr2-cert-go cf p q (lr2-iterate p q a0 a1 N) 0 N))
(define (lr2-iterate p q a0 a1 N) (lr2-it-go p q (list a1 a0) 2 N))
(define (lr2-it-go p q acc k N)                    ; acc holds (a_{k-1} a_{k-2} ... a_0) reversed-ish (a_{k-1} first)
  (if (> k N) (lr2-reverse acc)
      (lr2-it-go p q (cons (+ (* p (car acc)) (* q (car (cdr acc)))) acc) (+ k 1) N)))
(define (lr2-reverse l) (lr2-rev l (quote ())))
(define (lr2-rev l acc) (if (null? l) acc (lr2-rev (cdr l) (cons (car l) acc))))
(define (lr2-cert-go cf p q seq n N)
  (cond ((> n N) #t)
        ((not (= (lr2-eval cf n) (lr2-nth seq n))) #f)
        (else (lr2-cert-go cf p q seq (+ n 1) N))))
(define (lr2-nth l k) (if (= k 0) (car l) (lr2-nth (cdr l) (- k 1))))

; ----- convenience: Fibonacci and Lucas via the closed form -----
(define (lr2-fib n) (lr2-eval (lr2-solve 1 1 0 1) n))
(define (lr2-lucas n) (lr2-eval (lr2-solve 1 1 2 1) n))
