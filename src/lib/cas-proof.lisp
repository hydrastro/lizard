; -*- lisp -*-
; lib/cas-proof.lisp — A proof-producing CAS (the "verified CAS" idea).
;
; THE VISION: when the CAS computes something (e.g. d/dx x² = 2x), it
; also emits a JUSTIFICATION — a chain of named rules. Each rule is
; backed by a lemma; each lemma ultimately by the field axioms, by the
; construction of the reals, and finally by the ZFC axioms. So a CAS
; result can be UNFOLDED into a proof skeleton bottoming out at ZFC.
;
; This module supplies:
;   * a layered axiom/lemma database (ZFC → reals → fields → limits →
;     calculus → CAS rewrite rules)
;   * unfolding: trace any rule down to its ZFC foundations
;   * derivation trees: differentiate while recording rule citations

(import "match.lisp")
(import "cas.lisp")

; ============================================================
;  THE FOUNDATION DATABASE
; ============================================================
; Each entry: (name statement (dep ...)). A rule with no deps is a
; ZFC axiom — a leaf of every proof.

(define foundation-db
  (list
    ; ---------- Layer 0: ZFC axioms (the leaves) ----------
    (list 'zfc-extensionality "sets with the same elements are equal" '())
    (list 'zfc-empty-set      "there is a set with no elements" '())
    (list 'zfc-pairing        "for any a,b there is the set {a,b}" '())
    (list 'zfc-union          "every set has a union set" '())
    (list 'zfc-power-set      "every set has a power set" '())
    (list 'zfc-infinity       "there is an inductive (infinite) set" '())
    (list 'zfc-separation     "definable subclasses of a set are sets" '())
    (list 'zfc-replacement    "definable images of a set are sets" '())
    (list 'zfc-foundation     "every nonempty set is disjoint from some member" '())
    (list 'zfc-choice         "a choice function exists for any family" '())

    ; ---------- Layer 1: number constructions ----------
    (list 'nat-construction   "ℕ as the least inductive set (von Neumann ordinals)"
          '(zfc-infinity zfc-separation zfc-empty-set))
    (list 'int-construction   "ℤ as equivalence classes of ℕ×ℕ"
          '(nat-construction zfc-pairing zfc-replacement))
    (list 'rat-construction   "ℚ as equivalence classes of ℤ×(ℤ∖0)"
          '(int-construction zfc-power-set))
    (list 'real-construction  "ℝ as Dedekind cuts of ℚ"
          '(rat-construction zfc-power-set zfc-separation))
    (list 'real-completeness  "every bounded-above set of reals has a supremum"
          '(real-construction zfc-power-set))

    ; ---------- Layer 2: field & order axioms (theorems about ℝ) ----------
    (list 'field-add-assoc "(a+b)+c = a+(b+c)" '(real-construction))
    (list 'field-add-comm  "a+b = b+a" '(real-construction))
    (list 'field-add-id    "a+0 = a" '(real-construction))
    (list 'field-add-inv   "a+(-a) = 0" '(real-construction))
    (list 'field-mul-assoc "(a·b)·c = a·(b·c)" '(real-construction))
    (list 'field-mul-comm  "a·b = b·a" '(real-construction))
    (list 'field-mul-id    "a·1 = a" '(real-construction))
    (list 'field-mul-inv   "a·a⁻¹ = 1 (a≠0)" '(real-construction))
    (list 'field-distrib   "a·(b+c) = a·b + a·c" '(real-construction))
    (list 'field-zero-mul  "a·0 = 0" '(field-distrib field-add-inv))

    ; ---------- Layer 3: limits & continuity ----------
    (list 'limit-def       "lim definition via ε–δ over ℝ"
          '(real-completeness field-add-assoc field-distrib))
    (list 'limit-sum       "lim(f+g) = lim f + lim g"
          '(limit-def field-add-assoc field-add-comm))
    (list 'limit-product   "lim(f·g) = lim f · lim g"
          '(limit-def field-mul-assoc field-distrib))

    ; ---------- Layer 4: the derivative ----------
    (list 'derivative-def  "f'(x) = lim_{h→0} (f(x+h)−f(x))/h"
          '(limit-def))

    ; ---------- Layer 5: calculus differentiation rules ----------
    (list 'calc-constant "d/dx c = 0"
          '(derivative-def field-add-inv))
    (list 'calc-variable "d/dx x = 1"
          '(derivative-def field-mul-id))
    (list 'calc-sum "d/dx(u+v) = u' + v'"
          '(derivative-def limit-sum field-add-assoc))
    (list 'calc-difference "d/dx(u−v) = u' − v'"
          '(calc-sum field-add-inv))
    (list 'calc-product "d/dx(u·v) = u·v' + v·u'"
          '(derivative-def limit-product field-distrib))
    (list 'calc-quotient "d/dx(u/v) = (v·u' − u·v')/v²"
          '(calc-product field-mul-inv))
    (list 'calc-power "d/dx xⁿ = n·xⁿ⁻¹"
          '(calc-product nat-construction))
    (list 'calc-chain "d/dx f(g(x)) = f'(g(x))·g'(x)"
          '(derivative-def limit-product))
    (list 'calc-sin "d/dx sin x = cos x" '(derivative-def limit-def))
    (list 'calc-cos "d/dx cos x = −sin x" '(derivative-def limit-def))
    (list 'calc-exp "d/dx eˣ = eˣ" '(derivative-def limit-def))
    (list 'calc-ln  "d/dx ln x = 1/x" '(derivative-def limit-def))))

; ---- database access ----
(define (rule-entry name) (assoc name foundation-db))
(define (rule-statement name)
  (let ((e (rule-entry name))) (if e (car (cdr e)) "?")))
(define (rule-deps name)
  (let ((e (rule-entry name))) (if e (car (cdr (cdr e))) '())))
(define (axiom? name) (null? (rule-deps name)))

; ============================================================
;  UNFOLDING TO ZFC
; ============================================================
; Collect the ZFC axioms a rule ultimately depends on.

(define (unfold-to-axioms name)
  (if (axiom? name)
      (list name)
      (dedup (flatten-list (map unfold-to-axioms (rule-deps name))))))

(define (flatten-list lists)
  (if (null? lists) '()
    (append (car lists) (flatten-list (cdr lists)))))

(define (dedup lst)
  (if (null? lst) '()
    (if (member (car lst) (cdr lst))
        (dedup (cdr lst))
        (cons (car lst) (dedup (cdr lst))))))

; The full dependency tree of a rule (name + sub-trees).
(define (dep-tree name)
  (if (axiom? name)
      (list name)
      (cons name (map dep-tree (rule-deps name)))))

; ============================================================
;  DERIVATION TREES  (differentiate WITH justifications)
; ============================================================
; A derivation node: (deriv conclusion rule subproofs).

(define (deriv concl rule subs) (list 'deriv concl rule subs))
(define (deriv-concl d) (car (cdr d)))
(define (deriv-rule d) (car (cdr (cdr d))))
(define (deriv-subs d) (car (cdr (cdr (cdr d)))))

; Produce a derivation tree for d/dvar e.
(define (diff-proof e var)
  (let ((result (derivative e var)))
    (deriv
      (list 'd/d var e '= result)
      (diff-rule-name e var)
      (diff-subproofs e var))))

(define (diff-rule-name e var)
  (if (cas-const? e) 'calc-constant
    (if (cas-var? e) 'calc-variable
      (if (cas-op? e '+) 'calc-sum
        (if (cas-op? e '-) 'calc-difference
          (if (cas-op? e '*) 'calc-product
            (if (cas-op? e '/) 'calc-quotient
              (if (cas-op? e '^) 'calc-power
                (if (cas-op? e 'sin) 'calc-sin
                  (if (cas-op? e 'cos) 'calc-cos
                    (if (cas-op? e 'exp) 'calc-exp
                      (if (cas-op? e 'ln) 'calc-ln 'calc-chain))))))))))))

; Recursively build subproofs for the differentiable subexpressions.
(define (diff-subproofs e var)
  (if (or (cas-const? e) (cas-var? e)) '()
    (if (member (car e) '(+ - * /))
        (list (diff-proof (car (cdr e)) var)
              (diff-proof (car (cdr (cdr e))) var))
      (if (member (car e) '(^ sin cos exp ln))
          (list (diff-proof (car (cdr e)) var))
          '()))))

; ============================================================
;  PRETTY PRINTING
; ============================================================

(define (indent n)
  (if (<= n 0) "" (string-append "  " (indent (- n 1)))))

(define (print-deriv d depth)
  (display (indent depth))
  (display (deriv-concl d))
  (display "   [")
  (display (deriv-rule d))
  (display "]")
  (newline)
  (for-each (lambda (sub) (print-deriv sub (+ depth 1)))
            (deriv-subs d)))

(define (print-derivation d) (print-deriv d 0))

; Print the chain from a rule down to the ZFC axioms.
(define (print-foundations name)
  (display "Rule: ") (display name) (newline)
  (display "  states: ") (display (rule-statement name)) (newline)
  (display "  rests ultimately on these ZFC axioms:") (newline)
  (for-each (lambda (ax)
              (display "    • ") (display ax)
              (display " — ") (display (rule-statement ax)) (newline))
            (unfold-to-axioms name)))

; Print every layer between a rule and ZFC.
(define (print-dep-layers name)
  (print-layer (list name) 0))

(define (print-layer names depth)
  (if (null? names) #t
    (begin
      (display (indent depth))
      (display (car names))
      (if (axiom? (car names)) (display "  (ZFC axiom)") (display ""))
      (newline)
      (for-each (lambda (d) (print-layer (list d) (+ depth 1)))
                (rule-deps (car names)))
      (print-layer (cdr names) depth))))
