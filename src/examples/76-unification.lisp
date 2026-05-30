; -*- lisp -*-
; ============================================================
;  EXAMPLE 76 — Unification (K.5)
; ============================================================
;
; The kernel now has first-order unification: it can automatically
; solve metavariables by matching term structure.
;
; When you write (kernel-unify A B), the kernel tries to make A
; and B definitionally equal by solving any metavariables in them.

(display "=== First-order Unification ===") (newline)
(newline)

; First, create some holes
(display "  Creating holes:") (newline)
(define h0 (kernel-hole 'Nat))    ; ?0 : Nat
(define h1 (kernel-hole 'Bool))   ; ?1 : Bool
(newline)

; Unify ?0 with (succ 0) — should solve ?0 = (succ 0)
(display "  Unify ?0 with (succ 0):") (newline)
(kernel-unify '?0 '(succ 0))
(newline)

; Unify ?1 with true — should solve ?1 = true
(display "  Unify ?1 with true:") (newline)
(kernel-unify '?1 'true)
(newline)

; Create fresh holes and unify structurally
(define h2 (kernel-hole 'Nat))    ; ?2 : Nat
(define h3 (kernel-hole 'Nat))    ; ?3 : Nat
(newline)

; Unify (pair ?2 ?3) with (pair 0 (succ (succ 0)))
; Should solve ?2 = 0, ?3 = succ(succ(0))
(display "  Unify (pair ?2 ?3) with (pair 0 (succ (succ 0))):") (newline)
(kernel-unify '(pair ?2 ?3) '(pair 0 (succ (succ 0))))
(newline)

; Show final meta state
(display "  Final meta state:") (newline)
(kernel-meta-state)
(newline)

; Zonk a term containing solved metas
(display "  Zonk (Id Nat ?0 ?2): ")
(display (kernel-zonk '(Id Nat ?0 ?2))) (newline)
(display "  ↑ metas replaced with their solutions") (newline)
(newline)

(display "=== Type-level unification ===") (newline)
(newline)

; Create a type-level hole
(define h4 (kernel-hole '(Sort 0)))  ; ?4 : Sort(0)  (a type)
(newline)

; Unify (Pi (x ?4) ?4) with (Pi (x Nat) Nat)
; Should solve ?4 = Nat
(display "  Unify (Pi (x ?4) ?4) with (Pi (x Nat) Nat):") (newline)
(kernel-unify '(Pi (x ?4) ?4) '(Pi (x Nat) Nat))
(newline)

(display "=== End of unification ===") (newline)
