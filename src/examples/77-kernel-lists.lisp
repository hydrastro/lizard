; -*- lisp -*-
; ============================================================
;  EXAMPLE 77 — List type in the kernel
; ============================================================
;
; The kernel now has Lists as a built-in inductive type:
;   List A — the type of lists of A
;   nil    — the empty list
;   cons   — add an element to the front

(display "=== Kernel List type ===") (newline)
(newline)

; Type inference
(display "  (List Nat) : ") (display (kernel-infer '(List Nat))) (newline)
(display "  (List Bool) : ") (display (kernel-infer '(List Bool))) (newline)
(newline)

; Constructor inference
(display "  (cons 0 nil) : ") (display (kernel-infer '(cons 0 nil))) (newline)
(display "  (cons true nil) : ") (display (kernel-infer '(cons true nil))) (newline)
(display "  (cons 0 (cons (succ 0) nil)) : ")
(display (kernel-infer '(cons 0 (cons (succ 0) nil)))) (newline)
(newline)

; Type checking
(display "  (cons 0 nil) : (List Nat)? ")
(display (kernel-check '(cons 0 nil) '(List Nat))) (newline)
(newline)

; Definitional equality
(display "  (cons 0 nil) ≡ (cons 0 nil)? ")
(display (kernel-equal? '(cons 0 nil) '(cons 0 nil))) (newline)
(newline)

; Pairs of lists
(display "  (pair (cons 0 nil) true) : ")
(display (kernel-infer '(pair (cons 0 nil) true))) (newline)
(newline)

; Unification with list patterns
(display "=== List unification ===") (newline)
(define h0 (kernel-hole '(List Nat)))
(kernel-unify '?0 '(cons 0 (cons (succ 0) nil)))
(display "  ?0 solved to a list of Nats") (newline)
(kernel-meta-state)
(newline)

(display "=== Kernel type universe ===") (newline)
(display "  ") (newline)
(display "  Nat    : ") (display (kernel-infer 'Nat)) (newline)
(display "  Bool   : ") (display (kernel-infer 'Bool)) (newline)
(display "  Unit   : ") (display (kernel-infer 'Unit)) (newline)
(display "  List   : ") (display (kernel-infer '(List Nat))) (newline)
(display "  Pi     : ") (display (kernel-infer '(Pi (x Nat) Nat))) (newline)
(display "  Sigma  : ") (display (kernel-infer '(Sigma (x Nat) Bool))) (newline)
(display "  Id     : ") (display (kernel-infer '(Id Nat 0 0))) (newline)
(display "  Sort 0 : ") (display (kernel-infer '(Sort 0))) (newline)
(display "  Sort 1 : ") (display (kernel-infer '(Sort 1))) (newline)
(newline)

(display "=== Full kernel type inventory ===") (newline)
(display "  Types:  Nat Bool Unit (List A) (Pi (x A) B) (Sigma (x A) B) (Id A a b) (Sort n)") (newline)
(display "  Terms:  0 (succ n) true false * nil (cons h t) (lam (x A) b) (pair a b)") (newline)
(display "  Elims:  natrec bool-rec listrec proj1 proj2 J") (newline)
(display "  Meta:   ?n (holes + unification)") (newline)
(display "  Proofs: begin-proof + tactics + qed") (newline)
(newline)

(display "=== End ===") (newline)
