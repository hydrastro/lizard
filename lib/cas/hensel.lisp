; -*- lisp -*-
; lib/cas/hensel.lisp -- Hensel lifting of a polynomial factorization.
;
; If a monic polynomial factors as f = g h modulo a prime p with g, h coprime, that
; factorisation lifts uniquely to one modulo every power p^k.  This is the engine that turns
; factorisation over a small field into factorisation over the integers.  The lift proceeds
; one power at a time: with fixed mod-p Bezout cofactors s g + t h = 1, the defect
; delta = (f - g h)/p^k mod p is corrected by g <- g + p^k (t delta mod g) and
; h <- h + p^k (s delta + (t delta div g) h), which preserves f = g h to one higher power
; while keeping g monic.
;
; The finite-field polynomial arithmetic is reused from ffactor (its add/subtract/multiply
; reduce coefficients modulo any modulus, prime or not), with an extended Euclid over F_p
; supplying the cofactors.  Two facts certify each lift: the product G H reconstructs f
; modulo p^k, and G reduces to the original g modulo p.  So x^2 - 2 = (x-3)(x-4) mod 7 lifts
; to a factorisation mod 49, 343, ... whose product is x^2 - 2 each time.  Builds on
; ffactor.lisp.

(import "cas/ffactor.lisp")

(define (cadr l) (car (cdr l)))
(define (caddr l) (car (cdr (cdr l))))

; ---------- extended Euclid over F_p: returns (gcd s t) with s a + t b = gcd ----------
(define (pgcd-ext a b p)
  (if (pzero? b)
      (list (pmonic a p) (list (mod-inverse (lead-coef a p) p)) '())
      (let ((q (pquot a b p)) (rec (pgcd-ext b (pmod a b p) p)))
        (list (car rec) (caddr rec) (psub (cadr rec) (pmul q (caddr rec) p) p)))))

; ---------- one Hensel step: f = g h mod p^k  ->  (g' . h') with f = g' h' mod p^{k+1} ----------
(define (scale-pk poly p k) (let ((pk (expt p k))) (map (lambda (c) (* c pk)) poly)))
(define (hensel-delta f g h p k m1)
  (let ((e (psub f (pmul g h m1) m1)) (pk (expt p k)))   ; f - g h mod p^{k+1}, divisible by p^k
    (pclean (map (lambda (c) (imod (quotient c pk) p)) e) p)))
(define (hensel-step f g h s t p k)
  (let ((m1 (expt p (+ k 1))))
    (let ((delta (hensel-delta f g h p k m1)))
      (let ((dm (pdivmod (pmul t delta p) g p)))         ; t*delta = q*g + beta, deg beta < deg g
        (let ((beta (cdr dm)) (alpha (padd (pmul s delta p) (pmul (car dm) h p) p)))
          (cons (padd g (scale-pk beta p k) m1) (padd h (scale-pk alpha p k) m1)))))))

; ---------- iterate to the target power ----------
(define (hl f g h s t p k target) (if (>= k target) (cons g h) (let ((gh (hensel-step f g h s t p k))) (hl f (car gh) (cdr gh) s t p (+ k 1) target))))
(define (hensel-lift f g h p target)
  (let ((st (pgcd-ext g h p)))
    (hl (pclean f (expt p target)) (pclean g (expt p target)) (pclean h (expt p target)) (cadr st) (caddr st) p 1 target)))

; ---------- certificates ----------
(define (hensel-ok? f g h p target)
  (let ((gh (hensel-lift f g h p target)) (m (expt p target)))
    (and (equal? (pclean (pmul (car gh) (cdr gh) m) m) (pclean f m))
         (equal? (pclean (car gh) p) (pclean g p)))))
(define (bezout-ok? g h p)
  (let ((st (pgcd-ext g h p)))
    (equal? (padd (pmul (cadr st) g p) (pmul (caddr st) h p) p) (list 1))))

; ---------- display ----------
(define (hpoly->string f) (poly->string f))
(define (lift->string f g h p target)
  (let ((gh (hensel-lift f g h p target)))
    (string-append "[" (poly->string (car gh)) "] * [" (poly->string (cdr gh)) "]")))
