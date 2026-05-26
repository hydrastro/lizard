; -*- lisp -*-
; ============================================================
;  HOTT FRAGMENT TYPING — full coverage
; ============================================================
;
; lizard's type checker now covers a substantial part of the
; HOTT fragment, not just λΠ. Forms with typing rules:
;
;   Type formers:
;     Pi, Sigma, Sum, Id, Unit, Bot
;
;   Introduction (require expected type):
;     Lambda → Pi
;     Pair → Sigma
;     inl / inr → Sum
;     refl → Id
;     tt → Unit
;
;   Elimination (infer their type):
;     @ (application) of a Pi
;     fst / snd of a Sigma
;     Case of a Sum (non-dependent — branches must agree)
;     J (path induction) — uses motive
;     xport (transport with motive)
;     ap (congruence of a function on a path)
;
;   With cumulativity throughout.
;
; What's still not typed:
;   Universe-level Λ-binder for true polymorphism (universe
;   variables exist and can be compared, but aren't bound).
;   Inductive types other than the built-in ones (no schema).
;   Dependent Case eliminator (current Case is non-dep).

; ------------------------------------------------------------
; 1. Id and refl
; ------------------------------------------------------------

(display "=== Id formation and refl ===") (newline)

(define ctx_A (context (variable 'A (U 0)) (variable 'a 'A)))

(display "(infer ctx (Id A a a)) = ")
(display (infer ctx_A (Id 'A 'a 'a))) (newline)
(display "  (Id at A : (U 0) lives in (U 0) — Id is small)") (newline)

(display "(infer ctx (refl 'a)) = ")
(display (infer ctx_A (refl 'a))) (newline)
(display "  (refl synthesizes its identity type)") (newline)

(display "(check ctx (refl 'a) (Id A a a)) = ")
(display (check ctx_A (refl 'a) (Id 'A 'a 'a))) (newline)

; refl can ONLY prove reflexivity — not arbitrary identities
(define ctx_AB (context (variable 'A (U 0)) (variable 'a 'A) (variable 'b 'A)))
(display "(check ctx (refl 'a) (Id A a b)) where b≠a: ")
(display (check ctx_AB (refl 'a) (Id 'A 'a 'b))) (newline)
(display "  (correctly rejected — that would need a proof, not refl)") (newline)
(newline)

; ------------------------------------------------------------
; 2. The simple types: Unit, tt, Bot
; ------------------------------------------------------------

(display "=== Unit, tt, Bot ===") (newline)
(display "(infer empty Unit) = ") (display (infer (context) (Unit))) (newline)
(display "(infer empty tt) = ") (display (infer (context) (tt))) (newline)
(display "(check empty tt Unit) = ") (display (check (context) (tt) (Unit))) (newline)
(display "(infer empty Bot) = ") (display (infer (context) (Bot))) (newline)
(newline)

; ------------------------------------------------------------
; 3. Sigma intro and elim
; ------------------------------------------------------------

(display "=== Sigma ===") (newline)
(display "(infer (Sigma x (U 0) (U 0))) = ")
(display (infer (context) (Sigma 'x (U 0) (U 0)))) (newline)

(define ctx_AB2 (context (variable 'A (U 0)) (variable 'B (U 0))
                         (variable 'a 'A) (variable 'b 'B)))
(display "(check (Pair a b) (Sigma x A B)) = ")
(display (check ctx_AB2 (Pair 'a 'b) (Sigma 'x 'A 'B))) (newline)

(define ctx_p (context (variable 'A (U 0)) (variable 'B (U 0))
                       (variable 'p (Sigma 'x 'A 'B))))
(display "(infer (fst p)) = ") (display (infer ctx_p (fst 'p))) (newline)
(display "(infer (snd p)) = ") (display (infer ctx_p (snd 'p))) (newline)
(newline)

; ------------------------------------------------------------
; 4. Sum intro and elim
; ------------------------------------------------------------

(display "=== Sum ===") (newline)
(display "(infer (Sum (U 0) (U 0))) = ")
(display (infer (context) (Sum (U 0) (U 0)))) (newline)

(display "(check (inl a) (Sum A B)) = ")
(display (check ctx_AB2 (inl 'a) (Sum 'A 'B))) (newline)
(display "(check (inr b) (Sum A B)) = ")
(display (check ctx_AB2 (inr 'b) (Sum 'A 'B))) (newline)
(display "(check (inl a) (Sum B A)) — wrong side: ")
(display (check ctx_AB2 (inl 'a) (Sum 'B 'A))) (newline)
(display "  (correctly rejected)") (newline)

; Case as elim
(define ctx_case (context (variable 'A (U 0)) (variable 'B (U 0))
                          (variable 'C (U 0))
                          (variable 's (Sum 'A 'B))
                          (variable 'f (Pi 'a 'A 'C))
                          (variable 'g (Pi 'b 'B 'C))))
(display "(infer (Case s f g)) = ")
(display (infer ctx_case (Case 's 'f 'g))) (newline)
(display "  (both branches produce C — non-dependent Case)") (newline)

; Mismatched branches
(define ctx_bad (context (variable 'A (U 0)) (variable 'B (U 0))
                         (variable 'C1 (U 0)) (variable 'C2 (U 0))
                         (variable 's (Sum 'A 'B))
                         (variable 'f (Pi 'a 'A 'C1))
                         (variable 'g (Pi 'b 'B 'C2))))
(display "(infer (Case s f g)) with branches producing different types: ")
(display (infer ctx_bad (Case 's 'f 'g))) (newline)
(display "  (error, but printed as a generic error message)") (newline)
(newline)

; ------------------------------------------------------------
; 5. ap — congruence
; ------------------------------------------------------------

(display "=== ap ===") (newline)
(define ctx_apf (context (variable 'A (U 0)) (variable 'B (U 0))
                         (variable 'f (Pi 'x 'A 'B))
                         (variable 'a1 'A) (variable 'a2 'A)
                         (variable 'p (Id 'A 'a1 'a2))))
(display "(infer (ap f p)) = ") (newline)
(display "  ") (display (infer ctx_apf (ap 'f 'p))) (newline)
(display "  (ap of f along a path between a1 and a2 produces a path") (newline)
(display "   between (f a1) and (f a2) in B)") (newline)
(newline)

; ------------------------------------------------------------
; 6. xport — transport with motive
; ------------------------------------------------------------

(display "=== xport ===") (newline)
(define ctx_xp (context (variable 'A (U 0))
                        (variable 'P (Pi 'x 'A (U 0)))
                        (variable 'a 'A) (variable 'b 'A)
                        (variable 'p (Id 'A 'a 'b))
                        (variable 'v (@ 'P 'a))))
(display "ctx: P : A → U, p : Id A a b, v : P a") (newline)
(display "(infer (xport P p v)) = ") (newline)
(display "  ") (display (infer ctx_xp (xport 'P 'p 'v))) (newline)
(display "  (transport moves v from P(a) to P(b) along the path p)") (newline)
(newline)

; ------------------------------------------------------------
; 7. J — path induction
; ------------------------------------------------------------

(display "=== J (path induction) ===") (newline)
(define ctx_J 
  (context (variable 'A (U 0))
           (variable 'a 'A) (variable 'b 'A)
           (variable 'P (Pi 'x 'A (Pi 'p (Id 'A 'a 'x) (U 0))))
           (variable 'd (@ (@ 'P 'a) (refl 'a)))
           (variable 'q (Id 'A 'a 'b))))
(display "ctx: A:U, a,b:A, P:Pi x A. Pi p (Id A a x). U, d:P(a,refl), q:Id A a b")
(newline)
(display "(infer (J P d q)) = ") (newline)
(display "  ") (display (infer ctx_J (J 'P 'd 'q))) (newline)
(display "  (path induction applied at endpoint b along path q)") (newline)
(newline)

; ------------------------------------------------------------
; 8. A proof of refl-pi-reflexivity
; ------------------------------------------------------------

(display "=== A non-trivial proof ===") (newline)
(display "Claim: Pi x:U(0). Id (U 0) x x") (newline)
(display "Proof: Lambda x. refl x") (newline)
(display "(infer (Pi x (U 0) (Id (U 0) x x))) = ")
(display (infer (context) (Pi 'x (U 0) (Id (U 0) 'x 'x)))) (newline)
(display "Check the proof: ")
(display (check (context) (Lambda 'x (refl 'x))
                          (Pi 'x (U 0) (Id (U 0) 'x 'x)))) (newline)
(display "  (a proof that any type is reflexively equal to itself,") (newline)
(display "   type-checked end-to-end)") (newline)
(newline)

; ------------------------------------------------------------
; 9. Scope
; ------------------------------------------------------------

(display "=== Scope ===") (newline)
(display "  Now fully type-checked:") (newline)
(display "    Pi, Sigma, Sum, Id, Unit, Bot       (type formers)") (newline)
(display "    Lambda, Pair, inl, inr, refl, tt    (intros)") (newline)
(display "    @, fst, snd, Case, ap, xport, J     (elims)") (newline)
(display "    Universe cumulativity through universe-leq?") (newline)
(display "  Still missing:") (newline)
(display "    Universe-level Λ-binder (true polymorphism)") (newline)
(display "    Dependent Case eliminator") (newline)
(display "    Inductive type schemas (each new inductive needs a hardcoded rule)") (newline)
(display "    Nat, Bool, List with their typing rules") (newline)
