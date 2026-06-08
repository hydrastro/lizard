; -*- lisp -*-
; lib/cas/axmode.lisp -- an AXIOM MODE for lightweight, flexible theorem proving: load a set of axioms once, then
; CHECK any statement against them, getting a three-valued verdict -- proven, disproven, or independent -- without
; re-stating the axioms each time (docs/CAS.md -- a simpler proving UX over the Horn-clause engine of logic.lisp).
;
; The idea is the one a user wants for casual use: "here are my axioms; now tell me, for anything I write, whether
; it follows."  An axiom environment is a database of Horn clauses (facts and rules) built with logic.lisp.  Facts
; and rules are added with ax-assume; a goal is then resolved by the engine's backward chaining.  Negative
; knowledge is supported EXPLICITLY: an axiom (not P) is stored as an ordinary fact about the predicate `not`, and a
; goal G is reported DISPROVEN exactly when (not G) is derivable -- never by mere absence of a proof.  This keeps the
; verdict SOUND and three-valued:
;     proven       -- the goal is derivable from the axioms;
;     disproven    -- the negation of the goal is derivable from the axioms;
;     independent  -- neither the goal nor its negation is derivable (the honest "don't know" of an open world).
; A goal that is BOTH provable and refutable signals an INCONSISTENT axiom set, which ax-check reports as
; 'inconsistent rather than silently picking a side.  Everything reduces to logic.lisp's query/provable?, so the
; verdicts are exactly those of the trusted engine; this module only adds the environment, the negation convention,
; and the three-valued report.
;
; Public:
;   ax-env                      -> a fresh empty axiom environment (an empty clause database)
;   ax-assume env fact          -> env with the fact/axiom added (a ground or variable atom, e.g. '(human socrates))
;   ax-assume-rule env head body-> env with the rule head :- body added (body a list of atoms)
;   ax-assume-not env fact      -> env with the negative axiom (not fact) added
;   ax-proves? env goal         -> #t iff the goal is derivable from the axioms
;   ax-refutes? env goal        -> #t iff (not goal) is derivable from the axioms
;   ax-check env goal           -> 'proven | 'disproven | 'independent | 'inconsistent (the three-valued verdict)
;   ax-theorem env goal         -> #t iff 'proven (a convenience assertion form)
;   ax-witnesses env goal var   -> the list of instantiations of var making goal provable (the proof's witnesses)
;
; Verified: from {human(socrates), mortal(X):-human(X)} the goal mortal(socrates) is PROVEN, mortal(zeus) is
; INDEPENDENT, and querying mortal(?x) yields socrates; adding (not (mortal zeus)) makes mortal(zeus) DISPROVEN;
; a set containing both p and (not p) makes p 'inconsistent; transitive closure (anc rules) resolves multi-step.
;
; Builds on logic.lisp.

(import "logic.lisp")

; ----- the environment is just a clause database (a list); fresh = empty -----
(define (ax-env) (quote ()))

; ----- add a fact, a rule, or a negative fact -----
(define (ax-assume env fact) (cons (db-fact fact) env))
(define (ax-assume-rule env head body) (cons (db-rule head body) env))
(define (ax-assume-not env fact) (cons (db-fact (ax-negate fact)) env))
(define (ax-negate fact) (list (quote not) fact))     ; (not P), with P kept as a single nested term

; ----- the primitive queries reduce to the trusted engine -----
(define (ax-proves? env goal) (provable? env goal))
(define (ax-refutes? env goal) (provable? env (ax-negate goal)))

; ----- the three-valued verdict -----
(define (ax-check env goal) (ax-verdict (ax-proves? env goal) (ax-refutes? env goal)))
(define (ax-verdict p r)
  (cond ((if p r #f) (quote inconsistent))            ; both goal and its negation derivable
        (p (quote proven))
        (r (quote disproven))
        (else (quote independent))))

; ----- convenience: a clean boolean theorem assertion -----
(define (ax-theorem env goal) (equal? (ax-check env goal) (quote proven)))

; ----- the witnesses: instantiations of var that make goal provable -----
(define (ax-witnesses env goal var) (query-var env goal var))
