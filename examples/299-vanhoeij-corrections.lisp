; VAN HOEIJ CORRECTION TERMS for the integral basis of a GENERAL plane curve F(x,y) = 0 -- the last Rung-4 gap
; (docs/TRAGER_ROADMAP.md).  For a superelliptic curve y^n = g the integral basis is the pure-power form
; {y^j / d_j} (intbasis.lisp); for a general curve a basis element is w_j = (y^j + sum_{i<j} c_{j,i}(x) y^i)/d_j,
; with LOWER-degree-in-y "correction terms" c_{j,i} that cancel the poles the naive y^j/d_j would have.
;
; At a rational place x = a (Puiseux ramification q = 1) the branch is an ordinary power series y(x); the element
; (y - c(x))/(x-a)^k is integral there iff y - c vanishes to order >= k, so the correction is c(x) = the part of
; the branch BELOW order k ("subtract the singular part").  Integrality is certified by the general-F Puiseux
; valuation oracle.  For a ramified place (q > 1) or several branches needing a combined correction, the simple
; single-branch construction does not suffice and the verdict is an honest 'needs-place-combination.
(import "cas/vanhoeij.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Van Hoeij correction terms: making (y^j + corrections)/x^k integral on a general curve F(x,y) = 0.") (newline) (newline)

(display "a smooth branch y = x + x^2 + x^3 (the curve F = y - x - x^2 - x^3):") (newline)
(define F (list (list 0 -1 -1 -1) (list 1)))
(define br (car (vh-branches F 6)))
(display "  the correction below order 2 is c(x) = ") (display (vh-correction br 2)) (display "  = x (the part of the branch below x^2)") (newline)
(display "  the correction below order 3 is c(x) = ") (display (vh-correction br 3)) (display "  = x + x^2") (newline)
(chk "correction below order 2 is x, below order 3 is x + x^2" (if (equal? (vh-correction br 2) (list 0 1)) (equal? (vh-correction br 3) (list 0 1 1)) #f))

(display "the level-1 corrected basis elements:") (newline)
(define r2 (vh-correct-level1 F 2 6))
(display "  (y - x)/x^2 is integral  ->  ") (display r2) (newline)
(chk "(y - x)/x^2 is the corrected element at k = 2" (equal? r2 (list (quote corrected) (list 0 1) 2)))
(define r3 (vh-correct-level1 F 3 6))
(display "  (y - x - x^2)/x^3 is integral  ->  ") (display r3) (newline)
(chk "(y - x - x^2)/x^3 is the corrected element at k = 3" (equal? r3 (list (quote corrected) (list 0 1 1) 3)))

(display "the correction genuinely matters -- certified that the corrected element is integral but the naive one is not:") (newline)
(chk "(y - x)/x^2 is integral while y/x^2 has a pole" (vh-certify-correction F (list (list 0 -1) (list 1)) 2 6))

(display "when no correction is needed (y/x already integral), that is reported:") (newline)
(display "  level-1 at k = 1  ->  ") (display (vh-correct-level1 F 1 6)) (newline)
(chk "k = 1 needs no correction (y/x is already integral)" (equal? (vh-correct-level1 F 1 6) (quote no-correction-needed)))

(display "soundness -- a ramified place is not forced into a single-branch correction:") (newline)
(define Fc (list (list 0 0 0 -1) (list) (list 1)))   ; the cusp y^2 = x^3 (q = 2)
(display "  the cusp y^2 = x^3 at k = 2  ->  ") (display (vh-correct-level1 Fc 2 6)) (newline)
(chk "the ramified cusp reports needs-place-combination, not a guessed correction" (equal? (vh-correct-level1 Fc 2 6) (quote needs-place-combination)))

(newline)
(display "Van Hoeij corrections: on a general curve, the singular part of a branch is subtracted to raise the") (newline)
(display "valuation and cancel the pole, every corrected element certified integral -- the last Rung-4 piece.") (newline)
