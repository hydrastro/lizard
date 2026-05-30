; -*- lisp -*-
; lib/proof.lisp — Proof helpers and a library of verified
; kernel definitions (Track K).
;
; These build on the kernel primitives:
;   kernel-infer, kernel-check, kernel-reduce, kernel-equal?,
;   kernel-define, kernel-lookup, begin-proof, tactic-*, qed

; ============================================================
;  TYPE-CHECKING HELPERS
; ============================================================

; Returns #t if `term` has type `type` in the empty context.
(define (has-type? term type)
  (kernel-check term type))

; Returns #t if two terms are definitionally equal.
(define (defeq? a b)
  (kernel-equal? a b))

; Returns the normal form of a term (as a string).
(define (normalize term)
  (kernel-reduce term))

; ============================================================
;  VERIFIED COMBINATOR LIBRARY
; ============================================================
; Each is type-checked by the kernel when defined.

; Define the standard combinators in the kernel.
(define (install-combinators!)
  ; identity : Pi(x:Nat). Nat
  (kernel-define 'idNat '(lam (x Nat) #0) '(Pi (x Nat) Nat))
  ; const-zero : Pi(x:Nat). Nat
  (kernel-define 'constZero '(lam (x Nat) 0) '(Pi (x Nat) Nat))
  ; successor : Pi(x:Nat). Nat
  (kernel-define 'mySucc '(lam (x Nat) (succ #0)) '(Pi (x Nat) Nat))
  ; bool identity : Pi(b:Bool). Bool
  (kernel-define 'idBool '(lam (b Bool) #0) '(Pi (b Bool) Bool))
  #t)

; ============================================================
;  PROOF SCRIPTS
; ============================================================
; A proof script is a sequence of tactics. These helpers run
; common proof patterns.

; Prove a reflexivity goal: (Id A a a).
(define (prove-refl type-expr)
  (begin-proof type-expr)
  (tactic-refl)
  (qed))

; Prove an inhabitation goal by providing an exact witness.
(define (prove-exact type-expr witness)
  (begin-proof type-expr)
  (tactic-exact witness)
  (qed))

; Prove a Sum goal by choosing the left branch with a witness.
(define (prove-left type-expr witness)
  (begin-proof type-expr)
  (tactic-left)
  (tactic-exact witness)
  (qed))

; Prove a Sum goal by choosing the right branch with a witness.
(define (prove-right type-expr witness)
  (begin-proof type-expr)
  (tactic-right)
  (tactic-exact witness)
  (qed))

; ============================================================
;  PROPOSITIONS AS TYPES
; ============================================================
; Logic encoded via the Curry-Howard correspondence:
;   Proposition   Type
;   ───────────   ────
;   True          Unit
;   False         Empty
;   A and B       Sigma / pair
;   A or B        Sum
;   A implies B   Pi (function)
;   not A         A -> Empty

; The type of "A implies A" for A = Nat.
(define implies-self '(Pi (x Nat) Nat))

; The type of "Nat and Bool" (conjunction).
(define nat-and-bool '(Sigma (x Nat) Bool))

; The type of "Nat or Bool" (disjunction).
(define nat-or-bool '(Sum Nat Bool))

; A proof of "Nat implies Nat" is the identity function.
(define proof-implies-self '(lam (x Nat) #0))

; A proof of "Nat and Bool" is a pair.
(define proof-and '(pair 0 true))

; ============================================================
;  REPORTING
; ============================================================

(define (report-type label term)
  (display "  ")
  (display label)
  (display " : ")
  (display (kernel-infer term))
  (newline))

(define (report-reduce label term)
  (display "  ")
  (display label)
  (display " → ")
  (display (kernel-reduce term))
  (newline))
