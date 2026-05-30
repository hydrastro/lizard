; -*- lisp -*-
; ============================================================
;  M.5.6 companion — Comonad / Monad structure for Box / Diamond
;  As DEFINITIONS, not as proofs
; ============================================================
;
; In categorical semantics of modal type theory:
;
;   Box     should be a COMONAD (in S4+)
;   Diamond should be a MONAD   (in S4+)
;
; A comonad is (T, extract, duplicate) with three laws.
; A monad   is (T, return,  join)      with three laws.
;
; This file demonstrates that the OPERATIONS of these structures
; exist in lizard's modal calculus — that you can WRITE them, that
; they TYPECHECK at the right signatures, and that they're well-
; defined terms.
;
; What this file does NOT do:
;   - Prove the comonad/monad LAWS hold (would require a proof
;     language; lizard has a type-checker, not a theorem prover).
;   - Establish soundness, canonicity, or termination.
;
; The honest claim is: "the operations are definable at the right
; types." Whether they satisfy the categorical equations would
; require either (a) explicit proof terms, or (b) testing via
; normalization — both of which are research-grade and out of scope
; for this turn.

; ============================================================
;  COMONAD STRUCTURE FOR BOX (under S4)
; ============================================================

(display "=== Comonad structure for Box (under S4) ===") (newline)
(set-logic 'S4)
(display "  current-logic: ") (display (current-logic)) (newline)
(newline)

; -----------------------------------------------------------
;  extract : Box A → A   (this is the T-axiom)
; -----------------------------------------------------------

(display "--- extract : Box A → A ---") (newline)
(display "  Definition (open form, given b : Box A):") (newline)
(display "    extract(b) = (unbox y b in y)") (newline)
(display "") (newline)
(define ctx_b (context-extend (context) (variable 'b (Box (U 0)))))
(display "  Given b : Box (U 0):") (newline)
(display "    (unbox 'y 'b 'y) : ")
(display (infer ctx_b (unbox 'y 'b 'y))) (newline)
(display "    ↑ extracts (U 0) from Box (U 0). The T-axiom in action.") (newline)
(newline)

; -----------------------------------------------------------
;  duplicate : Box A → Box (Box A)   (this is the 4-axiom)
; -----------------------------------------------------------

(display "--- duplicate : Box A → Box (Box A) ---") (newline)
(display "  Definition (open form, given b : Box A):") (newline)
(display "    duplicate(b) = (unbox y b in (box (box y)))") (newline)
(display "") (newline)
(display "  Given b : Box (U 0):") (newline)
(display "    (unbox 'y 'b (box (box 'y))) : ")
(display (infer ctx_b (unbox 'y 'b (box (box 'y))))) (newline)
(display "    ↑ produces Box (Box (U 0)). The 4-axiom in action.") (newline)
(newline)

; -----------------------------------------------------------
;  The comonad laws (NOT proven)
; -----------------------------------------------------------

(display "--- Comonad laws (NOT proven) ---") (newline)
(display "  Three laws should hold for any comonad:") (newline)
(display "") (newline)
(display "    (1) extract ∘ duplicate = id_BoxA") (newline)
(display "    (2) (Box extract) ∘ duplicate = id_BoxA") (newline)
(display "    (3) duplicate ∘ duplicate = (Box duplicate) ∘ duplicate") (newline)
(display "") (newline)
(display "  Operationally in lizard, by beta reduction:") (newline)
(display "    (1) Given (box e) : Box A,") (newline)
(display "        duplicate (box e) ⟶ unbox y (box e) (box (box y))") (newline)
(display "                          ⟶ (box (box e))    [beta]") (newline)
(display "        extract (box (box e)) ⟶ unbox y (box (box e)) y") (newline)
(display "                              ⟶ (box e)         [beta]") (newline)
(display "        So extract ∘ duplicate ↓ id on closed forms. ✓ (smoke-tested)") (newline)
(display "") (newline)
(display "  Witnessing this for a specific value:") (newline)
(display "    extract(duplicate (box (U 0))):") (newline)
(display "    = (unbox 'z (unbox 'y (box (U 0)) (box (box 'y))) 'z)") (newline)
(display "    ↦ ") (display (reduce (unbox 'z (unbox 'y (box (U 0)) (box (box 'y))) 'z))) (newline)
(display "    ↑ reduces to (box (U 0)) — the original. Law (1) holds for this term.") (newline)
(display "") (newline)
(display "  NOT a proof — only a witness on one term. A real proof would") (newline)
(display "  require quantifying over all closed terms of type Box A.") (newline)
(newline)

; ============================================================
;  MONAD STRUCTURE FOR DIAMOND (under S5)
; ============================================================

(display "=== Monad structure for Diamond (under S5) ===") (newline)
(set-logic 'S5)
(display "  current-logic: ") (display (current-logic)) (newline)
(display "  ↑ S5 reverse-lookup hits CoC; toggle state is S5 (5-axiom on)") (newline)
(newline)

; -----------------------------------------------------------
;  return : A → Diamond A
; -----------------------------------------------------------

(display "--- return : A → Diamond A ---") (newline)
(display "  Definition: return = diamond") (newline)
(display "") (newline)
(define ctx_a (context-extend (context) (variable 'a (U 0))))
(display "  Given a : (U 0):") (newline)
(display "    (diamond 'a) : ")
(display (infer ctx_a (diamond 'a))) (newline)
(display "    ↑ Diamond (U 0). The unit of the monad.") (newline)
(newline)

; -----------------------------------------------------------
;  join : Diamond (Diamond A) → Diamond A
; -----------------------------------------------------------

(display "--- join : Diamond (Diamond A) → Diamond A ---") (newline)
(display "  Definition (open form, given dd : Diamond (Diamond A)):") (newline)
(display "    join(dd) = (let-diamond x dd in x)") (newline)
(display "") (newline)
(display "  Crucially, this requires the 5-axiom: x is bound in the") (newline)
(display "  valid context under S5, so the inner Diamond is accessible") (newline)
(display "  even though we want to RETURN it (not just use it locally).") (newline)
(display "") (newline)
(define ctx_dd (context-extend (context) (variable 'dd (Diamond (Diamond (U 0))))))
(display "  Given dd : Diamond (Diamond (U 0)):") (newline)
(display "    (let-diamond 'x 'dd 'x) : ")
(display (infer ctx_dd (let-diamond 'x 'dd 'x))) (newline)
(display "    ↑ Diamond (U 0). The multiplication μ of the monad.") (newline)
(newline)

; -----------------------------------------------------------
;  bind / >>=  : Diamond A → (A → Diamond B) → Diamond B
; -----------------------------------------------------------

(display "--- bind : Diamond A → (A → Diamond B) → Diamond B ---") (newline)
(display "  Definition (open form, given d : Diamond A, k : A → Diamond B):") (newline)
(display "    bind d k = (let-diamond x d (@ k x))") (newline)
(display "") (newline)
(define ctx_dk
  (context-extend
    (context-extend (context) (variable 'd (Diamond (U 0))))
    (variable 'k (Pi 'z (U 0) (Diamond (U 0))))))
(display "  Given d : Diamond (U 0), k : (U 0) → Diamond (U 0):") (newline)
(display "    (let-diamond 'x 'd (@ 'k 'x)) : ")
(display (infer ctx_dk (let-diamond 'x 'd (@ 'k 'x)))) (newline)
(display "    ↑ Diamond (U 0). The bind operation, deriving from join and fmap.") (newline)
(newline)

; -----------------------------------------------------------
;  The monad laws (NOT proven)
; -----------------------------------------------------------

(display "--- Monad laws (NOT proven) ---") (newline)
(display "  Three laws for any monad:") (newline)
(display "") (newline)
(display "    (1) join ∘ return       = id_DiamondA       (left unit)") (newline)
(display "    (2) join ∘ (Diamond return) = id_DiamondA   (right unit)") (newline)
(display "    (3) join ∘ join         = join ∘ (Diamond join)  (associativity)") (newline)
(display "") (newline)
(display "  Witness for law (1) on a specific term:") (newline)
(display "    join(return(diamond (U 0))):") (newline)
(display "    = (let-diamond 'y (diamond (diamond (U 0))) 'y)") (newline)
(display "    ↦ ") (display (reduce (let-diamond 'y (diamond (diamond (U 0))) 'y))) (newline)
(display "    ↑ reduces to (diamond (U 0)) — the original. Law (1) witnessed.") (newline)
(display "") (newline)

; ============================================================
;  COMONAD-FOR-BOX UNDER WEAKER LOGICS
; ============================================================

(display "=== Box comonad structure depends on the logic ===") (newline)
(display "") (newline)
(display "  Under K: neither extract nor duplicate exists.") (newline)
(set-logic 'K)
(display "    K: t-axiom-enabled = ") (display (logic-rule-enabled? 't-axiom-enabled)) (newline)
(display "    extract attempt: (unbox 'y 'b 'y) : ")
(display (infer ctx_b (unbox 'y 'b 'y))) (newline)
(display "    ↑ K rejects — body must be Box-typed") (newline)
(newline)

(display "  Under T: extract exists, but duplicate doesn't (no 4-axiom).") (newline)
(set-logic 'T)
(display "    T: t-axiom-enabled = ")
(display (logic-rule-enabled? 't-axiom-enabled)) (newline)
(display "       modal-4-axiom    = ") (display (logic-rule-enabled? 'modal-4-axiom)) (newline)
(display "    extract works:     (unbox 'y 'b 'y) : ")
(display (infer ctx_b (unbox 'y 'b 'y))) (newline)
(display "    duplicate attempt: (unbox 'y 'b (box (box 'y))) : ")
(display (infer ctx_b (unbox 'y 'b (box (box 'y))))) (newline)
(display "    ↑ T rejects because nested box can't see y (no 4-axiom)") (newline)
(newline)

(display "  Under S4: comonad structure exists.") (newline)
(set-logic 'S4)
(display "    extract:   (unbox 'y 'b 'y) : ")
(display (infer ctx_b (unbox 'y 'b 'y))) (newline)
(display "    duplicate: (unbox 'y 'b (box (box 'y))) : ")
(display (infer ctx_b (unbox 'y 'b (box (box 'y))))) (newline)
(newline)

; ============================================================
;  Summary
; ============================================================

(display "=== Status ===") (newline)
(display "") (newline)
(display "Operations definable (typecheck):") (newline)
(display "  Comonad for Box (under S4):") (newline)
(display "    ✓ extract   : Box A → A") (newline)
(display "    ✓ duplicate : Box A → Box (Box A)") (newline)
(display "  Monad for Diamond (under S5):") (newline)
(display "    ✓ return    : A → Diamond A") (newline)
(display "    ✓ join      : Diamond (Diamond A) → Diamond A") (newline)
(display "    ✓ bind      : Diamond A → (A → Diamond B) → Diamond B") (newline)
(display "") (newline)
(display "Laws witnessed on specific terms (NOT proven generally):") (newline)
(display "  - extract ∘ duplicate ↓ id      on (box (U 0))") (newline)
(display "  - join ∘ return       ↓ id      on (diamond (U 0))") (newline)
(display "") (newline)
(display "What would be needed for a real proof:") (newline)
(display "  - Quantification over closed terms (lizard has no proof binder)") (newline)
(display "  - Decision procedure for closed-form equality") (newline)
(display "  - Either: (a) a proof language on top, or (b) hand verification") (newline)
(display "") (newline)
(display "  These are research-grade and out of scope for M.5.6.") (newline)
(display "  The operational evidence here is suggestive, not conclusive.") (newline)
(display "") (newline)

(logic-config-reset)
(display "=== Comonad/Monad demonstration complete ===") (newline)
