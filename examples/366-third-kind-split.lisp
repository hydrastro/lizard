; SPLIT the norm N = A^2 - B^2 q to recover the third-kind element g = A + B*y on y^2 = q, completing for the
; constant-B case the construction whose first step was elliptic3norm (docs/CAS.md -- summit S1).
;
; The norm equation in full generality is the Pell-type / Jacobian-torsion problem and stays open.  The decidable
; slice B = c constant reduces to: is N + c^2 q a perfect square polynomial?  If so, A is its exact polynomial
; square root and g = A + c*y; otherwise 'no-split.  The recovered g is certified by recomputing A^2 - B^2 q = N.
(import "cas/elliptic3split.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Recovering g = A + B*y from its norm, for constant B, by exact polynomial square roots.") (newline) (newline)

(display "polynomial square roots are decided exactly:") (newline)
(chk "sqrt(x^2) = x" (equal? (esp-trim (esp-poly-sqrt (list 0 0 1))) (list 0 1)))
(chk "sqrt((x+1)^2) = x + 1" (equal? (esp-poly-sqrt (list 1 2 1)) (list 1 1)))
(chk "sqrt((x^2+1)^2) = x^2 + 1" (equal? (esp-poly-sqrt (list 1 0 2 0 1)) (list 1 0 1)))
(chk "x^2 + 1 is not a perfect square" (if (esp-is-square? (list 1 0 1)) #f #t))
(chk "an odd-degree polynomial is not a square" (if (esp-is-square? (list 0 0 0 1)) #f #t))

(define q (list 1 0 1))   ; q = x^2 + 1

(display "g = x + y on y^2 = x^2+1 has norm N = x^2 - (x^2+1) = -1; from N and q we recover A = x:") (newline)
(define N (list -1))
(display "  split: ") (display (esp-split-const N q 1)) (newline)
(chk "A = x is recovered for B = 1" (equal? (esp-trim (car (esp-split-const N q 1))) (list 0 1)))
(chk "g = x + y is certified: A^2 - B^2 q = N" (esp-verify N q (list 0 1) 1))
(chk "recover-g returns the certified g" (equal? (esp-recover-g N q 1) (list (quote g) (list 0 1) 1)))

(display "g = x^2 + y has norm N = x^4 - x^2 - 1; the construction recovers A = x^2:") (newline)
(define N2 (list -1 0 -1 0 1))
(chk "g = x^2 + y is recovered and certified" (esp-verify N2 q (list 0 0 1) 1))

(display "soundness: N = 1 gives no split, since x^2 + 2 is not a perfect square:") (newline)
(chk "no split is reported, not a fabricated A" (equal? (esp-recover-g (list 1) q 1) (list (quote no-split))))
(chk "and a wrong A = x + 1 fails the certificate" (if (esp-verify N q (list 1 1) 1) #f #t))

(newline)
(display "For constant B the norm now splits back into the actual g = A + B*y by an exact polynomial square root,") (newline)
(display "certified by recomputing the norm -- so with the residue reconstruction of N, the third-kind element is") (newline)
(display "built end to end in this case.  The general nonconstant-B split (Pell / Jacobian torsion) remains open.") (newline)
