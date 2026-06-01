; 209-elliptic-curve-crypto.lisp -- ECDH key exchange and ECDSA signatures.
;
; Using the group law from ec.lisp on a curve whose base point G has prime order ell, this
; runs the two classic elliptic-curve protocols.  ECDH: each side multiplies G by a secret
; scalar to publish a point and multiplies the other's point by its own scalar, both
; arriving at the shared secret (d_A d_B) G.  ECDSA: sign an integer z with key d (public
; Q = dG) via R = kG, r = x(R) mod ell, s = k^{-1}(z + r d) mod ell; verify by recomputing
; x(u1 G + u2 Q).  Certified by ECDH agreement, signature verification, and rejection of
; tampered messages and signatures.  `must` raises on failure.

(import "cas/eccrypto.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'eccrypto-check-failed)))
(define G (cons 5 1)) (define a 2) (define p 17) (define ell 19)
(define (dh-all da db) (if (> da 18) #t (if (> db 18) (dh-all (+ da 1) 1) (and (ecdh-agrees? da db G a p) (dh-all da (+ db 1))))))

(display "Elliptic-curve cryptography on y^2 = x^3 + 2x + 2 over F_17") (newline)
(display "base point G = (5,1) of prime order 19") (newline) (newline)

(display "1. ECDH key exchange") (newline)
(display "    Alice d=7 publishes ") (display (pt->string (ec-pub 7 G a p)))
(display ", Bob d=11 publishes ") (display (pt->string (ec-pub 11 G a p))) (newline)
(display "    shared secret 7*(11G) = ") (display (pt->string (ecdh-shared 7 (ec-pub 11 G a p) a p))) (newline)
(must "Alice and Bob agree (d=7, d=11)" (ecdh-agrees? 7 11 G a p))
(must "ECDH agreement holds for ALL 18x18 key pairs" (dh-all 1 1))
(newline)

(display "2. ECDSA signatures") (newline)
(display "    sign z=9 with d=13, k=5 gives ") (display (sig->string (ecdsa-sign 9 13 5 G a p ell))) (newline)
(must "a valid signature verifies (z=9,d=13,k=5)"   (ecdsa-ok? 9 13 5 G a p ell))
(must "valid signature (z=2,d=8,k=15)"              (ecdsa-ok? 2 8 15 G a p ell))
(must "valid signature (z=11,d=17,k=6)"             (ecdsa-ok? 11 17 6 G a p ell))
(must "valid signature (z=18,d=2,k=9)"              (ecdsa-ok? 18 2 9 G a p ell))
(newline)

(display "3. forgery and tampering are rejected") (newline)
(must "signature for z=9 does NOT verify for z=10"  (ecdsa-rejects-msg? 9 10 13 5 G a p ell))
(must "signature for z=2 does NOT verify for z=3"   (ecdsa-rejects-msg? 2 3 8 15 G a p ell))
(must "a tampered signature is rejected (z=9)"      (ecdsa-rejects-sig? 9 13 5 G a p ell))
(must "a tampered signature is rejected (z=11)"     (ecdsa-rejects-sig? 11 17 6 G a p ell))
(newline)

(display "all elliptic-curve crypto checks passed.") (newline)
