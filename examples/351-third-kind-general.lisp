; The GENERAL third-kind logarithmic part on y^2 = q(x): work with g = A + B*y where BOTH A and B are rational
; functions (B nonconstant allowed), beyond elliptic3complete's g = u + sqrt(q) (B = 1) (docs/CAS.md -- summit S1).
;
; For g = A + B y, the norm is N = g*conj(g) = A^2 - B^2 q, and the logarithmic derivative omega = g'/g obeys the
; exact identity omega + conj(omega) = N'/N, so the rational part of omega equals (1/2) N'/N.  This module computes
; the norm, certifies the general recognizer omega = g'/g (exact in K for nonconstant B), and checks the norm
; identity -- the sound, exact core of the general third-kind decision.  Constructing g from omega in full
; generality (the coupled norm/y-component system) is the Jacobian-torsion part of Trager's algorithm and is left
; honestly open; nothing here returns a guessed g.
(import "cas/elliptic3general.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "General third kind on y^2 = q: g = A + B*y with B nonconstant -- norm, recognition, the norm identity.") (newline) (newline)

(define q (rat-from-poly (list 1 0 1)))   ; y^2 = x^2 + 1

(display "g = (x^2+1) + x*y on y^2 = x^2+1: its norm A^2 - B^2 q collapses to x^2+1:") (newline)
(define g1 (af-make (rat-from-poly (list 1 0 1)) (rat-from-poly (list 0 1))))
(chk "norm of (x^2+1)+x*y is x^2+1" (rat-equal? (e3g-norm q g1) (rat-from-poly (list 1 0 1))))
(chk "the recognizer certifies omega = g'/g for g1" (e3g-recognize q g1 (e3g-logderiv q g1)))
(chk "the norm identity: rational part of omega equals (1/2) N'/N" (e3g-norm-relation? q g1))
(chk "integrate-known returns log(g1) once recognized" (equal? (car (e3g-integrate-known q g1)) (quote elementary-log)))

(display "g = x + x*y on the same curve -- a genuinely NONCONSTANT B = x:") (newline)
(define g2 (af-make (rat-from-poly (list 0 1)) (rat-from-poly (list 0 1))))
(chk "the recognizer certifies omega = g'/g for g2 (nonconstant B)" (e3g-recognize q g2 (e3g-logderiv q g2)))
(chk "the norm identity holds for g2 as well" (e3g-norm-relation? q g2))

(display "soundness: a wrong omega is rejected -- recognition never accepts a non-derivative:") (newline)
(define wrong (af-make (rat-from-poly (list 1)) (rat-zero)))
(chk "omega = 1 is NOT the log-derivative of g1" (if (e3g-recognize q g1 wrong) #f #t))

(newline)
(display "The third kind now covers g = A + B*sqrt(q) with B nonconstant: the norm A^2 - B^2 q, the exact identity") (newline)
(display "a = (1/2) N'/N that any such omega must satisfy, and certified recognition in the function field.  The") (newline)
(display "full construction of g from omega -- the coupled system, a Jacobian-torsion question -- remains the summit.") (newline)
