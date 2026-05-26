; -*- lisp -*-
; ============================================================
;  COMPUTATIONAL IDENTITY — the engine actually working
; ============================================================
;
; This file is DIFFERENT from 20-identity.lisp. The earlier file
; showed identity notation that didn't verify anything. This one
; uses lizard's judgmental equality engine, which actually decides
; equality on the identity fragment by running rewrite rules.
;
; The fragment covered: refl, Id-sym, Id-trans, transport, and
; their interactions. The rules:
;
;   sym(refl_a)         -->  refl_a
;   sym(sym(p))         -->  p
;   trans(refl_a, p)    -->  p
;   trans(p, refl_a)    -->  p
;   trans(trans(p,q),r) -->  trans(p, trans(q,r))
;   transport(refl_a,x) -->  x
;
; What this is NOT: a type checker. There is no check that p in
; sym(p) is actually an identity proof. There is no check that
; the endpoints of a trans agree. This is conversion, not typing.
;
; What this IS: a real implementation of the conversion relation
; on the identity-type fragment. With it, (equal? a b) returns #t
; if a and b are judgmentally equal in this fragment.

; ------------------------------------------------------------
; 1. The basic phenomenon
; ------------------------------------------------------------

(display "=== Basic computational identity ===") (newline)

; (refl x) and (Id-sym (Id-sym (refl x))) — structurally distinct,
; judgmentally equal:
(define p (refl 'x))
(define ssp (Id-sym (Id-sym (refl 'x))))

(display "p                       = ") (display p) (newline)
(display "Id-sym (Id-sym (refl x)) = ") (display ssp) (newline)
(display "(reduce ssp)             = ") (display (reduce ssp)) (newline)
(display "(equal? p ssp)           = ") (display (equal? p ssp)) (newline)
(newline)

; ------------------------------------------------------------
; 2. The structural pattern: equal subterms ⇒ equal terms
; ------------------------------------------------------------

(display "=== Structural recursion in reduce ===") (newline)

; Even deeply nested terms reduce correctly:
(define deep
  (Id-sym (Id-trans (Id-trans (refl 'a) (refl 'a))
                    (Id-sym (Id-sym (refl 'a))))))
(display "deep term:   ") (display deep) (newline)
(display "(reduce deep) = ") (display (reduce deep)) (newline)
(display "equal? to (refl a): ")
(display (equal? deep (refl 'a)))
(newline)
(newline)

; ------------------------------------------------------------
; 3. The HoTT identity-type laws, verified
; ------------------------------------------------------------

(display "=== HoTT identity-type laws, as equalities the engine sees ===") (newline)

; Symmetry is involutive: sym(sym(p)) = p
(display "sym is involutive:           ")
(display (equal? 'p (Id-sym (Id-sym 'p))))
(newline)

; Reflexivity is the symmetry-identity: sym(refl) = refl
(display "sym preserves refl:          ")
(display (equal? (refl 'x) (Id-sym (refl 'x))))
(newline)

; Reflexivity is the unit for trans (left and right):
(display "refl is left unit for trans: ")
(display (equal? 'p (Id-trans (refl 'x) 'p)))
(newline)
(display "refl is right unit:          ")
(display (equal? 'p (Id-trans 'p (refl 'x))))
(newline)

; Associativity of trans (after canonicalization):
(display "trans is associative:        ")
(display (equal? (Id-trans (Id-trans 'p 'q) 'r)
                 (Id-trans 'p (Id-trans 'q 'r))))
(newline)

; Transport along refl is identity:
(display "transport(refl,v) = v:       ")
(display (equal? 42 (transport (refl 'x) 42)))
(newline)
(newline)

; ------------------------------------------------------------
; 4. The PLUGGABLE PART: turning rules off mid-flight
; ------------------------------------------------------------

(display "=== Pluggable rules ===") (newline)
(display "currently registered flags:") (newline)
(display "  ") (display (flag-list)) (newline)

; By default, all rules are on. Turn off symmetry-involution:
(flag-set! 'reduce-sym-involutive #f)
(display "with sym-involutive OFF:") (newline)
(display "  (reduce (Id-sym (Id-sym p))) = ")
(display (reduce (Id-sym (Id-sym 'p))))
(newline)
(display "  (equal? p (Id-sym (Id-sym p))) = ")
(display (equal? 'p (Id-sym (Id-sym 'p))))
(newline)

; Turn it back on:
(flag-set! 'reduce-sym-involutive #t)
(display "with sym-involutive ON again:") (newline)
(display "  (equal? p (Id-sym (Id-sym p))) = ")
(display (equal? 'p (Id-sym (Id-sym 'p))))
(newline)
(newline)

; ------------------------------------------------------------
; 5. Equality is not just structural — it is conversion
; ------------------------------------------------------------

(display "=== Equality vs structural equality ===") (newline)

; Three terms that all reduce to the same normal form (refl x):
(define t1 (refl 'x))
(define t2 (Id-trans (refl 'x) (refl 'x)))
(define t3 (Id-sym (Id-sym (Id-trans (refl 'x) (refl 'x)))))

(display "t1 = ") (display t1) (newline)
(display "t2 = ") (display t2) (newline)
(display "t3 = ") (display t3) (newline)
(display "all reduce to: ") (display (reduce t3)) (newline)
(display "(equal? t1 t2) = ") (display (equal? t1 t2)) (newline)
(display "(equal? t1 t3) = ") (display (equal? t1 t3)) (newline)
(display "(equal? t2 t3) = ") (display (equal? t2 t3)) (newline)
(newline)

; A term that DOESN'T reduce to refl is correctly distinguished:
(display "(equal? (refl x) (refl y)) = ")
(display (equal? (refl 'x) (refl 'y))) (newline)
(display "(equal? (Id-trans p q) (Id-trans q p)) = ")
(display (equal? (Id-trans 'p 'q) (Id-trans 'q 'p))) (newline)
(newline)

; ------------------------------------------------------------
; 6. What this gets us, and what it doesn't
; ------------------------------------------------------------

(display "=== Scope ===") (newline)
(display "  Computational identity:    WORKING on the refl fragment") (newline)
(display "  Type checking:             NOT IMPLEMENTED") (newline)
(display "  Universe levels checked:   NOT IMPLEMENTED") (newline)
(display "  Pi/Sigma conversion:       NOT IMPLEMENTED") (newline)
(display "  Couniverse stratification: opaque tagging only") (newline)
(display "  Alpha-equivalence:         NOT IMPLEMENTED (names matter)") (newline)
(newline)
(display "Adding new rules: edit lizard_tt_step in src/tt_equality.c,") (newline)
(display "add a flag in lizard_tt_flags_init, write a test case.") (newline)
(display "This is the extension point for everything that comes next.") (newline)
