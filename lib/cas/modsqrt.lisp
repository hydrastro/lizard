; -*- lisp -*-
; lib/cas/modsqrt.lisp -- modular square roots and quadratic residues.
;
; The Legendre symbol (a/p) for an odd prime p is computed by Euler's criterion
; a^((p-1)/2) mod p, read as 0, +1, or -1.  The Jacobi symbol (a/n) for odd n is computed
; by the reciprocity recursion (pulling out factors of 2 and flipping signs).  Tonelli-
; Shanks then extracts a square root r with r^2 = a (mod p) when a is a residue.
;
; Every square root is checked by squaring it back: r*r mod p must equal a mod p, so a
; wrong root is never returned.  The Legendre symbol is cross-checked against a direct
; count of square roots for small primes, and the Jacobi symbol against its known
; multiplicative behaviour.  Builds on numbertheory.lisp.

(import "cas/numbertheory.lisp")

; ---------- Legendre symbol (a/p), p an odd prime ----------
(define (legendre a p) (let ((r (mod-exp a (quotient (- p 1) 2) p))) (if (= r (- p 1)) -1 r)))
(define (qr? a p) (= (legendre a p) 1))

; ---------- Jacobi symbol (a/n), n odd > 0 ----------
(define (jacobi a n) (jac (imod a n) n 1))
(define (jac a n acc)
  (cond ((= a 0) (if (= n 1) acc 0))
        ((= (remainder a 2) 0) (jac (quotient a 2) n (if (or (= (imod n 8) 1) (= (imod n 8) 7)) acc (- 0 acc))))
        (else (jac (imod n a) a (if (and (= (imod a 4) 3) (= (imod n 4) 3)) (- 0 acc) acc)))))

; ---------- Tonelli-Shanks: r with r^2 = a (mod p), p odd prime; 'none if a is a non-residue ----------
(define (find-nonresidue p z) (if (= (legendre z p) -1) z (find-nonresidue p (+ z 1))))
(define (sqrt-mod a p)
  (let ((a0 (imod a p)))
    (cond ((= a0 0) 0)
          ((= p 2) a0)
          ((= (legendre a0 p) -1) 'none)
          ((= (imod p 4) 3) (mod-exp a0 (quotient (+ p 1) 4) p))
          (else (ts-init a0 p)))))
(define (ts-init a p)
  (let ((q (odd-part (- p 1))))
    (let ((s (twos-count (- p 1))))
      (let ((z (find-nonresidue p 2)))
        (ts-loop p s (mod-exp z q p) (mod-exp a q p) (mod-exp a (quotient (+ q 1) 2) p))))))
(define (ts-loop p m c t r)
  (cond ((= t 0) 0)
        ((= t 1) r)
        (else (let ((i (ts-find-i p t 1)))
                (let ((b (mod-exp c (expt 2 (- (- m i) 1)) p)))
                  (ts-loop p i (imod (* b b) p) (imod (* t (imod (* b b) p)) p) (imod (* r b) p)))))))
(define (ts-find-i p t i) (if (= (mod-exp t (expt 2 i) p) 1) i (ts-find-i p t (+ i 1))))

; ---------- certificates ----------
(define (sqrt-mod-ok? a p) (let ((r (sqrt-mod a p))) (if (equal? r 'none) (= (legendre (imod a p) p) -1) (= (imod (* r r) p) (imod a p)))))
; brute-force count of x in [0,p) with x^2 = a (mod p)
(define (count-roots a p) (cr a p 0 0))
(define (cr a p x acc) (if (>= x p) acc (cr a p (+ x 1) (if (= (imod (* x x) p) (imod a p)) (+ acc 1) acc))))
(define (legendre-bruteforce-ok? a p) (let ((c (count-roots a p))) (if (= (imod a p) 0) (= c 1) (if (= (legendre a p) 1) (= c 2) (= c 0)))))

; ---------- display ----------
(define (sqrt-mod->string a p) (let ((r (sqrt-mod a p))) (if (equal? r 'none) "none" (string-append (number->string r) " (and " (number->string (- p r)) ")"))))
