; -*- lisp -*-
; ============================================================
;  HOTT FRAGMENT — identity computes on type formers
; ============================================================
;
; This file demonstrates the HOTT-flavored computation rules that
; lizard's engine now supports. To be precise about scope: lizard
; implements TWO rules from the HOTT family, plus the identity-
; algebra and pi-beta rules. The two HOTT-specific rules are:
;
;   1. ap-refl:   ap(f, refl_a) --> refl_{f a}
;                 Congruence: applying a function to a reflexivity
;                 proof gives reflexivity at the function's value.
;
;   2. Id-on-Pi:  Id (Pi x A B) f g --> Pi x A (Id B (f x) (g x))
;                 The COMPUTATIONAL version of functional
;                 extensionality. In MLTT, funext is a postulate.
;                 In HOTT, it's a derived consequence of univalence.
;                 Here, it's a head reduction rule.
;
; What HOTT additionally has, that lizard does NOT yet:
;   - Id on Sigma  (needs Pair constructor + transport composition)
;   - Id on Sum    (needs inl/inr constructors)
;   - Id on Unit   (needs Unit type)
;   - Id on Bool   (needs Bool + true/false)
;   - Univalence as a computational rule
;   - Higher inductive types
;   - The full coherence story relating all these
;
; What we have is a recognizable piece of the HOTT computational
; story. Two of the rules. Wired into the same engine that handles
; the identity algebra and pi-beta, so they compose with everything
; already there.

(display "=== Rule 1: ap-refl ===") (newline)
; ap takes a function and a path. When the path is refl, it reduces
; to refl at the function applied to the endpoint.

(display "ap(f, refl 'a)        = ")
(display (reduce (ap 'f (refl 'a)))) (newline)

(display "ap(id, refl 'q)       = ")
(display (reduce (ap (Lambda 'x 'x) (refl 'q)))) (newline)
(display "  (above cascades: ap-refl produces refl_{(id q)},")
(newline)
(display "   pi-beta reduces (id q) to q, result is refl q)") (newline)

(display "ap(K_x, refl 'y)      = ")
(display (reduce (ap (Lambda 'z 'x) (refl 'y)))) (newline)
(display "  (K_x is the constant function returning x;") (newline)
(display "   ap K_x p = refl_x for any p)") (newline)
(newline)

(display "=== Rule 2: Id on Pi types ===") (newline)
; The computational extensionality rule. An identity proof between
; two functions IS a function from arguments to identity proofs.

(display "Id (Pi x A B) f g = ")
(newline)
(display "  ") (display (reduce (Id (Pi 'x 'A 'B) 'f 'g))) (newline)
(display "(an identity between functions IS a Pi of pointwise identities)")
(newline)
(newline)

; Non-dependent function type — codomain doesn't mention x:
(display "Id (Nat -> Nat) f g = Id (Pi n Nat Nat) f g") (newline)
(display "  = ") (display (reduce (Id (Pi 'n 'Nat 'Nat) 'f 'g))) (newline)
(newline)

; Nested function types — the rule cascades:
(display "Id (Pi x A (Pi y B C)) f g =") (newline)
(display "  ")
(display (reduce (Id (Pi 'x 'A (Pi 'y 'B 'C)) 'f 'g))) (newline)
(display "(curried 2-ary function: Id becomes nested Pi of pointwise Ids)")
(newline)
(newline)

(display "=== equal? sees through these reductions ===") (newline)

(display "Id (Pi x A B) f g  ≡  Pi x A (Id B (f x) (g x))") (newline)
(display "  ") 
(display (equal? (Id (Pi 'x 'A 'B) 'f 'g)
                 (Pi 'x 'A (Id 'B (@ 'f 'x) (@ 'g 'x))))) (newline)

(display "ap id (refl q) ≡ refl q") (newline)
(display "  ")
(display (equal? (ap (Lambda 'x 'x) (refl 'q)) (refl 'q))) (newline)
(newline)

(display "=== Composition: HOTT rules + identity algebra ===") (newline)
; When the HOTT rules and identity algebra interact:

; ap of a function on (sym (sym (refl q))):
; First sym-involutive reduces sym(sym(refl q)) to refl q,
; then ap-refl fires.
(display "ap f (sym (sym (refl q))) = ")
(display (reduce (ap 'f (Id-sym (Id-sym (refl 'q)))))) (newline)
(display "  (sym-involutive then ap-refl)") (newline)

; Id-on-Pi applied to a Pi whose codomain is an Id type:
; Id (Pi x A (Id B u v)) f g
;   --> Pi x A (Id (Id B u v) (f x) (g x))
; Note that the inner Id is between PATHS (it's Id on an Id-type),
; which is the start of the "tower of identities" you see in HoTT.
(display "Id (Pi x A (Id B u v)) f g = ") (newline)
(display "  ")
(display (reduce (Id (Pi 'x 'A (Id 'B 'u 'v)) 'f 'g))) (newline)
(newline)

(display "=== Pluggability — turning HOTT rules off ===") (newline)
(flag-set! 'reduce-id-pi #f)
(display "with Id-on-Pi OFF:") (newline)
(display "  Id (Pi x A B) f g = ")
(display (reduce (Id (Pi 'x 'A 'B) 'f 'g))) (newline)
(flag-set! 'reduce-id-pi #t)

(flag-set! 'reduce-ap-refl #f)
(display "with ap-refl OFF:") (newline)
(display "  ap f (refl a) = ")
(display (reduce (ap 'f (refl 'a)))) (newline)
(flag-set! 'reduce-ap-refl #t)
(newline)

(display "=== Scope ===") (newline)
(display "  Identity algebra:        WORKING (sym/trans/transport rules)") (newline)
(display "  Pi-beta:                 WORKING (function application reduces)") (newline)
(display "  ap-refl:                 WORKING (congruence basic case)") (newline)
(display "  Id-on-Pi:                WORKING (computational funext)") (newline)
(display "  Id-on-Sigma:             NOT IMPLEMENTED (needs Pair)") (newline)
(display "  Id-on-Sum:               NOT IMPLEMENTED (needs inl/inr)") (newline)
(display "  Id-on-Unit:              NOT IMPLEMENTED (needs Unit)") (newline)
(display "  J-rule on refl:          NOT IMPLEMENTED") (newline)
(display "  Univalence-as-rule:      NOT IMPLEMENTED") (newline)
(display "  Higher inductive types:  NOT IMPLEMENTED") (newline)
(display "  Type checking:           NOT IMPLEMENTED") (newline)
