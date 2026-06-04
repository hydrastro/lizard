; EXACT NAMING OF COMPLEX ROOTS of a real polynomial: each non-real root is named by the rational data of the
; irreducible real quadratic it satisfies, completing polysolve3's real-root naming so the full root census is
; exact (docs/CAS.md -- summit S3, naming complex solutions).
;
; Non-real roots come in conjugate pairs, the roots of an irreducible real quadratic x^2 + p x + q, so a complex
; root is named (complex re im2) with re = -p/2 and im2 = (4q - p^2)/4 -- the actual roots being re +- sqrt(im2) i.
; Both are rational; the imaginary part is carried as its square, so no surd or floating value is formed.  Each
; named pair is verified by reconstructing its quadratic and checking it divides the polynomial.
(import "cas/cplxroots.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Naming complex roots exactly: (complex re im2) means the conjugate pair re +- sqrt(im2) i.") (newline) (newline)

(display "x^2 + 1 has the conjugate roots i and -i, named (complex 0 1):") (newline)
(chk "x^2 + 1 names (complex 0 1)" (equal? (cx-name-complex (list 1 0 1)) (list (list (quote complex) 0 1))))
(chk "the reconstructed quadratic is x^2 + 1" (equal? (cx-quadratic-of (list (quote complex) 0 1)) (list 1 0 1)))
(chk "and it divides x^2 + 1" (cx-divides-quadratic? (list 1 0 1) (list (quote complex) 0 1)))
(chk "x^2 + 1 has two complex roots" (= (cx-num-complex (list 1 0 1)) 2))

(display "x^2 + x + 1 has roots -1/2 +- (sqrt(3)/2) i, named (complex -1/2 3/4):") (newline)
(chk "x^2 + x + 1 names (complex -1/2 3/4)" (equal? (cx-name-complex (list 1 1 1)) (list (list (quote complex) (/ -1 2) (/ 3 4)))))
(chk "its quadratic divides x^2 + x + 1" (cx-divides-quadratic? (list 1 1 1) (list (quote complex) (/ -1 2) (/ 3 4))))

(display "for (x-1)(x^2+1): two complex roots, named after the real root x = 1 is removed:") (newline)
(define cubic (list -1 1 -1 1))
(chk "the cubic reports two complex roots" (= (cx-num-complex cubic) 2))
(chk "deflating the rational root leaves x^2 + 1, named (complex 0 1)" (equal? (cx-name-complex (ps3-deflate cubic (ps3-rational-roots cubic))) (list (list (quote complex) 0 1))))

(display "soundness: x^2 - 1 has positive discriminant, so it has no complex roots:") (newline)
(chk "x^2 - 1 names no complex roots" (null? (cx-name-complex (list -1 0 1))))
(chk "and reports zero complex roots" (= (cx-num-complex (list -1 0 1)) 0))

(display "a degree-four census: x^4 - 1 = (x^2-1)(x^2+1) has two real and two complex roots:") (newline)
(chk "x^4 - 1 has two complex roots" (= (cx-num-complex (list -1 0 0 0 1)) 2))

(newline)
(display "Complex roots are now named exactly by rational data -- real part and imaginary-part-squared -- each") (newline)
(display "certified by the dividing quadratic, completing the real-and-complex root census over the rationals.") (newline)
(display "Naming complex roots inside higher-degree irreducible factors needs full factorization, still ahead.") (newline)
