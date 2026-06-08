; -*- lisp -*-
; Univalence on the interaction net (companion to sangaku's Floor 5).  This example lives in lizard because it
; exercises lizard's SURFACE cubical layer (lib/cubical.lisp + lib/univalence.lisp), which ships with lizard and is
; natively on the path here.  It demonstrates the genuinely-cubical computation that the sangaku Floor-5 module
; carries and delegates: an equivalence is a path (univalence), and transport across the identity equivalence is the
; identity.
;
; Trust note (the same one sangaku's Floor 5 states): Glue / ua / Equiv live in the surface cubical layer, NOT in
; lizard's audited ~1,350-line trusted kernel (which has Path and Interval but not Glue/ua).  So the facts below are
; computed by the surface cubical evaluator -- a real, shipping evaluator, but a larger, less-audited trust base than
; kt_infer.  The Path/refl fragment that DOES live in the trusted kernel is verified kernel-side in sangaku's Floor 5.
(import "cubical.lisp")
(import "univalence.lisp")
(define (must label x) (display "  ") (display label) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote univalence-regression))))

(display "Univalence on the net (surface cubical layer): equivalence is identity, computed.") (newline) (newline)

; the identity equivalence computes to the identity function, forward and backward (transport across it is trivial)
(must "id-equiv forward on x computes to x" (equal? (equiv-id-fwd (quote A) (quote x)) (quote x)))
(must "id-equiv backward on y computes to y" (equal? (equiv-id-bwd (quote A) (quote y)) (quote y)))

; Glue interpolates: equal to T on a true face, to A on a false face
(must "Glue on a true face reduces to T" (equal? (glue-on-true (quote A) (quote T) (quote e)) (quote T)))
(must "Glue on a false face reduces to A" (equal? (glue-on-false (quote A) (quote T) (quote e)) (quote A)))

; ua turns an equivalence into a path in the universe (univalence)
(must "ua (id-equiv A) computes to a well-formed univalence term (not an error)" (not (error-object? (ua-id (quote A)))))

(newline)
(display "The identity equivalence transports trivially, Glue interpolates between a base and an equivalent type, and ua") (newline)
(display "turns an equivalence into a path -- univalence as computation, in lizard's surface cubical layer.  The") (newline)
(display "kernel-anchored Path/refl fragment of the same floor is verified against the trusted kernel in sangaku.") (newline)
