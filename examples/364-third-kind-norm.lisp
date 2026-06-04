; CONSTRUCT the norm N of a third-kind logarithm from its residue data: the first constructive step of building
; g = A + B*sqrt(q) from omega = a + b*y, recovering N = A^2 - B^2 q from the integer residues of 2a and verifying
; it reproduces a (docs/CAS.md -- summit S1, turning the residue decision into a partial construction).
;
; Since a = (1/2) N'/N, the function 2a = N'/N has an integer residue at each pole equal to the order of N there,
; so from the (pole, multiplicity) data the monic norm is N = prod (x - p_i)^{m_i}.  The construction is verified
; by checking (1/2) N'/N equals a exactly as rational functions.
(import "cas/elliptic3norm.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Reconstructing the norm N of a third-kind logarithm from the residues of its rational part.") (newline) (newline)

(display "residues {(1,1),(2,2)} reconstruct N = (x-1)(x-2)^2 = x^3 - 5x^2 + 8x - 4:") (newline)
(define res1 (list (cons 1 1) (cons 2 2)))
(display "  N = ") (display (e3n-build-norm res1)) (newline)
(chk "the reconstructed norm is x^3 - 5x^2 + 8x - 4" (equal? (e3n-build-norm res1) (list -4 8 -5 1)))
(chk "its total degree equals the sum of the multiplicities, 3" (= (e3n-degree-from-residues res1) 3))
(chk "and (1/2) N'/N reproduces the rational part a" (e3n-verifies? res1 (e3n-logder-half (e3n-build-norm res1))))

(display "residues {(0,1)} reconstruct N = x, with (1/2) N'/N = (1/2)/x:") (newline)
(define res2 (list (cons 0 1)))
(chk "the reconstructed norm is x" (equal? (e3n-build-norm res2) (list 0 1)))
(chk "it verifies against its rational part" (e3n-verifies? res2 (e3n-logder-half (e3n-build-norm res2))))

(display "residues {(1,1),(-1,1)} reconstruct N = x^2 - 1 (the norm of (x^2+1) + x*y-type data):") (newline)
(chk "the reconstructed norm is x^2 - 1" (equal? (e3n-build-norm (list (cons 1 1) (cons -1 1))) (list -1 0 1)))

(display "soundness: a rational part from a DIFFERENT norm does not verify:") (newline)
(chk "the a of N=(x-1)(x-2)^2 is rejected for the residues {(0,1)}" (if (e3n-verifies? res2 (e3n-logder-half (e3n-build-norm res1))) #f #t))

(newline)
(display "The norm of a third-kind logarithm is now reconstructed from its residue data and verified to reproduce") (newline)
(display "the rational part exactly -- the first constructive step toward building g.  Splitting N = A^2 - B^2 q into") (newline)
(display "the actual A and B, the Jacobian-torsion step, remains the open summit.") (newline)
