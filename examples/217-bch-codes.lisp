; 217-bch-codes.lisp -- binary BCH codes over GF(2^m), the cyclic generalization of Hamming.
;
; Fix a primitive alpha of GF(2^m).  The length-n = 2^m-1 code designed for t errors has
; generator g = lcm(M_1,...,M_{2t}), where M_i is the minimal polynomial of alpha^i over
; GF(2) -- the product over the cyclotomic coset {i,2i,4i,...} (mod n) of (x - alpha^j).  A
; k = n - deg g bit message is encoded as m*g over GF(2).  Decoding: syndromes S_j = r(alpha^j)
; in GF(2^m), Berlekamp-Massey over GF(2^m) for the error locator, a Chien search for the
; positions, and -- errors being binary -- a bit flip with no Forney step.  Certified against
; the textbook generator and by the round trip.  `must` raises on failure.

(import "cas/bch.lisp")
(define (must label x)
  (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'bch-check-failed)))
(define p 2) (define m 4) (define mod (gf-modulus p m)) (define n 15) (define alpha (gf-elt 2 p m))

(display "binary BCH codes over GF(2^m)") (newline) (newline)

(display "1. the [15,7] code over GF(16), primitive polynomial ") (display mod) (newline)
(define g (bch-generator alpha 2 mod p n))
(display "    generator g(x) coeffs (low->high) = ") (display g) (newline)
(display "    this is the textbook x^8+x^7+x^6+x^4+1") (newline)
(must "generator matches the textbook [15,7] BCH generator" (equal? (trim g) (list 1 0 0 0 1 0 1 1 1)))
(must "deg g = 8, so k = 7" (and (= (bch-deg g) 8) (= (bch-k n g) 7)))
(newline)

(define msg (list 1 0 1 1 0 0 1))
(define cw (bch-encode msg g n))
(display "2. encode / correct up to t = 2 errors") (newline)
(display "    message  = ") (display msg) (newline)
(display "    codeword = ") (display cw) (newline)
(must "a codeword has all-zero syndromes" (bch-all-zero? (bch-syndromes cw alpha 2 mod p)))
(must "clean decode returns the message" (bch-clean-ok? msg alpha 2 mod p n g))
(must "1 error corrected"  (bch-roundtrip-ok? msg alpha 2 mod p n g (list 5)))
(must "2 errors corrected" (bch-roundtrip-ok? msg alpha 2 mod p n g (list 2 11)))
(newline)

(display "3. other BCH codes: a t=3 [15,5] and the t=1 [7,4] Hamming code over GF(8)") (newline)
(define g3 (bch-generator alpha 3 mod p n))
(must "t=3 design gives a [15,5] code" (and (= (bch-deg g3) 10) (= (bch-k n g3) 5)))
(must "[15,5] corrects 3 errors" (bch-roundtrip-ok? (list 1 0 1 1 0) alpha 3 mod p n g3 (list 0 7 14)))
(define mod3 (gf-modulus 2 3)) (define a3 (gf-elt 2 2 3)) (define g7 (bch-generator a3 1 mod3 2 7))
(display "    [7,4] generator over GF(8) = ") (display (trim g7)) (display " (= x^3+x+1)") (newline)
(must "[7,4] Hamming generator is x^3+x+1" (equal? (trim g7) (list 1 1 0 1)))
(must "[7,4] corrects a single error" (bch-roundtrip-ok? (list 1 0 1 1) a3 1 mod3 2 7 g7 (list 3)))
(newline)

(display "all BCH checks passed.") (newline)
