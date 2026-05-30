; -*- lisp -*-
; ============================================================
;  EXAMPLE 63 — Module loader (Phase C)
; ============================================================
;
; lizard now has a module system with caching and search paths.
;
;   (import "path.lisp")       — load once, cache, return last value
;   (load "path.lisp")         — always re-evaluate (no caching)
;   (module-loaded? "path")    — check if already imported
;   (module-search-path)       — list the search path
;   (add-module-path! "dir")   — prepend a directory to the search path
;
; The default search path includes "lib/". Files are resolved by
; trying the raw path first, then each search directory prepended.

(display "=== Module search path ===") (newline)
(display "  ") (display (module-search-path)) (newline)
(display "  ↑ default includes lib/") (newline)
(newline)

(display "=== Import a module from lib/ ===") (newline)
(display "  (module-loaded? \"math-utils.lisp\"): ")
(display (module-loaded? "math-utils.lisp")) (newline)
(display "  ↑ not yet loaded") (newline)
(newline)

(import "math-utils.lisp")

(display "  After import:") (newline)
(display "  (module-loaded? \"math-utils.lisp\"): ")
(display (module-loaded? "math-utils.lisp")) (newline)
(display "  ↑ now cached") (newline)
(newline)

(display "  Using imported definitions:") (newline)
(display "  (square 7): ") (display (square 7)) (newline)
(display "  (cube 3): ") (display (cube 3)) (newline)
(newline)

(display "=== Second import is a no-op (cached) ===") (newline)
(display "  (import \"math-utils.lisp\"): ")
(display (import "math-utils.lisp")) (newline)
(display "  ↑ returns cached result without re-evaluating") (newline)
(newline)

(display "=== Import the prelude ===") (newline)
(import "prelude.lisp")
(display "  prelude imported successfully") (newline)
(newline)

(display "=== Add a custom search path ===") (newline)
(add-module-path! "examples")
(display "  (module-search-path): ") (display (module-search-path)) (newline)
(display "  ↑ examples/ now on the search path") (newline)
(newline)

(display "=== End of module loader example ===") (newline)
