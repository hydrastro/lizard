; 207-secret-sharing.lisp -- Shamir (t, n) threshold secret sharing over F_p.
;
; A secret is the constant term of a degree t-1 polynomial; the n shares are its values at
; x = 1..n.  Any t shares recover the secret by Lagrange interpolation at 0; any t-1 shares
; reveal nothing, since every candidate secret is consistent with them.  Two certificates:
; reconstruction from any t-subset returns the secret, and the security property holds
; (t-1 shares are consistent with two distinct secrets).  `must` raises on failure.

(import "cas/shamir.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'shamir-check-failed)))
(define p 2003)
(define shares (make-shares 1234 (list 166 94) 5 p))      ; (3,5) sharing of 1234

(display "Shamir threshold secret sharing over F_p") (newline) (newline)

(display "1. a (3,5) sharing of the secret 1234 over F_2003") (newline)
(display "    shares: ") (display (shares->string shares)) (newline)
(must "shares 1,2,3 reconstruct 1234"   (= (reconstruct (take-n shares 3) p) 1234))
(must "shares 3,4,5 reconstruct 1234"   (= (reconstruct (drop-n shares 2) p) 1234))
(must "any three of the five suffice"   (reconstruct-ok? 1234 (list 166 94) 5 p))
(newline)

(display "2. fewer than the threshold reveals nothing") (newline)
(must "only 2 shares do NOT give the secret"  (not (= (reconstruct (take-n shares 2) p) 1234)))
(must "2 shares are consistent with both 1234 and 999" (security-ok? 1234 (list 166 94) 5 p 1234 999))
(must "2 shares are consistent with both 1234 and 7"   (security-ok? 1234 (list 166 94) 5 p 1234 7))
(newline)

(display "3. larger parameters") (newline)
(must "(4,7) sharing of 777777 over F_1000003 reconstructs"  (reconstruct-ok? 777777 (list 11 222 3333) 7 1000003))
(must "(4,7) security: t-1 shares consistent with two secrets" (security-ok? 777777 (list 11 222 3333) 7 1000003 777777 12345))
(must "(2,4) sharing of 42 over F_101 reconstructs"          (reconstruct-ok? 42 (list 17) 4 101))
(newline)

(display "all secret-sharing checks passed.") (newline)
