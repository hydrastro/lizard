; AXIOM MODE: a lightweight, flexible way to do theorem proving -- load a set of axioms once, then CHECK any
; statement against them and get a three-valued verdict (proven / disproven / independent), without re-stating the
; axioms each time (docs/CAS.md -- a simpler proving UX over the Horn-clause engine).
;
; An axiom environment is a database of facts and rules.  A goal is PROVEN when it is derivable, DISPROVEN when its
; negation is derivable (negative axioms are stored explicitly, so a disproof is never inferred from mere absence),
; and INDEPENDENT when neither holds -- the honest "don't know" of an open world.  A set proving both a statement
; and its negation is flagged INCONSISTENT rather than silently resolved.
(import "cas/axmode.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))
(define (mem? x l) (cond ((null? l) #f) ((equal? x (car l)) #t) (else (mem? x (cdr l)))))

(display "Axiom mode: assume axioms once, then check any statement -- proven, disproven, or independent.") (newline) (newline)

(display "axioms: human(socrates), human(plato), and the rule mortal(X) :- human(X).") (newline)
(define base
  (ax-assume-rule
    (ax-assume (ax-assume (ax-env) (quote (human socrates))) (quote (human plato)))
    (quote (mortal ?x)) (list (quote (human ?x)))))

(display "  check (mortal socrates) -> ") (display (ax-check base (quote (mortal socrates)))) (newline)
(must "mortal(socrates) is PROVEN" (equal? (ax-check base (quote (mortal socrates))) (quote proven)))
(must "mortal(plato) is PROVEN" (equal? (ax-check base (quote (mortal plato))) (quote proven)))
(must "the theorem form agrees" (ax-theorem base (quote (mortal socrates))))

(display "  check (mortal zeus) -> ") (display (ax-check base (quote (mortal zeus)))) (newline)
(must "mortal(zeus) is INDEPENDENT (neither it nor its negation follows)" (equal? (ax-check base (quote (mortal zeus))) (quote independent)))

(display "  the provable instances of mortal(?x): ") (display (ax-witnesses base (quote (mortal ?x)) (quote ?x))) (newline)
(must "socrates is among the witnesses" (mem? (quote socrates) (ax-witnesses base (quote (mortal ?x)) (quote ?x))))
(must "plato is among the witnesses" (mem? (quote plato) (ax-witnesses base (quote (mortal ?x)) (quote ?x))))

(display "now add the negative axiom (not (mortal zeus)) -- a god is not mortal:") (newline)
(define withneg (ax-assume-not base (quote (mortal zeus))))
(display "  check (mortal zeus) -> ") (display (ax-check withneg (quote (mortal zeus)))) (newline)
(must "mortal(zeus) is now DISPROVEN" (equal? (ax-check withneg (quote (mortal zeus))) (quote disproven)))
(must "mortal(socrates) is still PROVEN" (equal? (ax-check withneg (quote (mortal socrates))) (quote proven)))

(display "transitive rules: parent facts plus anc(X,Y):-parent(X,Y) and anc(X,Z):-parent(X,Y),anc(Y,Z):") (newline)
(define anc
  (ax-assume-rule
    (ax-assume-rule
      (ax-assume (ax-assume (ax-env) (quote (parent abe homer))) (quote (parent homer bart)))
      (quote (anc ?x ?y)) (list (quote (parent ?x ?y))))
    (quote (anc ?x ?z)) (list (quote (parent ?x ?y)) (quote (anc ?y ?z)))))
(must "anc(abe, bart) is PROVEN through a multi-step derivation" (equal? (ax-check anc (quote (anc abe bart))) (quote proven)))
(must "anc(bart, abe) is INDEPENDENT (the relation is not symmetric)" (equal? (ax-check anc (quote (anc bart abe))) (quote independent)))

(display "soundness: a set asserting both p and (not p) is flagged INCONSISTENT, not silently resolved:") (newline)
(must "p is reported inconsistent" (equal? (ax-check (ax-assume-not (ax-assume (ax-env) (quote (p))) (quote (p))) (quote (p))) (quote inconsistent)))

(newline)
(display "Axiom mode lets you state axioms once and then ask, for any statement, whether it is proven, disproven,") (newline)
(display "or independent -- a flexible front end to the proof engine, sound by construction (a disproof requires the") (newline)
(display "negation to be derivable, and a contradictory axiom set is flagged rather than hidden).") (newline)
