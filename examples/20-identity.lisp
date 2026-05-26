; -*- lisp -*-
; ============================================================
;  Identity manipulation and equivalence — notation playground
; ============================================================
;
; *** READ THIS FIRST ***
;
; This file sketches the *surface* of identity-type reasoning in the
; style of HoTT and HOTT (Higher Observational Type Theory). NONE of
; the manipulations below verify anything. (Id-sym p) does not check
; that p is an identity proof; (Id-trans p q) does not check that
; p's endpoint matches q's startpoint; (transport p x) does not
; check that p relates two types one of which contains x;
; (equivalence A B f g) does not check that f and g are inverse.
;
; What lizard provides is a vocabulary — a set of opaque carriers
; you can construct, print, and pattern-match on. Making any of this
; ACTUALLY MEAN something — i.e., reducing (Id-sym (Id-sym p)) to p,
; or (Id-trans p (refl _)) to p — requires a separate type checker
; with normalization rules. That checker is not part of lizard.
;
; The point of this file: with the notation, you can write the
; *shape* of the HoTT identity lemmas, study the patterns, and
; define functions that walk identity-shaped trees. Treat it as
; whiteboard practice for what the rules of a future kernel would
; look like.

; ------------------------------------------------------------
; 1. The basic identity operations
; ------------------------------------------------------------

(define p (refl 'x))                 ; reflexivity
(define q (Id-sym p))                ; symmetry      (claimed: q : Id _ _ x)
(define r (Id-trans p q))            ; transitivity  (claimed: composition)
(define t (transport p 'value))      ; transport along a path
(define e (equivalence 'A 'B 'f 'g)) ; A ≃ B via f, g

(display "p = ") (display p) (newline)
(display "q = ") (display q) (newline)
(display "r = ") (display r) (newline)
(display "t = ") (display t) (newline)
(display "e = ") (display e) (newline)
(newline)

; ------------------------------------------------------------
; 2. Reducing identity trees as notation
; ------------------------------------------------------------
;
; A future checker would have these reduction rules:
;
;   Id-sym (refl x)       ==>  refl x       (refl is self-symmetric)
;   Id-sym (Id-sym p)     ==>  p            (sym is an involution)
;   Id-trans (refl _) p   ==>  p            (refl is a left identity)
;   Id-trans p (refl _)   ==>  p            (refl is a right identity)
;   transport (refl _) v  ==>  v            (refl transports trivially)
;
; lizard DOES NOT apply these reductions. But we can write a
; *user-level* simplifier that tries to apply them structurally.
; Whether the resulting terms would be "definitionally equal" in
; a real type theory is for the checker to decide; here we just
; pattern-match on tags.

(define (simplify-id t)
  (cond
    ; Id-sym (refl x) → refl x
    ((and (Id-sym? t) (refl? (Id-sym-path t)))
     (Id-sym-path t))
    ; Id-sym (Id-sym p) → p
    ((and (Id-sym? t) (Id-sym? (Id-sym-path t)))
     (simplify-id (Id-sym-path (Id-sym-path t))))
    ; Id-trans (refl _) p → p
    ((and (Id-trans? t) (refl? (Id-trans-p t)))
     (simplify-id (Id-trans-q t)))
    ; Id-trans p (refl _) → p
    ((and (Id-trans? t) (refl? (Id-trans-q t)))
     (simplify-id (Id-trans-p t)))
    ; transport (refl _) v → v
    ((and (transport? t) (refl? (transport-path t)))
     (transport-value t))
    ; otherwise: walk into the structure
    ((Id-sym? t)
     (Id-sym (simplify-id (Id-sym-path t))))
    ((Id-trans? t)
     (Id-trans (simplify-id (Id-trans-p t))
               (simplify-id (Id-trans-q t))))
    ((transport? t)
     (transport (simplify-id (transport-path t))
                (transport-value t)))
    (else t)))

(display "Simplification examples:") (newline)
(display "  (Id-sym (refl x))         ==> ") (display (simplify-id (Id-sym (refl 'x)))) (newline)
(display "  (Id-sym (Id-sym p))       ==> ") (display (simplify-id (Id-sym (Id-sym 'p)))) (newline)
(display "  (Id-trans (refl _) p)     ==> ") (display (simplify-id (Id-trans (refl 'a) 'p))) (newline)
(display "  (Id-trans p (refl _))     ==> ") (display (simplify-id (Id-trans 'p (refl 'a)))) (newline)
(display "  (transport (refl _) v)    ==> ") (display (simplify-id (transport (refl 'a) 'v))) (newline)
(newline)

; A non-trivial nested case — Id-sym of an Id-trans:
(display "  (Id-sym (Id-trans (refl _) p)) ==> ")
(display (simplify-id (Id-sym (Id-trans (refl 'a) 'p))))
(newline)
; The result is (Id-sym p) — we reduced inside before symmetrizing.

(newline)

; ------------------------------------------------------------
; 3. The shape of the J-rule
; ------------------------------------------------------------
;
; The J-rule says: to define a property P over all identity proofs,
; it suffices to define it for refl. In a real checker, given:
;
;   C : (a, b : A) -> (p : Id A a b) -> Type
;   c : (a : A) -> C a a (refl a)
;
; we get J : (a, b : A) -> (p : Id A a b) -> C a b p
;
; In lizard we can sketch the DISPATCH shape — how a checker
; would case-split. We can't verify any side condition.

(define (J-dispatch p on-refl on-sym on-trans on-transport on-other)
  (cond ((refl? p)      (on-refl     p))
        ((Id-sym? p)    (on-sym      p))
        ((Id-trans? p)  (on-trans    p))
        ((transport? p) (on-transport p))
        (else           (on-other    p))))

(display "J-dispatch:") (newline)
(display "  on (refl x)         : ")
(display (J-dispatch (refl 'x)
           (lambda (p) "refl-case")
           (lambda (p) "sym-case")
           (lambda (p) "trans-case")
           (lambda (p) "transport-case")
           (lambda (p) "other-case")))
(newline)
(display "  on (Id-sym (refl x)): ")
(display (J-dispatch (Id-sym (refl 'x))
           (lambda (p) "refl-case")
           (lambda (p) "sym-case")
           (lambda (p) "trans-case")
           (lambda (p) "transport-case")
           (lambda (p) "other-case")))
(newline)
(newline)

; ------------------------------------------------------------
; 4. Equivalence as a candidate identity
; ------------------------------------------------------------
;
; In HoTT, univalence says (Id U A B) ≃ Equiv(A, B). With the
; notation, we can WRITE this claim — though obviously we can't
; PROVE it. (To prove it, you would need: a check that the
; forward and backward maps are inverse up to identity, plus
; some higher coherence. None of that exists here.)

(define univalence-statement
  (judgment (empty-context)
            (equivalence (Id (U 1) 'A 'B)
                         (equivalence 'A 'B 'f 'g)
                         'idtoequiv
                         'ua)
            (U 1)))

(display "univalence-statement:") (newline)
(display "  ") (display univalence-statement) (newline)
(display "  (this is just structure — nothing is proved)") (newline)
(newline)

; ------------------------------------------------------------
; 5. Tally of what's checked
; ------------------------------------------------------------
(display "What this file verified: ") (display 'nothing) (newline)
(display "What it provided: ")        (display 'notation) (newline)
(display "What a thesis would still need: ")
(display 'a-real-bidirectional-type-checker-with-conversion)
(newline)
