; -*- lisp -*-
; ============================================================
;  TRANSPORT-ON-TYPE-FORMERS
; ============================================================
;
; HOTT's computational story for each type former has TWO halves:
;
;   * The Id-on-X rule: what identity at type X looks like.
;     These were added in 25-hott-expanded.lisp.
;
;   * The transport-on-X rule: how to use a path along X.
;     These are added in this file.
;
; In real HOTT, transport carries an explicit motive — it's
; transport_P(p, x) where P : A → U is a type family describing
; the type of x at each point of the underlying space.
;
; lizard's new `xport` constructor takes the motive explicitly:
;
;   (xport (Lambda x T) path value)
;
; The body T of the motive Lambda tells the engine which per-
; type-former rule to apply. The five rules:
;
;   xport _ (refl _) v               --> v
;   xport (Lambda _ Unit) p tt       --> tt
;   xport (Lambda _ (Sum A B)) p (inl a)
;     --> inl (xport (Lambda _ A) p a)
;   xport (Lambda _ (Sigma _ A B)) p (Pair a b)
;     --> Pair (xport (Lambda _ A) p a) (xport (Lambda _ B) p b)
;   xport (Lambda _ (Pi y A B)) p f
;     --> Lambda y (xport (Lambda _ B) p (@ f y))
;
; All are non-dependent. The dependent versions (where B genuinely
; depends on the binder) require composing transport with the
; binder's substitution and are future work.

; ------------------------------------------------------------
; 1. The trivial case: transport along refl
; ------------------------------------------------------------

(display "=== xport along refl ===") (newline)
(display "xport _ (refl _) v          = ")
(display (reduce (xport (Lambda 'x 'P) (refl 'a) 'v))) (newline)
(display "  (transport along refl is identity, regardless of motive)") (newline)
(newline)

; ------------------------------------------------------------
; 2. Unit — transport doesn't do anything
; ------------------------------------------------------------

(display "=== Unit ===") (newline)
(display "xport (Lambda _ Unit) p tt  = ")
(display (reduce (xport (Lambda 'x (Unit)) 'p (tt)))) (newline)
(display "  (Unit is contractible; transport always lands at tt)") (newline)
(newline)

; ------------------------------------------------------------
; 3. Sum — transport preserves the constructor
; ------------------------------------------------------------

(display "=== Sum ===") (newline)
(display "xport (Lambda _ (Sum A B)) p (inl a) =") (newline)
(display "  ")
(display (reduce (xport (Lambda 'x (Sum 'A 'B)) 'p (inl 'a)))) (newline)
(display "xport (Lambda _ (Sum A B)) p (inr b) =") (newline)
(display "  ")
(display (reduce (xport (Lambda 'x (Sum 'A 'B)) 'p (inr 'b)))) (newline)
(display "  (transport pushes inside, preserving constructor)") (newline)
(newline)

; ------------------------------------------------------------
; 4. Sigma — transport works pointwise on pairs
; ------------------------------------------------------------

(display "=== Sigma (non-dependent) ===") (newline)
(display "xport (Lambda _ (Sigma _ A B)) p (Pair a b) =") (newline)
(display "  ")
(display (reduce (xport (Lambda 'x (Sigma 'y 'A 'B)) 'p (Pair 'a 'b))))
(newline)
(display "  (componentwise transport)") (newline)
(newline)

; ------------------------------------------------------------
; 5. Pi — transport gives a new function that transports outputs
; ------------------------------------------------------------

(display "=== Pi (non-dependent) ===") (newline)
(display "xport (Lambda _ (Pi y A B)) p f =") (newline)
(display "  ")
(display (reduce (xport (Lambda 'x (Pi 'y 'A 'B)) 'p 'f))) (newline)
(display "  (transport over a function type gives a Lambda whose body") (newline)
(display "   applies the original f and transports the result)") (newline)
(newline)

; ------------------------------------------------------------
; 6. Cascading: transport over a nested type
; ------------------------------------------------------------

(display "=== Cascading rules ===") (newline)

; A value of type Sum (Sigma A B) C, transported:
(display "xport over Sum(Sigma(A,B), C) on inl(Pair(a,b)) =") (newline)
(display "  ")
(display (reduce (xport (Lambda 'x (Sum (Sigma 'y 'A 'B) 'C))
                        'p (inl (Pair 'a 'b))))) (newline)
(display "  (xport-sum then xport-sigma)") (newline)

; A pair of pairs:
(display "xport over Sigma(Sigma A B, C) on Pair(Pair a b, c) =") (newline)
(display "  ")
(display (reduce (xport (Lambda 'x (Sigma 'y (Sigma 'z 'A 'B) 'C))
                        'p (Pair (Pair 'a 'b) 'c)))) (newline)
(display "  (xport-sigma at the top, then xport-sigma again for inner pair)")
(newline)
(newline)

; ------------------------------------------------------------
; 7. xport + Id rules together
; ------------------------------------------------------------

(display "=== xport interacting with Id rules ===") (newline)

; xport along a path between functions, where Id-Pi fires first:
; The path itself is in (Id (Pi y A B) f g), which would reduce
; to (Pi y A (Id B (f y) (g y))) — but that's the TYPE of the path,
; not the path itself. The path stays as 'p.
(display "transport an inl over a path:") (newline)
(display "  ")
(display (reduce (xport (Lambda 'x (Sum 'A 'B)) (refl 'q) (inl 'a))))
(newline)
(display "  (xport-refl fires; structural value retained)") (newline)
(newline)

; ------------------------------------------------------------
; 8. equal? sees through transport
; ------------------------------------------------------------

(display "=== Equality across xport ===") (newline)
(display "xport _ (refl _) v ≡ v:        ")
(display (equal? (xport (Lambda 'x 'P) (refl 'a) 'v) 'v)) (newline)
(display "xport over Unit ≡ tt:          ")
(display (equal? (xport (Lambda 'x (Unit)) 'p (tt)) (tt))) (newline)
(newline)

; ------------------------------------------------------------
; 9. Pluggable
; ------------------------------------------------------------

(display "=== Toggleability ===") (newline)
(flag-set! 'reduce-xport-sum #f)
(display "xport-sum OFF — Sum value no longer pushed through:") (newline)
(display "  ")
(display (reduce (xport (Lambda 'x (Sum 'A 'B)) 'p (inl 'a)))) (newline)
(flag-set! 'reduce-xport-sum #t)

(flag-set! 'reduce-xport-refl #f)
(display "xport-refl OFF — transport along refl stops being identity:")
(newline)
(display "  ")
(display (reduce (xport (Lambda 'x 'P) (refl 'a) 'v))) (newline)
(flag-set! 'reduce-xport-refl #t)
(newline)

; ------------------------------------------------------------
; 10. Honest scope reminder
; ------------------------------------------------------------

(display "=== Scope ===") (newline)
(display "  Implemented (5 transport rules):") (newline)
(display "    xport-refl, xport-unit, xport-sum, xport-sigma, xport-pi") (newline)
(display "  All non-dependent. The motive's body picks the rule.") (newline)
(display "  Still missing:") (newline)
(display "    dependent transport-on-Pi/Sigma (needs binder composition)") (newline)
(display "    xport over Nat, Bool, List (need those type formers first)") (newline)
(display "    transport coherence laws") (newline)
