; -*- lisp -*-
; lib/tactics-ext.lisp — Tactic combinators (Track K roadmap item).
;
; The built-in tactics (tactic-intro, tactic-refl, etc.) operate on
; the current proof state via side effects and return #t/#f. These
; combinators sequence and compose them.
;
; Because tactics are side-effecting calls, combinators take THUNKS
; (zero-argument lambdas) so application can be controlled.
;
;   (define t (lambda () (tactic-refl)))
;   (run-tactic t)

(import "proof.lisp")

; ============================================================
;  CORE COMBINATORS
; ============================================================

; Run a single tactic thunk; return its boolean result.
(define (run-tactic t) (t))

; Sequence: run tactics left to right. Stop and return #f on the
; first failure; return #t if all succeed.
(define (tseq . thunks)
  (define (go ts)
    (if (null? ts) #t
      (if (run-tactic (car ts))
          (go (cdr ts))
          #f)))
  (go thunks))

; Try: run a tactic thunk; succeed regardless (swallow failure).
(define (ttry t)
  (run-tactic t)
  #t)

; First: try each tactic in order; stop at the first that succeeds.
; Returns #t if any succeeded, #f if all failed.
(define (tfirst . thunks)
  (define (go ts)
    (if (null? ts) #f
      (if (run-tactic (car ts))
          #t
          (go (cdr ts)))))
  (go thunks))

; Repeat: run a tactic thunk until it fails. Always succeeds.
; (Bounded to avoid infinite loops in the scaffold.)
(define (trepeat t)
  (define (go n)
    (if (= n 0) #t
      (if (run-tactic t)
          (go (- n 1))
          #t)))
  (go 100))

; ============================================================
;  TACTIC THUNK CONSTRUCTORS
; ============================================================
; Wrap the built-in tactics as thunks for use with combinators.

(define (t-intro name) (lambda () (tactic-intro name)))
(define (t-exact term) (lambda () (tactic-exact term)))
(define (t-refl) (lambda () (tactic-refl)))
(define (t-assumption) (lambda () (tactic-assumption)))
(define (t-simpl) (lambda () (tactic-simpl)))
(define (t-split) (lambda () (tactic-split)))
(define (t-left) (lambda () (tactic-left)))
(define (t-right) (lambda () (tactic-right)))

; ============================================================
;  HIGH-LEVEL PROOF STRATEGIES
; ============================================================

; "auto" — try the common closing tactics: refl, then assumption.
(define (t-auto)
  (lambda ()
    (tfirst (t-refl) (t-assumption))))

; "crush" — simplify then try to close automatically.
(define (t-crush)
  (lambda ()
    (tseq (t-simpl) (t-auto))))

; Run a full proof: begin, apply a tactic strategy, qed.
(define (prove-with type-expr strategy)
  (begin-proof type-expr)
  (run-tactic strategy)
  (qed))

; ============================================================
;  EXAMPLES OF COMPOSED STRATEGIES
; ============================================================

; Prove a reflexivity goal with simpl+refl (handles goals that
; need a reduction step first).
(define (prove-by-computation type-expr)
  (prove-with type-expr (t-crush)))

; Prove a Sum goal by trying left then right with a witness.
(define (prove-sum type-expr witness)
  (begin-proof type-expr)
  (tfirst
    (lambda () (tseq (t-left) (t-exact witness)))
    (lambda () (tseq (t-right) (t-exact witness))))
  (qed))
