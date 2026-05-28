; -*- lisp -*-
; ============================================================
;  EXAMPLE 126 — Proof-producing CAS: from calculus to ZFC
; ============================================================
;
; THE DREAM: a CAS result that unfolds into a proof skeleton
; bottoming out at the ZFC axioms.

(import "match.lisp")
(import "cas.lisp")
(import "cas-proof.lisp")

(display "╔════════════════════════════════════════════════════╗") (newline)
(display "║  From a derivative, down to the axioms of set theory ║") (newline)
(display "╚════════════════════════════════════════════════════╝") (newline)
(newline)

(display "=== THE COMPUTATION ===") (newline)
(display "  d/dx x² = ") (display (cas->string (derivative '(^ x 2) 'x))) (newline)
(newline)

(display "=== THE DERIVATION TREE (with rule citations) ===") (newline)
(print-derivation (diff-proof '(^ x 2) 'x))
(newline)

(display "=== A LARGER DERIVATION: d/dx (x² + sin x) ===") (newline)
(print-derivation (diff-proof '(+ (^ x 2) (sin x)) 'x))
(newline)

(display "=== WHY IS THE POWER RULE TRUE? (down to ZFC) ===") (newline)
(print-foundations 'calc-power)
(newline)

(display "=== WHY IS THE PRODUCT RULE TRUE? (down to ZFC) ===") (newline)
(print-foundations 'calc-product)
(newline)

(display "=== THE FULL DEPENDENCY CHAIN: calc-power → ZFC ===") (newline)
(print-dep-layers 'calc-power)
(newline)

(display "=== EVERY ZFC AXIOM A DERIVATIVE RESTS ON ===") (newline)
(display "  d/dx of any polynomial ultimately uses these axioms:") (newline)
(for-each (lambda (ax)
            (display "    • ") (display ax) (newline))
          (unfold-to-axioms 'calc-power))
(newline)

(display "╔════════════════════════════════════════════════════╗") (newline)
(display "║  d/dx x² = 2x   is not a black box: it unfolds to a  ║") (newline)
(display "║  finite proof tree whose leaves are ZFC axioms.      ║") (newline)
(display "║  This is the architecture of a verified CAS.         ║") (newline)
(display "╚════════════════════════════════════════════════════╝") (newline)
