; -*- lisp -*-
; lib/cas/lucas.lisp -- the Lucas-Lehmer test and Lucas sequences.
;
; The Lucas-Lehmer test is the efficient, exact primality proof for Mersenne numbers
; M_p = 2^p - 1 (p an odd prime): set s_0 = 4 and s_{i+1} = s_i^2 - 2 (mod M_p); then M_p
; is prime iff s_{p-2} = 0.  It needs only p-2 modular squarings, so it certifies primes
; far past trial division -- M_31 and M_127 fall out at once.  These are exactly the
; exponents behind the even perfect numbers (Euclid-Euler), tying back to the perfect
; module.
;
; The module also provides the Lucas sequences U_n, V_n with parameters (P, Q):
;     U_0 = 0, U_1 = 1, V_0 = 2, V_1 = P,   X_{n+1} = P X_n - Q X_{n-1},
; which specialise to the Fibonacci and Lucas numbers at (P, Q) = (1, -1).  The companion
; identity  V_n^2 - D U_n^2 = 4 Q^n  (with D = P^2 - 4Q) holds for all n and is used as a
; certificate.  Lucas-Lehmer is additionally cross-checked against the independent
; Miller-Rabin test.  Builds on numbertheory.lisp.

(import "cas/numbertheory.lisp")

; ---------- Lucas-Lehmer ----------
(define (mersenne p) (- (expt 2 p) 1))
(define (ll-iter s m k) (if (= k 0) s (ll-iter (remainder (- (* s s) 2) m) m (- k 1))))
(define (lucas-lehmer p)                 ; p odd prime; #t iff M_p is prime
  (let ((m (mersenne p))) (= (ll-iter 4 m (- p 2)) 0)))
(define (mersenne-prime-ll? p)
  (cond ((= p 2) #t) ((not (prime? p)) #f) (else (lucas-lehmer p))))

; ---------- Lucas sequences U_n, V_n with parameters (P, Q) ----------
; carry the pair (U_n . V_n) forward one step
(define (lucas-step P Q u v) (cons (- (* P u) (* Q v)) u))   ; helper not used directly; see below
(define (lucas-pair P Q n) (lp P Q n 0 1 2 P))               ; returns (U_n . V_n)
(define (lp P Q n u0 u1 v0 v1)
  (if (= n 0) (cons u0 v0)
      (lp P Q (- n 1) u1 (- (* P u1) (* Q u0)) v1 (- (* P v1) (* Q v0)))))
(define (lucas-u P Q n) (car (lucas-pair P Q n)))
(define (lucas-v P Q n) (cdr (lucas-pair P Q n)))

; ---------- certificates ----------
(define (lucas-lehmer-agrees? p) (equal? (mersenne-prime-ll? p) (prime? (mersenne p))))
(define (lucas-identity-ok? P Q n)        ; V_n^2 - D U_n^2 = 4 Q^n,  D = P^2 - 4Q
  (let ((pr (lucas-pair P Q n)))
    (= (- (* (cdr pr) (cdr pr)) (* (- (* P P) (* 4 Q)) (* (car pr) (car pr)))) (* 4 (expt Q n)))))

; ---------- collections / display ----------
(define (range a b) (if (> a b) '() (cons a (range (+ a 1) b))))
(define (mersenne-prime-exponents lo hi) (filter mersenne-prime-ll? (range lo hi)))
(define (fib n) (lucas-u 1 -1 n))         ; Fibonacci  F_n
(define (lucasnum n) (lucas-v 1 -1 n))    ; Lucas number L_n
(define (ll->string p) (if (mersenne-prime-ll? p) (string-append "M_" (number->string p) " = 2^" (number->string p) "-1 is prime") (string-append "M_" (number->string p) " is composite")))
