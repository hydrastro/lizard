; -*- lisp -*-
; lib/namespace.lisp — A value-level module system (Phase 5 / Track R).
;
; lizard's environments are global, so this provides namespaces at
; the VALUE level: a module is a record of named bindings. You look
; up exports explicitly, optionally with selection and renaming —
; mirroring Racket's (import (only ...)) / (import (rename ...)).
;
; A module is a tagged alist:
;   (module name (export1 . value1) (export2 . value2) ...)

; ============================================================
;  CONSTRUCTION
; ============================================================

; (make-module 'math (list (cons 'pi 3) (cons 'e 2)))
(define (make-module name bindings)
  (cons 'module (cons name bindings)))

(define (module? x)
  (and (pair? x) (equal? (car x) 'module)))

(define (module-name m) (car (cdr m)))
(define (module-bindings m) (cdr (cdr m)))

; ============================================================
;  EXPORT LIST BUILDER
; ============================================================
; Build a binding alist from alternating name/value pairs.
;   (exports 'pi 3 'e 2) => ((pi . 3) (e . 2))

(define (exports . kvs)
  (build-exports kvs))

(define (build-exports kvs)
  (if (null? kvs) '()
    (if (null? (cdr kvs)) '()
      (cons (cons (car kvs) (car (cdr kvs)))
            (build-exports (cdr (cdr kvs)))))))

; ============================================================
;  ACCESS
; ============================================================

; Look up an export by name (returns #f if not exported).
(define (module-ref m name)
  (let ((pair (assoc name (module-bindings m))))
    (if pair (cdr pair) #f)))

; Does the module export this name?
(define (module-provides? m name)
  (if (assoc name (module-bindings m)) #t #f))

; List all exported names.
(define (module-exports m)
  (map car (module-bindings m)))

; ============================================================
;  IMPORT FORMS (mirroring Racket)
; ============================================================

; (import-all m) — get the full binding alist.
(define (import-all m)
  (module-bindings m))

; (import-only m '(pi e)) — only the named exports.
(define (import-only m names)
  (filter (lambda (pair) (member (car pair) names))
          (module-bindings m)))

; (import-except m '(secret)) — everything but the named.
(define (import-except m names)
  (filter (lambda (pair) (not (member (car pair) names)))
          (module-bindings m)))

; (import-rename m '((pi . PI) (e . E))) — rename on import.
(define (import-rename m renames)
  (map (lambda (pair)
         (let ((new-name (assoc (car pair) renames)))
           (if new-name
               (cons (cdr new-name) (cdr pair))
               pair)))
       (module-bindings m)))

; (import-prefix m 'math:) — prefix all names.
(define (import-prefix m prefix)
  (map (lambda (pair)
         (cons (symbol-prefix prefix (car pair)) (cdr pair)))
       (module-bindings m)))

(define (symbol-prefix prefix sym)
  (string->symbol
    (string-append (symbol->string prefix) (symbol->string sym))))

; ============================================================
;  NAMESPACE = COMBINED IMPORTS
; ============================================================
; A namespace is an alist built from several imports. Look up
; names in it with namespace-ref.

(define (make-namespace) '())

(define (namespace-add ns bindings)
  (append bindings ns))

(define (namespace-ref ns name)
  (let ((pair (assoc name ns)))
    (if pair (cdr pair) #f)))

(define (namespace-names ns)
  (map car ns))

; Detect name collisions between two binding sets.
(define (import-conflicts a b)
  (filter (lambda (name) (member name (map car b)))
          (map car a)))
