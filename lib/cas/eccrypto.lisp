; -*- lisp -*-
; lib/cas/eccrypto.lisp -- elliptic-curve cryptography (ECDH and ECDSA).
;
; On a curve with a base point G of known prime order ell, the group law from ec.lisp
; supports the standard public-key protocols.  ECDH: each party multiplies G by a private
; scalar to publish a point, and multiplies the other's point by its own scalar; both reach
; (d_A d_B) G, the shared secret.  ECDSA: to sign an integer z under private key d (public
; Q = dG) one picks a nonce k, sets R = kG, r = x(R) mod ell, and s = k^{-1}(z + r d) mod
; ell; verification recomputes x(u1 G + u2 Q) with u1 = z s^{-1}, u2 = r s^{-1} and checks
; it equals r.
;
; Four facts certify the implementation: ECDH agreement (d_A(d_B G) = d_B(d_A G)), valid
; ECDSA signatures verify, a signature fails to verify against a different message, and a
; tampered signature fails to verify.  The demo curve is y^2 = x^3 + 2x + 2 over F_17 with
; G = (5,1) of prime order 19.  Builds on ec.lisp.

(import "cas/ec.lisp")

; ---------- ECDH ----------
(define (ec-pub d G a p) (ec-mul d G a p))                  ; public point d*G
(define (ecdh-shared d P a p) (ec-mul d P a p))             ; shared secret d*(other public)
(define (ecdh-agrees? da db G a p)
  (equal? (ecdh-shared da (ec-pub db G a p) a p) (ecdh-shared db (ec-pub da G a p) a p)))

; ---------- ECDSA ----------
(define (ecdsa-sign z d k G a p ell)
  (let ((R (ec-mul k G a p)))
    (let ((r (imod (car R) ell)))
      (cons r (imod (* (mod-inverse k ell) (+ z (* r d))) ell)))))
(define (ecdsa-verify z sig Q G a p ell)
  (let ((r (car sig)) (s (cdr sig)))
    (if (or (= (imod r ell) 0) (= (imod s ell) 0)) #f
      (let ((w (mod-inverse s ell)))
        (let ((X (ec-add (ec-mul (imod (* z w) ell) G a p) (ec-mul (imod (* r w) ell) Q a p) a p)))
          (and (not (equal? X 'O)) (= (imod (car X) ell) r)))))))

; ---------- certificates ----------
(define (ecdsa-ok? z d k G a p ell)
  (ecdsa-verify z (ecdsa-sign z d k G a p ell) (ec-pub d G a p) G a p ell))
(define (ecdsa-rejects-msg? z z2 d k G a p ell)
  (and (not (= (imod z ell) (imod z2 ell)))
       (not (ecdsa-verify z2 (ecdsa-sign z d k G a p ell) (ec-pub d G a p) G a p ell))))
(define (ecdsa-rejects-sig? z d k G a p ell)
  (let ((sig (ecdsa-sign z d k G a p ell)))
    (not (ecdsa-verify z (cons (car sig) (imod (+ (cdr sig) 1) ell)) (ec-pub d G a p) G a p ell))))

; ---------- display ----------
(define (sig->string sig) (string-append "(r=" (number->string (car sig)) ", s=" (number->string (cdr sig)) ")"))
