; RADICAL ideal membership -- the Nullstellensatz / variety-consequence test -- by the Rabinowitsch trick, on the
; Groebner solver (docs/CAS.md -- frontier 4, multivariate): decide whether a polynomial vanishes on the entire
; VARIETY of a system, which is strictly stronger than ordinary ideal membership.
;
; By Hilbert's Nullstellensatz, p vanishes at every common zero of I iff p in sqrt(I), and the Rabinowitsch
; reduction makes this decidable: p in sqrt(I) iff 1 in <I, 1 - t*p> for a fresh variable t.  We lift the system,
; adjoin 1 - t*p, and test inconsistency via the Groebner consistency check -- the verdict carries the same
; Groebner certificate.  The classic separation: x is in sqrt(<x^2>) (x = 0 wherever x^2 = 0) yet x is NOT in
; <x^2> as an ideal.
(import "cas/radmember.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Radical membership: does p vanish on the whole variety of the system? (stronger than ideal membership.)") (newline) (newline)

(define x (list (cons 1 (list 1))))
(define x2 (list (cons 1 (list 2))))
(define x3 (list (cons 1 (list 3))))

(display "the classic separation -- x vanishes wherever x^2 (or x^3) does, but is not an ideal multiple of x^2:") (newline)
(chk "x is in radical<x^2> (Nullstellensatz)" (rad-member? x (list x2) 1))
(chk "x is in radical<x^3>" (rad-member? x (list x3) 1))
(chk "yet x is NOT in <x^2> as an ideal -- radical is strictly larger" (if (in-ideal? x (groebner (list x2))) #f #t))

(display "shifted roots: x - 1 vanishes wherever (x-1)^2 does; x does not (it would need a root at 0):") (newline)
(define xm1 (list (cons 1 (list 1)) (cons -1 (list 0))))
(define xm1sq (list (cons 1 (list 2)) (cons -2 (list 1)) (cons 1 (list 0))))   ; (x-1)^2
(chk "(x-1) is in radical<(x-1)^2>" (rad-member? xm1 (list xm1sq) 1))
(chk "x is NOT in radical<(x-1)^2> (it does not vanish at x=1)" (if (rad-member? x (list xm1sq) 1) #f #t))

(display "a two-variable system: the variety of <x^2, y> is the single point (0,0), where x vanishes:") (newline)
(define x2b (list (cons 1 (list 2 0))))
(define y (list (cons 1 (list 0 1))))
(chk "x is in radical<x^2, y>" (rad-member? x2b (list x2b y) 2))

(newline)
(display "Radical membership decides the geometric question -- vanishing on every common zero -- via Rabinowitsch") (newline)
(display "and the Groebner consistency certificate, separating it from ideal membership.  The reduction adjoins a") (newline)
(display "variable, so its reach is bounded by the Groebner engine's capacity on the resulting system.") (newline)
