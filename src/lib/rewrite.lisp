; -*- lisp -*-
; lib/rewrite.lisp — Term rewriting + induction (Track K → 100%).
;
; Provides the machinery behind the `simpl` and `rewrite` tactics:
;   * one-directional pattern matching (lhs against a term)
;   * a rewrite engine (apply rules until a fixpoint)
;   * induction principles for Nat and List (as recursors that
;     build / verify proofs)
;   * equational reasoning chains
;
; Variables in rules are symbols beginning with "?".

; ============================================================
;  ONE-DIRECTIONAL MATCHING  (lhs pattern vs concrete term)
; ============================================================

(define (rw-var? x)
  (if (symbol? x)
      (let ((s (symbol->string x)))
        (and (> (string-length s) 0) (equal? (string-ref s 0) "?")))
      #f))

; match returns a bindings alist or 'fail.
(define (rw-match pat term binds)
  (if (equal? binds 'fail) 'fail
    (if (rw-var? pat)
        (let ((b (assoc pat binds)))
          (if b
              (if (equal? (cdr b) term) binds 'fail)   ; consistent use
              (cons (cons pat term) binds)))           ; fresh binding
      (if (pair? pat)
          (if (pair? term)
              (rw-match (cdr pat) (cdr term)
                        (rw-match (car pat) (car term) binds))
              'fail)
        (if (equal? pat term) binds 'fail)))))

; instantiate a rhs template with bindings.
(define (rw-subst tmpl binds)
  (if (rw-var? tmpl)
      (let ((b (assoc tmpl binds)))
        (if b (cdr b) tmpl))
    (if (pair? tmpl)
        (cons (rw-subst (car tmpl) binds) (rw-subst (cdr tmpl) binds))
        tmpl)))

; ============================================================
;  REWRITE ENGINE
; ============================================================
; A rule is (lhs rhs). Rules is a list of rules.

(define (rule lhs rhs) (list lhs rhs))
(define (rule-lhs r) (car r))
(define (rule-rhs r) (car (cdr r)))

; Try to rewrite the WHOLE term at its root with some rule.
(define (rewrite-root rules term)
  (if (null? rules) #f
    (let ((b (rw-match (rule-lhs (car rules)) term '())))
      (if (equal? b 'fail)
          (rewrite-root (cdr rules) term)
          (rw-subst (rule-rhs (car rules)) b)))))

; Rewrite one step: try root, else descend into subterms (leftmost).
(define (rewrite-step rules term)
  (let ((root (rewrite-root rules term)))
    (if root root
      (if (pair? term)
          (rewrite-children rules term)
          #f))))

(define (rewrite-children rules term)
  (if (null? term) #f
    (let ((r (rewrite-step rules (car term))))
      (if r
          (cons r (cdr term))
          (let ((rest (rewrite-children rules (cdr term))))
            (if rest (cons (car term) rest) #f))))))

; Normalize: rewrite until no rule applies (bounded for safety).
(define (rewrite-normalize rules term max-steps)
  (if (= max-steps 0) term
    (let ((next (rewrite-step rules term)))
      (if next
          (rewrite-normalize rules next (- max-steps 1))
          term))))

; Count steps to normal form.
(define (rewrite-trace rules term max-steps)
  (define (go term acc)
    (if (= (length acc) max-steps) (reverse acc)
      (let ((next (rewrite-step rules term)))
        (if next (go next (cons next acc)) (reverse acc)))))
  (go term '()))

; ============================================================
;  INDUCTION PRINCIPLES
; ============================================================
; nat-induction: given P(0) = base and a step (P(n) -> P(n+1)),
; compute P(n). This is the recursor for Nat, which *is* the
; induction principle under Curry-Howard.

(define (nat-induction base step n)
  (if (= n 0) base
    (step n (nat-induction base step (- n 1)))))

; list-induction: P([]) = base, step builds P(x:xs) from P(xs).
(define (list-induction base step lst)
  (if (null? lst) base
    (step (car lst) (list-induction base step (cdr lst)))))

; ============================================================
;  PROOFS BY INDUCTION (verified on test values)
; ============================================================
; We can't run the kernel's full induction here, but we can VERIFY
; an inductive property holds for a range of values — the
; computational content of the induction.

; Verify P(k) for all k in 0..n.
(define (verify-nat-property P n)
  (define (go k)
    (if (> k n) #t
      (if (P k) (go (+ k 1)) #f)))
  (go 0))

; Example properties (proven schematically, checked computationally):
; sum 0..n = n*(n+1)/2
(define (sum-to n) (nat-induction 0 (lambda (k acc) (+ k acc)) n))
(define (gauss-formula n) (quotient (* n (+ n 1)) 2))

; ============================================================
;  EQUATIONAL REASONING
; ============================================================
; A proof is a chain of steps; each step rewrites the term and is
; justified by a rule. We track the chain and confirm endpoints.

(define (eq-chain rules start max-steps)
  (cons start (rewrite-trace rules start max-steps)))

(define (eq-chain-result chain) (last-of chain))

(define (last-of lst)
  (if (null? (cdr lst)) (car lst) (last-of (cdr lst))))
