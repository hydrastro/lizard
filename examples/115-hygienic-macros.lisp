; -*- lisp -*-
; ============================================================
;  EXAMPLE 115 — Track R: recursive & hygienic expansion
; ============================================================

(import "match.lisp")
(import "syntax-rules.lisp")
(import "macro-expand.lisp")

(display "=== A MACRO ENVIRONMENT ===") (newline)
; Define several macros, some expanding into others.
(define menv
  (macro-env-add
    (macro-env-add
      (macro-env-add (make-macro-env)
        'my-when (make-macro '()
                   (list (list '(my-when c body) '(if c body #f)))))
      'my-unless (make-macro '()
                   (list (list '(my-unless c body)
                               '(my-when (not c) body)))))   ; expands to my-when!
    'my-list (make-macro '()
               (list (list '(my-list e ...) '(list e ...))))))

(display "  defined: my-when, my-unless (→my-when), my-list") (newline)
(newline)

(display "=== ONE-LEVEL EXPANSION ===") (newline)
(display "  (my-unless done (cleanup)) expands once to:") (newline)
(display "    ") (display (expand-once '(my-unless done (cleanup)) menv)) (newline)
(newline)

(display "=== RECURSIVE EXPANSION (to fixpoint) ===") (newline)
; my-unless → my-when → if, all the way down
(display "  (my-unless done (cleanup)) fully expands to:") (newline)
(display "    ") (display (expand-all '(my-unless done (cleanup)) menv)) (newline)
(newline)

(display "=== EXPANSION INSIDE NESTED FORMS ===") (newline)
(display "  (begin (my-when a (f)) (my-list 1 2 3)):") (newline)
(display "    ")
(display (expand-all '(begin (my-when a (f)) (my-list 1 2 3)) menv)) (newline)
(newline)

(display "=== GENSYM (fresh names) ===") (newline)
(display "  gensym tmp: ") (display (gensym 'tmp)) (newline)
(display "  gensym tmp: ") (display (gensym 'tmp)) (newline)
(display "  (each call yields a unique symbol)") (newline)
(newline)

(display "=== HYGIENE: avoiding variable capture ===") (newline)
; A swap macro introduces a temporary. Without hygiene, if the user
; also uses that temp name, it gets captured. The hygienic macro
; renames its introduced 'tmp to a fresh gensym.
(define hyg-menv
  (macro-env-add (make-macro-env)
    'swap
    (make-hygienic-macro '() '(tmp)
      (list (list '(swap a b)
                  '(let ((tmp a)) (set! a b) (set! b tmp)))))))

(display "  Non-hygienic template uses 'tmp'. Hygienic expansion") (newline)
(display "  renames it so user code can safely use 'tmp' too:") (newline)
(display "  (swap x y) → ") (newline)
(display "    ") (display (expand-all-h '(swap x y) hyg-menv)) (newline)
(display "  (swap tmp y) → ") (newline)
(display "    ") (display (expand-all-h '(swap tmp y) hyg-menv)) (newline)
(display "  (note: the template's tmp became tmp.N — no capture of") (newline)
(display "   the user's own tmp variable)") (newline)
(newline)

(display "=== Track R milestone: recursive + hygienic expansion ✓ ===") (newline)
