; -*- lisp -*-
; ============================================================
;  PHASE H.1 — HIT scaffolding
;  Architecture for Higher Inductive Types
; ============================================================
;
; Higher Inductive Types are the technical engine of HoTT. They
; let you declare not just types with point constructors (the
; ordinary inductive case) but types with PATH constructors —
; equalities between elements baked into the type's definition.
;
; The canonical first example is the circle S¹:
;   constructor:  base : S¹
;   path:         loop : base ≡ base
;
; A circle is "one point and a loop on it." Its non-trivial
; fundamental group emerges from the loop constructor.
;
; ----- What H.1 delivers -----
;
; H.1 provides the DATA STRUCTURE for HIT declarations. You can
; declare HITs, register them, look them up, query their pieces,
; and use them as types in inference. What H.1 does NOT yet
; deliver is COMPUTATION on HIT instances — applying recursors,
; reducing `(S¹-rec base loop b l)` to the case-appropriate
; value, comp on path constructors. Those require per-HIT or
; per-shape work that is the hard research problem.
;
; H.1 is the scaffold. Specific HITs and generic computation
; rules plug into it in later phases.
;
; ----- Forms -----
;
;   (HIT 'name CTOR-OR-PATH ...)        declaration + registry side effect
;   (HIT-constructor 'name TYPE ...)    point constructor record
;   (HIT-path 'name SOURCE TARGET)      path constructor record
;   (HIT-ref 'name)                     use a registered HIT as a type
;   (HIT-app 'cname ARG ...)            apply a constructor
;   (HIT-lookup 'name)                  registry query
;
; All HIT structures are first-class data — you can pass them,
; store them, inspect them, compare them. Substitution flows
; through their fields. Alpha-equality is structural.

; ============================================================
;  1. Declaring the circle S¹
; ============================================================

(display "=== The circle S¹ ===") (newline)
(define s1 (HIT 'S1
                (HIT-constructor 'base)
                (HIT-path 'loop 'base 'base)))
(display "  ") (display s1) (newline)
(newline)

(display "S¹ is registered automatically:") (newline)
(display "  (HIT-lookup 'S1) = ") (display (HIT-lookup 'S1)) (newline)
(newline)

(display "Using S¹ as a type:") (newline)
(display "  (HIT-ref 'S1) : ")
(display (infer (context) (HIT-ref 'S1))) (newline)
(display "  ↑ every HIT lives at (U 0) for now") (newline)
(newline)

(display "Applying the base constructor:") (newline)
(display "  (HIT-app 'base) : ")
(display (infer (context) (HIT-app 'base))) (newline)
(display "  ↑ HIT-app types at (HIT-ref 'host)") (newline)
(newline)

; ============================================================
;  2. Propositional truncation ‖A‖
; ============================================================

(display "=== Propositional truncation ‖A‖ ===") (newline)
(display "Another canonical HIT — squashes a type into a proposition:") (newline)
(display "  inj : A → ‖A‖") (newline)
(display "  squash : (x y : ‖A‖) → x ≡ y") (newline)
(newline)

(define ptrunc (HIT 'Trunc
                    (HIT-constructor 'inj (U 0))
                    (HIT-path 'squash 'inj 'inj)))
(display "  Declared: ") (display ptrunc) (newline)
(newline)

(display "Two HITs co-exist in the registry:") (newline)
(display "  S1:    ") (display (HIT-lookup 'S1)) (newline)
(display "  Trunc: ") (display (HIT-lookup 'Trunc)) (newline)
(newline)

(display "Constructor lookup picks the right host even when") (newline)
(display "constructor names are unique across HITs:") (newline)
(display "  (HIT-app 'base) : ")
(display (infer (context) (HIT-app 'base))) (newline)
(display "  (HIT-app 'inj)  : ")
(display (infer (context) (HIT-app 'inj))) (newline)
(newline)

; ============================================================
;  3. A 2-constructor HIT (trees)
; ============================================================

(display "=== Binary trees ===") (newline)
(display "An ordinary inductive type (no paths) declared as a HIT:") (newline)
(define tree (HIT 'Tree
                  (HIT-constructor 'leaf)
                  (HIT-constructor 'node)))
(display "  ") (display tree) (newline)
(newline)

(display "(HIT-app 'leaf) : ")
(display (infer (context) (HIT-app 'leaf))) (newline)
(display "(HIT-app 'node) : ")
(display (infer (context) (HIT-app 'node))) (newline)
(newline)

; ============================================================
;  4. What HIT-app does NOT do yet
; ============================================================

(display "=== H.1 boundary: no computation rules ===") (newline)
(display "HIT-app stays as data under reduction:") (newline)
(display "  (reduce (HIT-app 'base)) = ")
(display (reduce (HIT-app 'base))) (newline)
(display "  ↑ no head reduction; the instance is a literal term") (newline)
(newline)

(display "Reducing under HIT-app recurses into args:") (newline)
(display "  (reduce (HIT-app 'node (U-max (U 0) (U 1)) (U 2))) = ")
(display (reduce (HIT-app 'node (U-max (U 0) (U 1)) (U 2)))) (newline)
(display "  ↑ args got normalized; the constructor stays") (newline)
(newline)

(display "Errors when constructor isn't registered:") (newline)
(display "  (infer (context) (HIT-app 'unknown)) = ")
(display (infer (context) (HIT-app 'unknown))) (newline)
(newline)

; ============================================================
;  5. What this enables for the future
; ============================================================

(display "=== What this scaffolds ===") (newline)
(display "") (newline)
(display "With this data structure in place, future phases can:") (newline)
(display "") (newline)
(display "  H.2 — Add a specific HIT's computation rules.") (newline)
(display "        E.g. S¹-rec: pattern-match on a circle inhabitant,") (newline)
(display "        with the 'loop' case requiring a path argument.") (newline)
(display "") (newline)
(display "  H.3 — Add path-application machinery: applying a path") (newline)
(display "        constructor to an interval variable produces a") (newline)
(display "        usable element of the HIT type.") (newline)
(display "") (newline)
(display "  H.4 — comp interaction: how composition operations behave") (newline)
(display "        on a HIT type. This is the research-grade piece;") (newline)
(display "        Cubical Agda's HIT support is the result of several") (newline)
(display "        papers, and lizard's version will develop incrementally.") (newline)
(display "") (newline)
(display "  H.5 — Generic recursor: instead of hardcoding S¹-rec,") (newline)
(display "        generate a recursor automatically from the declaration") (newline)
(display "        shape. This is the long-term goal: declare a HIT and") (newline)
(display "        get its eliminator computed by the system.") (newline)

(display "=== Phase H.1 complete ===") (newline)
