; -*- lisp -*-
; lib/cas/polycrt.lisp -- the Chinese Remainder Theorem over F_p[x].
;
; Given pairwise-coprime polynomial moduli m_1, ..., m_r over F_p and target residues
; a_1, ..., a_r, there is a unique polynomial f of degree below deg(M), M = prod m_i, with
; f = a_i mod m_i for every i.  It is assembled as f = sum_i a_i M_i (M_i^{-1} mod m_i),
; where M_i = M / m_i and the inverse is obtained from extended Euclid over F_p (the moduli
; being coprime makes each M_i a unit modulo m_i).
;
; Certified by reducing the reconstruction modulo each modulus and checking it matches the
; residue, together with the degree bound deg(f) < deg(M).  Because reduction modulo a
; linear polynomial x - c is evaluation at c, CRT with moduli (x - c_i) and constant
; residues reproduces ordinary polynomial interpolation -- a cross-check against the
; Lagrange form.  Builds on ffactor.lisp.

(import "cas/ffactor.lisp")

(define (cadr l) (car (cdr l)))
(define (caddr l) (car (cdr (cdr l))))

; ---------- extended Euclid over F_p: (gcd s t) with s a + t b = gcd (monic) ----------
(define (pcrt-gcdext a b p)
  (if (pzero? b)
      (list (pmonic a p) (list (mod-inverse (lead-coef a p) p)) '())
      (let ((q (pquot a b p)) (rec (pcrt-gcdext b (pmod a b p) p)))
        (list (car rec) (caddr rec) (psub (cadr rec) (pmul q (caddr rec) p) p)))))
(define (pinv-mod a m p) (pmod (cadr (pcrt-gcdext (pmod a m p) m p)) m p))   ; a^{-1} mod m

; ---------- CRT reconstruction ----------
(define (prod-list ms p) (if (null? ms) (list 1) (pmul (car ms) (prod-list (cdr ms) p) p)))
(define (crt-sum rs ms M p)
  (if (null? rs) (list 0)
      (let ((mi (car ms)))
        (let ((Mi (pquot M mi p)))
          (padd (pmul (car rs) (pmul Mi (pinv-mod Mi mi p) p) p) (crt-sum (cdr rs) (cdr ms) M p) p)))))
(define (poly-crt residues moduli p) (let ((M (prod-list moduli p))) (pmod (crt-sum residues moduli M p) M p)))

; ---------- linear moduli from points, for the interpolation cross-check ----------
(define (linmods pts p) (map (lambda (c) (psub (list 0 1) (list (imod c p)) p)) pts))   ; x - c
(define (peval f x p) (if (null? f) 0 (imod (+ (car f) (* x (peval (cdr f) x p))) p)))

; ---------- certificates ----------
(define (cong? f a m p) (equal? (pmod f m p) (pmod a m p)))
(define (all-cong f rs ms p) (cond ((null? rs) #t) ((cong? f (car rs) (car ms) p) (all-cong f (cdr rs) (cdr ms) p)) (else #f)))
(define (poly-crt-ok? residues moduli p)
  (let ((f (poly-crt residues moduli p)))
    (and (all-cong f residues moduli p) (< (pdeg f) (pdeg (prod-list moduli p))))))
(define (interp-ok? pts vals p)
  (let ((f (poly-crt (map list vals) (linmods pts p) p)))
    (vals-match f pts vals p)))
(define (vals-match f pts vals p) (cond ((null? pts) #t) ((= (peval f (car pts) p) (imod (car vals) p)) (vals-match f (cdr pts) (cdr vals) p)) (else #f)))

; ---------- display ----------
(define (crt->string residues moduli p) (poly->string (poly-crt residues moduli p)))
