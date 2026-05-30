; -*- lisp -*-
; ============================================================
;  PHASE M.5.9 Turn 2a — Judgment-kind tracking infrastructure
; ============================================================
;
; M.5.9 Turn 1 added the symmetric-S5 AST nodes (dia, poss-coerce),
; the modal-symmetric toggle, and three-context API wrappers. Turn 2a
; adds judgment-kind tracking infrastructure.
;
; ----- What this turn ships -----
;
; A NEW INTERNAL API: alongside lizard_tt_infer2 / lizard_tt_infer3,
; there are now lizard_tt_infer2_kind / lizard_tt_infer3_kind that
; return BOTH the inferred type AND its judgment kind:
;
;     typedef enum {
;       LIZARD_KIND_TRUE  = 0,   /* A true       */
;       LIZARD_KIND_VALID = 1,   /* A valid      */
;       LIZARD_KIND_POSS  = 2    /* A poss       */
;     } lizard_judgment_kind_t;
;
;     lizard_ast_node_t *lizard_tt_infer2_kind(
;         valid_ctx, truth_ctx, t, heap,
;         lizard_judgment_kind_t *out_kind);
;
; The out_kind parameter is OPTIONAL — pass NULL if you don't care.
; The existing infer2 / infer3 are now thin wrappers that pass NULL.
;
; ----- What does NOT change in Turn 2a -----
;
; Semantics are UNCHANGED. Every existing rule produces LIZARD_KIND_TRUE,
; matching the implicit assumption of all pre-M.5.9 code.
;
; - All 56 existing tests pass with zero regressions.
; - All 58 examples run unchanged.
; - The (dia e) and (poss-coerce e) forms still error with the
;   "not yet implemented (M.5.9 Turn 2)" message — Turn 2b will
;   replace this with real symmetric typing rules.
;
; ----- Why this approach -----
;
; The alternative would have been to change infer2's signature
; directly, breaking every internal caller. Instead, infer2 stays the
; way it is (returns just the type), and a parallel infer2_kind is
; available for new code that needs the kind.
;
; This is the C-idiomatic "optional output parameter" pattern. New
; symmetric-aware rules in Turn 2b will use it; everything else stays
; the same.

(display "=== M.5.9 Turn 2a — infrastructure-only turn ===") (newline)
(display "") (newline)
(display "User-visible behavior unchanged. The kernel's new internal") (newline)
(display "API surfaces in C as lizard_tt_infer2_kind / infer3_kind.") (newline)
(display "From Lisp, no observable difference yet.") (newline)
(newline)

; ============================================================
;  Verification: all existing behavior preserved
; ============================================================

(display "=== Existing modal behavior ===") (newline)
(set-logic 'S5)
(display "  (set-logic 'S5), current-logic: ") (display (current-logic)) (newline)
(display "") (newline)

(display "  Box typing:") (newline)
(display "    (infer (context) (box (U 0))): ")
(display (infer (context) (box (U 0)))) (newline)
(display "") (newline)

(display "  Diamond typing:") (newline)
(display "    (infer (context) (diamond (U 0))): ")
(display (infer (context) (diamond (U 0)))) (newline)
(display "") (newline)

(display "  Unbox extraction:") (newline)
(display "    (infer (context) (unbox 'x (box (U 0)) 'x)): ")
(display (infer (context) (unbox 'x (box (U 0)) 'x))) (newline)
(display "") (newline)

(display "  let-diamond:") (newline)
(display "    (infer (context) (let-diamond 'x (diamond (U 0)) 'x)): ")
(display (infer (context) (let-diamond 'x (diamond (U 0)) 'x))) (newline)
(display "") (newline)

(display "  box-app (K-axiom):") (newline)
(define ctxk
  (context-extend
    (context-extend (context) (variable 'f (Box (Pi '_ (U 0) (U 0)))))
    (variable 'a (Box (U 0)))))
(display "    (infer ctx (box-app 'f 'a)): ")
(display (infer ctxk (box-app 'f 'a))) (newline)
(display "") (newline)

(display "  diamond-bind (Diamond Kleisli):") (newline)
(define ctxd
  (context-extend
    (context-extend (context) (variable 'g (Pi '_ (U 0) (Diamond (U 0)))))
    (variable 'd (Diamond (U 0)))))
(display "    (infer ctx (diamond-bind 'g 'd)): ")
(display (infer ctxd (diamond-bind 'g 'd))) (newline)
(newline)

; ============================================================
;  Turn 2a leaves symmetric forms still unimplemented
; ============================================================

(display "=== Symmetric forms still pending (Turn 2b) ===") (newline)
(display "  (infer (context) (dia (U 0))):") (newline)
(display "    ") (display (infer (context) (dia (U 0)))) (newline)
(display "  ↑ explicit not-yet-implemented; Turn 2b wires the rule") (newline)
(display "") (newline)

(display "  (infer (context) (poss-coerce (U 0))):") (newline)
(display "    ") (display (infer (context) (poss-coerce (U 0)))) (newline)
(newline)

; ============================================================
;  The plan: what Turn 2b does
; ============================================================

(display "=== Turn 2b — the symmetric typing rules ===") (newline)
(display "") (newline)
(display "Now that judgment-kind tracking is wired, Turn 2b can add:") (newline)
(display "") (newline)
(display "  (poss-coerce e) typing rule:") (newline)
(display "    e : A true       (any judgment kind via infer2_kind)") (newline)
(display "    ──────────────────────────────────") (newline)
(display "    (poss-coerce e) : A   with kind POSS") (newline)
(display "") (newline)
(display "  (dia e) typing rule:") (newline)
(display "    e : A   with kind POSS  (checked via infer2_kind)") (newline)
(display "    ──────────────────────────────────") (newline)
(display "    (dia e) : Diamond A   with kind TRUE") (newline)
(display "") (newline)
(display "  Symmetric Box-intro (when modal-symmetric is on):") (newline)
(display "    Δ; ·; · ⊢ e : A true") (newline)
(display "    ──────────────────────────────────") (newline)
(display "    Δ; Γ; · ⊢ (box e) : Box A   with kind TRUE") (newline)
(display "    (drops Γ; same as asymmetric form but kind-aware)") (newline)
(display "") (newline)
(display "  Symmetric let-diamond (when modal-symmetric is on):") (newline)
(display "    Δ; Γ; · ⊢ d : Diamond A true") (newline)
(display "    Δ; Γ; x:A poss ⊢ body : C   with kind POSS") (newline)
(display "    ────────────────────────────────────────────") (newline)
(display "    Δ; Γ; Ω ⊢ (let-diamond x d body) : C   with kind POSS") (newline)
(display "    (places x in Ω; body must produce POSS judgment)") (newline)
(newline)

(display "Estimated scope: Turn 2b is the actual semantic work. It") (newline)
(display "may need a Turn 2c for regressions if the symmetric rules") (newline)
(display "interact unexpectedly with existing modal code.") (newline)
(newline)

(logic-config-reset)
(display "=== Phase M.5.9 Turn 2a complete ===") (newline)
