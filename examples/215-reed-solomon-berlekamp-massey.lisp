; 215-reed-solomon-berlekamp-massey.lisp -- spectral RS decoding via Berlekamp-Massey.
;
; Complementary to the Berlekamp-Welch decoder of example 208.  Here a primitive element
; alpha of F_p generates all nonzero residues (n = p-1); the generator g(x) =
; prod_{j=1}^{2t}(x-alpha^j) gives an [n,k] code (k=n-2t) whose clean codewords have zero
; spectrum at alpha^1..alpha^{2t}.  The syndromes of a corrupted word obey a linear
; recurrence; Berlekamp-Massey synthesizes the shortest LFSR -- the error locator -- a Chien
; search finds the error positions, and a Vandermonde solve gives the magnitudes.  Certified
; by the round trip and by the locator's roots matching the injected error positions.

(import "cas/rsbm.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'rsbm-check-failed)))
(define p 11) (define a (primitive-root 11)) (define t 2)
(define msg (list 3 1 4 1 5 9))
(define cw (rsbm-encode msg a t p))

(display "Reed-Solomon decoding by syndromes + Berlekamp-Massey") (newline) (newline)

(display "1. a ") (display (rsbm-info 11 2)) (newline)
(display "    primitive element alpha = ") (display a) (newline)
(display "    generator g(x) = ") (display (gen-poly a t p)) (display "  (degree 2t = 4)") (newline)
(display "    message  = ") (display msg) (newline)
(display "    codeword = ") (display cw) (newline)
(must "clean codeword has zero spectrum at alpha^1..alpha^2t" (rsbm-spectrum-zero? msg a t p))
(must "clean decode returns the message" (rsbm-clean-ok? msg a t p))
(newline)

(display "2. Berlekamp-Massey error correction (t = 2)") (newline)
(display "    1 error  @pos 3:    decode = ") (display (rsbm-decode (rsbm-corrupt cw (list (cons 3 7)) p) a t p)) (newline)
(display "    2 errors @pos 1,6:  decode = ") (display (rsbm-decode (rsbm-corrupt cw (list (cons 1 5) (cons 6 8)) p) a t p)) (newline)
(must "1 error corrected"  (rsbm-roundtrip-ok? msg a t p (list (cons 3 7))))
(must "2 errors corrected" (rsbm-roundtrip-ok? msg a t p (list (cons 1 5) (cons 6 8))))
(must "locator roots are exactly the error positions" (rsbm-locator-ok? msg a t p (list (cons 1 5) (cons 6 8))))
(must "2 errors elsewhere corrected" (rsbm-roundtrip-ok? msg a t p (list (cons 0 3) (cons 9 10))))
(newline)

(display "3. beyond capacity, and a larger code") (newline)
(display "    3 errors (one too many): decode = ") (display (rsbm-decode (rsbm-corrupt cw (list (cons 0 1) (cons 4 2) (cons 9 3)) p) a t p)) (newline)
(must "3 errors cannot be corrected (decode reports fail)"
      (equal? (rsbm-decode (rsbm-corrupt cw (list (cons 0 1) (cons 4 2) (cons 9 3)) p) a t p) 'fail))
(must "[12,6] code over F_13 corrects 3 errors"
      (rsbm-roundtrip-ok? (list 1 2 3 4 5 6) (primitive-root 13) 3 13 (list (cons 1 5) (cons 6 9) (cons 11 12))))
(newline)

(display "all Berlekamp-Massey RS checks passed.") (newline)
