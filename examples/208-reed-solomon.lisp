; 208-reed-solomon.lisp -- Reed-Solomon error-correcting codes over F_p.
;
; A k-symbol message is the coefficients of a polynomial of degree < k; its codeword is the
; evaluations at n distinct points.  This [n,k] code has minimum distance n-k+1 and corrects
; t = floor((n-k)/2) symbol errors.  Decoding is Berlekamp-Welch: the error-locator E and a
; numerator N with N(x_i) = r_i E(x_i) are found by solving a linear system over F_p, and the
; message is the exact quotient N/E.  The certificate is the round trip -- corrupt a codeword
; in up to t places and decoding returns the original message.  `must` raises on failure.

(import "cas/reedsolomon.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'rs-check-failed)))
(define p 11) (define xs (rs-points 10)) (define k 4)
(define msg (list 3 1 4 1))
(define cw (rs-encode msg xs p))

(display "Reed-Solomon error correction") (newline) (newline)

(display "1. a ") (display (rs-info 10 4 11)) (newline)
(display "    message  = ") (display msg) (newline)
(display "    codeword = ") (display cw) (newline)
(must "minimum distance is n-k+1 = 7"  (= (rs-distance 10 4) 7))
(must "corrects t = 3 errors"          (= (rs-t 10 4) 3))
(must "clean decode returns the message" (rs-clean-ok? msg xs k p))
(newline)

(display "2. correcting up to t = 3 errors") (newline)
(display "    1 error  @pos 2:    decode = ") (display (rs-decode (rs-corrupt cw (list (cons 2 5)) p) xs k p)) (newline)
(display "    2 errors @pos 0,5:  decode = ") (display (rs-decode (rs-corrupt cw (list (cons 0 7) (cons 5 3)) p) xs k p)) (newline)
(display "    3 errors @pos 1,4,8: decode = ") (display (rs-decode (rs-corrupt cw (list (cons 1 2) (cons 4 9) (cons 8 6)) p) xs k p)) (newline)
(must "1 error corrected"  (rs-roundtrip-ok? msg xs k p (list (cons 2 5))))
(must "2 errors corrected" (rs-roundtrip-ok? msg xs k p (list (cons 0 7) (cons 5 3))))
(must "3 errors corrected" (rs-roundtrip-ok? msg xs k p (list (cons 1 2) (cons 4 9) (cons 8 6))))
(must "3 errors elsewhere corrected" (rs-roundtrip-ok? msg xs k p (list (cons 6 4) (cons 7 8) (cons 9 1))))
(newline)

(display "3. beyond capacity, and other codes") (newline)
(display "    4 errors (one too many): decode = ") (display (rs-decode (rs-corrupt cw (list (cons 0 1) (cons 1 1) (cons 2 1) (cons 3 1)) p) xs k p)) (newline)
(must "4 errors cannot be corrected (decode reports fail)" (equal? (rs-decode (rs-corrupt cw (list (cons 0 1) (cons 1 1) (cons 2 1) (cons 3 1)) p) xs k p) 'fail))
(must "a second message decodes through 3 errors" (rs-roundtrip-ok? (list 10 0 5 9) xs k p (list (cons 1 3) (cons 4 7) (cons 8 2))))
(must "[8,2] code over F_29 corrects 3 errors" (rs-roundtrip-ok? (list 17 23) (rs-points 8) 2 29 (list (cons 1 11) (cons 3 22) (cons 7 5))))
(newline)

(display "all Reed-Solomon checks passed.") (newline)
