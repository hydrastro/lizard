; -*- lisp -*-
; ============================================================
;  EXAMPLE 105 — Module / namespace system (Phase 5)
; ============================================================

(import "match.lisp")
(import "namespace.lisp")

(display "=== DEFINING MODULES ===") (newline)
(define math-mod
  (make-module 'math
    (exports 'pi 314 'e 271 'tau 628 'secret 999)))
(define string-mod
  (make-module 'strings
    (exports 'empty "" 'space " " 'pi 3)))  ; note: pi collides

(display "  math exports: ") (display (module-exports math-mod)) (newline)
(display "  strings exports: ") (display (module-exports string-mod)) (newline)
(newline)

(display "=== ACCESSING EXPORTS ===") (newline)
(display "  math:pi = ") (display (module-ref math-mod 'pi)) (newline)
(display "  math:tau = ") (display (module-ref math-mod 'tau)) (newline)
(display "  provides 'e? ") (display (module-provides? math-mod 'e)) (newline)
(display "  provides 'phi? ") (display (module-provides? math-mod 'phi)) (newline)
(newline)

(display "=== SELECTIVE IMPORT (only) ===") (newline)
(display "  import (only math pi e):") (newline)
(display "    ") (display (import-only math-mod '(pi e))) (newline)
(newline)

(display "=== IMPORT EXCEPT ===") (newline)
(display "  import (except math secret):") (newline)
(display "    ") (display (import-except math-mod '(secret))) (newline)
(newline)

(display "=== IMPORT WITH RENAMING ===") (newline)
(display "  import (rename math (pi PI) (e E)):") (newline)
(display "    ") (display (import-rename math-mod '((pi . PI) (e . E)))) (newline)
(newline)

(display "=== IMPORT WITH PREFIX ===") (newline)
(display "  import (prefix math: math):") (newline)
(display "    ") (display (import-prefix math-mod 'math:)) (newline)
(newline)

(display "=== NAMESPACE: COMBINING IMPORTS ===") (newline)
(define ns
  (namespace-add
    (namespace-add (make-namespace)
                   (import-prefix math-mod 'm:))
    (import-prefix string-mod 's:)))
(display "  combined names: ") (display (namespace-names ns)) (newline)
(display "  m:pi = ") (display (namespace-ref ns 'm:pi)) (newline)
(display "  s:space = [") (display (namespace-ref ns 's:space)) (display "]") (newline)
(newline)

(display "=== COLLISION DETECTION ===") (newline)
(display "  conflicts between math and strings: ")
(display (import-conflicts (module-bindings math-mod)
                           (module-bindings string-mod))) (newline)
(display "  (both export 'pi — prefixing avoids the clash)") (newline)
(newline)

(display "=== Phase 5 milestone: namespaces ✓ ===") (newline)
