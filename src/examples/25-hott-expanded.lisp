; -*- lisp -*-
; ============================================================
;  HOTT FRAGMENT, EXPANDED
; ============================================================
;
; lizard's HOTT-fragment now covers:
;
;   IDENTITY ALGEBRA (4 rules):
;     sym(refl_a)              --> refl_a
;     sym(sym(p))              --> p
;     trans(refl, p) / p, refl --> p
;     trans(trans, _) assoc
;
;   PI (1 rule):
;     (@ (Lambda x b) a)       --> b[a/x]   (pi-beta)
;
;   AP — congruence over the path category (3 rules):
;     ap(f, refl_a)            --> refl_{f a}
;     ap(f, sym p)             --> sym (ap f p)
;     ap(f, trans p q)         --> trans (ap f p) (ap f q)
;
;   SIGMA INTRO/ELIM (3 rules):
;     fst (Pair a b)           --> a
;     snd (Pair a b)           --> b
;     Id (Sigma _ A B) (Pair a b) (Pair a' b')
;       --> Sigma _ (Id A a a') (Id B b b')    (non-dependent)
;
;   SUM INTRO/ELIM (5 rules):
;     Case (inl a) f g         --> (@ f a)
;     Case (inr b) f g         --> (@ g b)
;     Id (Sum A B) (inl a) (inl a')  --> Id A a a'
;     Id (Sum A B) (inr b) (inr b')  --> Id B b b'
;     Id (Sum A B) (inl _) (inr _)   --> Bot   (and symmetric)
;
;   UNIT (1 rule):
;     Id Unit x y              --> Unit       (contractibility)
;
;   PATH INDUCTION (1 rule):
;     J P d (refl a)           --> d
;
; That's 18 rules total. Each is flag-controlled. Each composes
; with all the others through bottom-up normalization with
; memoization. Together they're a recognizable piece of HOTT's
; computational story — though far from all of it.

; ------------------------------------------------------------
; 1. Sigma intro/elim
; ------------------------------------------------------------

(display "=== Sigma ===") (newline)
(display "fst (Pair a b)               = ")
(display (reduce (fst (Pair 'a 'b)))) (newline)
(display "snd (Pair a b)               = ")
(display (reduce (snd (Pair 'a 'b)))) (newline)
(display "fst (Pair (Lambda x x) g) z  = ")
(display (reduce (@ (fst (Pair (Lambda 'x 'x) 'g)) 'z))) (newline)
(display "  (composed: fst-beta + pi-beta)") (newline)
(newline)

; ------------------------------------------------------------
; 2. Sum intro/elim
; ------------------------------------------------------------

(display "=== Sum ===") (newline)
(display "Case (inl x) f g             = ")
(display (reduce (Case (inl 'x) 'f 'g))) (newline)
(display "Case (inr y) f g             = ")
(display (reduce (Case (inr 'y) 'f 'g))) (newline)
(display "Case (inl 5) id g            = ")
(display (reduce (Case (inl 5) (Lambda 'x 'x) 'g))) (newline)
(newline)

; ------------------------------------------------------------
; 3. Identity-on-type-formers — the HOTT distinctive
; ------------------------------------------------------------

(display "=== Id computes on type formers ===") (newline)

(display "Id on Pi:") (newline)
(display "  Id (Pi x A B) f g = ") (newline)
(display "    ") (display (reduce (Id (Pi 'x 'A 'B) 'f 'g))) (newline)

(display "Id on Sigma (non-dependent):") (newline)
(display "  Id (Sigma _ A B) (Pair a b) (Pair a' b') = ") (newline)
(display "    ") 
(display (reduce (Id (Sigma 'x 'A 'B)
                     (Pair 'a 'b)
                     (Pair 'a-prime 'b-prime)))) (newline)
(display "  (identity between pairs is a pair of identities)") (newline)

(display "Id on Sum, matching constructors:") (newline)
(display "  Id (Sum A B) (inl a) (inl a') = ")
(display (reduce (Id (Sum 'A 'B) (inl 'a) (inl 'a-prime)))) (newline)
(display "  Id (Sum A B) (inr b) (inr b') = ")
(display (reduce (Id (Sum 'A 'B) (inr 'b) (inr 'b-prime)))) (newline)

(display "Id on Sum, conflicting constructors:") (newline)
(display "  Id (Sum A B) (inl a) (inr b) = ")
(display (reduce (Id (Sum 'A 'B) (inl 'a) (inr 'b)))) (newline)
(display "  (inl ≠ inr as points of A+B, so Id reduces to Bot)") (newline)

(display "Id on Unit (contractibility):") (newline)
(display "  Id Unit x y = ")
(display (reduce (Id (Unit) 'x 'y))) (newline)
(display "  (any two inhabitants of Unit are equal; Id collapses to Unit)")
(newline)
(newline)

; ------------------------------------------------------------
; 4. Path induction — J on refl
; ------------------------------------------------------------

(display "=== J — path induction ===") (newline)
(display "J P d (refl a)               = ")
(display (reduce (J 'P 'd (refl 'a)))) (newline)
(display "J P d (sym (sym (refl a)))   = ")
(display (reduce (J 'P 'd (Id-sym (Id-sym (refl 'a)))))) (newline)
(display "  (cascades: sym-involutive then J-refl)") (newline)
(newline)

; ------------------------------------------------------------
; 5. ap is a functor on the path category
; ------------------------------------------------------------

(display "=== ap congruence ===") (newline)
(display "ap f (refl a)                = ")
(display (reduce (ap 'f (refl 'a)))) (newline)
(display "ap f (sym p)                 = ")
(display (reduce (ap 'f (Id-sym 'p)))) (newline)
(display "ap f (trans p q)             = ")
(display (reduce (ap 'f (Id-trans 'p 'q)))) (newline)
(display "  (ap pushes through the path algebra)") (newline)

; A composed case: ap of sym of trans
(display "ap f (sym (trans p q))       = ")
(display (reduce (ap 'f (Id-sym (Id-trans 'p 'q))))) (newline)
(display "  (ap-sym then ap-trans inside)") (newline)
(newline)

; ------------------------------------------------------------
; 6. Mixing the rules
; ------------------------------------------------------------

(display "=== Composition across the fragment ===") (newline)

; Build pair, project, apply
(display "(@ (fst (Pair id K)) 'q)     = ")
(display (reduce (@ (fst (Pair (Lambda 'x 'x)
                               (Lambda 'x (Lambda 'y 'x))))
                    'q))) (newline)

; Case on a constructor with a Lambda branch — full beta cascade
(display "(Case (inl 5) (Lambda x x) g) = ")
(display (reduce (Case (inl 5) (Lambda 'x 'x) 'g))) (newline)

; J on a normalized path
(display "(J P d (Id-trans (refl a) (sym (sym (refl a))))) = ")
(display (reduce (J 'P 'd (Id-trans (refl 'a)
                                    (Id-sym (Id-sym (refl 'a))))))) (newline)
(display "  (trans-refl-l + sym-involutive + J-refl)") (newline)
(newline)

; ------------------------------------------------------------
; 7. equal? sees through every reduction
; ------------------------------------------------------------

(display "=== Equality across rules ===") (newline)
(display "(fst (Pair a b)) ≡ a:           ")
(display (equal? (fst (Pair 'a 'b)) 'a)) (newline)
(display "(Case (inl x) f g) ≡ (@ f x):   ")
(display (equal? (Case (inl 'x) 'f 'g) (@ 'f 'x))) (newline)
(display "(J P d (refl a)) ≡ d:           ")
(display (equal? (J 'P 'd (refl 'a)) 'd)) (newline)
(display "(Id (Sum A B) (inl a) (inr b)) ≡ Bot: ")
(display (equal? (Id (Sum 'A 'B) (inl 'a) (inr 'b)) (Bot))) (newline)
(newline)

; ------------------------------------------------------------
; 8. The flag system — every rule is pluggable
; ------------------------------------------------------------

(display "=== Flags ===") (newline)
(display "All registered flags:") (newline)
(display "  ") (display (flag-list)) (newline)
(newline)

; Toggle off each HOTT rule in sequence:
(flag-set! 'reduce-id-sum #f)
(display "Id-sum OFF — conflict no longer reduces:") (newline)
(display "  Id (Sum A B) (inl a) (inr b) = ")
(display (reduce (Id (Sum 'A 'B) (inl 'a) (inr 'b)))) (newline)
(flag-set! 'reduce-id-sum #t)

(flag-set! 'reduce-j-refl #f)
(display "J-refl OFF:") (newline)
(display "  J P d (refl a) = ")
(display (reduce (J 'P 'd (refl 'a)))) (newline)
(flag-set! 'reduce-j-refl #t)
(newline)

; ------------------------------------------------------------
; 9. Honest scope
; ------------------------------------------------------------

(display "=== What this fragment covers vs full HOTT ===") (newline)
(display "  Implemented:") (newline)
(display "    pi-beta, identity algebra, ap congruence (3 cases)") (newline)
(display "    Id on Pi, Sigma (non-dep), Sum, Unit") (newline)
(display "    fst/snd beta, Case beta, J-refl") (newline)
(display "    18 rules total, all flag-controlled, composable") (newline)
(display "  Not yet implemented:") (newline)
(display "    Id on Sigma (dependent case)") (newline)
(display "    Id on Nat, Bool") (newline)
(display "    transport's per-type-former computation rules") (newline)
(display "    Univalence as computation rule") (newline)
(display "    Higher inductive types") (newline)
(display "    Coherence laws") (newline)
(display "    Type checking (no typing rules in lizard)") (newline)
