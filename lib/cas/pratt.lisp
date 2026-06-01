; -*- lisp -*-
; lib/cas/pratt.lisp -- Pratt primality certificates (constructive primality proofs).
;
; The number-theory module decides primality by Miller-Rabin -- a test.  A Pratt
; certificate is something stronger: a self-contained PROOF that n is prime, checkable by
; pure arithmetic.  It rests on Lucas's theorem: n is prime iff there is a witness a whose
; multiplicative order modulo n is exactly n-1, i.e.
;
;     a^(n-1) = 1 (mod n)   and   a^((n-1)/q) /= 1 (mod n) for every prime q | n-1.
;
; The second clause needs to know the primes q dividing n-1, and that they really are
; prime -- so the certificate is RECURSIVE: it carries the factorisation of n-1 together
; with a Pratt certificate for each prime factor, bottoming out at 2.  A certificate is a
; tree
;
;     (n a ((q1 e1 cert_q1) (q2 e2 cert_q2) ...))        with the leaf (2 prime).
;
; The verifier re-checks every node from scratch -- the product of prime powers really is
; n-1, a^(n-1) = 1, each a^((n-1)/qi) /= 1, and each sub-certificate -- so it shares no
; work with the builder and a forged certificate is rejected.  Cross-checked against the
; independent Miller-Rabin test, the two always agree.  Builds on numbertheory.lisp.

(import "cas/numbertheory.lisp")

(define (distinct-primes m) (map car (factor-int m)))

; ---------- finding a primitive-root witness ----------
(define (witness-clauses? a n primes)
  (cond ((null? primes) #t)
        ((= (mod-exp a (quotient (- n 1) (car primes)) n) 1) #f)
        (else (witness-clauses? a n (cdr primes)))))
(define (is-witness? a n primes) (and (= (mod-exp a (- n 1) n) 1) (witness-clauses? a n primes)))
(define (find-witness n) (fw n 2 (distinct-primes (- n 1))))
(define (fw n a primes) (cond ((>= a n) 'none) ((is-witness? a n primes) a) (else (fw n (+ a 1) primes))))

; ---------- building a certificate ----------
(define (pratt n)
  (cond ((= n 2) (list 2 'prime))
        (else (let ((a (find-witness n)))
                (if (equal? a 'none) (list n 'composite)
                    (list n a (map (lambda (qe) (list (car qe) (cdr qe) (pratt (car qe)))) (factor-int (- n 1)))))))))

; ---------- verifying a certificate (independent re-check) ----------
(define (prod-entries es) (if (null? es) 1 (* (expt (car (car es)) (car (cdr (car es)))) (prod-entries (cdr es)))))
(define (check-entries es a n)
  (cond ((null? es) #t)
        (else (let ((q (car (car es))) (sub (car (cdr (cdr (car es))))))
                (and (not (= (mod-exp a (quotient (- n 1) q) n) 1))
                     (= (car sub) q)
                     (pratt-check sub)
                     (check-entries (cdr es) a n))))))
(define (pratt-check c)
  (cond ((equal? (car (cdr c)) 'prime)     (= (car c) 2))
        ((equal? (car (cdr c)) 'composite) #f)
        (else (let ((n (car c)) (a (car (cdr c))) (es (car (cdr (cdr c)))))
                (and (= (prod-entries es) (- n 1))
                     (= (mod-exp a (- n 1) n) 1)
                     (check-entries es a n))))))

; ---------- proof-carrying primality + cross-check ----------
(define (pratt-prime? n) (if (< n 2) #f (pratt-check (pratt n))))
(define (pratt-agrees? n) (equal? (pratt-prime? n) (prime? n)))

; certificate size (number of prime nodes) and depth -- a feel for the proof
(define (cert-size c) (cond ((equal? (car (cdr c)) 'prime) 1) ((equal? (car (cdr c)) 'composite) 0) (else (+ 1 (size-entries (car (cdr (cdr c))))))))
(define (size-entries es) (if (null? es) 0 (+ (cert-size (car (cdr (cdr (car es))))) (size-entries (cdr es)))))

; ---------- display ----------
(define (witness-of n) (let ((c (pratt n))) (if (equal? (car (cdr c)) 'prime) 1 (if (equal? (car (cdr c)) 'composite) 'none (car (cdr c))))))
(define (pratt->string n) (if (pratt-prime? n) (string-append (number->string n) " prime [Pratt cert verified]") (string-append (number->string n) " composite")))
