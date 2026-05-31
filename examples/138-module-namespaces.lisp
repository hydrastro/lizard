; -*- lisp -*-
; examples/138-module-namespaces.lisp
;
; Phase 3 — real module namespaces.  `module` evaluates a body in a hermetic
; environment; `export` chooses the public names (no export form => every
; top-level definition is public); `import` brings a namespace's exports into
; the caller, qualified `alias:name` by default, or unqualified with
; `:only (...)` / `:all`.  The historic `(import "file")` form (load a library
; unqualified into the caller) is unchanged.
;
; Self-checking: each `must` raises if its claim is false, so a clean exit means
; every property held.

(define (must label x)
  (display "  ") (display label) (display " : ")
  (display (if x "ok" "FAIL")) (newline)
  (if x #t (raise 'module-regression)))

;; Two modules that both define `foo`.  Because each body is hermetic, the two
;; `foo`s never collide; the importer sees them qualified.
(module alpha
  (define foo 1)
  (define helper 100)            ; private: never exported
  (export foo))
(module beta
  (define foo 2)
  (export foo))
(import alpha)                    ; qualified by default => alpha:foo
(import beta)                     ; qualified by default => beta:foo

(must "colliding names don't interfere (alpha:foo=1, beta:foo=2)"
      (and (= alpha:foo 1) (= beta:foo 2)))
(must "private bindings stay private (alpha:helper is unbound)"
      (not (defined? 'alpha:helper)))

;; export / import round-trips a value unchanged.
(module box (define payload 42) (export payload))
(import box)
(must "export/import round-trips (box:payload=42)" (= box:payload 42))

;; :as renames the qualifier.
(module geometry (define pi 3) (export pi))
(import geometry :as g)
(must ":as renames the namespace (g:pi=3)" (= g:pi 3))

;; :only brings selected names in unqualified; unlisted ones are not bound.
(module arith
  (define inc (lambda (n) (+ n 1)))
  (define dec (lambda (n) (- n 1)))
  (export inc dec))
(import arith :only (inc))
(must ":only imports listed names unqualified ((inc 9)=10)" (= (inc 9) 10))

;; :all brings every export in unqualified.
(import arith :all)
(must ":all imports every export unqualified ((dec 9)=8)" (= (dec 9) 8))

;; A module with no export form publishes all of its top-level definitions.
(module pair-mod (define a1 7) (define a2 8))
(import pair-mod)
(must "no export form => export all (pair-mod:a1=7, pair-mod:a2=8)"
      (and (= pair-mod:a1 7) (= pair-mod:a2 8)))

;; The symbolic-algebra library loads as a namespaced module: a real file, its
;; own internal imports resolved hermetically, used through the `cas:` prefix.
(import "cas.lisp" :as cas)
(must "CAS loads as a namespaced library (cas:simplify (+ x 0) = x)"
      (equal? (cas:simplify (quote (+ x 0))) (quote x)))

(display "all module-namespace checks passed") (newline)
